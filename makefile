EXTRA_CCOPTIONS=-D_NO_ROOT_
NDIR=./

MAKEDIR=./
LIBNAME=mscommonlib.a
INCLUDES=-I./
# INCLUDES=-I$(SRCDIR)/cmn/mathlib -I$(SRCDIR)/ccd/cfg \
#			-I$(SRCDIR)/cmn/datan -I.
include msfitslib.mak

include lib.mak

