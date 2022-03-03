include ../make.cfg/flags.mak

all :
	g++ main.cpp lc_table.cpp -o dump_lc -g -I $(BIGHORNS)/software/common -I../fitslib/ $(COMMON_LIBS) -D_UNIX
	cp dump_lc $(BIGHORNS)/bin/
	
clean :
	rm -f dump_lc *.o
		