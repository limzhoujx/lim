#include <lim/websocket/websocket_request_session.h>
#include <lim/websocket/websocket_bootstrap_config.h>
#include <lim/http/http_request_session.h>
#include <lim/base/string_utils.h>
#include <lim/base/base64.h>
#include <lim/base/sha1.h>
#include <string.h>
#include <sstream>
#include <vector>

namespace lim {
	WebSocketRequestSession::WebSocketRequestSession(SocketChannel& channel, BootstrapConfig &config) :
    HttpFullRequestSession(channel, config), current_state_(HTTP_HANDSHAKE), current_handle_(NULL) {

	}

	/**
	*发送websocket frame报文
	* @param websocket frame响应报文
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool WebSocketRequestSession::WriteWebSocketFrame(WebSocketFrame &frame, WriteCompleteCallback callback) {
		frame.FrameMasked() = false;
		return WriteMessage(frame, callback);
	}
	
	/**
	*注册处理路由
	* @param path 路径
	* @param handle 回调函数
	*/
	bool WebSocketRequestSession::RegistHandleRouter(const std::string &path, WebSocketFrameHandle &handle) {
		if (handle == NULL) {
			return false;
		}

		if (handle_routers_.find(path) == handle_routers_.end()) {
			handle_routers_.insert(std::pair<std::string, WebSocketFrameHandle>(path, handle));
		} else {
			handle_routers_[path] = handle;
		}
		return true;
	}

	//创建报文解码器
	MessageDecoder* WebSocketRequestSession::CreateDecoder() {
		delete message_decoder_;
		message_decoder_ = NULL;

		WebSocketBootstrapConfig &config = (WebSocketBootstrapConfig &)config_;
		if (current_state_ == HTTP_HANDSHAKE) {
			int max_first_line_size = config.GetMaxFirstLineSize();
			int max_header_size = config.GetMaxHeaderSize();
			int max_content_size = config.GetMaxContentSize();
			return (new HttpFullRequestDecoder(max_first_line_size, max_header_size, max_content_size));
		} else {
			int max_payload_size = config.GetMaxPayloadSize();
			return (new WebSocketFrameDecoder(max_payload_size, true));
		}
	}

	//握手处理
	bool WebSocketRequestSession::Handshake(HttpFullRequest& request) {
		if (strcasecmp(request.RequestLine().GetMethod().c_str(), "GET")) {
			HttpFullResponse http_response(403, "Forbidden", "HTTP/1.1");
			WriteMessage(http_response);

			std::stringstream ss;
			ss << "websocket invalid handshake reqeust method: " << request.RequestLine().GetMethod();
			WebSocketHandshakeError error(ss.str());
			HandleMessageError(error);
			return false;
		} 

		if (handle_routers_.find(request.RequestLine().GetUriPath()) == handle_routers_.end()) {		
			HttpFullResponse response(404, "Not Found");
			WriteHttpResponse(response);

			std::stringstream ss;
			ss << "websocket invalid handshake reqeust path: " << request.RequestLine().GetUriPath();
			WebSocketHandshakeError error(ss.str());
			HandleMessageError(error);
			return false;
		}
		current_handle_ = handle_routers_[request.RequestLine().GetUriPath()];

		std::string connection;
		if (!request.Headers().GetHeaderValue("Connection", connection) || strcasecmp(connection.c_str(), "Upgrade")) {
			HttpFullResponse http_response(400, "Bad Request", "HTTP/1.1");
			WriteHttpResponse(http_response);

			std::stringstream ss;
			ss << "websocket invalid handshake reqeust connection: " << connection;
			WebSocketHandshakeError error(ss.str());
			HandleMessageError(error);
			return false;
		}

		std::string upgrade;
		if (!request.Headers().GetHeaderValue("Upgrade", upgrade) || strcasecmp(upgrade.c_str(), "websocket")) {
			HttpFullResponse http_response(400, "Bad Request", "HTTP/1.1");
			WriteHttpResponse(http_response);

			std::stringstream ss;
			ss << "websocket invalid handshake reqeust upgrade: " << upgrade;
			WebSocketHandshakeError error(ss.str());
			HandleMessageError(error);
			return false;
		}

		std::string sec_websocket_key = "";
		if (!request.Headers().GetHeaderValue("Sec-WebSocket-Key", sec_websocket_key) || sec_websocket_key.length() == 0) {
			HttpFullResponse http_response(400, "Bad Request", "HTTP/1.1");
			WriteHttpResponse(http_response);

			std::stringstream ss;
			ss << "websocket invalid handshake reqeust sec-websocket-key: " << sec_websocket_key;
			WebSocketHandshakeError error(ss.str());
			HandleMessageError(error);
			return false;
		}

		std::string sec_websocket_accept = sec_websocket_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		unsigned char message_digest[20];
		Sha1(sec_websocket_accept.c_str(), sec_websocket_accept.length(), message_digest);
		std::string accept_key = Base64Encode(message_digest, sizeof(message_digest));

		HttpFullResponse http_response(101, "Switching Protocols", "HTTP/1.1");
		http_response.Headers().SetHeaderValue("Upgrade", "websocket");
		http_response.Headers().SetHeaderValue("Connection", "Upgrade");
		http_response.Headers().SetHeaderValue("Sec-WebSocket-Accept", accept_key);
		WriteHttpResponse(http_response);
		return true;
	}

	//解码后的报文处理函数
	bool WebSocketRequestSession::HandleMessage(Message &message) {
		if (typeid(message) == typeid(HttpFullRequest)) {    
			bool is_ok = Handshake((HttpFullRequest&)message);
			if (is_ok) {
				current_state_ = WEBSOCKET_FRAME;
				message_decoder_ = CreateDecoder();
			}
			return is_ok;
		} else {
			return current_handle_((WebSocketFrame&)message);
		}
	}

	WebSocketFullRequestSession::WebSocketFullRequestSession(SocketChannel& channel, BootstrapConfig &config) :
		WebSocketRequestSession(channel, config) {

	}

	//创建报文解码器
	MessageDecoder* WebSocketFullRequestSession::CreateDecoder() {
		delete message_decoder_;
		message_decoder_ = NULL;

		WebSocketBootstrapConfig &config = (WebSocketBootstrapConfig &)config_;
		if (current_state_ == HTTP_HANDSHAKE) {
			int max_first_line_size = config.GetMaxFirstLineSize();
			int max_header_size = config.GetMaxHeaderSize();
			int max_content_size = config.GetMaxContentSize();
			return (new HttpFullRequestDecoder(max_first_line_size, max_header_size, max_content_size));
		} else {
			int max_payload_size = config.GetMaxPayloadSize();
			return (new WebSocketFullFrameDecoder(max_payload_size, true));
		}
	}
}
