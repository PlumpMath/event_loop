lib_LIBRARIES = libeloop.a
libeloop_a_SOURCES = \
	Eloop_callback_itf.h \
	cexceptNew.c \
	cexcept.h \
	eloop.c \
	eloop.h \
	common.c \
	common.h \
	os_internal.c \
	os.h \
	build_config.h \
	includes.h \
	taskimpl.h \
	Task.h \
	context.c \
	asm.S \
	dns.c

libeloop_a_CPPFLAGS = \
	-DUSE_ELOOP=1 -DOS_NO_C_LIB_DEFINES  -DUSE_IP_SOCKET=0 -DUSE_DOMAIN_SOCKET=1 

