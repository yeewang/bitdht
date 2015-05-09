/*
 * bdlog.cc
 *
 *  Created on: 2015年4月16日
 *      Author: Yi
 */

#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "bdlog.h"

TDLog LOG;

TDLog::TDLog()
{
	init();
	setProperties();
}

TDLog::~TDLog()
{
}

void TDLog::init()
{
#if 0
	log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
	appender1->setLayout(new log4cpp::BasicLayout());

	log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", "program.log");
	appender2->setLayout(new log4cpp::BasicLayout());

	log4cpp::Category& root = log4cpp::Category::getRoot();
	root.setPriority(log4cpp::Priority::INFO);
	root.addAppender(appender1);

	log4cpp::Category& sub1 = log4cpp::Category::getInstance(std::string("sub1"));
	sub1.addAppender(appender2);
#endif
}

void TDLog::setProperties()
{
#if 0
	std::string initFileName = "log4cpp.properties";
	log4cpp::PropertyConfigurator::configure(initFileName);
#endif
}

void TDLog::info(const char *str, ...)
{
	if (fp == NULL)
		return;

	fprintf(fp,"%10s: ", time().c_str());
	va_list arglist;
	va_start(arglist,str);
	vfprintf(fp, str, arglist);
	va_end(arglist);
	fprintf(fp,"\n");
}

const std::string TDLog::time()
{
	char outstr[200];
	time_t t;
	struct tm *tmp;

	t = ::time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		return "";
	}

	if (strftime(outstr, sizeof(outstr), "%D %H:%M:%S", tmp) == 0) {
		fprintf(stderr, "strftime returned 0");
		return "";
	}

	return outstr;
}
