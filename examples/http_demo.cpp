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
  class HttpServer : public HttpFullRequestSession {
  public:
    HttpServer(SocketChannel &channel, BootstrapConfig &config) :
      HttpFullRequestSession(channel, config) {

      HttpRequstHandle callback1 = std::bind(&HttpServer::PostTestHandle, this, std::placeholders::_1);
      HttpRequstHandle callback2 = std::bind(&HttpServer::GetChunkedHandle, this, std::placeholders::_1);
      RegistHandleRouter("POST", "/test", callback1);
      RegistHandleRouter("GET", "/chunked", callback2);
    }

    virtual ~HttpServer() = default;
    
  private:
    bool PostTestHandle(Message &request) {
      printf("=============PostTestHandle==========\n");
      HttpFullResponse http_response(200, "OK", "HTTP/1.1");
      int length = http_response.Content().Content().WriteBytes("{\"aa\":8}", strlen("{\"aa\":8}"));
      http_response.Headers().SetHeaderValue("Connection", "close");
      http_response.Headers().SetHeaderValue("Content-Type", "application/json");
      http_response.Headers().SetHeaderValue("Content-Length", std::to_string(length));
      WriteHttpResponse(http_response, [&] {
        //printf("=============close==========\n");
        Signal(ExecuteEvent::KILL_EVENT);
      });
      return true;
    }

    bool GetChunkedHandle(Message &request) {
      //printf("=============GetChunkedHandle==========\n");
      HttpResponse http_response(200, "OK", "HTTP/1.1");
      http_response.Headers().SetHeaderValue("Connection", "keep-alive");
      http_response.Headers().SetHeaderValue("Transfer-Encoding", "chunked");
      WriteHttpResponse(http_response);

      HttpContent first_content;
      first_content.IsChunked() = true;
      first_content.Content().WriteBytes("first content,", strlen("first content,"));
      WriteHttpConent(first_content);

      HttpContent second_content;
      second_content.IsChunked() = true;
      second_content.Content().WriteBytes("second content,", strlen("second content,"));
      WriteHttpConent(second_content);

      HttpContent last_content;
      last_content.IsChunked() = true;
      last_content.IsLast() = true;
      last_content.Content().WriteBytes("last content", strlen("last content"));
      WriteHttpConent(last_content, [&] {
        //printf("=============close==========\n");
        Signal(ExecuteEvent::KILL_EVENT);
      });
      return true;
    }
  };

  class HttpClient : public HttpFullResponseSession {
  public:
    HttpClient(SocketChannel &channel, BootstrapConfig &config) :
      HttpFullResponseSession(channel, config) {
    }

    virtual ~HttpClient() = default;

    virtual bool HandleInitEvent() {
      HttpFullResponseSession::HandleInitEvent();
      HttpFullRequest http_request("GET", "/chunked", "HTTP/1.1");
      http_request.Headers().SetHeaderValue("Connection", "close");
      WriteHttpRequest(http_request, NULL);
      return true;
    }

    virtual bool HandleMessage(Message &message) {
    	HttpFullResponse &http_response = (HttpFullResponse&)message;
			printf("========%s========\n", http_response.Content().Content().ToString().c_str());
      Signal(ExecuteEvent::KILL_EVENT);
			return true;
    }
  };
}
using namespace lim;
int main() {
  Logger *logger = Logger::GetLogger("http_demo");
  SocketChannel::InitEnviroment();

  EventLoop server_event_loop;
  ExecuteThread server_execute_thread;
  EventLoopGroup worker_event_loop_group;
  ExecuteThreadGroup worke_execute_thread_group;

  HttpBootstrapConfig config(worker_event_loop_group, worke_execute_thread_group, server_event_loop, server_execute_thread);
  config.SetTimeout(30 * 1000);
  config.SetLoggerCallback([&](LoggerLevel level, const std::string &message) {
    TRACE_ERROR(logger, "%s", message.c_str());
  });
  Bootstrap strap = Bootstrap(config);
  strap.Bind<ServerChannelSession<HttpServer>>("0.0.0.0", 8095);
  strap.Connect<HttpClient>("127.0.0.1", 8095, [&](HttpClient &client) -> void {
  	
	});

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 5));
  }

  return 0;
}
