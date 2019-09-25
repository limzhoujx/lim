#ifndef LIM_WEBSOCKET_FRAME_DECODER_H
#define LIM_WEBSOCKET_FRAME_DECODER_H
#include <lim/base/message_decoder.h>
#include <lim/websocket/websocket_frame_message.h>

namespace lim {
	class WebSocketHandshakeError: public MessageError {
	public:
		WebSocketHandshakeError(const std::string &error_message);
		virtual ~WebSocketHandshakeError() = default;
	};
	class WebSocketMessageError: public MessageError {
	public:
		WebSocketMessageError(const std::string &error_message);
		virtual ~WebSocketMessageError() = default;
	};
	class WebSocketFrameDecoder: public MessageDecoder {
	private:
		enum { READING_FIRST, READING_SECOND, READING_SIZE, MASKING_KEY, PAYLOAD, CORRUPT };
	public:
		WebSocketFrameDecoder(int max_payload_size, bool expected_frame_masked_flag);
		virtual ~WebSocketFrameDecoder() = default;

		virtual void Reset();
		/**
		*解码函数
		* @param buffer 数据缓冲区
		* @param message_callback 解码后的message回调函数
		* @param error_callback 错误信息回调函数
		* @param is_socket_closed socket连接是否已断开
		*/
		virtual bool Decode(ByteBuffer &buffer, 
                      HandleMessageCallback &callback, 
                      HandleErrorCallback &error_callback, 
                      bool is_socket_closed);

  protected:
    WebSocketFrame *CreateWebSocketFream(ByteBuffer &buffer);
    WebSocketFrame *CloneWebSocketFream(WebSocketFrame &frame);
		//websocket frame处理函数
    virtual bool DoContent(WebSocketFrame &frame, HandleMessageCallback &message_callback, HandleErrorCallback &error_callback);
		
	protected:
		int current_state_; /***当前状态***/
		int max_payload_size_; /***websocket报文最大长度***/
		bool expected_frame_masked_flag_; /***掩码信息是否必须***/

		bool frame_final_flag_; /***描述消息是否结束***/
		int frame_rsv_; /***扩展位***/
		int frame_opcode_; /***扩展位***/
		bool frame_masked_; /***消息类型***/

		char frame_mask_[4]; /***掩码信息***/
    int64_t frame_payload_length_; /***消息长度***/
	};
	
	class WebSocketFullFrameDecoder : public WebSocketFrameDecoder {
	public:
		WebSocketFullFrameDecoder(int max_payload_size, bool expected_frame_masked_flag);
		virtual ~WebSocketFullFrameDecoder();

	protected:
		//websocket frame处理函数
		virtual bool DoContent(WebSocketFrame &frame, HandleMessageCallback &message_callback, HandleErrorCallback &error_callback);
  
	private:
		WebSocketFrame *full_frames_[16];
	};
}
#endif
