#ifndef LIM_HTTP_BASE_DECODER_H
#define LIM_HTTP_BASE_DECODER_H
#include <lim/http/http_message.h>

namespace lim {
	class HttpMessageError: public MessageError {
	public:
		HttpMessageError(const std::string &error_message): MessageError(error_message) {
		}
		virtual ~HttpMessageError() = default;
	};
		
	class HttpBaseDecoder: public MessageDecoder {
	private:
		enum { READ_INITIAL, READ_HEADER, READ_CHUNK_SIZE, READ_FIXED_LENGTH_CONTENT,
					 READ_VARIABLE_LENGTH_CONTENT, READ_CHUNKED_CONTENT, READ_CHUNK_DELIMITER,
					 READ_CHUNK_FOOTER };
	public:
		/**
		*Http解码器基类
		* @param max_first_line_size 首行数据最大长度
		* @param max_header_size header最大长度
		* @param max_content_size content最大长度
		*/
		HttpBaseDecoder(int max_first_line_size, int max_header_size, int max_content_size);
		virtual ~HttpBaseDecoder() = default;

		virtual void Reset();
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
                      bool is_socket_closed);
		
	protected:
		virtual bool IsChunked() = 0;
		virtual int64_t GetContentLength() = 0;
		virtual bool CreatHttpMessage(const std::string &http_first_line) = 0;
		virtual void SetHeaderValue(const std::string &header_name, const std::string &header_value) = 0;
		//http message处理函数
		virtual bool DoHttpMessage(HandleMessageCallback &message_callback) = 0;
		//http content处理函数
		virtual bool DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback) = 0;

	private:
		int ReadContent(ByteBuffer &buffer, HttpContent &content);

	protected:
		int current_state_; /***当前状态***/
		int64_t chunk_size_; /***content/chunk大小***/
		HttpHeaders trailing_headers_; /***只存在于chunk方式***/

		int max_first_line_size_; /***首行数据最大长度***/
		int max_header_size_; /***header最大长度***/
		int max_content_size_; /***content最大长度***/
		int bytes_for_current_state_; /***当前状态已经接收的字节数***/
	};
}
#endif
