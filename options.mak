#if [ -d $(GRAPHICS_YES) ];\
#	then \
#ifeq ($(strip $(foo)),)
#          TEXT-IF-EMPTY
#          endif
#
#ifdef GRAPHICS_YES
#	echo Graphics enabled and compiled ...
#	GRAPH_LIBS=-lSDL -lpthread 
#	GRAPH_INCLUDES=-I/usr/include/SDL 
#else	 
#	echo Graphics disabled 
#	GRAPH_LIBS=-lSDL -lpthread 
#	GRAPH_INCLUDES=-I/usr/include/SDL 
#fi
