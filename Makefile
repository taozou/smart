# Defines that can be used to customize build flavor.

DEFINES=

#DEFINES+=-DDEBUG  # use this to enable debug-only functionality
#DEFINES+=-DPERF   # use this to enable perf testing / tracing

# Include paths.  The webstor library depends on the following
# libraries:
#
# * curl
# * libxml2
# * openssl
#

INCLUDES=-I/usr/include/libxml2 

# Library search paths.  Similar to include paths.

LIBRARIES=

### RULES ###

CXXFLAGS+=$(DEFINES) $(INCLUDES) $(LIBRARIES) -Wno-enum-compare
LOADLIBES+=-lcurl -lssl -lxml2 -O3
CC=mpic++

.PHONY: all
all: smart webstor.a
	
smart: smart.cpp
	$(CC) $(CXXFLAGS) smart.cpp webstor.a $(LOADLIBES) -o smart

.PHONY: clean
clean:
	rm -f smart webstor.a

smart: webstor.a

webstor.a: webstor.a(asyncurl.o s3conn.o sysutils.o)

