#
# Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
# Copyright (C) Advanced Micro Devices, Inc. 2022. ALL RIGHTS RESERVED.
#

if HAVE_ROCM

sources =    \
    mc_rocm.h \
    mc_rocm.c

module_LTLIBRARIES         = libucc_mc_rocm.la
libucc_mc_rocm_la_SOURCES  = $(sources)
libucc_mc_rocm_la_CPPFLAGS = $(AM_CPPFLAGS) $(HIP_CPPFLAGS) $(ROCM_CPPFLAGS) $(BASE_CPPFLAGS)
libucc_mc_rocm_la_CFLAGS   = $(BASE_CFLAGS)
libucc_mc_rocm_la_LDFLAGS  = -version-info $(SOVERSION) --as-needed $(HIP_LDFLAGS) $(ROCM_LDFLAGS)
libucc_mc_rocm_la_LIBADD   = $(HIP_LIBS) $(ROCM_LIBS) \
                             $(UCC_TOP_BUILDDIR)/src/libucc.la

include $(top_srcdir)/config/module.am
endif
