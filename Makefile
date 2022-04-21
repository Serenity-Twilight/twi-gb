#========================================================================
# List twi source files to compile and link here.
define twi_std_source
	log.c
	utils.c
endef
# List twi_gb source files to compile and link here.
define twi_gb_source 
	gb.c
	log.c
	main.c
	mem.c
	ppu.c
endef
# List all testing files for twi-gb to compile and link here.
define twi_gb_test
	cpu.c
	main.c
endef
# List all twi-gb source files which are required for the testing binary
# to link.
define twi_gb_test_src
	log.c
	mem.c
endef
#========================================================================
# DO NOT MODIFY ANYTHING BELOW THIS POINT
# UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING!
#========================================================================
# Fully expand twi-std directories.
std_files := $(addprefix src/twi/std/,$(twi_std_source))
# Fully expand debug and release source files, include std files.
src_files := $(addprefix src/twi/gb/,$(twi_gb_source)) $(std_files)
# Fully expand testing source files, include std files.
define test_files := 
	$(addprefix src/twi/gb/test/,$(twi_gb_test))
	$(addprefix src/twi/gb/,$(twi_gb_test_src))
	$(std_files)
endef

# Debug object files
dobj_files := $(patsubst src/%.c,dobj/%.o,$(src_files))
# Release object files
obj_files := $(patsubst src/%.c,obj/%.o,$(src_files))
# Test object files
tobj_files := $(patsubst src/%.c,tobj/%.o,$(test_files))

# Make files, which track what each source file depends on to build.
make_files := $(patsubst src/%.c,make/%.mk,$(src_files) $(test_files))

# Directories
includes_dir = includes
libs_dir = libs

# Compilation flags
CFLAGS := -I$(includes_dir) -L$(libs_dir) $(shell pkgconf --libs sdl2 glew)
DBG_CFLAGS := -g
RLS_CFLAGS :=

# If C compiler isn't set, use default of cc.
CC ?= cc

#========================================================================
# RELEASE RULES
#========================================================================
r: release
rel: release
rls: release
release: twi-gb

# Link release binary
twi-gb twi-gb.exe: $(obj_files)
	$(CC) $(CFLAGS) $(RLS_CFLAGS) $(obj_files) -o $@

# Compile release object files
obj/%.o:
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(RLS_CFLAGS) src/$*.c -o $@

#========================================================================
# DEBUG RULES
#========================================================================
d: debug
dbg: debug
debug: dtwi-gb

# Link debug binary
dtwi-gb dtwi-gb.exe: $(dobj_files)
	$(CC) $(CFLAGS) $(DBG_CFLAGS) $(dobj_files) -o $@

# Compile debug object files
dobj/%.o:
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(DBG_CFLAGS) src/$*.c -o $@

#========================================================================
# TEST RULES
#========================================================================
t: test
tst: test
test: ttwi-gb

# Link test binary
ttwi-gb ttwi-gb.exe: $(tobj_files)
	$(CC) $(CFLAGS) $(DBG_CFLAGS) $(tobj_files) -o $@

# Compile test object files
tobj/%.o:
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(DBG_CFLAGS) src/$*.c -o $@

#========================================================================
# Uses specified C compiler to generate a makefile (.mk) for the object
# file to-be-created from the prerequisite source file.
# This resulting makefile contains a single rule with the debug and
# release object files as its targets, and the base source file and
# every non-system source and header file which it includes as its
# prerequisites.

make/%.mk: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MM -MT "dobj/$*.o obj/$*.o tobj/$*.o" -MF $@ $^
#========================================================================

#========================================================================
# Deletes all automatically-generated files.
.PHONY: clean
clean:
	rm -rf tobj dobj obj make ttwi-gb ttwi-gb.exe dtwi-gb dtwi-gb.exe twi-gb twi-gb.exe
#========================================================================

# Include all automatically-generated makefiles.
# This is done below all rules to ensure that included rules will never
# be the default rules.
include $(make_files)

