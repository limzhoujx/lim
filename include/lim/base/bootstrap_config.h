#ifndef LIM_BOOTSTRAP_CONFIG_H
#define LIM_BOOTSTRAP_CONFIG_H
#include <lim/base/event_loop.h>
#include <lim/base/execute_task.h>

namespace lim {
  enum class LoggerLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR };
  /**
  * 日志回调函数申明
  * @param [in] LoggerLevel 日志等级
  * @param [in] string 日志内容
  */
  typedef std::function<void(LoggerLevel, const std::string&)> LoggerCallback;
  
  class BootstrapConfig {
  public:
    BootstrapConfig(EventLoopGroup &event_loop_group, ExecuteThreadGroup &execute_thread_group):
      event_loop_group_(event_loop_group), execute_thread_group_(execute_thread_group),
      server_event_loop_(event_loop_group.Next()), server_execute_thread_(execute_thread_group.Next()),
      max_buffer_size_(1024*1024*4), logger_callback_(NULL), timeout_millisec_(60*1000) {
    }

    BootstrapConfig(EventLoopGroup &event_loop_group,
                  ExecuteThreadGroup &execute_thread_group,
                  EventLoop &server_event_loop,
                  ExecuteThread &server_execute_thread) :
      event_loop_group_(event_loop_group), execute_thread_group_(execute_thread_group),
      server_event_loop_(server_event_loop), server_execute_thread_(server_execute_thread),
      max_buffer_size_(1024 * 1024 * 4), logger_callback_(NULL), timeout_millisec_(60 * 1000) {

    }

    virtual ~BootstrapConfig() = default;

    //获取一个可用的事件监听器(用于connect/accept对象)
    EventLoop &NextEventLoop() { return event_loop_group_.Next(); }
    //获取一个可用的工作线程(用于connect/accept对象)
    ExecuteThread &NextExecuteThread() { return execute_thread_group_.Next(); }

    //获取事件监听器(用于listen对象)
    EventLoop &ServerEventLoop() { return server_event_loop_; }
    //获取工作线程(用于listen对象)
    ExecuteThread &ServerExecuteThread() { return server_execute_thread_; }

    //设置日志回调函数
    void SetLoggerCallback(LoggerCallback callback) { logger_callback_ = callback; }
    //获取日志回调函数
    LoggerCallback GetLoggerCallback() { return logger_callback_; }

    //设置最大接收缓存大小
    void SetMaxBufferSize(int max_buffer_size) { max_buffer_size_ = max_buffer_size; }
    //获取最大接收缓存大小
    int GetMaxBufferSize() { return max_buffer_size_; }

    //设置超时时间(毫秒)
    void SetTimeout(int timeout_millisec) { timeout_millisec_ = timeout_millisec; }
    //获取超时时间(毫秒)
    int GetTimeout() { return timeout_millisec_; }

	protected:
    int max_buffer_size_; /***最大接收缓存大小***/
    int timeout_millisec_; /***超时时间(毫秒),-1为不启用超时***/
    LoggerCallback logger_callback_; /***日志回调函数***/
		
    EventLoopGroup &event_loop_group_; /***事件监听器数组(用于connect/accept对象)***/
    ExecuteThreadGroup &execute_thread_group_; /***工作线程池(用于connect/accept对象)***/
		
    EventLoop &server_event_loop_; /***事件监听器(用于listen对象)***/
    ExecuteThread &server_execute_thread_; /***工作线程(用于listen对象)***/  
  };
}
#endif
