UNAME := $(shell uname -s)
CXX=g++
CC=gcc
ARCH=`uname -m`
#OPTFLAGS=-g -O0
OPTFLAGS=-O3 -DNDEBUG
CXXFLAGS=-Wall -fPIC $(OPTFLAGS)
LDFLAGS=-L/usr/local/lib -lz -lpthread $(OPTFLAGS)
PROG=synchdata

main: $(MODULES) $(PROG).o
	$(CXX) Base64.cxx $(LDFLAGS) $(PROG).o -o ./$(PROG)

%.o: %.cxx %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(PROG)
