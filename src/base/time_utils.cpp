#include <lim/base/time_utils.h>
#include <chrono>
#include <string>
#include <time.h>
#include <sys/timeb.h>

namespace lim {
  int64_t CurrentMilliTime() {
		auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
  }

	std::string GetCurrentTimeString(const char* format) {
		int64_t millisec = CurrentMilliTime();
		return TimeToString(millisec, format);
	}
	
	std::string TimeToString(uint64_t millisec, const char *format) {
		struct tm tm_time;
		struct timeb tb_time;
		char time_string[128] = {0};

		ftime(&tb_time);
		tb_time.time = millisec /1000;
		tb_time.millitm = millisec %1000;
	
		const int kArrayDaysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		//当前时间加上时区
		if (tb_time.dstflag == 0) {
			tb_time.time -= tb_time.timezone*60;
		}
	
		//取秒时间
		tm_time.tm_sec = tb_time.time%60;
	
		//取分钟时间
		tm_time.tm_min = (tb_time.time/60)%60;

		//取过去多少个小时
		int total_hours = (int)(tb_time.time/3600);

		//取过去多少个四年
		int four_year_count = total_hours /((365*4+1) * 24L);
	
		//计算年份
		tm_time.tm_year = (four_year_count << 2)+70;
		
		//四年中剩下的小时数
		int left_hours = total_hours %((365*4+1) * 24L);

		//取一星期的第几天(1970-1-1是4)
		tm_time.tm_wday = (total_hours/24 + 4) % 7;
	
  
		//校正闰年影响的年份，计算一年中剩下的小时数
		for (;;) {
			//一年的小时数
			int total_hours_per_year = 365 * 24;

			//判断闰年
			if ((tm_time.tm_year & 3) == 0) {
				//是闰年，一年则多24小时，即一天
				total_hours_per_year += 24;
			}

			if (left_hours < total_hours_per_year) {
				break;
			}
		
			tm_time.tm_year++;
			left_hours -= total_hours_per_year;
		}

		//取小时时间
		tm_time.tm_hour = left_hours%24;
	
		//一年中剩下的天数
		int left_days_of_one_year = left_hours/24;

		//假定为闰年
		left_days_of_one_year++;

		//取一年的第几天(假定为非闰年)
		tm_time.tm_yday = left_days_of_one_year % 365;
	
		//校正润年的误差，计算月份，日期
		if ((tm_time.tm_year & 3) == 0)	{
			tm_time.tm_yday --;
		
			if (left_days_of_one_year > 60) {
				left_days_of_one_year--;
			} else {
				if (left_days_of_one_year == 60) {
					//计算月日
					tm_time.tm_mon = 1;
					tm_time.tm_mday = 29;

					if (format == NULL) {
						sprintf(time_string, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
							tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, 
							tm_time.tm_min, tm_time.tm_sec, tb_time.millitm);
 					} else {
						sprintf(time_string, format, 
							tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, 
							tm_time.tm_min, tm_time.tm_sec, tb_time.millitm);
 					}
					return time_string;
				}
			}
		}

		//计算月日
		for (tm_time.tm_mon = 0; kArrayDaysPerMonth[tm_time.tm_mon] < left_days_of_one_year; tm_time.tm_mon++) {
			left_days_of_one_year -= kArrayDaysPerMonth[tm_time.tm_mon];
		}
		tm_time.tm_mday = left_days_of_one_year;
	
		if (format == NULL) {
			sprintf(time_string, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
				tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, 
				tm_time.tm_min, tm_time.tm_sec, tb_time.millitm);
 		} else {
			sprintf(time_string, format, 
				tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, 
				tm_time.tm_min, tm_time.tm_sec, tb_time.millitm);
 		}
		return time_string;
	}
}
