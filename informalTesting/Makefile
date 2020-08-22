CPP=clang++ --std=c++11
CPPFLAGS=-g -Wall -O2 -Wextra
LDLIBS=-L/usr/local/lib -L../lib -lpthread -lharecpp 
INCDIR=-I../include
OBJDIR=../obj
LIBDIR=../lib

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%)

all: GeneralUnitTest StatisticsTest


GeneralUnitTest: 
	$(CPP) $(CPPFLAGS) $(INCDIR) GeneralUnitTest.cpp -o $@ $(LDLIBS)

StatisticsTest: 
	$(CPP) $(CPPFLAGS) $(INCDIR) StatisticsTest.cpp -o $@ $(LDLIBS)

.PHONY: clean 

clean:
	rm -f *.o
	rm -f GeneralUnitTest
	rm -f StatisticsTest

