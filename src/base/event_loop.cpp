#include <lim/base/event_loop.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/epoll.h>
#include <unistd.h>
#endif
#include <assert.h>

namespace lim {
	#define MAX_SOCKET_CHANNEL_NUM 1024
	#define EVENT_TIMEOUT_MILLSEC 10
	EventLoop::EventLoop(): is_running_(true) {
#ifdef _WIN32
		FD_ZERO(&read_channel_set_);
		FD_ZERO(&write_channel_set_);
		max_socket_channel_ = 0;
#else
		fd_epoll_ = epoll_create(MAX_SOCKET_CHANNEL_NUM);
		assert(fd_epoll_ != 0);
#endif
		io_thread_ = std::thread(&EventLoop::Run, this);
	}

	EventLoop::~EventLoop() {
		is_running_ = false;
		io_thread_.join();
#ifndef _WIN32
		close(fd_epoll_);
#endif
	}

	void EventLoop::AddChannel(const SocketChannel &channel, ExecuteTask *execute_task, bool has_write_op) {
		std::lock_guard<std::mutex> guard(mutex_);
#ifdef _WIN32
		if (channel.socket_channel_ > max_socket_channel_) {
			max_socket_channel_ = channel.socket_channel_;
		}
		
		if (!FD_ISSET(channel.socket_channel_, &read_channel_set_)) {
			FD_SET(channel.socket_channel_, &read_channel_set_);
		}

		if (has_write_op && !FD_ISSET(channel.socket_channel_, &write_channel_set_))
			FD_SET(channel.socket_channel_, &write_channel_set_);
		else if (!has_write_op && FD_ISSET(channel.socket_channel_, &write_channel_set_)) 
			FD_CLR(channel.socket_channel_, &write_channel_set_);
#else
		struct epoll_event ev;
		ev.data.fd = channel.socket_channel_;
		ev.events = EPOLLET|EPOLLIN;
		if (has_write_op) {
			ev.events |= EPOLLOUT;
		}

		if (channel_task_map_.find(channel.socket_channel_) == channel_task_map_.end())
			epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, channel.socket_channel_, &ev);
		else
			epoll_ctl(fd_epoll_, EPOLL_CTL_MOD, channel.socket_channel_, &ev);
#endif

