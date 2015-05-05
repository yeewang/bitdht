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

#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/Priority.hh"

class TDLog {
public:
	TDLog();
	virtual ~TDLog();

	log4cpp::Category& log();
	void info(const char *str, ...);

private:
	void init();
	void setProperties();

	FILE * fp;
};

// #define LOG TDLog().log()

extern TDLog LOG;

#endif /* UTIL_BDLOG_H_ */
