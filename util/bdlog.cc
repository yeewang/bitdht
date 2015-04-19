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

#include <log4cpp/PropertyConfigurator.hh>

#include "bdlog.h"


BDLog::BDLog()
{
	init();
	setProperties();
}

BDLog::~BDLog()
{
	log4cpp::Category::shutdown();
}

void BDLog::init()
{
	log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
	appender1->setLayout(new log4cpp::BasicLayout());

	log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", "program.log");
	appender2->setLayout(new log4cpp::BasicLayout());

	log4cpp::Category& root = log4cpp::Category::getRoot();
	root.setPriority(log4cpp::Priority::INFO);
	root.addAppender(appender1);

	log4cpp::Category& sub1 = log4cpp::Category::getInstance(std::string("sub1"));
	sub1.addAppender(appender2);
}

void BDLog::setProperties()
{
	std::string initFileName = "log4cpp.properties";
	log4cpp::PropertyConfigurator::configure(initFileName);
}

log4cpp::Category& BDLog::log()
{
	log4cpp::Category& root = log4cpp::Category::getRoot();
	return root;
}
