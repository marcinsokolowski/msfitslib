include ../flags_main.mak

all :
	g++ main.cpp -o avg_images -g -I ../ $(MWA_FRB)/lib/libmscommonlib.a $(COMMON_LIBS) -D_UNIX
	mkdir -p $(MWA_FRB)/bin/
	cp avg_images $(MWA_FRB)/bin/
	
clean :
	rm -f avg_images *.o
		