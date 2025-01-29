CC=g++
CFLAGS=-g -c -Wall -Wno-non-virtual-dtor -I./ -D _POSIX_ 

LIB=libufix.a
LIB_TARGET=/octech/lib/octech/cpp
LIB_INCLUDE=/octech/lib/octech/include/fix
LIB_SRCS=$(wildcard *.cpp)
LIB_OBJS=$(LIB_SRCS:.cpp=.o)

all: lib install clean
.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

lib: $(LIB)
$(LIB): $(LIB_OBJS)
	ar rcs $(LIB) $(LIB_OBJS)

install:
	sudo mv $(LIB) $(LIB_TARGET)
	sudo cp -f *.h $(LIB_INCLUDE)         
clean:	
	rm -rf *.o
