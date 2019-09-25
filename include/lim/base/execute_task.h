#ifndef LIM_EXECUTE_TASK_H
#define LIM_EXECUTE_TASK_H
#include <deque>
#include <vector>
#include <mutex>
#include <thread> 
#include <condition_variable>

namespace lim {
	namespace ExecuteEvent { 
		const int NONE_EVENT = 0;
		const int INIT_EVENT = 1;
		const int READ_EVENT = 2;
		const int WRITE_EVENT = 4;
		const int KILL_EVENT = 8;
		const int USER_EVENT = 16;
	};
	
	class ExecuteTask;
	class ExecuteThread;
	class ExecuteTimer;
	using MaxHeapUnit = std::tuple<int64_t, ExecuteTimer*, bool>; //std::tuple<timestamp, ExecuteTimer, is_in_heap>
	using TimeoutCallback = std::function<void()>;
	//定时器(底层通过最大堆实现)
	class ExecuteTimer {
	public:
		/**
		*定时器构造函数
		* @param execute_thread 定时器执行线程
		* @param callback 定时器回调函数
		*/
		ExecuteTimer(ExecuteThread &execute_thread, TimeoutCallback callback);
		virtual ~ExecuteTimer();

		void Start(int milli_sceonds = 0);
		void Cancel();
		
	private:
		ExecuteTimer(const ExecuteTimer& other) = delete;
		ExecuteTimer &operator=(const ExecuteTimer& other) = delete;

	private:
		TimeoutCallback callback_;
		MaxHeapUnit timeout_unit_;
		ExecuteThread &execute_thread_;
		friend class ExecuteThread;
	};
	
	using DeQueueUnit = std::tuple<ExecuteTask*, bool>; //std::tuple<ExecuteTask, is_in_queue>
	//可执行任务,任务对象的生命周期由宿主线程管理(如果任务对象不触发事件和超时,有可能造成内存泄漏)
	class ExecuteTask {
	public:
		/**
		*可执行任务构造函数
		* @param execute_thread 执行线程
		*/
		ExecuteTask(ExecuteThread &execute_thread);
		virtual ~ExecuteTask();
		
	private:
		ExecuteTask(const ExecuteTask& other) = delete;
		ExecuteTask &operator=(const ExecuteTask& other) = delete;

	public:
		//事件触发函数
		void Signal(int execute_events);
		//获取事件信息
		int GetEvents();
		//获取执行线程
		ExecuteThread &GetExecuteThread() { return execute_thread_; }

		//初始化
		virtual bool Initialize();

	private:
		//Kill事件处理函数
		virtual bool HandleKillEvent();
		//初始化事件处理函数
		virtual bool HandleInitEvent();
		//读事件处理函数
		virtual bool HandleReadEvent();
		//写事件处理函数
		virtual bool HandleWriteEvent();
		//自定义事件处理函数
		virtual bool HandleUserEvent(int user_events);
	
	protected:
		virtual bool Run();

	private:
		std::mutex mutex_;
		int execute_events_;  /***当前触发的事件***/
		DeQueueUnit deque_unit_; /***事件处理队列单元***/
		ExecuteThread &execute_thread_; /***绑定的执行线程***/
		friend class ExecuteThread;
  };
	
	class ExecuteThread {
	public:
		ExecuteThread();
		virtual ~ExecuteThread();

	private:
		ExecuteThread(const ExecuteThread& other) = delete;
		ExecuteThread &operator=(const ExecuteThread& other) = delete;
		
	protected:
		virtual void Run();

		void AddToQueue(DeQueueUnit *deque_unit);
		void RemoveFromQueue(DeQueueUnit *deque_unit);

		void AddToMaxHeap(MaxHeapUnit *heap_unit);
		void RemoveFromMaxHeap(MaxHeapUnit *heap_unit);

	protected:
		bool is_running_;
		std::thread thread_;
		std::mutex mutex_;
		std::condition_variable condvar_;
    
		std::vector<MaxHeapUnit*> timeout_heap_; /***定时器列表(最大堆)***/
		std::deque<DeQueueUnit*> task_que_; /***任务列表***/
		friend class ExecuteTimer;
		friend class ExecuteTask;
  };

	class ExecuteThreadGroup {
	public:
		ExecuteThreadGroup(int execute_thread_num = 0);
		virtual ~ExecuteThreadGroup();

	private:
		ExecuteThreadGroup(const ExecuteThreadGroup& other) = delete;
		ExecuteThreadGroup &operator=(const ExecuteThreadGroup& other) = delete;

	public:
		ExecuteThread &Next();
		
	protected:
		std::mutex mutex_;
		int execute_thread_num_;
		int execute_thread_index_;
		ExecuteThread **execute_threads_;
	};
}
#endif
