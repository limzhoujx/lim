#include <lim/websocket/websocket_response_session.h>
#include <lim/websocket/websocket_bootstrap_config.h>
#include <lim/http/http_response_session.h>
#include <lim/base/string_utils.h>
#include <lim/base/base64.h>
#include <lim/base/sha1.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <time.h>

namespace lim {
	WebSocketResponseSession::WebSocketResponseSession(SocketChannel& channel, BootstrapConfig &config) :
		HttpFullResponseSession(channel, config), current_state_(HTTP_HANDSHAKE) {

	}

	/**
	*发送websocket frame报文
	* @param websocket frame响应报文
	* @param callback 发送结束后的回调函数
	* @return 失败返回false, 成功返回true
	*/
	bool WebSocketResponseSession::WriteWebSocketFrame(WebSocketFrame &frame, WriteCompleteCallback callback) {
		frame.FrameMasked() = true;
		return WriteMessage(frame, callback);
	}
	
	//创建报文解码器
	MessageDecoder* WebSocketResponseSession::CreateDecoder() {
		delete message_decoder_;
		message_decoder_ = NULL;

		WebSocketBootstrapConfig &config = (WebSocketBootstrapConfig &)config_;
		if (current_state_ == HTTP_HANDSHAKE) {
			int max_first_line_size = config.GetMaxFirstLineSize();
			int max_header_size = config.GetMaxHeaderSize();
			int max_content_size = config.GetMaxContentSize();
			return (new HttpFullResponseDecoder(max_first_line_size, max_header_size, max_content_size));
		} else {
			int max_payload_size = config.GetMaxPayloadSize();
			return (new WebSocketFrameDecoder(max_payload_size, false));
		}
	}

	//发送握手报文
  void WebSocketResponseSession::DoHandshake(const std::string &path, std::map< std::string, std::string> *header) {
		//generate sec_websocket_key
		std::srand((unsigned int)(time(NULL)));
		char nonce[16] = {0};
		for (int i = 0; i < sizeof(nonce); i++) {
			nonce[i] = (char)(std::rand()%255);
		}

		std::string sec_websocket_key = Base64Encode((uint8_t*)nonce, sizeof(nonce));
    std::string sec_websocket_accept = sec_websocket_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char message_digest[20];
		Sha1(sec_websocket_accept.c_str(), sec_websocket_accept.length(), message_digest);
		expected_accept_key_ = Base64Encode(message_digest, sizeof(message_digest));
		
		HttpFullRequest request("GET", path, "HTTP/1.1");
		request.Headers().SetHeaderValue("Connection", "Upgrade");
		request.Headers().SetHeaderValue("Upgrade", "websocket");
		request.Headers().SetHeaderValue("Sec-WebSocket-Key", sec_websocket_key);
		request.Headers().SetHeaderValue("Sec-WebSocket-Version", "13");
    if (header != NULL) {
      for (std::map<std::string, std::string>::iterator iter = header->begin(); iter != header->end(); iter++) {
        request.Headers().SetHeaderValue(iter->first, iter->second);
      }
    }
    
		WriteHttpRequest(request, NULL);
	}
	
	//握手处理
	bool WebSocketResponseSession::Handshake(HttpFullResponse& response) {
		if (response.StatusLine().GetStatusCode() != 101) {
			std::stringstream ss;
			ss << "websocket invalid handshake response getStatus: " << response.StatusLine().GetStatusCode();
			WebSocketHandshakeError error_message(ss.str());
			HandleMessageError(error_message);
			return false;
		}

		std::string connection;
		if (!response.Headers().GetHeaderValue("Connection", connection) || strcasecmp(connection.c_str(), "Upgrade")) {
			std::stringstream ss;
			ss << "websocket invalid handshake response connection: " << connection;
			WebSocketHandshakeError error_message(ss.str());
			HandleMessageError(error_message);
			return false;
		}

		std::string upgrade;
		if (!response.Headers().GetHeaderValue("Upgrade", upgrade) || strcasecmp(upgrade.c_str(), "websocket")) {
			std::stringstream ss;
			ss << "websocket invalid handshake response upgrade: " << upgrade;
			WebSocketHandshakeError error_message(ss.str());
			HandleMessageError(error_message);
			return false;
		}

		std::string sec_websocket_accept;
		if (!response.Headers().GetHeaderValue("Sec-Websocket-Accept", sec_websocket_accept) || strcmp(sec_websocket_accept.c_str(), expected_accept_key_.c_str())) {
      std::stringstream ss;
			ss << "invalid handshake response sec-websocket-accept: " << sec_websocket_accept;
			WebSocketHandshakeError error_message(ss.str());
			HandleMessageError(error_message);
			return false;
		}

		return true;
	}

  //握手成功后
  void WebSocketResponseSession::HandleHandshaked() {

  }

	//解码后的报文处理函数
	bool WebSocketResponseSession::HandleMessage(Message &message) {
		if (typeid(message) == typeid(HttpFullResponse)) {    
			bool is_ok = Handshake((HttpFullResponse&)message);
			if (is_ok) {
				current_state_ = WEBSOCKET_FRAME;
				message_decoder_ = CreateDecoder();
				HandleHandshaked();
			}
			return is_ok;
		} else {
			return HandleWebSocketFrame((WebSocketFrame&)message);
		}
	}

	WebSocketFullResponseSession::WebSocketFullResponseSession(SocketChannel& channel, BootstrapConfig &config) :
		WebSocketResponseSession(channel, config) {

	}

	//创建报文解码器
	MessageDecoder* WebSocketFullResponseSession::CreateDecoder() {
		delete message_decoder_;
		message_decoder_ = NULL;

		WebSocketBootstrapConfig &config = (WebSocketBootstrapConfig &)config_;
		if (current_state_ == HTTP_HANDSHAKE) {
			int max_first_line_size = config.GetMaxFirstLineSize();
			int max_header_size = config.GetMaxHeaderSize();
			int max_content_size = config.GetMaxContentSize();
			return (new HttpFullResponseDecoder(max_first_line_size, max_header_size, max_content_size));
		} else {
			int max_payload_size = config.GetMaxPayloadSize();
			return (new WebSocketFullFrameDecoder(max_payload_size, false));
		}
	}
}
