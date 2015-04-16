
LIBS = \
	-I /sources/opensource/log4cpp/include \
	-l/sources/opensource/log4cpp/src/.lib/liblog2cpp.a
	
AM_CPPFLAGS= -g -Wall -D BE_DEBUG -lpthread -D DEBUG_MGR \
	-I/sources/opensource/log4cpp/include

noinst_LIBRARIES = libbitdht.a

libbitdht_a_SOURCES = bitdht/bencode.c \
		bitdht/bdobj.cc \
		bitdht/bdmsgs.cc \
		bitdht/bdpeer.cc \
		bitdht/bdquery.cc \
		bitdht/bdhash.cc \
		bitdht/bdstore.cc \
		bitdht/bdnode.cc \
		bitdht/bdmanager.cc \
		bitdht/bdstddht.cc \
		bitdht/bdhistory.cc \
		util/bdnet.cc \
		util/bdthreads.cc \
		util/bdlog.cc \
		udp/udplayer.cc \
		udp/udpstack.cc \
		udp/udpbitdht.cc 
