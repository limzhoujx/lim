#include <lim/base/connected_channel_session.h>
#include <lim/base/time_utils.h>
#include <assert.h>

namespace lim {
  ConnectedChannelSession::ConnectedChannelSession(SocketChannel &channel, BootstrapConfig &config):
		channel_(channel), event_loop_(config.NextEventLoop()), config_(config), message_decoder_(NULL), 
		ExecuteTask(config.NextExecuteThread()), recv_buffer_(config.GetMaxBufferSize()), timeout_timer_(NULL) {

		last_read_timestamp_ = CurrentMilliTime();
		last_write_timestamp_ = CurrentMilliTime();
		
#ifdef ENABLE_OPENSSL
		read_waiton_flag_ = write_waiton_flag_ = 0;
#endif

		if (config_.GetTimeout() > 0) {
			timeout_timer_ = new ExecuteTimer(GetExecuteThread(), [&]()->void {
				int64_t timestamp = (last_read_timestamp_ < last_write_timestamp_ ? last_read_timestamp_ : last_write_timestamp_);
				if (CurrentMilliTime() - timestamp >= config_.GetTimeout()) {
					LoggerCallback logger_callback = config_.GetLoggerCallback();
					if (logger_callback != NULL) {
						logger_callback(LoggerLevel::LOG_ERROR, "[" + channel_.ToString() + "] " + "channel timeout");
					}
					this->Signal(ExecuteEvent::KILL_EVENT);
				} else {
					timeout_timer_->Start(config_.GetTimeout());
				}
			});
			timeout_timer_->Start(config_.GetTimeout());
		}
	}

	ConnectedChannelSession::~ConnectedChannelSession() {
		delete timeout_timer_;
		event_loop_.RemoveChannel(channel_);
		delete message_decoder_;
	}

