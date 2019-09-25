#ifndef LIM_EVENT_LOOP_H
#define LIM_EVENT_LOOP_H
#ifdef _WIN32
#include <windows.h>
#endif
#include <lim/base/socket_channel.h>
#include <lim/base/execute_task.h>
#include <map>
#include <mutex>
#include <thread> 

namespace lim {
	class EventLoop: public ExecuteThread {
	public:
		EventLoop();
		virtual ~EventLoop();
		
	private:
		EventLoop(const EventLoop& other) = delete;
		EventLoop &operator=(const EventLoop& other) = delete;

	public:
		void AddChannel(const SocketChannel &channel, ExecuteTask *execute_task, bool has_write_op=false);
		void RemoveChannel(const SocketChannel &channel);
		
	private:
		void ClearChannels();
		virtual void Run();

	private:
#ifdef _WIN32
		fd_set read_channel_set_;
		fd_set write_channel_set_;
		int max_socket_channel_;
#else
		int fd_epoll_;
#endif
		std::mutex mutex_;
		std::map<int, ExecuteTask*> channel_task_map_;

		bool is_running_;
		std::thread io_thread_;
  };

	class EventLoopGroup {
	public:
		EventLoopGroup(int event_loop_num = 0);
		virtual ~EventLoopGroup();

	private:
		EventLoopGroup(const EventLoopGroup& other) = delete;
		EventLoopGroup &operator=(const EventLoopGroup& other) = delete;

	public:
		EventLoop &Next();
		
	protected:
		std::mutex mutex_;
		int event_loop_num_;
		int event_loop_index_;
		EventLoop **event_loops_;
	};
}
#endif
