#include <lim/websocket/websocket_frame_message.h>
#include <lim/base/string_utils.h>
#include <sstream>
#include <string.h>
#include <assert.h>
#include <time.h>

namespace lim {
	WebSocketFrame::WebSocketFrame(int frame_opcode, bool frame_final_flag, int frame_rsv):
		frame_opcode_(frame_opcode), frame_final_flag_(frame_final_flag), frame_rsv_(frame_rsv),
		frame_masked_(false) {

	}

	int WebSocketFrame::ToBytes(ByteBuffer &buffer) {
		int length = frame_content_.ReadableBytes();
    
		uint8_t first_byte = 0;
		if (frame_final_flag_) {
			first_byte |= 0x80;
		}
		first_byte |= frame_rsv_ % 8 << 4;
		first_byte |= frame_opcode_ % 128;
		buffer.WriteUInt8(first_byte);

		int mask_length = frame_masked_ ? 4 : 0;
		if (length <= 125) {
			int size = 2 + mask_length;
			if (frame_masked_ || length <= 1024) {
				size += length;
			}
			
			uint8_t second_byte = (uint8_t)(frame_masked_ ? 0x80 | length : length);
			buffer.WriteUInt8(second_byte);
		} else if (length <= 65535) {
			int size = 4 + mask_length;
			if (frame_masked_ || length <= 1024) {
				size += length;
			}

			uint8_t second_byte = (uint8_t)(frame_masked_ ? 254 : 126);
			buffer.WriteUInt8(second_byte);
			buffer.WriteUInt16(length);
		} else {
			int size = 10 + mask_length;
			if (frame_masked_ || length <= 1024) {
				size += length;
			}

			uint8_t second_byte = (uint8_t)(frame_masked_ ? 255 : 127);
			buffer.WriteUInt8(second_byte);
			buffer.WriteUInt64(length);
		}
		
		//write payload
		if (frame_masked_) {
			std::srand((unsigned int)(time(NULL)));
			char frame_mask[4] = {0};
			for (int i = 0; i < sizeof(frame_mask); i++) {
				frame_mask[i] = (char)(std::rand()%255);
			}
			
			buffer.WriteBytes(frame_mask, sizeof(frame_mask));
			for (int i = 0; i < length; i++) {
				uint8_t data = frame_content_.ReadUInt8();
				buffer.WriteUInt8(data ^ frame_mask[(i % 4)]);
			}
		} else {
			buffer.WriteBytes(frame_content_);
		}
		return 0;
	}

	ContinuationWebSocketFrame::ContinuationWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(0, frame_final_flag, frame_rsv) {

	}

	TextWebSocketFrame::TextWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(1, frame_final_flag, frame_rsv) {

	}

	BinaryWebSocketFrame::BinaryWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(2, frame_final_flag, frame_rsv) {

	}

	CloseWebSocketFrame::CloseWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(8, frame_final_flag, frame_rsv) {

	}

	PingWebSocketFrame::PingWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(9, frame_final_flag, frame_rsv) {

	}

	PongWebSocketFrame::PongWebSocketFrame(bool frame_final_flag, int frame_rsv):
		WebSocketFrame(10, frame_final_flag, frame_rsv) {

	}
}

