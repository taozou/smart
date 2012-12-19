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

CXXFLAGS+=$(DEFINES) $(INCLUDES) $(LIBRARIES) -Wno-enum-compare -O3
LOADLIBES+=-lcurl -lssl -lxml2 
CC=mpic++

.PHONY: all
all: smart smart.a
	
smart: smart.cpp
	$(CC) $(CXXFLAGS) smart.cpp smart.a $(LOADLIBES) -o smart
	
.PHONY: clean
clean:
	rm -f smart smart.a 

smart: smart.a

smart.a: smart.a(asyncurl.o s3conn.o sysutils.o selector.o aggregator.o)
	
.cpp.a:
	$(CC) $(CXXFLAGS) -c $< -o $*.o
	$(AR) r $@ $*.o
	$(RM) $*.o
