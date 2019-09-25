#ifndef LIM_WEBSOCKET_BOOTSTRAP_CONFIG_H
#define LIM_WEBSOCKET_BOOTSTRAP_CONFIG_H
#include <map>
#include <lim/http/http_bootstrap_config.h>

namespace lim {
	class WebSocketBootstrapConfig : public HttpBootstrapConfig {
	public:
		WebSocketBootstrapConfig(EventLoopGroup &event_loop_group, ExecuteThreadGroup &execute_thread_group):
			HttpBootstrapConfig(event_loop_group, execute_thread_group), max_payload_size_(1024 * 1024 * 4) {

		}
		WebSocketBootstrapConfig(EventLoopGroup &event_loop_group,
                            ExecuteThreadGroup &execute_thread_group,
                            EventLoop &server_event_loop,
                            ExecuteThread &server_execute_thread) :
			HttpBootstrapConfig(event_loop_group, execute_thread_group, server_event_loop, server_execute_thread),
			max_payload_size_(1024 * 1024 * 4) {
      SetTimeout(-1);
		}
		virtual ~WebSocketBootstrapConfig() = default;

		void SetMaxPayloadSize(int max_payload_size) { max_payload_size_ = max_payload_size_; }
		int GetMaxPayloadSize() { return max_payload_size_; }

	protected:
		int max_payload_size_; /***websocket报文最大长度***/
	};
}
#endif

