include ../flags_main.mak

all :
	g++ main.cpp -o pacer_dirty_image -g -I ../ $(MWA_FRB)/lib/libmscommonlib.a $(MWA_FRB)/lib/libmsfitslib.a $(COMMON_LIBS) -lfftw3 -D_UNIX
	mkdir -p $(MWA_FRB)/bin/
	cp pacer_dirty_image $(MWA_FRB)/bin/
	
clean :
	rm -f pacer_dirty_image *.o
		