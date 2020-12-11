#CPP=clang++ -fsanitize=thread --std=c++11
#CPP=clang++ -fsanitize=leak -fno-omit-frame-pointer --std=c++11
CPP=clang++ --std=c++11
CPPFLAGS=-g -Wall -O2 -Wextra
LDLIBS=-L/usr/local/lib -L../lib -lpthread -lharecpp -lgtest
INCDIR=-I../include
OBJDIR=./obj
BINDIR=./bin
SRCDIR=./src

SRC=$(wildcard $(SRCDIR)/*.cpp)
OBJ=$(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: build $(BINDIR)/testOne

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp 
	$(CPP) $(CPPFLAGS) -fPIC $(INCDIR) -c $< -o $@

$(BINDIR)/testOne: $(OBJ)
	$(CPP) $(CPPFLAGS) $(INCLUDE) $(OBJ) -o $@ $(LDLIBS)


build:
	@mkdir -p ./bin
	@mkdir -p ./obj

.PHONY: clean test

clean:
	rm -rf bin
	rm -rf obj

test: all
	bin/testOne

