#include <lim/base/execute_task.h>
#include <lim/base/time_utils.h>
#include <assert.h>
#include <algorithm>
#include <chrono>

namespace lim {
	#define EXECUTE_TASK_SLEEP_TIME 10
	ExecuteTimer::ExecuteTimer(ExecuteThread &execute_thread, TimeoutCallback callback):
		execute_thread_(execute_thread), callback_(callback) {

		timeout_unit_ = std::make_tuple(0, this, false);
	}
			
	ExecuteTimer::~ExecuteTimer() {
		Cancel();
	}

	void ExecuteTimer::Start(int milli_sceonds) {
		std::get<0>(timeout_unit_) = CurrentMilliTime() + milli_sceonds;
		execute_thread_.AddToMaxHeap(&timeout_unit_);
	}
	
	void ExecuteTimer::Cancel() {
		execute_thread_.RemoveFromMaxHeap(&timeout_unit_);
	}
	
	ExecuteTask::ExecuteTask(ExecuteThread &execute_thread): 
		execute_thread_(execute_thread), execute_events_(ExecuteEvent::NONE_EVENT) {
			
		deque_unit_ = std::make_tuple(this, false);
  }

	ExecuteTask::~ExecuteTask() {
		execute_thread_.RemoveFromQueue(&deque_unit_);
	}

	//事件触发函数
	void ExecuteTask::Signal(int execute_events) {
		std::lock_guard<std::mutex> guard(mutex_);
		execute_events_ |= execute_events;
		execute_thread_.AddToQueue(&deque_unit_);
	}

	//获取事件信息
	int ExecuteTask::GetEvents() {
		std::lock_guard<std::mutex> guard(mutex_);
		int execute_events = execute_events_;
		execute_events_ = 0;
		return execute_events;
  }

	//初始化
	bool ExecuteTask::Initialize() {
		return true;
	}

	//Kill事件处理函数
	bool ExecuteTask::HandleKillEvent() {
		return false;
	}

	//初始化事件处理函数
	bool ExecuteTask::HandleInitEvent() {
		return true;
	}

	//读事件处理函数
	bool ExecuteTask::HandleReadEvent() {
		return true;
	}

	//写事件处理函数
	bool ExecuteTask::HandleWriteEvent() {
		return true;
	}

	//自定义事件处理函数
	bool ExecuteTask::HandleUserEvent(int user_events) {
		return true;
	}
	
	bool ExecuteTask::Run() {
		int events = GetEvents();
		if (events & ExecuteEvent::KILL_EVENT) {
			return HandleKillEvent();
		}

		if ((events & ExecuteEvent::INIT_EVENT) && !HandleInitEvent()) {
			return false;
		}
		
		if ((events & ExecuteEvent::READ_EVENT) && !HandleReadEvent()) {
			return false;
		}

		if ((events & ExecuteEvent::WRITE_EVENT) && !HandleWriteEvent()) {
			return false;
		}

		if ((events >= ExecuteEvent::USER_EVENT) && !HandleUserEvent(events)) {
			return false;
		}
		return true;
	}

	bool MaxHeapUnitCompare(const MaxHeapUnit *a, const MaxHeapUnit *b) {
		return std::get<0>(*a) > std::get<0>(*b);
	}
	
	ExecuteThread::ExecuteThread() : is_running_(true) {
		thread_ = std::thread(&ExecuteThread::Run, this);
		std::make_heap(timeout_heap_.begin(), timeout_heap_.end(), MaxHeapUnitCompare);
	}
	
	ExecuteThread::~ExecuteThread() {
		is_running_ = false;
		condvar_.notify_one();
		thread_.join();
	}

