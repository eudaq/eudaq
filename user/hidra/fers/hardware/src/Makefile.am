#	Copyright (C) 2020-2023 CAEN SpA
#
#	This file is part of the CAEN Dig2 Low Level Library. PTHREAD_LIBS

lib_LTLIBRARIES = libcaenferslib.la
libcaenferslib_la_SOURCES = \
	FERSlib.c \
	FERS_readout.c \
	FERS_LLeth.c \
	FERS_LLtdl.c \
	FERS_LLusb.cpp \
	FERS_adapters.c \
	FERS_configure.c \
	FERS_configure_5202.c \
	FERS_configure_5203.c \
	FERS_configure_5204.c \
	FERS_paramparser.c \
	FERS_MultiPlatform.c
libcaenferslib_la_CPPFLAGS = \
	-I$(top_srcdir)/include 
libcaenferslib_la_CFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(LIBUSB_CFLAGS)	
libcaenferslib_la_CXXFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(LIBUSB_CFLAGS)
libcaenferslib_la_LIBADD = \
	$(PTHREAD_LIBS) \
	$(LIBUSB_LIBS)
libcaenferslib_la_LDFLAGS = \
	-version-info 0:0:0