		if (channel_task_map_.find(channel.socket_channel_) == channel_task_map_.end()) {
			channel_task_map_.insert(std::make_pair(channel.socket_channel_, execute_task));
		}
	}

	void EventLoop::RemoveChannel(const SocketChannel &channel) {
		std::lock_guard<std::mutex> guard(mutex_);
		auto iter = channel_task_map_.find(channel.socket_channel_);
		if (iter == channel_task_map_.end()) {
			return;
		}
		
#ifdef _WIN32
		if (channel.socket_channel_ == max_socket_channel_) {
			max_socket_channel_ --;
		}
		
		if (FD_ISSET(channel.socket_channel_, &read_channel_set_)) {
			FD_CLR(channel.socket_channel_, &read_channel_set_);
		}

		if (FD_ISSET(channel.socket_channel_, &write_channel_set_)) {
			FD_CLR(channel.socket_channel_, &write_channel_set_);
		}
#else
		struct epoll_event ev;
		ev.data.fd = channel.socket_channel_;
		ev.events = EPOLLET|EPOLLIN|EPOLLOUT;
		epoll_ctl(fd_epoll_, EPOLL_CTL_DEL, channel.socket_channel_, &ev);
#endif
		channel_task_map_.erase(iter);
	}

	void EventLoop::ClearChannels() {
		auto iter = channel_task_map_.begin();
		while (iter != channel_task_map_.end()) {
			SocketChannel channel(iter->first);
			ExecuteTask *execute_task = iter->second;
			channel_task_map_.erase(iter);
				
			delete execute_task;
			channel.Close();
			iter = channel_task_map_.begin();
		}
	}
	
	void EventLoop::Run() {
#ifdef _WIN32
		int max_socket_channel;
		fd_set read_channel_set, write_channel_set;
		while (is_running_) {
			{
				std::lock_guard<std::mutex> guard(mutex_);
				max_socket_channel = max_socket_channel_;
				memcpy(&read_channel_set, &read_channel_set_, sizeof(fd_set));
				memcpy(&write_channel_set, &write_channel_set_, sizeof(fd_set));
			}
	
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = EVENT_TIMEOUT_MILLSEC * 1000;
			int num = select(max_socket_channel + 1, &read_channel_set, &write_channel_set, NULL, &tv);
			if (num <= 0) { //error[<0] or timeout[==0]
				continue;
			}

			std::lock_guard<std::mutex> guard(mutex_);
			for (auto iter = channel_task_map_.begin(); iter != channel_task_map_.end(); iter++) {
				int execute_events = ExecuteEvent::NONE_EVENT;
				ExecuteTask *execute_task = iter->second;
				if (FD_ISSET(iter->first, &read_channel_set) && FD_ISSET(iter->first, &read_channel_set_))
					execute_events |= ExecuteEvent::READ_EVENT;
				if (FD_ISSET(iter->first, &write_channel_set) && FD_ISSET(iter->first, &write_channel_set_))
					execute_events |= ExecuteEvent::WRITE_EVENT;

				if (execute_events != ExecuteEvent::NONE_EVENT)
					execute_task->Signal(execute_events);
			}
		}
#else
		struct epoll_event events[MAX_SOCKET_CHANNEL_NUM];
		while (is_running_) {
			int num = epoll_wait(fd_epoll_, events, MAX_SOCKET_CHANNEL_NUM, EVENT_TIMEOUT_MILLSEC);
			for (int i = 0; i < num; i++) {
				if (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)) {
					continue;
				}
	
				std::lock_guard<std::mutex> guard(mutex_);
				auto iter = channel_task_map_.find(events[i].data.fd);
				if (iter == channel_task_map_.end()) {
					struct epoll_event ev;
					ev.data.fd = events[i].data.fd;
					ev.events = EPOLLIN | EPOLLOUT;
					epoll_ctl(fd_epoll_, EPOLL_CTL_DEL, events[i].data.fd, &ev);
					continue;
				}

				int execute_events = ExecuteEvent::NONE_EVENT;
				ExecuteTask *execute_task = iter->second;
				if (events[i].events & EPOLLIN)
					execute_events |= ExecuteEvent::READ_EVENT;
				if (events[i].events & EPOLLOUT)
					execute_events |= ExecuteEvent::WRITE_EVENT;
				
				if (execute_events != ExecuteEvent::NONE_EVENT) {
					//printf("event: %d\n", execute_events);
					execute_task->Signal(execute_events);
				}
			}
		}
#endif		
		ClearChannels();
	}

	EventLoopGroup::EventLoopGroup(int event_loop_num): 
		event_loop_num_(event_loop_num), event_loop_index_(0) {
		if (event_loop_num_ <= 0) {
			event_loop_num_ = std::thread::hardware_concurrency();
		}
		
		event_loops_ = new EventLoop*[event_loop_num_];
		assert(event_loops_);

		for (int i = 0; i < event_loop_num_; i++) {
			event_loops_[i] = new EventLoop();
			assert(event_loops_[i]);
		}
	}

	EventLoopGroup::~EventLoopGroup() {
		for (int i = 0; i < event_loop_num_; i++) {
			delete event_loops_[i];
			event_loops_[i] = NULL;
		}
		delete[]event_loops_;
	}

	EventLoop &EventLoopGroup::Next() {
		std::lock_guard<std::mutex> guard(mutex_);
		int index = event_loop_index_;
		event_loop_index_ = (event_loop_index_ + 1) % event_loop_num_;
		return (*event_loops_[index]);
	}
}

