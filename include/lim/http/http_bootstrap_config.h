#ifndef LIM_HTTP_BOOTSTRAP_CONFIG_H
#define LIM_HTTP_BOOTSTRAP_CONFIG_H
#include <map>
#include <lim/base/bootstrap_config.h>

namespace lim {
	class HttpBootstrapConfig : public BootstrapConfig {
	public:
		HttpBootstrapConfig(EventLoopGroup &event_loop_group, ExecuteThreadGroup &execute_thread_group) :
			BootstrapConfig(event_loop_group, execute_thread_group),
			max_first_line_size_(1024 * 8), max_header_size_(1024 * 64),
			max_content_size_(1024 * 1024 * 4) {

    }
		HttpBootstrapConfig(EventLoopGroup &event_loop_group,
															ExecuteThreadGroup &execute_thread_group,
															EventLoop &server_event_loop,
															ExecuteThread &server_execute_thread) :
			BootstrapConfig(event_loop_group, execute_thread_group, server_event_loop, server_execute_thread),
			max_first_line_size_(1024 * 8), max_header_size_(1024 * 64), max_content_size_(1024 * 1024 * 4),
			user_agent_("lim-http-client"), server_name_("lim-server") {

    }
		virtual ~HttpBootstrapConfig() = default;

		void SetMaxFirstLineSize(int max_first_line_size) { max_first_line_size_ = max_first_line_size; }
		int GetMaxFirstLineSize() { return max_buffer_size_; }

		void SetMaxHeaderSize(int max_header_size) { max_header_size_ = max_header_size; }
		int GetMaxHeaderSize() { return max_header_size_; }

		void SetMaxContentSize(int max_content_size) { max_content_size_ = max_content_size; }
		int GetMaxContentSize() { return max_content_size_; }

		void SetUserAgent(std::string user_agent) { user_agent_ = user_agent; }
    std::string GetUserAgent() { return user_agent_; }

		void SetServerName(std::string server_name) { server_name_ = server_name; }
    std::string GetServerName() { return server_name_; }
		
	protected:
		int max_first_line_size_; /***首行数据最大长度***/
		int max_header_size_; /***header最大长度***/
		int max_content_size_; /***content最大长度***/
		std::string user_agent_; /***User-Agent字段名称***/
		std::string server_name_; /***Server字段名称***/
  };
}
#endif