	/**
	*异步发送数据
	* @param buffer 发送缓冲区
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool ConnectedChannelSession::WriteData(ByteBuffer &buffer, WriteCompleteCallback callback) {
		std::lock_guard<std::mutex> guard(mutex_);
		write_unit_que_.push_back(std::make_tuple(buffer, callback));
		event_loop_.AddChannel(channel_, this, true);
		return true;
	}

	/**
	*异步发送消息报文
	* @param message 消息报文
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool ConnectedChannelSession::WriteMessage(Message &message, WriteCompleteCallback callback) {
		ByteBuffer buffer;
		message.ToBytes(buffer);

		std::lock_guard<std::mutex> guard(mutex_);
		write_unit_que_.push_back(std::make_tuple(buffer, callback));
		event_loop_.AddChannel(channel_, this, true);
		return true;
	}

	//初始化事件处理函数
	bool ConnectedChannelSession::HandleInitEvent() {
		message_decoder_ = CreateDecoder();
#ifdef ENABLE_OPENSSL
    //SSL握手
    if (channel_.IsSSLChannel() && channel_.GetSSLContext()->IsClientContext()) {
      event_loop_.AddChannel(channel_, this, true);
      return true;
    }
#endif
		event_loop_.AddChannel(channel_, this);
		return true;
	}

	//读事件处理函数
	bool ConnectedChannelSession::HandleReadEvent() {		
#ifdef ENABLE_OPENSSL
		//SSL握手
		if (channel_.IsSSLChannel()) {
			 if (!channel_.IsSSLHandshaked()) {
        int waiton_flag;
			 	if (!channel_.SSLHandshake(waiton_flag)) {
					SSLHandshakeError error_mssage("ssl handshake error");
					HandleMessageError(error_mssage);
					return false;
				}

				if (!channel_.IsSSLHandshaked()) { //SSL握手未完成
          if (waiton_flag == SSL_ERROR_WANT_WRITE) {
            event_loop_.AddChannel(channel_, this, true);
          }
					return true;
				} else { //SSL握手完成
					if (channel_.SSLCheckHostName()) {
						return HandleSSLHandshaked();
					}

					SSLHandshakeError error_mssage("ssl verify host name[" + channel_.GetRemoteHostName() + "] failed");
					HandleMessageError(error_mssage);
					return false;
					
				}
			}

			if (write_waiton_flag_ == SSL_ERROR_WANT_READ) {
				write_waiton_flag_ = 0;
				event_loop_.AddChannel(channel_, this, true);
				return true;
			}
		}
#endif
		bool is_done = false;
		HandleMessageCallback message_callback = std::bind(&ConnectedChannelSession::HandleMessage, this, std::placeholders::_1);
		HandleErrorCallback error_callback = std::bind(&ConnectedChannelSession::HandleMessageError, this, std::placeholders::_1);
		while (!is_done) {
#ifdef ENABLE_OPENSSL
			int recv_length = 0;
			if (channel_.IsSSLChannel()) {
        recv_length = channel_.SSLReadBytes(recv_buffer_, read_waiton_flag_);
			} else {
				recv_length = channel_.ReadBytes(recv_buffer_);
			}
#else
			int recv_length = channel_.ReadBytes(recv_buffer_);
#endif
			if (recv_length > 0) {
				last_read_timestamp_ = CurrentMilliTime();
			}
			
			if (recv_length == -1) { //连接已关闭
				message_decoder_->Decode(recv_buffer_, message_callback, error_callback, true);
				ChannelClosedError error_mssage("socket has been closed by peer");
				HandleMessageError(error_mssage);
				return false;
			} else if (recv_buffer_.WritableBytes() > 0) { //数据读完
#ifdef ENABLE_OPENSSL
				if (read_waiton_flag_ == SSL_ERROR_WANT_WRITE) {
					event_loop_.AddChannel(channel_, this, true);
				}
#endif
				is_done = true;
			} else if (recv_buffer_.WritableBytes() == 0) { //读缓冲区满
				//atfter consume, if buffer full, then overflow
				if (!message_decoder_->Decode(recv_buffer_, message_callback, error_callback, false)) {
					return false;
				} else if (recv_buffer_.WritableBytes() == 0) { //缓冲区数据没有被消费(没有收到完整的数据包)
					ReadBufferOverflowError error_mssage("receive buffer is overflow");
					HandleMessageError(error_mssage);
					return false;
				}
			}
		}

		//解码处理接收到的数据
		while (recv_buffer_.ReadableBytes() > 0) {
			if (!message_decoder_->Decode(recv_buffer_, message_callback, error_callback, false)) {
				return false;
			} 
		}
		return true;
	}

	//写事件处理函数
	bool ConnectedChannelSession::HandleWriteEvent() {
#ifdef ENABLE_OPENSSL
		//SSL握手
    if (channel_.IsSSLChannel()) {
      if (!channel_.IsSSLHandshaked()) {
        int waiton_flag;
        if (!channel_.SSLHandshake(waiton_flag)) {
          SSLHandshakeError error_mssage("ssl handshake error");
          HandleMessageError(error_mssage);
          return false;
        }

        if (!channel_.IsSSLHandshaked()) { //SSL握手未完成
          return true;
        } else { //SSL握手完成
          if (channel_.SSLCheckHostName()) {
            return HandleSSLHandshaked();
          }

          SSLHandshakeError error_mssage("ssl verify host name[" + channel_.GetRemoteHostName() + "] failed");
          HandleMessageError(error_mssage);
          return false;

        }
      }

			if (read_waiton_flag_ == SSL_ERROR_WANT_WRITE) {
				read_waiton_flag_ = 0;
				Signal(ExecuteEvent::READ_EVENT);
        return true;
			}
		}
#endif

		bool is_done = false;
		while (!is_done) {
			{
				std::lock_guard<std::mutex> guard(mutex_);
				if (write_unit_que_.size() == 0) {
					is_done = true;
					continue;
				}
			}

			mutex_.lock();
			WriteUnit &unit = write_unit_que_.front();
			mutex_.unlock();
			
			ByteBuffer &buffer = std::get<0>(unit);
			WriteCompleteCallback callback = std::get<1>(unit);
#ifdef ENABLE_OPENSSL
      int send_length = 0;
      if (channel_.IsSSLChannel()) {
        send_length = channel_.SSLWriteBytes(buffer, write_waiton_flag_);
      } else {
        send_length = channel_.WriteBytes(buffer);
      }
#else
      int send_length = channel_.WriteBytes(buffer);
#endif
			if (send_length > 0) {
				last_write_timestamp_ = CurrentMilliTime();
			}
			
			if (send_length == -1) { //连接已关闭
				ChannelClosedError error_mssage("socket has been closed by peer");
				HandleMessageError(error_mssage);
				return false;
			} else if (buffer.ReadableBytes() > 0) { //数据未发送完毕
				is_done = true;
			} else { //发送完毕		
				{
					std::lock_guard<std::mutex> guard(mutex_);
					write_unit_que_.pop_front();
					//如果发送队列为空，移除"写"事件监听
					if (write_unit_que_.size() == 0) {
						event_loop_.AddChannel(channel_, this);
					}
				}
			
				if (callback != NULL) {
					callback();
				}
			}
		}
		return true;
	}

	//错误信息处理函数
	void ConnectedChannelSession::HandleMessageError(MessageError &error) {
		LoggerCallback logger_callback = config_.GetLoggerCallback();
		if (logger_callback != NULL) {
			logger_callback(LoggerLevel::LOG_ERROR, "[" + channel_.ToString() + "] " + error.GetErrorMessage());
		}
	}

#ifdef ENABLE_OPENSSL
	//SSL握手成功处理函数
	bool ConnectedChannelSession::HandleSSLHandshaked() {
		return true;
	}
#endif	

}
