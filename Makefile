SOURCE = mxt.c vars-standalone.c varsA.c varsP.c varsS.c varsV.c lowlevel_timer.c sincos.c

# On Mac OS X, we'll want this:
SUPPORTSOURCE = regex.c

OBJECTS = $(SOURCE:.c=.o) $(SUPPORTSOURCE:.c=.o)
UNIBIN=$(shell machdepUNIBIN)
CC = gccopt
CFLAGS = $(shell machdepLDOPTS shobj)
DEBUG =
CHECK = -c
CLIB=$(HOME)/work/lib
LIBNAME=vars_sa
TLIB=$(CLIB)/lib$(LIBNAME).so

DEFINES = -DVARS_STANDALONE -DSYMBOLTABLE

.c.o:
	$(CC) $(UNIBIN) $(DEFINES) $(DEBUG) $(CFLAGS) -o $@ $(CHECK) $<

all: $(TLIB) Makefile alloca.c lowlevel_timer.h Macros.h

Makefile: vars-standalone.h varsedit.h
	-rm -f *.o
	touch -r vars-standalone.h Makefile

EFENCE =
LIBS = $(EFENCE) -lm -ldl

$(TLIB): $(OBJECTS) tags
	$(CC) $(UNIBIN) $(shell machdepLDOPTS shlib $(TLIB)) -o $(TLIB) $(OBJECTS) -ldl

try: try.c $(TLIB)
	$(CC) $(UNIBIN) $(DEFINES) $(DEBUG) $(CFLAGS) -o $(<:.c=.o) $(CHECK) $< 
	$(CC) $(UNIBIN) $(DEFINES) $(DEBUG) -o $@ $(<:.c=.o) -L$(CLIB) -l$(LIBNAME)  $(LIBS)

tags: $(SOURCE) $(SUPPORTSOURCE) *.h
	ctags $+

clean:
	-rm -f $(OBJECTS) try try.o core

distclean: clean
	-rm -f $(TLIB) tags
