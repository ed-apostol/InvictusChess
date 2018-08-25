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
	logNONE = 0, logIN, logOUT, logERROR, logWARNING, logINFO, logDEBUG
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
	static const LogLevel ClearanceLevel = logDEBUG;
	Log() {}
	template <typename T>
	Log& operator << (const T& object) {
		if (level > logNONE && level <= ClearanceLevel) _buffer << object;
		return *this;
	}
	~Log() {
		if (level > logNONE && level <= ClearanceLevel) {
			static const std::string LevelText[7] = { "", "inp", "out", "err", "wrn", "inf", "dbg" };
			_buffer << "\n";
			if (out) {
				if (level == logOUT) std::cout << _buffer.str();
				else std::cout << LevelText[level] << " " << _buffer.str();
			}
			if (logtofile) {
				static spinlock_t splck;
				std::lock_guard<spinlock_t> lock(splck);
				LogToFile::Inst() << LevelText[level] << " " << Utils::getTime() << " " << _buffer.str();
			}
		}
	}
private:
	std::ostringstream _buffer;
};

typedef Log<logIN> PrinInput;
typedef Log<logOUT> PrintOutput;
typedef Log<logERROR> PrintError;
typedef Log<logWARNING> PrintWarning;
typedef Log<logINFO> PrintInfo;
typedef Log<logDEBUG> PrintDebug;
typedef Log<logIN, false, true> LogInput;
typedef Log<logOUT, false, true> LogOutput;
typedef Log<logERROR, false, true> LogError;
typedef Log<logWARNING, false, true> LogWarning;
typedef Log<logINFO, false, true> LogInfo;
typedef Log<logDEBUG, false, true> LogDebug;
typedef Log<logIN, true, true> LogAndPrintInput;
typedef Log<logOUT, true, true> LogAndPrintOutput;
typedef Log<logERROR, true, true> LogAndPrintError;
typedef Log<logWARNING, true, true> LogAndPrintWarning;
typedef Log<logINFO, true, true> LogAndPrintInfo;
typedef Log<logDEBUG, true, true> LogAndPrintDebug;