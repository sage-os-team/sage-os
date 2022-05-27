WORK_DIR  = $(shell pwd)

ifeq ($(target),)
export target := init
endif

SRCS = trampoline.S
SRCS += $(addsuffix .c, $(target))
SRCS += $(shell find $(WORK_DIR)/src -name "*.c")
OBJS = $(addsuffix .o, $(notdir $(basename $(SRCS))))

INC_PATH += $(WORK_DIR)/include
INCFLAGS += $(addprefix -I, $(INC_PATH))

CROSS_COMPILE = x86_64-linux-gnu-

CFLAGS  = -O1 -m64 -fPIC -mno-sse -ggdb $(INCFLAGS) -Wall -Werror
CFLAGS += -ffreestanding -fno-builtin -nostdlib -fno-stack-protector
CFLAGS += -fno-asynchronous-unwind-tables

LDFLAGS  = -static --omagic --pic-executable --no-dynamic-linker
LDFLAGS += --gc-sections

_init: $(SRCS) Makefile latest
	@echo "user: make target $(target)"
	@echo "-------------------------------"
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(SRCS)
	$(CROSS_COMPILE)ld $(LDFLAGS) -o $@ -e _start $(OBJS)
	$(CROSS_COMPILE)objcopy -S -j .text* -j.rodata* -j .data* -j .bss* --set-section-flags .bss=alloc,contents -O binary $@
	xxd -i $@ > ../kernel/src/uproc/initcode.inc

clean:
	rm -f *.o _*

latest:
