#ifndef LIM_HTTP_REQUEST_SESSION_H
#define LIM_HTTP_REQUEST_SESSION_H
#include <lim/http/http_base_decoder.h>
#include <lim/base/connected_channel_session.h>

namespace lim {
	class HttpRequestDecoder: public HttpBaseDecoder {
	public:
		/**
		*Http请求解码器
		* @param max_first_line_size 首行数据最大长度
		* @param max_header_size header最大长度
		* @param max_content_size content最大长度
		*/
		HttpRequestDecoder(int max_first_line_size, int max_header_size, int max_content_size);
		virtual ~HttpRequestDecoder();

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
		HttpRequest *http_request_;
	};

	typedef std::function<bool(Message&)> HttpRequstHandle;
	class HttpRequestSession: public ConnectedChannelSession {
	public:
		HttpRequestSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~HttpRequestSession() = default;

		/**
		*发送http响应报文(自动加上Server头信息)
		* @param response http响应报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteHttpResponse(HttpResponse &response, WriteCompleteCallback callback = NULL);

		/**
		*发送http body响应报文
		* @param content http body响应报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteHttpConent(HttpContent &content, WriteCompleteCallback callback = NULL);
	protected:
		/**
		*注册处理路由
		* @param method 方法名称
		* @param path 路径
		* @param handle 回调函数
		*/
		bool RegistHandleRouter(const std::string &method, const std::string &path, HttpRequstHandle &handle);

	private:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
		//解码后的报文处理函数
		virtual bool HandleMessage(Message &message);

	protected:
		//std::map<path, std::pair<method, handle>>
		std::map<std::string, std::pair<std::string, HttpRequstHandle>> handle_routers_;
		HttpRequstHandle current_handle_;
	};

	class HttpFullRequestDecoder: public HttpRequestDecoder {
	public:
		/**
		*Http full请求解码器
		* @param max_first_line_size 首行数据最大长度
		* @param max_header_size header最大长度
		* @param max_content_size content最大长度
		*/
		HttpFullRequestDecoder(int max_first_line_size, int max_header_size, int max_content_size);
		virtual ~HttpFullRequestDecoder() = default;

	protected:
		virtual bool CreatHttpMessage(const std::string &http_first_line);
		//http message处理函数
		virtual bool DoHttpMessage(HandleMessageCallback &message_callback);
		//http content处理函数
		virtual bool DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback);
	};

	class HttpFullRequestSession : public HttpRequestSession {
	public:
		HttpFullRequestSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~HttpFullRequestSession() = default;

	private:	
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
	};
}
#endif
