define GB_SRC_FILES =
	gb/core.c
	gb/cpu.c
	gb/cpu/interpreter.c
	gb/cpu/opc/decoder.c
	gb/cpu/opc/string.c
	gb/log.c
	gb/mem.c
	gb/mem/io.c
	gb/pad.c
	gb/ppu.c
	gb/ppu/shared.c
	gb/sch.c
	gb/video/sdl.c
endef

define PAK_LOADER_SRC_FILES =
	gb/log.c
	gb/pak.c
	gb/pak/header.c
	gb/pak/mbc.c
	gb/pak/mbc/common.c
	gb/pak/mbc/none.c
	prx/incbuf.c
	prx/io.c
endef

SDL_FLAGS = $(shell pkgconf --cflags --libs sdl2)
CFLAGS = -Iincl

# Prepend source files with "src/" directory.
GB_SRC_FILES := $(patsubst %,src/%,$(GB_SRC_FILES))
PAK_LOADER_SRC_FILES := $(patsubst %,src/%,$(PAK_LOADER_SRC_FILES))
# Generate list of required object files from source files.
GB_OBJ_FILES = $(patsubst src/%.c,obj/%.o,$(GB_SRC_FILES))
PAK_LOADER_OBJ_FILES = $(patsubst src/%.c,obj/%.o,$(PAK_LOADER_SRC_FILES))

all: debug
d: debug
dbg: debug
debug: obj/main.o $(GB_OBJ_FILES)
	gcc $(CFLAGS) $(SDL_FLAGS) $^ -o dgb

r: release
rls: release
release:
	@echo Release build to-be-implemented.
	@echo Use debug build at this time.

test-hex: tobj/ppu-hex-grid.o $(GB_OBJ_FILES)
	gcc $(CFLAGS) $(SDL_FLAGS) $^ -o test-hex

cpu-test: tsrc/gb/cpu-interpreter.c $(filter-out src/gb/cpu-interpreter.c, $(GB_SRC_FILES))
	gcc $(CFLAGS) -I./ $^ -o $@

pak-dump: tsrc/gb/pak-dump.c $(PAK_LOADER_OBJ_FILES)
	gcc $(CFLAGS) $(SDL_FLAGS) $^ -o $@

clean:
	rm -rf obj tobj
	rm -f cpu-test dgb pak-dump test-hex

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

tobj/%.o: tsrc/%.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

