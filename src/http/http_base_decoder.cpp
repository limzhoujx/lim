#include <lim/http/http_base_decoder.h>
#include <lim/base/string_utils.h>
#include <vector>
#include <string.h>

namespace lim {
	/**
	*Http解码器基类
	* @param max_first_line_size 首行数据最大长度
	* @param max_header_size header最大长度
	* @param max_content_size content最大长度
	*/
	HttpBaseDecoder::HttpBaseDecoder(int max_first_line_size, int max_header_size, int max_content_size): 
		current_state_(READ_INITIAL), chunk_size_(0), max_first_line_size_(max_first_line_size), 
		max_header_size_(max_header_size), max_content_size_(max_content_size), bytes_for_current_state_(0) {

	}

	void HttpBaseDecoder::Reset() {
		trailing_headers_.Reset();
		chunk_size_ = 0;
		bytes_for_current_state_ = 0;
		current_state_ = READ_INITIAL;
	}

	int HttpBaseDecoder::ReadContent(ByteBuffer &buffer, HttpContent &content) {
		int length = 0;
		if (chunk_size_ <= buffer.ReadableBytes()) {
			if (current_state_ != READ_CHUNKED_CONTENT) {
				content.IsLast() = true;
			}
			length = content.Content().WriteBytes(buffer, (int)chunk_size_);
		} else {
			length = content.Content().WriteBytes(buffer);
		}
		chunk_size_ -= length;
		return length;
	}

