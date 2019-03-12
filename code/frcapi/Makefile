CC=gcc

SOURCES=$(wildcard *.c)
LIBOBJS=$(patsubst %.c,%.o,$(SOURCES))

LIB=frcapi.a

CFLAGS  = -DMAKE_APP -g -Wall -I. -I../include
LDFLAGS = 
all: $(LIBOBJS) $(LIB)
$(LIBOBJS):%.o:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)


clean:
	rm -rf *.o
	rm -rf *.a

