cc = gcc

d: debug
dbg: debug
debug: dbg-dirs dbg-twi-gb

dbg-dirs:
	mkdir -p dbg-obj/twi dbg-make/twi

.PHONY: clean
clean:
	rm -rf obj dbg-obj make dbg-make dbg-twi-gb dbg-twi-gb.exe twi-gb twi-gb.exe

