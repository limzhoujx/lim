#include <lim/websocket/websocket_frame_decoder.h>
#include <lim/base/string_utils.h>
#include <vector>
#include <sstream>

namespace lim {

	WebSocketHandshakeError::WebSocketHandshakeError(const std::string &error_message): 
		MessageError(error_message) {
	}

	WebSocketMessageError::WebSocketMessageError(const std::string &error_message):
		MessageError(error_message) {
	}

	WebSocketFrameDecoder::WebSocketFrameDecoder(int max_payload_size, bool expected_frame_masked_flag): 
		current_state_(READING_FIRST), max_payload_size_(max_payload_size), 
		expected_frame_masked_flag_(expected_frame_masked_flag) {
	
	}

	void WebSocketFrameDecoder::Reset() {
		frame_payload_length_ = 0;
		current_state_ = READING_FIRST;
	}

	WebSocketFrame *WebSocketFrameDecoder::CreateWebSocketFream(ByteBuffer &buffer) {
		WebSocketFrame *frame = NULL;
		if (frame_opcode_ == 9) {
			frame = new PingWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else if (frame_opcode_ == 10) {
			frame = new PongWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else if (frame_opcode_ == 8) {
			frame = new CloseWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else if (frame_opcode_ == 1) {
			frame = new TextWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else if (frame_opcode_ == 2) {
			frame = new BinaryWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else if (frame_opcode_ == 0) {
			frame = new ContinuationWebSocketFrame(frame_final_flag_, frame_rsv_);
		} else {
			return NULL;
		}
		frame->FrameContent() = buffer;
		return frame;
	}

	WebSocketFrame *WebSocketFrameDecoder::CloneWebSocketFream(WebSocketFrame &frame) {
		WebSocketFrame *clone_frame = NULL;
		if (frame_opcode_ == 9) {
			clone_frame = new PingWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else if (frame_opcode_ == 10) {
			clone_frame = new PongWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else if (frame_opcode_ == 8) {
			clone_frame = new CloseWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else if (frame_opcode_ == 1) {
			clone_frame = new TextWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else if (frame_opcode_ == 2) {
			clone_frame = new BinaryWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else if (frame_opcode_ == 0) {
			clone_frame = new ContinuationWebSocketFrame(frame.FrameFinalFlag(), frame.FrameRsv());
		} else {
			clone_frame = new WebSocketFrame(frame_opcode_, frame.FrameFinalFlag(), frame.FrameRsv());
		}
		clone_frame->FrameContent() = frame.FrameContent();
		return clone_frame;
	}

	//websocket frame处理函数
	bool WebSocketFrameDecoder::DoContent(WebSocketFrame &frame, HandleMessageCallback &message_callback, HandleErrorCallback &error_callback) {
		return message_callback(frame);
	}

	/**
	*解码函数
	* @param buffer 数据缓冲区
	* @param message_callback 解码后的message回调函数
	* @param error_callback 错误信息回调函数
	* @param is_socket_closed socket连接是否已断开
	*/
	bool WebSocketFrameDecoder::Decode(ByteBuffer &buffer, 
                                    HandleMessageCallback &message_callback, 
                                    HandleErrorCallback &error_callback, 
                                    bool is_socket_closed) {
    switch (current_state_) {
      case READING_FIRST: {
        Reset();
        if (buffer.ReadableBytes() < 1) {
          return true;
        }
				
        uint8_t first_byte = buffer.ReadUInt8();
        frame_final_flag_ = ((first_byte & 0x80) != 0);
        frame_rsv_ = ((first_byte & 0x70) >> 4);
        frame_opcode_ = (first_byte & 0xF);
				
        current_state_ = READING_SECOND;
      }
      case READING_SECOND: {
        if (buffer.ReadableBytes() < 1) {
          return true;
        }
				
        uint8_t second_byte = buffer.ReadUInt8();
        frame_masked_ = ((second_byte & 0x80) != 0);
        frame_payload_length_ = (second_byte & 0x7F);
        if (expected_frame_masked_flag_ != frame_masked_) {
					WebSocketMessageError error_mssage("received a websocket frame that is not masked as expected");
          error_callback(error_mssage);
          return false;
        }
				
        current_state_ = READING_SIZE;
      }
      case READING_SIZE: {
        if (frame_payload_length_ == 126) {
          if (buffer.ReadableBytes() < 2) {
            return true;
          }
          frame_payload_length_ = buffer.ReadUInt16();
        } else if (frame_payload_length_ == 127) {
          if (buffer.ReadableBytes() < 8) {
            return true;
          }
          frame_payload_length_ = buffer.ReadUInt64();
        }

        if (frame_payload_length_ > max_payload_size_) {
					WebSocketMessageError error_mssage("websocket frame payload is too long");
					error_callback(error_mssage);
          return false;
        }
        
        current_state_ = MASKING_KEY;
      }
      case MASKING_KEY: {
        if (frame_masked_) {
          if (buffer.ReadableBytes() < 4) {
            return true;
          }
          buffer.ReadBytes(frame_mask_, sizeof(frame_mask_));
        }
        current_state_ = PAYLOAD;
      }
      case PAYLOAD: {
        if (frame_payload_length_ > buffer.ReadableBytes()) {
          return true;
        }
				
        ByteBuffer content;
        if (frame_masked_) {
          for (int64_t i = 0; i < frame_payload_length_; i++) {
            uint8_t data = buffer.ReadUInt8();
            content.WriteUInt8(data ^ frame_mask_[(int)(i % 4)]);
          }
        } else {
          content.WriteBytes(buffer);
        }

        WebSocketFrame *frame = CreateWebSocketFream(content);
        if (frame == NULL) {
          std::stringstream ss;
          ss << "websocket unsupported opcode: " << frame_opcode_;
					WebSocketMessageError error_mssage(ss.str());
					error_callback(error_mssage);
          return false;
        } else {
          bool is_ok = DoContent(*frame, message_callback, error_callback);
          delete frame;
          if (!is_ok) {
            return false;
          }
        }
        current_state_ = READING_FIRST;
      }
    }
    return true;
  }
  
  WebSocketFullFrameDecoder::WebSocketFullFrameDecoder(int max_payload_size, bool expected_frame_masked_flag) :
    WebSocketFrameDecoder(max_payload_size, expected_frame_masked_flag) {
    
    for (int i = 0; i < 16; i++) {
      full_frames_[i] = NULL;
    }
  }

  WebSocketFullFrameDecoder::~WebSocketFullFrameDecoder() {
    for (int i = 0; i < 16; i++) {
      delete full_frames_[i];
    }
  }

	//websocket frame处理函数
  bool WebSocketFullFrameDecoder::DoContent(WebSocketFrame &frame, HandleMessageCallback &message_callback, HandleErrorCallback &error_callback) {
    if (full_frames_[frame.FrameOpCode()] == NULL) {
      full_frames_[frame.FrameOpCode()] = CloneWebSocketFream(frame);
    } else {
      int message_size = frame.FrameContent().ReadableBytes() + full_frames_[frame.FrameOpCode()]->FrameContent().ReadableBytes();
      if (message_size > max_payload_size_) {
				WebSocketMessageError error_mssage("websocket frame size is too long");
        error_callback(error_mssage);

        delete full_frames_[frame.FrameOpCode()];
        full_frames_[frame.FrameOpCode()] = NULL;
        return false;
      }
      full_frames_[frame.FrameOpCode()]->FrameContent().WriteBytes(frame.FrameContent());
    }

    if (frame.FrameFinalFlag()) {
      full_frames_[frame.FrameOpCode()]->FrameFinalFlag() = true;
      bool is_ok = message_callback(*full_frames_[frame.FrameOpCode()]);

      delete full_frames_[frame.FrameOpCode()];
      full_frames_[frame.FrameOpCode()] = NULL;
      return is_ok;
    }
    return true;
  }
}
