.PHONY: clean

mkfs: mkfs.c
	gcc -O1 -Wall -Werror -o $@ -I../kernel/framework $<

clean:
	rm -rf mkfs
