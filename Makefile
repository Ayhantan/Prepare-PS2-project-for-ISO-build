EE_BIN = ps2game.elf
EE_OBJS = src/main.o src/game.o src/input.o src/render.o src/audio.o
ISO_BIN = ps2game.iso
ISO_ROOT = iso_root
ISO_ELF = PS2GAME.ELF
ISO_WAV = PIXEL.WAV
ISO_IRX = AUDSRV.IRX

EE_INCS += -I$(PS2DEV)/gsKit/include
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib
EE_LIBS += -lgskit -ldmakit -lpad -laudsrv

all: $(EE_BIN)

iso: $(ISO_BIN)

$(ISO_BIN): $(EE_BIN) SYSTEM.CNF assets/pixel.wav assets/audsrv.irx
	mkdir -p $(ISO_ROOT)
	cp SYSTEM.CNF $(ISO_ROOT)/SYSTEM.CNF
	cp $(EE_BIN) $(ISO_ROOT)/$(ISO_ELF)
	cp assets/pixel.wav $(ISO_ROOT)/$(ISO_WAV)
	cp assets/audsrv.irx $(ISO_ROOT)/$(ISO_IRX)
	xorriso -as mkisofs -V PS2GAME -o $(ISO_BIN) $(ISO_ROOT)

clean:
	rm -rf $(EE_BIN) $(ISO_BIN) $(ISO_ROOT) src/*.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
