include ../flags_main.mak

all :
	g++ main.cpp -o running_median -g -I ../ $(MWA_FRB)/lib/libmscommonlib.a $(MWA_FRB)/lib/libmsfitslib.a $(COMMON_LIBS) -D_UNIX
	mkdir -p $(MWA_FRB)/bin/
	cp running_median $(MWA_FRB)/bin/
	
clean :
	rm -f running_median *.o
		