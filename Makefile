
EE_BIN=test.elf
ISO_TARGET=test.iso
EE_OBJS=main.o gs.o drawbuffer.o inputs.o ps2math.o

CC=ee-gcc
DVP=dvp-as
VCL=openvcl

EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include 

EE_CFLAGS += -Wall --std=c99 -DPRINT_DMA_LOGS

EE_LDFLAGS = -L$(PS2SDK)/ee/common/lib -L$(PS2SDK)/ee/lib
EE_LIBS = -lpad -ldma -lgraph -ldraw -lpatches -lmc -lc -lm -lkernel -ldebug -Xlinker

PS2HOST=192.168.20.99

all: deploy

vcl/%.o: vcl/%.vcl
	$(VCL) -G $< | $(DVP) -o $@ -

run:
	ps2client -t 10 -h $(PS2HOST) execee host:$(EE_BIN)

reset:
	ps2client -t 5 -h $(PS2HOST) reset

clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(ISO_TARGET)

$(ISO_TARGET): $(EE_BIN) SYSTEM.CNF
	mkisofs -l -o $(ISO_TARGET) $(EE_BIN) SYSTEM.CNF

deploy: $(ISO_TARGET)
	cp $(ISO_TARGET) /mnt/$(ISO_TARGET)

include Makefile.pref
include Makefile.eeglobal
