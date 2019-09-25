#ifndef LIM_WEBSOCKET_RESPONSE_SESSION_H
#define LIM_WEBSOCKET_RESPONSE_SESSION_H
#include <lim/websocket/websocket_frame_decoder.h>
#include <lim/http/http_response_session.h>

namespace lim {
	class WebSocketResponseSession: public HttpFullResponseSession {
	protected:
		enum { HTTP_HANDSHAKE, WEBSOCKET_FRAME };
	public:
    WebSocketResponseSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~WebSocketResponseSession() = default;

		/**
		*发送websocket frame报文
		* @param websocket frame响应报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteWebSocketFrame(WebSocketFrame &frame, WriteCompleteCallback callback = NULL);
			
	protected:
		//发送握手报文
		void DoHandshake(const std::string &path, std::map< std::string, std::string> *header = NULL);
		
	private:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
		//解码后的报文处理函数
		virtual bool HandleMessage(Message &message);
		//握手处理
		bool Handshake(HttpFullResponse& response);

		//握手成功后
		virtual void HandleHandshaked();
		//websocket frame报文处理
		virtual bool HandleWebSocketFrame(WebSocketFrame&) = 0;

	protected:
		int current_state_; /***当前状态***/
		std::string expected_accept_key_; /***握手报文后期望的Sec-Websocket-Accept值***/
	};
	
	class WebSocketFullResponseSession: public WebSocketResponseSession {
	public:
    WebSocketFullResponseSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~WebSocketFullResponseSession() = default;

	protected:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
	};
}
#endif
