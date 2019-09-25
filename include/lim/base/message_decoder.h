#ifndef LIM_MESSAGE_CODER_H
#define LIM_MESSAGE_CODER_H
#include <lim/base/byte_buffer.h>
#include <functional>

namespace lim {
	class Message {
	public:
		Message() = default;
		virtual ~Message() = default;
		
	public:
		virtual int ToBytes(ByteBuffer &buffer) = 0;
	};
	class MessageError {
	public:
		MessageError(const std::string &error_message): error_message_(error_message) {
		}
		virtual ~MessageError() = default;
		
	public:
		std::string GetErrorMessage() { return error_message_; }

	protected:
		std::string error_message_;
	};
		
	typedef std::function<bool(Message&)> HandleMessageCallback;
	typedef std::function<void(MessageError&)> HandleErrorCallback;
	class MessageDecoder {
	public:
		MessageDecoder() = default;
		virtual ~MessageDecoder() = default;

		virtual void Reset() = 0;
		/**
		*解码函数
		* @param buffer 数据缓冲区
		* @param message_callback 解码后的message回调函数
		* @param error_callback 错误信息回调函数
		* @param is_socket_closed socket连接是否已断开
		*/
		virtual bool Decode(ByteBuffer &buffer, 
                        HandleMessageCallback &message_callback, 
                        HandleErrorCallback &error_callback, 
                        bool is_socket_closed) = 0;
	};
}
#endif
