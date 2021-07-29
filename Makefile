
EE_BIN=test.elf
ISO_TARGET=test.iso
EE_OBJS=main.o gs.o drawbuffer.o inputs.o ps2math.o

DOCKER_IMG=ps2build

CC=EE_GCC
DVP=dvp-as
VCL=openvcl

EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include 

EE_CFLAGS += -Wall -Wextra --std=c99 -DPRINT_DMA_LOGS

EE_LDFLAGS = -L$(PS2SDK)/ee/common/lib -L$(PS2SDK)/ee/lib
EE_LIBS = -lpad -ldma -lgraph -ldraw -lpatches -lmc -lc -lm -lkernel -ldebug

PS2SDK=/usr/local/ps2dev/ps2sdk

PS2HOST=192.168.20.99


include .lintvars

all: deploy

vcl/%.o: vcl/%.vcl
	$(VCL) -G $< | $(DVP) -o $@ -

run:
	ps2client -t 10 -h $(PS2HOST) execee host:$(EE_BIN)

reset:
	ps2client -t 5 -h $(PS2HOST) reset

.PHONY: clean
clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(ISO_TARGET)

.PHONY: lint
lint:
	cpplint --filter=$(CPPLINT_FILTERS) --counting=total --linelength=$(CPPLINT_LINE_LENGTH) --extensions=c,h *.c *.h

.PHONY: format
format:
	sudo docker run -v $(shell pwd):/workdir unibeautify/clang-format -i -sort-includes *.c *.h

$(ISO_TARGET): $(EE_BIN) SYSTEM.CNF
	mkisofs -l -o $(ISO_TARGET) $(EE_BIN) SYSTEM.CNF

deploy: $(ISO_TARGET)
	cp $(ISO_TARGET) /mnt/$(ISO_TARGET)

docker:
	docker build -t $(DOCKER_IMG) .

docker-build:
	docker run -v $(shell pwd):/src $(DOCKER_IMG) make

ifdef PLATFORM
include /usr/local/ps2dev/ps2sdk/samples/Makefile.eeglobal
include /usr/local/ps2dev/ps2sdk/samples/Makefile.pref
endif
