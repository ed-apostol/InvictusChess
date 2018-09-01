/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include "utils.h"

enum LogLevel {
	logIN, logOUT, logINFO
};

struct LogToFile : public std::ofstream {
	static LogToFile& Inst() {
		static LogToFile logger;
		return logger;
	}
	LogToFile(const std::string& f = "invictus.log") : std::ofstream(f.c_str(), std::ios::app) {}
	~LogToFile() {
		if (is_open()) close();
	}
};

template <LogLevel level, bool out = true, bool logtofile = false>
class Log {
public:
	Log() {}
	template <typename T>
	Log& operator << (const T& object) {
		_buffer << object;
		return *this;
	}
	~Log() {
		static const std::string LevelText[7] = { "->", "<-", "==" };
		_buffer << "\n";
		static spinlock_t splock;
		std::lock_guard<spinlock_t> lock(splock);
		if (out) std::cout << _buffer.str();
		if (logtofile) LogToFile::Inst() << Utils::getTime() << " " << LevelText[level] << " " << _buffer.str();
	}
private:
	std::ostringstream _buffer;
};

typedef Log<logIN> PrinInput;
typedef Log<logOUT> PrintOutput;
typedef Log<logINFO> PrintInfo;
typedef Log<logIN, false, true> LogInput;
typedef Log<logOUT, false, true> LogOutput;
typedef Log<logINFO, false, true> LogInfo;
typedef Log<logIN, true, true> LogAndPrintInput;
typedef Log<logOUT, true, true> LogAndPrintOutput;
typedef Log<logINFO, true, true> LogAndPrintInfo;
