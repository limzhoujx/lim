#ifndef LIM_HTTP_MESSAGE_H
#define LIM_HTTP_MESSAGE_H
#include <map>
#include <lim/base/byte_buffer.h>
#include <lim/base/message_decoder.h>

namespace lim {
	class HttpRequestLine: public Message {
	public:
		HttpRequestLine(const std::string &method, const std::string &uri, const std::string &version = "HTTP/1.1");
		virtual ~HttpRequestLine() = default;

		std::string GetVersion() { return version_; }
		void SetVersion(const std::string &version) { version_ = version; }

		std::string GetMethod() { return method_; }
		void SetMethod(const std::string &method) { method_ = method; }

		std::string GetUri() { return uri_; }
		void SetUri(const std::string &uri) { uri_ = uri; }

		std::string GetUriPath();
		std::string GetQueryValue(const std::string &query_name);

		virtual int ToBytes(ByteBuffer &buffer);

	private:
		std::string method_;
		std::string uri_;
		std::string version_;
	};
	
	class HttpStatusLine: public Message {
	public:
		HttpStatusLine(int status_code, const std::string &reason_phrase, const std::string &version = "HTTP/1.1");
		virtual ~HttpStatusLine() = default;

		std::string GetVersion() { return version_; }
		void SetVersion(const std::string &version) { version_ = version; }

		int GetStatusCode() { return status_code_; }
		void SetMethod(int status_code) { status_code_ = status_code; }

		std::string GetReasonPhrase() { return reason_phrase_; }
		void SetReasonPhrase(const std::string &reason_phrase) { reason_phrase_ = reason_phrase; }

		virtual int ToBytes(ByteBuffer &buffer);

	private:
		int status_code_;
		std::string reason_phrase_;
		std::string version_;
	};

	class HttpHeaders: public Message {
	public:
		HttpHeaders() = default;
		virtual ~HttpHeaders() = default;

		bool IsContainsHeader(const std::string &header_name);

		bool GetHeaderValue(const std::string &header_name, std::string &header_value);
		void SetHeaderValue(const std::string &header_name, const std::string &header_value);
    
		bool RemoveHeader(const std::string &header_name);

		bool IsChunked();
		
		int64_t GetContentLength(int64_t default_value = -1);

		void Reset() { header_map_.clear(); }

		virtual int ToBytes(ByteBuffer &buffer);

	private:
		std::map<std::string, std::pair<std::string, std::string> > header_map_;
	};

	class HttpContent:public Message {
	public:
		HttpContent();
		HttpContent(ByteBuffer &content);
		virtual ~HttpContent() = default;

		bool &IsLast() { return is_last_; }
		bool &IsChunked() { return is_chunked_; }
		ByteBuffer &Content() { return content_; }
		HttpHeaders &TrailingHeaders() { return trailing_headers_; }

		virtual int ToBytes(ByteBuffer &buffer);
		
	protected:
		bool is_last_;
		bool is_chunked_;
		ByteBuffer content_;
		HttpHeaders trailing_headers_;
	};

	class HttpRequest: public Message {
	public:
		HttpRequest(const std::string &method, const std::string &uri, const std::string &version = "HTTP1.1");
		virtual ~HttpRequest() = default;

		HttpRequestLine &RequestLine() { return http_request_line_; }
		HttpHeaders &Headers() { return http_headers_; }

		virtual int ToBytes(ByteBuffer &buffer);

	protected:
		HttpRequestLine http_request_line_;
		HttpHeaders http_headers_;
	};

	class HttpResponse: public Message {
	public:
		HttpResponse(int status_code, const std::string &reason_phrase, const std::string &version = "HTTP/1.1");
		virtual ~HttpResponse() = default;

		HttpStatusLine &StatusLine() { return http_status_line_; }
		HttpHeaders &Headers() { return http_headers_; }

		virtual int ToBytes(ByteBuffer &buffer);

	protected:
		HttpStatusLine http_status_line_;
		HttpHeaders http_headers_;
	};

	class HttpFullRequest: public HttpRequest {
	public:
		HttpFullRequest(const std::string &method, const std::string &uri, const std::string &version = "HTTP1.1");
		virtual ~HttpFullRequest() = default;

		HttpContent &Content() { return http_content_; }

		virtual int ToBytes(ByteBuffer &buffer);

	protected:
		HttpContent http_content_;
	};

	class HttpFullResponse: public HttpResponse {
	public:
		HttpFullResponse(int status_code, const std::string &reason_phrase, const std::string &version = "HTTP/1.1");
		virtual ~HttpFullResponse() = default;

		HttpContent &Content() { return http_content_; }

		virtual int ToBytes(ByteBuffer &buffer);

	protected:
		HttpContent http_content_;
	};
}
#endif

