LIBNAME=libeloop.a

CC = gcc
CXX = g++

AR = ar
ARFLAGS = ru

SRCS = cexceptNew.c \
	   common.c \
	   context.c \
	   dns.c \
	   eloop.c \
	   os_unix.c \
	   asm.S

OBJS_1 = $(patsubst %.c, %.o, $(SRCS))
OBJS = $(patsubst %.S, %.o, $(OBJS_1))

CFLAGS = -DHAVE_CONFIG_H -I.  -DUSE_ELOOP=1 -DUSE_IP_SOCKET=0 -DUSE_DOMAIN_SOCKET=1 -O2

default: linux
	
linux: $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME) $(OBJS) 

clean:
	$(RM) $(OBJS) $(LIBNAME)


all: clean default
