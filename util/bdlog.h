/*
 * bdlog.h
 *
 *  Created on: 2015年4月16日
 *      Author: Yi
 */

#ifndef UTIL_BDLOG_H_
#define UTIL_BDLOG_H_

#include <sstream>
#include <syslog.h>

class TDLog {
public:
	TDLog();
	virtual ~TDLog();

	void info(const char *str, ...);

private:
	void init();
	void setProperties();
	const std::string time();

	FILE * fp;
};

extern TDLog LOG;

#endif /* UTIL_BDLOG_H_ */
