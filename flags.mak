# precompiler flags :
# 
#     _PISOFT_HAVE_CORBA_
#     _PISOFT_HAVE_DB_ 
# 
#     _ENABLE_PROFILER_  - writing timings of algortihms to $TRACEDIR/ccd.trace
#     _ENABLE_TRACING_   - compiling traces or not - in optimization version 
#                          should be disabled  
#     _ENABLE_VISUALIZATION_ - event visualization
# 

include options.mak

C_COMP = gcc
CPP_COMP = g++
# CCFLAGS=-g -fexternal-templates
# LDFLAGS=-g -fexternal-templates
# EXTERNAL_CFITSIO_LIBS=$(PI_EXTERNALS)/lib/libCCfits.a $(PI_EXTERNALS)/lib/libcfitsio.a
EXTERNAL_CFITSIO_LIBS=
# CMN_LIBS=$(NDIR)/slib/libdatan.a $(NDIR)/slib/libdatsrc.a -ldl -lpthread $(EXTERNAL_CFITSIO_LIBS) $(CORBA_LIBS)
# CMN_LIBS=$(NDIR)/slib/libdatan.a $(NDIR)/slib/libdatsrc.a -ldl -lpthread -lcurl -llog4c $(USB_LIBUSB_DRIVER_LIBS)
# Ubuntu16 : CMN_LIBS=$(NDIR)/slib/libdatsrc.a -ldl -lpthread -lcurl -llog4c $(USB_LIBUSB_DRIVER_LIBS)
# Ubuntu18 :
CMN_LIBS=$(NDIR)/slib/libdatsrc.a -ldl -lpthread -llog4c $(USB_LIBUSB_DRIVER_LIBS)



# shared libs :
# CCFLAGS=$(CCFLAGS_LOCAL) $(OPT) -fPIC -shared -D_DEBUG
CCFLAGS=$(CCFLAGS_LOCAL) $(OPT) -fPIC -shared -std=c++11

# static libs :
# CCFLAGS=$(CCFLAGS_LOCAL) $(OPT)

LDFLAGS=$(OPT) $(CORBA_LDFLAGS)
# OLD used : -D__cplusplus
# PREPROC=$(DEBUG) -D_UNIX -D_ENABLE_PROFILER_ -D_ENABLE_TRACING_ -D_ENABLE_VISUALIZATION_ -D_MONTE_CARLO_ -D$(CORBA_DEFINE) -D$(PIDB_DEFINE)YES_  -D_FULL_PI_SOFT_VERSION_ -D__$(PIDB_DRIVER_TYPE)__ $(USB_LIBUSB_DRIVER_FLAGS) -D_EVENT_COLLECTOR_ENABLED_$(EVENT_COLLECTOR_ENABLED)_
PREPROC=$(DEBUG) -D_UNIX -D_ENABLE_PROFILER_ -D_ENABLE_TRACING_ -D_ENABLE_VISUALIZATION_ -D_NO_ROOT_
# in order to skip not reporting things like rates to have best performance add :
# -D_OPTIMIZED_VERSION_


# SDL_LIBS=`sdl-config --libs`
# GRAPH_LIBS=$(SDL_LIBS) $(NDIR)/slib/libccdvisu.a
# Ubuntu16 : ADD_LIBS=-ldl $(USER_ADD_LIBS) -lcurl
ADD_LIBS=-ldl $(USER_ADD_LIBS)
# ADD_INCLUDES=$(GRAPH_INCLUDES)
