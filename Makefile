#========================================================================
# List twi source files to compile and link here.
define twi_std_source
	log.c
endef
# List twi_gb source files to compile and link here.
define twi_gb_source 
	main.c
endef
#========================================================================
# DO NOT MODIFY ANYTHING BELOW THIS POINT
# UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING!
#========================================================================

# Build list of sources with full directories from source variables
# above.
define src_files := 
	$(addprefix src/twi/std/,$(twi_std_source))
	$(addprefix src/twi/gb/,$(twi_gb_source))
endef

# Debug object files
dobj_files := $(patsubst src/%.c,dobj/%.o,$(src_files))
# Release object files
obj_files := $(patsubst src/%.c,obj/%.o,$(src_files))

# Debug and release make files.
make_files := $(patsubst src/%.c,make/%.mk,$(src_files))

# Directories
includes_dir = includes
libs_dir = libs

# Compilation flags
CFLAGS := -I$(includes_dir) -L$(libs_dir)
DBG_CFLAGS := -g -DTWI_GB_PROGNAME=\"dtwi-gb\"
RLS_CFLAGS := -DTWI_GB_NDEBUG -DTWI_GB_PROGNAME=\"twi-gb\"

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
# Uses specified C compiler to generate a makefile (.mk) for the object
# file to-be-created from the prerequisite source file.
# This resulting makefile contains a single rule with the debug and
# release object files as its targets, and the base source file and
# every non-system source and header file which it includes as its
# prerequisites.

make/%.mk: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MM -MT "dobj/$*.o obj/$*.o" -MF $@ $^
#========================================================================

#========================================================================
# Deletes all automatically-generated files.
.PHONY: clean
clean:
	rm -rf dobj obj dmake make dtwi-gb dtwi-gb.exe twi-gb twi-gb.exe
#========================================================================

# Include all automatically-generated makefiles.
# This is done below all rules to ensure that included rules will never
# be the default rules.
include $(make_files)
