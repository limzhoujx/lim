#include <stdio.h>
#include <string.h>
#include <lim/base/time_utils.h>
#include <lim/base/bootstrap.h>
#include <lim/base/server_channel_session.h>
#include <lim/http/http_bootstrap_config.h>
#include <lim/http/http_request_session.h>
#include <lim/http/http_response_session.h>
#include <lim/websocket/websocket_bootstrap_config.h>
#include <lim/websocket/websocket_request_session.h>
#include <lim/websocket/websocket_response_session.h>
#include <lim/websocket/websocket_frame_message.h>
#include <lim/base/logger.h>
#include <lim/base/base64.h>
#include <iostream>
#include <fstream>

namespace lim {
  class WebSocketServer : public WebSocketFullRequestSession {
  public:
    WebSocketServer(SocketChannel &channel, BootstrapConfig &config) :
      WebSocketFullRequestSession(channel, config) {

      WebSocketFrameHandle callback = std::bind(&WebSocketServer::HandleWebSocketFrame, this, std::placeholders::_1);
      RegistHandleRouter("/test", callback);
    }

    virtual ~WebSocketServer() = default;

    virtual bool HandleWebSocketFrame(WebSocketFrame &frame) {
      printf("frame type: %d\n", frame.FrameOpCode());
      if (typeid(frame) == typeid(TextWebSocketFrame)) {
        printf("text frame: %s\n", frame.FrameContent().ToString().c_str());
        frame.FrameContent().WriteBytes(", hello", strlen(", hello"));
        WriteMessage(frame);
      } else if (typeid(frame) == typeid(PingWebSocketFrame)) {
        PongWebSocketFrame pong(true, 0);
        pong.FrameContent() = frame.FrameContent();
        WriteMessage(pong);
        printf("send pong\n");
      }
      //printf("aaaaaaaaaaaaaaaaaaa\n");
      return true;
    }
  };
  
  class WebSocketClient : public WebSocketFullResponseSession {
  public:
    WebSocketClient(SocketChannel &channel, BootstrapConfig &config) :
      WebSocketFullResponseSession(channel, config) {
      timer_ = new ExecuteTimer(GetExecuteThread(), [&]()->void {
        TextWebSocketFrame frame(true, 0);
        frame.FrameContent().WriteBytes("lim zhoujx", strlen("lim zhoujx"));
        WriteWebSocketFrame(frame);
        timer_->Start(10000);
      });
    }

    virtual ~WebSocketClient() = default;

    virtual bool HandleInitEvent() {
      WebSocketFullResponseSession::HandleInitEvent();
      DoHandshake("/test");   
      timer_->Start(1000);
      return true;
    }

    virtual bool HandleWebSocketFrame(WebSocketFrame &frame) {
      printf("client frame type: %d\n", frame.FrameOpCode());
      if (typeid(frame) == typeid(TextWebSocketFrame)) {
        printf("client text frame: %s\n", frame.FrameContent().ToString().c_str());
      } else if (typeid(frame) == typeid(PingWebSocketFrame)) {
        PongWebSocketFrame pong(true, 0);
        pong.FrameContent() = frame.FrameContent();
        WriteMessage(pong);
        printf("client send pong\n");
      }
      //printf("aaaaaaaaaaaaaaaaaaa\n");
      return true;
    }
  private:
    ExecuteTimer *timer_;
  };
}
using namespace lim;
int main() {
  
  Logger *logger = Logger::GetLogger("websocket_demo");
  SocketChannel::InitEnviroment();

  EventLoop server_event_loop;
  ExecuteThread server_execute_thread;
  EventLoopGroup worker_event_loop_group;
  ExecuteThreadGroup worke_execute_thread_group;

  WebSocketBootstrapConfig web_socket_config(worker_event_loop_group, worke_execute_thread_group, server_event_loop, server_execute_thread);
  web_socket_config.SetLoggerCallback([&](LoggerLevel level, const std::string &message) {
    TRACE_ERROR(logger, "%s", message.c_str());
  });
  Bootstrap web_socket_strap = Bootstrap(web_socket_config);
  web_socket_strap.Bind<ServerChannelSession<WebSocketServer>>("0.0.0.0", 8091);
  web_socket_strap.Connect<WebSocketClient>("127.0.0.1", 8091, [&](WebSocketClient &client) -> void {

  });

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 5));
  }

  return 0;
}