.PNONY:all

CC := gcc -std=gnu99
LD := gcc

#---------

TARGET ?= gzip.so
LUAPATH ?= lua/
ZLIBPATH ?= zlib

#-----------platform
PLATS=linux macosx
platform:
	@echo "'make PLATFORM' where PLATFORM is one of these:"
	@echo "$(PLATS)"
CCFLAG = -g -O2 -Wall -Wextra -I$(LUAPATH) -I$(ZLIBPATH)
LDFLAG := -lm -ldl

linux:LDFLAG += -Wl,-E -lrt
macosx:LDFLAG += -Wl,-no_compact_unwind
linux macosx:LDFLAG += -lpthread

linux:SHARED:=--share -fPIC
macosx:SHARED=-dynamiclib -fPIC -Wl,-undefined,dynamic_lookup

linux: PLAT := linux
macosx: PLAT := macosx

linux macosx: all

all: $(TARGET)

$(TARGET):lgzip.c $(ZLIBPATH)/libz.a
	$(CC) $(CCFLAG) -o $@ $^ $(SHARED)

$(ZLIBPATH)/libz.a:$(ZLIBPATH)/Makefile
	make -C $(ZLIBPATH)

$(ZLIBPATH)/Makefile:$(ZLIBPATH)/configure
	cd $(ZLIBPATH);./configure;cd ../

$(ZLIBPATH)/configure:
	git submodule update --init

