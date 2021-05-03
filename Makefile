#========================================================================
# List twi_gb source files to compile and link here.
define twi_gb_source 
	main.c
endef
#========================================================================
# DO NOT MODIFY ANYTHING BELOW THIS POINT
# UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING!
#========================================================================

# TODO: Build list of object and make files from list of source files.
# TODO: Append full filepath to each source file in source file lists.

# If C compiler isn't set, use default of cc.
cc ?= cc

#========================================================================
# DEBUG RULES
#========================================================================
d: debug
dbg: debug
debug: ddirs dtwi-gb

# Build debug binary.
dtwi-gb dtwi-gb.exe: 

#========================================================================
# Uses specified C compiler to generate a makefile (.mk) for the object
# file to-be-created from the prerequisite source file.
# This resulting makefile contains a single rule with the object file
# as the target and every non-system header included by the original
# source file as a prerequisite.

# Debug makefiles
dmake/%.mk: src/%.c
	mkdir -p $(@D)
	$(cc) -MM -MT dobj/$*.o -MF $@ $^
#========================================================================

#========================================================================
# Deletes all automatically-generated files.
.PHONY: clean
clean:
	rm -rf obj dobj make dmake dtwi-gb dtwi-gb.exe twi-gb twi-gb.exe
#========================================================================

# Include all automatically-generated makefiles.
# TODO
