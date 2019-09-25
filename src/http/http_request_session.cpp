#include <lim/http/http_request_session.h>
#include <lim/http/http_bootstrap_config.h>
#include <lim/base/string_utils.h>
#include <assert.h>
#include <string.h>

namespace lim {
	/**
	*Http请求解码器
	* @param max_first_line_size 首行数据最大长度
	* @param max_header_size header最大长度
	* @param max_content_size content最大长度
	*/
	HttpRequestDecoder::HttpRequestDecoder(int max_first_line_size, 
																							int max_header_size, 
																							int max_content_size): 
		http_request_(NULL), HttpBaseDecoder(max_first_line_size, max_header_size, max_content_size) {

	}

	HttpRequestDecoder::~HttpRequestDecoder() {
		delete http_request_;
	}

	void HttpRequestDecoder::Reset() {
		HttpBaseDecoder::Reset();
		delete http_request_;
		http_request_ = NULL;
	}
	
	bool HttpRequestDecoder::IsChunked() {
		return http_request_->Headers().IsChunked();
	}
	
	int64_t HttpRequestDecoder::GetContentLength() {
		return http_request_->Headers().GetContentLength(0);
	}

	bool HttpRequestDecoder::CreatHttpMessage(const std::string &http_first_line) {
		const int HTTP_REQUEST_LINE_FIELD_COUNT = 3;
		std::vector<std::string> fields = split(http_first_line, " ");
		if (fields.size() != HTTP_REQUEST_LINE_FIELD_COUNT) {
			return false;
		}

		if (strcasecmp(fields[2].c_str(), "HTTP/1.0") != 0 && strcasecmp(fields[2].c_str(), "HTTP/1.1") != 0) {
			return false;
		}

		http_request_ = new HttpRequest(fields[0], fields[1], fields[2]);
		return true;
	}

	void HttpRequestDecoder::SetHeaderValue(const std::string &header_name, const std::string &header_value) {
		http_request_->Headers().SetHeaderValue(header_name, header_value);
	}

	//http message处理函数
	bool HttpRequestDecoder::DoHttpMessage(HandleMessageCallback &message_callback) {
		return message_callback(*http_request_);
	}

	//http content处理函数
	bool HttpRequestDecoder::DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback) {
		return message_callback(content);
	}
	
	HttpRequestSession::HttpRequestSession(SocketChannel &channel, BootstrapConfig &config):
		ConnectedChannelSession(channel, config), current_handle_(NULL) {
		
	}

	/**
	*发送http响应报文(自动加上Server头信息)
	* @param response http响应报文
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool HttpRequestSession::WriteHttpResponse(HttpResponse &response, WriteCompleteCallback callback) {
		HttpBootstrapConfig &config = (HttpBootstrapConfig&)config_;
		response.Headers().SetHeaderValue("Server", config.GetServerName());
		return WriteMessage(response, callback);
	}

	/**
	*发送http body响应报文
	* @param content http body响应报文
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool HttpRequestSession::WriteHttpConent(HttpContent &content, WriteCompleteCallback callback) {
		return WriteMessage(content, callback);
	}
	
	//创建报文解码器
	MessageDecoder *HttpRequestSession::CreateDecoder() {
		HttpBootstrapConfig &config = (HttpBootstrapConfig &)config_;
		int max_first_line_size = config.GetMaxFirstLineSize();
		int max_header_size = config.GetMaxHeaderSize();
		int max_content_size = config.GetMaxContentSize();
		return (new HttpRequestDecoder(max_first_line_size, max_header_size, max_content_size));
	}

	/**
	*注册处理路由
	* @param method 方法名称
	* @param path 路径
	* @param handle 回调函数
	*/
	bool HttpRequestSession::RegistHandleRouter(const std::string &method, const std::string &path, HttpRequstHandle &handle) {
		if (handle == NULL) {
			return false;
		}

		if (handle_routers_.find(path) == handle_routers_.end()) {
			handle_routers_.insert(std::pair<std::string, std::pair<std::string, HttpRequstHandle>>(path, std::make_pair(method, handle)));
		} else {
			handle_routers_[path] = std::make_pair(method, handle);
		}
		return true;
	}

	//解码后的报文处理函数
	bool HttpRequestSession::HandleMessage(Message &message) {
		if (typeid(message) == typeid(HttpContent)) {
			if (current_handle_ == NULL) {
				HttpFullResponse response(404, "Not Found");
				return WriteHttpResponse(response, [&] {
					Signal(ExecuteEvent::KILL_EVENT);
				});
			} else {
				return current_handle_(message);
			}
		} else {
			HttpRequest &request = (HttpRequest&)message;
			std::string path = request.RequestLine().GetUriPath();
			std::string method = request.RequestLine().GetMethod();
			if (handle_routers_.find(path) == handle_routers_.end()) {
				HttpFullResponse response(404, "Not Found");
				return WriteHttpResponse(response, [&] {
					Signal(ExecuteEvent::KILL_EVENT);
				});
			} else if (strcasecmp(handle_routers_[path].first.c_str(), method.c_str())) {
				HttpFullResponse response(404, "Not Found");
				return WriteHttpResponse(response, [&] {
					Signal(ExecuteEvent::KILL_EVENT);
				});
			} else {
				current_handle_ = handle_routers_[path].second;
				return current_handle_(request);
			}
		}
	}

	HttpFullRequestDecoder::HttpFullRequestDecoder(int max_first_line_size, int max_header_size, int max_content_size): 
		HttpRequestDecoder(max_first_line_size, max_header_size, max_content_size) {

	}

	bool HttpFullRequestDecoder::CreatHttpMessage(const std::string &http_first_line) {
		const int HTTP_REQUEST_LINE_FIELD_COUNT = 3;
		std::vector<std::string> fields = split(http_first_line, " ");
		if (fields.size() != HTTP_REQUEST_LINE_FIELD_COUNT) {
			return false;
		}

		if (strcasecmp(fields[2].c_str(), "HTTP/1.0") != 0 && strcasecmp(fields[2].c_str(), "HTTP/1.1") != 0) {
			return false;
		}

		http_request_ = new HttpFullRequest(fields[0], fields[1], fields[2]);
		return true;
	}

	//http message处理函数
	bool HttpFullRequestDecoder::DoHttpMessage(HandleMessageCallback &message_callback) {
		return true;
	}

	//http content处理函数
	bool HttpFullRequestDecoder::DoHttpContent(HttpContent &content, HandleMessageCallback &message_callback) {
		HttpFullRequest *request = (HttpFullRequest *)http_request_;
		content.ToBytes(request->Content().Content());
		if (content.IsLast()) 
			return message_callback(*request);
		else
			return true;
	}
	
	HttpFullRequestSession::HttpFullRequestSession(SocketChannel &channel, BootstrapConfig &config):
		HttpRequestSession(channel, config) {
		
	}

	//创建报文解码器
	MessageDecoder *HttpFullRequestSession::CreateDecoder() {
		HttpBootstrapConfig &config = (HttpBootstrapConfig &)config_;
		int max_first_line_size = config.GetMaxFirstLineSize();
		int max_header_size = config.GetMaxHeaderSize();
		int max_content_size = config.GetMaxContentSize();
		return (new HttpFullRequestDecoder(max_first_line_size, max_header_size, max_content_size));
	}
}
