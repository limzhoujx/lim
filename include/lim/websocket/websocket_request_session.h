#ifndef LIM_WEBSOCKET_REQUEST_SESSION_H
#define LIM_WEBSOCKET_REQUEST_SESSION_H
#include <lim/websocket/websocket_frame_decoder.h>
#include <lim/http/http_request_session.h>

namespace lim {
	typedef std::function<bool(WebSocketFrame&)> WebSocketFrameHandle;
	class WebSocketRequestSession: public HttpFullRequestSession {
	protected:
		enum { HTTP_HANDSHAKE, WEBSOCKET_FRAME };
	public:
		WebSocketRequestSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~WebSocketRequestSession() = default;

		/**
		*发送websocket frame报文
		* @param websocket frame响应报文
		* @param callback 发送结束后的回调函数
		* @return 失败返回false, 成功返回true
		*/
		bool WriteWebSocketFrame(WebSocketFrame &frame, WriteCompleteCallback callback = NULL);
		
	protected:
		/**
		*注册处理路由
		* @param path 路径
		* @param handle 回调函数
		*/
		bool RegistHandleRouter(const std::string &path, WebSocketFrameHandle &handle);

	private:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
		//解码后的报文处理函数
		virtual bool HandleMessage(Message &message);
		//握手处理
		bool Handshake(HttpFullRequest& request);

	protected:
		int current_state_; /***当前状态***/
		//std::map<path, handle>
		std::map<std::string, WebSocketFrameHandle> handle_routers_;
		WebSocketFrameHandle current_handle_;
	};
	
	class WebSocketFullRequestSession: public WebSocketRequestSession {
	public:
		WebSocketFullRequestSession(SocketChannel &channel, BootstrapConfig &config);
		virtual ~WebSocketFullRequestSession() = default;

	protected:
		//创建报文解码器
		virtual MessageDecoder *CreateDecoder();
	};
}
#endif
