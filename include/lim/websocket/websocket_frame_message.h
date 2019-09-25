#ifndef LIM_WEBSOCKET_FRAME_MESSAGE_H
#define LIM_WEBSOCKET_FRAME_MESSAGE_H
#include <map>
#include <lim/base/byte_buffer.h>
#include <lim/base/message_decoder.h>

namespace lim {
	class WebSocketFrame: public Message {
	public:
		WebSocketFrame(int frame_opcode, bool frame_final_flag, int frame_rsv);
		virtual ~WebSocketFrame() = default;

		int &FrameOpCode() { return frame_opcode_; }
		bool &FrameFinalFlag() { return frame_final_flag_; }
		int &FrameRsv() { return frame_rsv_; }
		bool &FrameMasked() { return frame_masked_; }
		ByteBuffer &FrameContent() { return frame_content_; }
		virtual int ToBytes(ByteBuffer &buffer);

	private:
		bool frame_final_flag_; /***描述消息是否结束***/
		int frame_rsv_; /***扩展位***/
		int frame_opcode_; /***扩展位***/
		bool frame_masked_; /***消息类型***/
		ByteBuffer frame_content_; /***消息内容***/
  };

	class ContinuationWebSocketFrame: public WebSocketFrame {
	public:
		ContinuationWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~ContinuationWebSocketFrame() = default;
	};

	class TextWebSocketFrame: public WebSocketFrame {
	public:
		TextWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~TextWebSocketFrame() = default;
	};

	class BinaryWebSocketFrame: public WebSocketFrame {
	public:
		BinaryWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~BinaryWebSocketFrame() = default;
	};

	class CloseWebSocketFrame: public WebSocketFrame {
	public:
		CloseWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~CloseWebSocketFrame() = default;
	};

	class PingWebSocketFrame: public WebSocketFrame {
	public:
		PingWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~PingWebSocketFrame() = default;
	};
	
	class PongWebSocketFrame: public WebSocketFrame {
	public:
		PongWebSocketFrame(bool frame_final_flag, int frame_rsv);
		virtual ~PongWebSocketFrame() = default;
	};
}
#endif
