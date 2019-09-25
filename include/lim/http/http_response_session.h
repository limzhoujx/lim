#ifndef LIM_HTTP_RESPONSE_SESSION_H
#define LIM_HTTP_RESPONSE_SESSION_H
#include <lim/http/http_base_decoder.h>
#include <lim/base/connected_channel_session.h>

namespace lim {
	class HttpResponseDecoder: public HttpBaseDecoder {
	public:
		/**
		*Http响应解码器
		* @param max_first_line_size 首行数据最大长度
		* @param max_header_size header最大长度
		* @param max_content_size content最大长度
		*/
		HttpResponseDecoder(int max_first_line_size, int max_header_size, int max_content_size);
		virtual ~HttpResponseDecoder();

		virtual void Reset();

	protected:
		virtual bool IsChunked();
		virtual int64_t GetContentLength();
		virtual bool CreatHttpMessage(const std::string &http_first_line);
		virtual void SetHeaderValue(const std::string &header_name, const std::string &header_value);
		//http message处理函数
		virtual bool DoHttpMessage(HandleMessageCallback &message_callback);
		//http content处理函数
		virtual bool DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback);

	protected:
		HttpResponse *http_response_;
	};
	
	class HttpResponseSession : public ConnectedChannelSession {
	public:
		HttpResponseSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~HttpResponseSession() = default;
		
		/**
		*发送http请求报文(自动加上User-Agent、Host头信息)
		* @param response http响应报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteHttpRequest(HttpRequest &request, WriteCompleteCallback callback = NULL);
		/**
		*发送http body请求报文
		* @param content http body请求报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteHttpConent(HttpContent &content, WriteCompleteCallback callback = NULL);
	private:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
		//解码后的报文处理函数
		virtual bool HandleMessage(Message &message) = 0;
	};

	class HttpFullResponseDecoder: public HttpResponseDecoder {
	public:
		/**
		*Http full响应解码器
		* @param max_first_line_size 首行数据最大长度
		* @param max_header_size header最大长度
		* @param max_content_size content最大长度
		*/
		HttpFullResponseDecoder(int max_first_line_size, int max_header_size, int max_content_size);
		virtual ~HttpFullResponseDecoder() = default;

	protected:
		virtual bool CreatHttpMessage(const std::string &http_first_line);
		//http message处理函数
		virtual bool DoHttpMessage(HandleMessageCallback &message_callback);
		//http content处理函数
		virtual bool DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback);
	};
	
	class HttpFullResponseSession : public HttpResponseSession {
	public:
		HttpFullResponseSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~HttpFullResponseSession() = default;

	private:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
	};
}
#endif
