/*
 * bdlog.h
 *
 *  Created on: 2015年4月16日
 *      Author: Yi
 */

#ifndef UTIL_BDLOG_H_
#define UTIL_BDLOG_H_

#include <iostream>
#include <syslog.h>

enum LogPriority {
    kLogEmerg   = LOG_EMERG,   // system is unusable
    kLogAlert   = LOG_ALERT,   // action must be taken immediately
    kLogCrit    = LOG_CRIT,    // critical conditions
    kLogErr     = LOG_ERR,     // error conditions
    kLogWarning = LOG_WARNING, // warning conditions
    kLogNotice  = LOG_NOTICE,  // normal, but significant, condition
    kLogInfo    = LOG_INFO,    // informational message
    kLogDebug   = LOG_DEBUG    // debug-level message
};

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class BDLog : public std::basic_streambuf<char, std::char_traits<char> > {
public:
	BDLog();
    explicit BDLog(std::string ident, int facility);
	virtual ~BDLog();

	static BDLog *instance();

	template<typename... Args>
	void info(Args... args);

	template<typename... Args>
	void debug(Args... args);

	template<typename... Args>
	void error(Args... args);

protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
    std::string buffer_;
    int facility_;
    int priority_;
    char ident_[50];
};

#endif /* UTIL_BDLOG_H_ */
