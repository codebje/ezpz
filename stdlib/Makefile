TRIPLE=ez80-none-elf-
CC=$(TRIPLE)clang
AS=$(TRIPLE)clang
LD=$(TRIPLE)ld
AR=$(TRIPLE)ar -r

CFLAGS=-W -Wall -Wextra -Iinclude
AFLAGS=

all: libezpz.a

CSRCS :=
ASRCS :=

include src/Make.defs

COBJS := $(addprefix obj/,$(CSRCS:.c=.o))
AOBJS := $(addprefix obj/,$(ASRCS:.S=.o))

$(COBJS): obj/%.o: %.c
	mkdir -p obj
	$(CC) -c $(CFLAGS) $< -o $@

$(AOBJS): obj/%.o: %.S
	mkdir -p obj
	$(CC) -c $(AFLAGS) $< -o $@

libezpz.a: $(COBJS) $(AOBJS)
	$(AR) $@ $^
