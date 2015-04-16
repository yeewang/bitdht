/*
 * bdlog.h
 *
 *  Created on: 2015年4月16日
 *      Author: Yi
 */

#ifndef UTIL_BDLOG_H_
#define UTIL_BDLOG_H_

#include <log4cpp/Category.hh>

extern log4cpp::Category &bdlog;

void bdlog_init();

#endif /* UTIL_BDLOG_H_ */
