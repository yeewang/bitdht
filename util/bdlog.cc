/*
 * bdlog.cc
 *
 *  Created on: 2015年4月16日
 *      Author: Yi
 */

#include "bdlog.h"

#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "bdlog.h"


BDLog::BDLog(std::string ident, int facility) {
	facility_ = facility;
	priority_ = LOG_DEBUG;
	strncpy(ident_, ident.c_str(), sizeof(ident_));
	ident_[sizeof(ident_)-1] = '\0';

	openlog(ident_, LOG_PID, facility_);
}

BDLog::~BDLog()
{
	closelog();
}

BDLog * BDLog::instance()
{
	static BDLog *log = NULL;
	if (log == NULL) {
		log = new BDLog("Tandem", LOG_LOCAL0);
		std::clog.rdbuf(log);
	}
	return log;
}

int BDLog::sync() {
	if (buffer_.length()) {
		syslog(priority_, "%s", buffer_.c_str());
		buffer_.erase();
		priority_ = LOG_DEBUG; // default to debug for each message
	}
	return 0;
}

int BDLog::overflow(int c) {
	if (c != EOF) {
		buffer_ += static_cast<char>(c);
	} else {
		sync();
	}
	return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority) {
	static_cast<BDLog *>(os.rdbuf())->priority_ = (int)log_priority;
	return os;
}

template<typename... Args>
void BDLog::info(Args... args)
{
	syslog(LOG_INFO, args...);
}

template<typename... Args>
void BDLog::debug(Args... args)
{
	syslog(LOG_DEBUG, args...);
}

template<typename... Args>
void BDLog::error(Args... args)
{
	syslog(LOG_ERR, args...);
}
