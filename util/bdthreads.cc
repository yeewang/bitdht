/*
 * util/bdthreads.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2004-2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bdthreads.h"
#include <unistd.h>    /* for usleep() */
#include "util/bdlog.h"

/*******
 * #define DEBUG_THREADS 1
 *******/

#define DEBUG_THREADS 1

#ifdef DEBUG_THREADS
#include <iostream>
#endif

extern "C" void* bdthread_init(void* p)
{
#ifdef DEBUG_THREADS
	std::ostringstream debug;
	debug << "bdthread_init()";
	LOG.info(debug.str().c_str());
#endif

	bdThread *thread = (bdThread *) p;
	if (!thread)
	{
#ifdef DEBUG_THREADS
		std::ostringstream debug;
		debug << "bdthread_init() Error Invalid thread pointer.";
		LOG.info(debug.str().c_str());
#endif
		return 0;
	}
	thread -> run();
	return 0;
}


pthread_t  createThread(bdThread &thread)
{
	pthread_t tid;
	void  *data = (void *) (&thread);

#ifdef DEBUG_THREADS
	std::ostringstream debug;
	debug << "createThread() creating a bdThread";
#endif

	thread.mMutex.lock();
	{
		pthread_create(&tid, 0, &bdthread_init, data);
		thread.mTid = tid;
	}

#ifdef DEBUG_THREADS
	debug << "createThread() created Thread.mTid: ";

#if defined(_WIN32) || defined(__MINGW32__)
	debug << "WIN32: Cannot print mTid ";
#else
	debug << thread.mTid;
#endif

	LOG.info(debug.str().c_str());
#endif

	thread.mMutex.unlock();

	return tid;
}

bdThread::bdThread()
{
#ifdef DEBUG_THREADS
	LOG.info("bdThread::bdThread()");
#endif

#if defined(_WIN32) || defined(__MINGW32__)
	memset (&mTid, 0, sizeof(mTid));
#else
	mTid = 0;
#endif
}

void bdThread::join() /* waits for the the mTid thread to stop */
{
	std::ostringstream debug;

#ifdef DEBUG_THREADS
	debug << "bdThread::join() Called! Waiting for Thread.mTid: ";

#if defined(_WIN32) || defined(__MINGW32__)
	debug << "WIN32: Cannot print mTid ";
#else
	debug << mTid;
#endif

#endif

	LOG.info(debug.str().c_str());
	debug.str("");

	mMutex.lock();
	{
#if defined(_WIN32) || defined(__MINGW32__)
		/* Its a struct in Windows compile and the member .p ist checked in the pthreads library */
#else
		if(mTid > 0)
#endif
			pthread_join(mTid, NULL);

#ifdef DEBUG_THREADS
		debug << "bdThread::join() Joined Thread.mTid: ";

#if defined(_WIN32) || defined(__MINGW32__)
		debug << "WIN32: Cannot print mTid ";
#else
		debug << mTid;
#endif

		debug << "bdThread::join() Setting mTid = 0";
#endif

#if defined(_WIN32) || defined(__MINGW32__)
		memset (&mTid, 0, sizeof(mTid));
#else
		mTid = 0;
#endif

	}
	mMutex.unlock();

	LOG.info(debug.str().c_str());
}

void bdThread::stop() 
{
#ifdef DEBUG_THREADS
	LOG.info("bdThread::stop() Called!");
#endif

	pthread_exit(NULL);
}



