SOURCES = $(wildcard *.c)
OBJS   ?= $(patsubst %.c,%.o,$(SOURCES))

CFLAGS  += -DMAKE_APP -g -Wall -I. -I../include -I../frcapi -I$(OCTEON_ROOT)/executive
LDFLAGS = 

all: $(TARGET) link

include $(FRCDIR)/version.mk

%.o: %.c
	$(CC) $(CFLAGS) $(ECFLAGS) -c $< -o $@

$(TARGET): $(VOBJ) $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(ELDFLAGS) $(LDLIBS) ../frcapi/frcapi.a

clean: $(VCLEAN)
	rm -rf *.o
	rm -rf $(TARGET)
	rm -rf $(OCTBIN)/$(TARGET)

install:
	cp -f $(TARGET) 

link:
	@ln -fs `pwd`/$(TARGET) $(OCTBIN)/$(TARGET)
