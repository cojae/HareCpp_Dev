CPP=clang++ --std=c++11
CPPFLAGS=-g -Wall -O2 -Wextra
LDLIBS=-L/usr/local/lib -lpthread -lrabbitmq
INCDIR=-I../include
OBJDIR=../obj
LIBDIR=../lib

PCH_SRC=../include/pch.hpp
PCH_OUT=$(OBJDIR)/lib.pch.gch

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=$(OBJDIR)/%.o)

all: build $(LIBDIR)/libharecpp.so

$(OBJDIR)/%.o: %.cpp
	$(CPP) $(CPPFLAGS) -fPIC $(INCDIR) -c $< -o $@

$(PCH_OUT): $(PCH_SRC)
	$(CPP) $(CPPFLAGS) -fPIC $(INCDIR) -c $< -o $@


$(LIBDIR)/libharecpp.so: $(PCH_OUT) $(OBJ)
	$(CPP) $(CPPFLAGS) $(INCLUDE) $(PCH) $(OBJ) -shared -o $@ $(LDLIBS)


.PHONY: clean build check

clean:
	rm -rf $(LIBDIR)
	rm -rf $(OBJDIR)

build:
	@mkdir -p $(LIBDIR)
	@mkdir -p $(OBJDIR)

check:
	cppcheck --enable=all --std=c++11 -v -I ../include/ . 2>&1 | tee log.log