	void ExecuteThread::Run() {
		while (is_running_) {
			{
				std::unique_lock<std::mutex> lock(mutex_);
				condvar_.wait(lock, [this] {
					return (this->timeout_heap_.size() > 0 || this->task_que_.size() > 0 || this->is_running_ == false);
				});
			}

			//1.优先处理事件信息
			ExecuteTask *execute_task = NULL;
			{
				//1.1.从队列中获取事件单元
				std::unique_lock<std::mutex> guard(mutex_);
				if (task_que_.size() > 0) {
					execute_task = std::get<0>(*task_que_.front());
				}
			}
		
			if (execute_task != NULL) {
				//1.2.移除事件单元并执行,Run返回false时销毁对象
				RemoveFromQueue(&execute_task->deque_unit_);
				if (!execute_task->Run()) {
					delete execute_task;
				}
			}

			//2.执行定时器
			ExecuteTimer *execute_timer = NULL;
			{
				//2.1.从最大堆中获取超时的定时器
				std::unique_lock<std::mutex> guard(mutex_);
				if (timeout_heap_.size() > 0 && std::get<0>(*(timeout_heap_[0])) <= CurrentMilliTime()) {
					execute_timer = std::get<1>(*timeout_heap_[0]);
				}
			}

			//2.2.移除定时器并执行超时回调函数
			if (execute_timer != NULL) {
				RemoveFromMaxHeap(&execute_timer->timeout_unit_);
				if (execute_timer->callback_ != NULL) {
					execute_timer->callback_();
				}
			}

			//3.如果处理线程没有事件需要处理,进行sleep释放CPU占用
			if (execute_task == NULL) {
				std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTE_TASK_SLEEP_TIME));
			}
		}

		//4.线程退出时销毁队列中的任务, 线程不负责定时器的销毁
		auto iter = task_que_.begin();
		while (iter != task_que_.end()) {
			ExecuteTask *execute_task = std::get<0>(**iter);
			task_que_.erase(iter);
				
			delete execute_task;
			iter = task_que_.begin();
		}
	}
	
	void ExecuteThread::AddToQueue(DeQueueUnit *deque_unit) {
		std::unique_lock<std::mutex> guard(mutex_);
		if (std::get<1>(*deque_unit) == false) { //if the unit is not in the queue
			task_que_.push_back(deque_unit);
			std::get<1>(*deque_unit) = true;
			condvar_.notify_one();
		}
	}
	
	void ExecuteThread::RemoveFromQueue(DeQueueUnit *deque_unit) {
		std::unique_lock<std::mutex> guard(mutex_);
		if (std::get<1>(*deque_unit) == false) //if the unit is not in the queue
			return;
	
		for (auto iter = task_que_.begin(); iter != task_que_.end(); iter++) {
			if (*iter == deque_unit) {
				task_que_.erase(iter);
				std::get<1>(*deque_unit) = false;
				break;
			}
		}
	}
	
	void ExecuteThread::AddToMaxHeap(MaxHeapUnit *heap_unit) {
		std::unique_lock<std::mutex> guard(mutex_);
		if (std::get<2>(*heap_unit) == true) { //if the unit is in the heap
			for (auto iter = timeout_heap_.begin(); iter != timeout_heap_.end(); iter++) {
				if (*iter == heap_unit) {
					timeout_heap_.erase(iter);
					std::get<2>(*heap_unit) = false;
					//std::make_heap(timeout_heap_.begin(), timeout_heap_.end(), MaxHeapUnitCompare);
					break;
				}
			}
		}
	
		timeout_heap_.push_back(heap_unit);
		std::get<2>(*heap_unit) = true;
		push_heap(timeout_heap_.begin(), timeout_heap_.end(), MaxHeapUnitCompare);
		condvar_.notify_one();
	}
	
	void ExecuteThread::RemoveFromMaxHeap(MaxHeapUnit *heap_unit) {
		std::unique_lock<std::mutex> guard(mutex_);
		if (std::get<2>(*heap_unit) == false) //if the unit is not in the heap
			return;
	
		for (auto iter = timeout_heap_.begin(); iter != timeout_heap_.end(); iter++) {
			if (*iter == heap_unit) {
				timeout_heap_.erase(iter);
				std::get<2>(*heap_unit) = false;
				std::make_heap(timeout_heap_.begin(), timeout_heap_.end(), MaxHeapUnitCompare);
				break;
			}
		}
	}
		
	ExecuteThreadGroup::ExecuteThreadGroup(int execute_thread_num): 
		execute_thread_num_(execute_thread_num), execute_thread_index_(0) {
		if (execute_thread_num_ <= 0) {
      execute_thread_num_ = 2*std::thread::hardware_concurrency();
    }
		
		execute_threads_ = new ExecuteThread*[execute_thread_num_];
    assert(execute_threads_);

    for (int i = 0; i < execute_thread_num_; i++) {
      execute_threads_[i] = new ExecuteThread();
      assert(execute_threads_[i]);
    }
	}

	ExecuteThreadGroup::~ExecuteThreadGroup() {
		for (int i = 0; i < execute_thread_num_; i++) {
			delete execute_threads_[i];
			execute_threads_[i] = NULL;
    }
    delete[]execute_threads_;
	}

	ExecuteThread &ExecuteThreadGroup::Next() {
		std::lock_guard<std::mutex> guard(mutex_);
		int index = execute_thread_index_;
		execute_thread_index_ = (execute_thread_index_ + 1) % execute_thread_num_;
		return (*execute_threads_[index]);
	}
}

