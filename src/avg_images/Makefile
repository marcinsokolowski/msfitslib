include ../flags_main.mak

all :
	g++ main.cpp -o avg_images -g -I ../ $(COMMON_LIBS) -D_UNIX
	cp avg_images $(BIGHORNS)/bin/
	
clean :
	rm -f avg_images *.o
		