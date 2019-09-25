#ifndef LIM_CONNECTED_CHANNEL_SESSION_H
#define LIM_CONNECTED_CHANNEL_SESSION_H
#include <lim/config.h>
#include <lim/base/socket_channel.h>
#include <lim/base/execute_task.h>
#include <lim/base/event_loop.h>
#include <lim/base/byte_buffer.h>
#include <lim/base/message_decoder.h>
#include <lim/base/bootstrap_config.h>

namespace lim {
	using WriteCompleteCallback = std::function<void()>;
	using WriteUnit = std::tuple<ByteBuffer, WriteCompleteCallback>;
	class ChannelClosedError : public MessageError {
	public:
		ChannelClosedError(const std::string &error_message): MessageError(error_message) {
		};
		virtual ~ChannelClosedError() = default;
	};
	class ReadBufferOverflowError : public MessageError {
	public:
		ReadBufferOverflowError(const std::string &error_message): MessageError(error_message) {
		};
		virtual ~ReadBufferOverflowError() = default;
	};
#ifdef ENABLE_OPENSSL
	class SSLHandshakeError : public MessageError {
	public:
		SSLHandshakeError(const std::string &error_message): MessageError(error_message) {
		};
		virtual ~SSLHandshakeError() = default;
	};
#endif
	class ConnectedChannelSession: public ExecuteTask {
	public:
		ConnectedChannelSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~ConnectedChannelSession();
		
		/**
		*异步发送数据
		* @param buffer 发送缓冲区
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteData(ByteBuffer &buffer, WriteCompleteCallback callback = NULL);
		/**
		*异步发送消息报文
		* @param message 消息报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteMessage(Message &message, WriteCompleteCallback callback = NULL);
		
	protected:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder() = 0;
		//解码后的报文处理函数
		virtual bool HandleMessage(Message &message) = 0;

		//初始化事件处理函数
		virtual bool HandleInitEvent();
		//读事件处理函数
		virtual bool HandleReadEvent();
		//写事件处理函数
		virtual bool HandleWriteEvent();
		//错误信息处理函数
		virtual void HandleMessageError(MessageError &error);
#ifdef ENABLE_OPENSSL
		//SSL握手成功处理函数
		virtual bool HandleSSLHandshaked();
#endif	

	protected:
		SocketChannel channel_; /***socket连接对象***/
		EventLoop &event_loop_; /***事件监听器***/
		ByteBuffer recv_buffer_; /***接收缓存***/
		
		BootstrapConfig &config_; /***引导配置(报文协议相关)***/
		MessageDecoder *message_decoder_; /***报文解码器(报文协议相关)***/
		
		int64_t last_read_timestamp_; /***最近一次读时间戳(毫秒)***/
		int64_t last_write_timestamp_; /***最近一次写时间戳(毫秒)***/
		ExecuteTimer *timeout_timer_; /***读写超时计时器(超时退出)***/

#ifdef ENABLE_OPENSSL
		int read_waiton_flag_;
		int write_waiton_flag_;
#endif
		
		std::mutex mutex_;
		std::deque<WriteUnit> write_unit_que_;
  };
}
#endif
