#Modify this to point to the PJSIP location.
PJBASE=../pjproject-2.12/

include $(PJBASE)/build.mak

CC      = $(PJ_CC)
#CC=g++
LDFLAGS = $(PJ_LDFLAGS)
LDLIBS  = $(PJ_LDLIBS)
CFLAGS  = $(PJ_CFLAGS)
CPPFLAGS= ${CFLAGS}

INCLUDEDIR=../pjproject-2.12/pjsip/include -I ../pjproject-2.12/pjlib/include -I ../pjproject-2.12/pjlib-util/include -I ../pjproject-2.12/pjmedia/include -I ../pjproject-2.12/pjnath/include

# If your application is in a file named myapp.cpp or myapp.c
# this is the line you will need to build the binary.
all: bigapp VSSP

main.o: main.c
	$(CC) -c -I$(INCLUDEDIR) $< -o $@

VSSP: smallapp.c
	$(CC) -o $@  $< $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)

SIMPLE: simple_pjsua.c
	$(CC) -o $@  $< $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)

bigapp: pjsua_app.c main.o
	$(CC) -o $@ -I$(INCLUDEDIR) main.o $< $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f main.o bigapp smallapp VSSP

