#ifndef _LIM_LOGGER_
#define _LIM_LOGGER_
#include <string>
#include <mutex>

namespace lim {
	class Logger {
	public:
		enum { LOG_LEVEL_DEBUG = 1, LOG_LEVEL_INFO = 2, LOG_LEVEL_WARN = 4, LOG_LEVEL_ERROR = 8 };
		~Logger();

	private:
		Logger();
		Logger(const Logger &other) = delete;
		Logger &operator=(const Logger &other) = delete;

	public:
		//获取日志对象
		static Logger *GetLogger(const std::string &log_name);
		/**
		*输出日志
		* @param file 代码文件名称
		* @param line 代码文件行号
		* @param level 日志等级
		* @param format 日志输出格式
		*/
		void Trace(const std::string &file, int line, int level, const char *format, ...);

		//设置日志文件输出方式包含的日志等级类别
		void SetLogFileLevel(int log_file_level);
		//设置控制台输出方式包含的日志等级类别
		void SetLogConsoleLevel(int log_console_level);
		//设置日志文件路径
		bool SetLogFilePath(const std::string &log_path);

  private:
		//初始化
		bool Initialize(const std::string &log_name);
		//滚动日志文件
		void RollFile(const std::string day_time);
		//按天和文件大小滚动日志文件
		void LogRollOver();
  private:
		std::mutex mutex_;
		FILE * log_file_fd_; /***日志文件句柄***/
		std::string log_path_; /***日志文件路径***/
		std::string log_name_; /***日志文件名称***/
		
		int log_file_level_; /***文件输出日志等级***/
		int log_console_level_; /***控制台输出日志等级***/
		
		int log_rotate_max_file_size_; /***单个日志文件最大字节数***/
		int log_rotate_max_file_num_; /***每天日志文件的最大个数***/
		std::string last_day_time_; /***上一次日期***/
	};

#define		TRACE_DEBUG(logger, ...)		logger->Trace(__FILE__, __LINE__, Logger::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define		TRACE_INFO(logger, ...)		logger->Trace(__FILE__, __LINE__, Logger::LOG_LEVEL_INFO, __VA_ARGS__)
#define		TRACE_WARN(logger, ...)	logger->Trace(__FILE__, __LINE__, Logger::LOG_LEVEL_WARN, __VA_ARGS__)
#define		TRACE_ERROR(logger, ...)	logger->Trace(__FILE__, __LINE__, Logger::LOG_LEVEL_ERROR, __VA_ARGS__)
}

#endif

