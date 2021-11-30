include flags.mak

default : $(OBJECTS)
#	ar sru $(LIBNAME) $^
	 gcc -v -shared -Wl,-soname,lib$(LIBNAME) -o sh$(LIBNAME) $(OBJECTS) $(C_OBJECTS)
all   : lib libstatic

lib   : custom_target sh$(LIBNAME)

sh$(LIBNAME) : $(IDL_GEN_FILES) $(OBJECTS) $(C_OBJECTS) $(CORBA_OBJECTS)
#	ar sru $(LIBNAME) $^
	 gcc -v -shared -Wl,-soname,lib$(LIBNAME) -o sh$(LIBNAME) $(OBJECTS) $(CORBA_OBJECTS) $(C_OBJECTS) 
#	ld -o sh$(LIBNAME) $^ -shared
#	ranlib $(LIBNAME)
	@mkdir -p $(NDIR)/lib
	@mkdir -p $(NDIR)/slib
#	cp $(LIBNAME) $(NDIR)/lib/lib$(LIBNAME)
	cp sh$(LIBNAME) $(NDIR)/slib/lib$(LIBNAME)
#$(OBJECTS) : $(HEADERS)
#	$(CPP_COMP) -c $(CCFALGS) $(subst .h,.cpp,$<)

$(CORBA_OBJECTS) : %.o : %.cc
	 $(CPP_COMP) $(INCLUDES) $(CMN_INCLUDES) $(PREPROC) $(CCFLAGS) -c $<

$(IDL_GEN_FILES) : %.cc: %.idl
	 idl --poa $<
# 	 $(IDL) --poa $<
# 	 $(IDL) --poa --boa $<

$(OBJECTS) : %.o :  %.cpp
	 $(CPP_COMP) $(INCLUDES) $(CMN_INCLUDES) $(PREPROC) $(CCFLAGS) -c $<

$(C_OBJECTS) : %.o :  %.c
	 $(C_COMP) $(CCFLAGS) -c $<
#	 $(C_COMP) $(INCLUDES) $(CMN_INCLUDES) $(PREPROC) $(CCFLAGS) -c $<


custom_target :
	if [[  -s premakefile.sh ]]; then echo "Executing pre-makefile actions, specific for this library"; ./premakefile.sh; fi

#$(OBJECTS) : %.o : %.cxx 
#	$(CPP_COMP) $(PREPROC) $(CCFLAGS) -c $<
#	ar rcs $(LIBNAME) $<
#$(OBJECTS) : %o : %h
#	$(CPP_COMP) $(CCFLAGS) $<	 	
.PHONY : clean
clean :
	rm -f $(LIBNAME) $(OBJECTS) $(C_OBJECTS) sh$(LIBNAME) $(CORBA_OBJECTS) $(IDL_GEN_FILES) static_$(LIBNAME)

libstatic :
	ar sru static_$(LIBNAME) *.o
	@mkdir -p $(NDIR)/lib
	cp static_$(LIBNAME) $(NDIR)/lib/lib$(LIBNAME)