	/**
	*解码函数
	* @param buffer 数据缓冲区
	* @param message_callback 解码后的message回调函数
	* @param error_callback 错误信息回调函数
	* @param is_socket_closed socket连接是否已断开
	*/
	bool HttpBaseDecoder::Decode(ByteBuffer &buffer,
                            HandleMessageCallback &message_callback, 
                            HandleErrorCallback &error_callback,
                            bool is_socket_closed) {
		switch (current_state_) {
			case READ_INITIAL: {
				Reset();
				//1. get one line
				std::string http_first_line;
				if (!buffer.GetLine(http_first_line, "\r\n")) {
					if (max_first_line_size_ <= buffer.ReadableBytes()) {
						//first line size too long
						HttpMessageError error_mssage("http first line is too long");
						error_callback(error_mssage);
						return false;
					}
					return true;
				}

				bytes_for_current_state_ = http_first_line.length() + strlen("\r\n");
				if (max_first_line_size_ < bytes_for_current_state_) {
					//first line size too long	
					HttpMessageError error_mssage("http first line is too long");
					error_callback(error_mssage);
					return false;
				}
				
				//2. create http message
				if (!CreatHttpMessage(http_first_line)) {
					HttpMessageError error_mssage("create message error: " + http_first_line);
					error_callback(error_mssage);
					return false;
				}

				current_state_ = READ_HEADER;
				bytes_for_current_state_ = 0;
			}
			case READ_HEADER: {
				//1. get one line
				std::string http_header_line;
				if (!buffer.GetLine(http_header_line, "\r\n")) {
					if (max_header_size_ <= bytes_for_current_state_ + buffer.ReadableBytes()) {
						//header size too long
						HttpMessageError error_mssage("http header is too long");
						error_callback(error_mssage);
						return false;
					}
					return true;
				}

				bytes_for_current_state_ += http_header_line.length() + strlen("\r\n");
				if (max_header_size_ < bytes_for_current_state_) {
					//header size too long
					HttpMessageError error_mssage("http header is too long");
					error_callback(error_mssage);
					return false;
				}
	      
				//2. check if the last head line
				if (http_header_line.empty()) {
					//2.1 http message call back 
					if (!DoHttpMessage(message_callback))
						return false;
					
					//2.2 get next state
					if (IsChunked()) {
						current_state_ = READ_CHUNK_SIZE;
					} else {
						chunk_size_ = GetContentLength();
 						if (chunk_size_ == 0) {
							HttpContent empty_content;
							empty_content.IsLast() = true;
							if (!DoHttpContent(empty_content, message_callback)) {
								return false;
							}
							current_state_ = READ_INITIAL;
						} else if (chunk_size_ > 0) {
							current_state_ = READ_FIXED_LENGTH_CONTENT;
						} else {
							current_state_ = READ_VARIABLE_LENGTH_CONTENT;
						}
					}
					bytes_for_current_state_ = 0;
				} else {
					//2.1 parse header line
					std::vector<std::string> key_value = split(http_header_line, ":");
					if (key_value.size() >= 2) {
						std::string value;
						for (int i = 1; i < (int)key_value.size(); i++) {
							if (i == 1)
								value += key_value[i];
							else
								value += ":" + key_value[i];
						}
						SetHeaderValue(trim(key_value[0]), trim(value));
					}
				}
				break;
			}
			case READ_CHUNK_SIZE: {
				//1. get one line
				std::string http_chunk_size_line;
				if (!buffer.GetLine(http_chunk_size_line, "\r\n")) {
					if (max_content_size_ >= 0 && (max_content_size_ <= bytes_for_current_state_ + buffer.ReadableBytes())) {
						//content size too long
						HttpMessageError error_mssage("http content is too long");
						error_callback(error_mssage);
						return false;
					}
					break;
				}

				bytes_for_current_state_ += http_chunk_size_line.length() + strlen("\r\n");
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				
				//2. chunk size from hex string to int
				chunk_size_ = std::strtol(http_chunk_size_line.c_str(), 0, 16);
				if (chunk_size_ == 0)
					current_state_ = READ_CHUNK_FOOTER;
				else 
					current_state_ = READ_CHUNKED_CONTENT;

				break;
			}
			case READ_CHUNKED_CONTENT: {
				HttpContent chunked_content;
				bytes_for_current_state_ += ReadContent(buffer, chunked_content);
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				
				if (!DoHttpContent(chunked_content, message_callback)) {
					return false;
				}

				if (chunk_size_ == 0) {
					current_state_ = READ_CHUNK_DELIMITER;
				}
				break;
			}
			case READ_CHUNK_DELIMITER: {
				std::string http_chunk_delimiter_line;
				if (!buffer.GetLine(http_chunk_delimiter_line, "\r\n")) {
					if (max_content_size_ >= 0 && (max_content_size_ <= bytes_for_current_state_ + buffer.ReadableBytes())) {
						//content size too long
						HttpMessageError error_mssage("http content is too long");
						error_callback(error_mssage);
						return false;
					}
					break;
				}

				bytes_for_current_state_ += http_chunk_delimiter_line.length() + strlen("\r\n");
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				current_state_ = READ_CHUNK_SIZE;
				break;
			}
			case READ_CHUNK_FOOTER: {
				//1. get one line
				std::string http_chunk_footer_line;
				if (!buffer.GetLine(http_chunk_footer_line, "\r\n")) {
					if (max_content_size_ >= 0 && (max_content_size_ <= bytes_for_current_state_ + buffer.ReadableBytes())) {
						//content size too long
						HttpMessageError error_mssage("http content is too long");
						error_callback(error_mssage);
						return false;
					}
					break;
				}

				bytes_for_current_state_ += http_chunk_footer_line.length() + strlen("\r\n");
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				
				//2. check if the last head line
				if (http_chunk_footer_line.empty()) {
					HttpContent last_chunked_content;
					last_chunked_content.IsLast() = true;
					
					if (!DoHttpContent(last_chunked_content, message_callback)) {
						return false;
					}
					
					current_state_ = READ_INITIAL;
				} else {
					std::vector<std::string> key_value = split(http_chunk_footer_line, ":");
					if (key_value.size() >= 2) {
						std::string value;
						for (int i = 1; i < (int)key_value.size(); i++) {
							if (i == 1)
								value += key_value[i];
							else
								value += ": " + key_value[i];
						}
						trailing_headers_.SetHeaderValue(trim(key_value[0]), trim(value));
					}
				}
				break;
			}
			case READ_FIXED_LENGTH_CONTENT: {
				HttpContent fixed_content;
				bytes_for_current_state_ += ReadContent(buffer, fixed_content);
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				
				if (!DoHttpContent(fixed_content, message_callback)) {
					return false;
				}
				
				if (chunk_size_ == 0) {
					current_state_ = READ_INITIAL;
				}
				break;
			}
			case READ_VARIABLE_LENGTH_CONTENT: {
				HttpContent variable_content;
				if (is_socket_closed) {
					variable_content.IsLast() = true;
				}
					
				bytes_for_current_state_ += variable_content.Content().WriteBytes(buffer);
				if (max_content_size_ >= 0 && (max_content_size_ < bytes_for_current_state_)) {
					//content size too long
					HttpMessageError error_mssage("http content is too long");
					error_callback(error_mssage);
					return false;
				}
				
				if (!DoHttpContent(variable_content, message_callback)) {
					return false;
				}
				break;
			}
		}
		return true;
	}
}
