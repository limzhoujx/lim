#include <lim/http/http_message.h>
#include <lim/base/string_utils.h>
#include <sstream>
#include <string.h>
#include <assert.h>

namespace lim {
	HttpRequestLine::HttpRequestLine(const std::string &method, const std::string &uri, const std::string &version):
		method_(method), uri_(uri), version_(version) {

	}

	std::string HttpRequestLine::GetUriPath() {
		if (uri_.find("?") != -1)
			return uri_.substr(0, uri_.find("?"));
		else
			return uri_;
	}

	std::string HttpRequestLine::GetQueryValue(const std::string &query_name) {
		if (uri_.find("?") == -1) {
			return "";
		}
    
		std::string query = uri_.substr(uri_.find("?") + 1);
		std::string full_query_name = query_name + "=";
		if (query.find(full_query_name) == -1) {
      return "";
		}

		std::string query_value = query.substr(query.find(full_query_name) + 1);
		if (query_value.find("&") == -1)
			return query_value;
		else
			return query_value.substr(0, query_value.find("&"));
	}

	int HttpRequestLine::ToBytes(ByteBuffer &buffer) {
		std::string http_request_line = method_ + " " + uri_ + " " + version_ + "\r\n";
		return buffer.WriteBytes(http_request_line.c_str(), http_request_line.length());
	}

	HttpStatusLine::HttpStatusLine(int status_code, const std::string &reason_phrase, const std::string &version):
		status_code_(status_code), reason_phrase_(reason_phrase), version_(version) {

	}
	
	int HttpStatusLine::ToBytes(ByteBuffer &buffer) {
		std::string http_status_line = version_ + " " + std::to_string(status_code_) + " " + reason_phrase_ + "\r\n";
		return buffer.WriteBytes(http_status_line.c_str(), http_status_line.length());
	}

	bool HttpHeaders::IsContainsHeader(const std::string &header_name) {
    std::string lower_header_name = header_name;
    toupper(lower_header_name);

		if (header_map_.find(lower_header_name) == header_map_.end())
			return false;
		else
			return true;
	}

	bool HttpHeaders::GetHeaderValue(const std::string &header_name, std::string &header_value) {
		std::string lower_header_name = header_name;
		toupper(lower_header_name);

		if (header_map_.find(lower_header_name) == header_map_.end()) {
			return false;
		}

		header_value = header_map_[lower_header_name].second;
		return true;
	}

	void HttpHeaders::SetHeaderValue(const std::string &header_name, const std::string &header_value) {
    std::string lower_header_name = header_name;
    toupper(lower_header_name);

    header_map_[lower_header_name] = std::pair<std::string, std::string>(header_name, header_value);
	}

	bool HttpHeaders::RemoveHeader(const std::string &header_name) {
    std::string lower_header_name = header_name;
    toupper(lower_header_name);

		if (header_map_.find(lower_header_name) == header_map_.end()) {
			return false;
		}

		header_map_.erase(lower_header_name);
		return true;
	}

	bool HttpHeaders::IsChunked() {
		std::string transfer_encoding_value;
    if (!GetHeaderValue("Transfer-Encoding", transfer_encoding_value)) {
      return false;
    }
		
    if (strcmp(transfer_encoding_value.c_str(), "chunked") == 0) {
      return true;
    }
         
		return false;
	}
	
	int64_t HttpHeaders::GetContentLength(int64_t default_value) {
    std::string content_length_value;
    if (!GetHeaderValue("Content-Length", content_length_value)) {
      return default_value;
    }
		
		return stoll(content_length_value);
	}

	int HttpHeaders::ToBytes(ByteBuffer &buffer) {
		std::string http_header_lines;

    for (auto &iter : header_map_) {
      http_header_lines += iter.second.first + ": " + iter.second.second + "\r\n";
    }

		http_header_lines += "\r\n";
		return buffer.WriteBytes(http_header_lines.c_str(), http_header_lines.length());
	}

	HttpContent::HttpContent(): is_last_(false), is_chunked_(false) {

	}
	
	HttpContent::HttpContent(ByteBuffer &content): 
		content_(content), is_last_(false), is_chunked_(false) {

	}

	int HttpContent::ToBytes(ByteBuffer &buffer) {
		int length = 0;
		if (!IsChunked()) {
			length += buffer.WriteBytes(content_);
		} else {
			if (content_.ReadableBytes() > 0) {
				std::stringstream ss;
				ss << std::hex << content_.ReadableBytes() << "\r\n";
				length += buffer.WriteBytes(ss.str().c_str(), ss.str().length());
				length += buffer.WriteBytes(content_);
				length += buffer.WriteBytes("\r\n", strlen("\r\n"));
			}

			if (IsLast()) {
				//the last chunked
				length += buffer.WriteBytes("0", strlen("0"));
				length += buffer.WriteBytes("\r\n", strlen("\r\n"));

				//trailing headers
				length += TrailingHeaders().ToBytes(buffer);
				//length += buffer.WriteBytes("\r\n", strlen("\r\n"));
			}
		}
		return length;
	}

	HttpRequest::HttpRequest(const std::string &method, const std::string &uri, const std::string &version):
		http_request_line_(method, uri, version) {

	}

	int HttpRequest::ToBytes(ByteBuffer &buffer) {
		int length = http_request_line_.ToBytes(buffer);
		length += http_headers_.ToBytes(buffer);
		return length;
	}

	HttpResponse::HttpResponse(int status_code, const std::string &reason_phrase, const std::string &version):
		http_status_line_(status_code, reason_phrase, version) {

	}
		
	int HttpResponse::ToBytes(ByteBuffer &buffer) {
		int length = http_status_line_.ToBytes(buffer);
		length += http_headers_.ToBytes(buffer);
		return length;
	}

	HttpFullRequest::HttpFullRequest(const std::string &method, const std::string &uri, const std::string &version):
		HttpRequest(method, uri, version) {
			
		http_content_.IsLast() = true;
	}

	int HttpFullRequest::ToBytes(ByteBuffer &buffer) {
		int length = http_request_line_.ToBytes(buffer);
		length += http_headers_.ToBytes(buffer);

		http_content_.IsChunked() = http_headers_.IsChunked();
		length += http_content_.ToBytes(buffer);
		return length;
	}

	HttpFullResponse::HttpFullResponse(int status_code, const std::string &reason_phrase, const std::string &version):
		HttpResponse(status_code, reason_phrase, version) {

		http_content_.IsLast() = true;
	}

	int HttpFullResponse::ToBytes(ByteBuffer &buffer) {
		int length = http_status_line_.ToBytes(buffer);
		length += http_headers_.ToBytes(buffer);

		http_content_.IsChunked() = http_headers_.IsChunked();
		length += http_content_.ToBytes(buffer);
		return length;
	}
}

