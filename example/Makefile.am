
AM_CXXFLAGS = -Wall  -I..
AM_CFLAGS = = -Wall  -I..

if DEBUG
AM_CFLAGS += -g3 -O0 -D BE_DEBUG -D DEBUG_MGR
AM_CXXFLAGS += -g3 -O0 -D BE_DEBUG -D DEBUG_MGR
else
AM_CFLAGS += -O2
AM_CXXFLAGS += -O2
endif

LIBS = -L.. -lbitdht -lpthread

bin_PROGRAMS = bdexample

bdexample_SOURCES = bdhandler.cc bdexample.cc

