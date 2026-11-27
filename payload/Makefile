LIBPS4 := $(PS4SDK)/libPS4
CC := gcc
OBJCOPY := objcopy
ODIR := build
SDIR := source
IDIRS := -I$(LIBPS4)/include
LDIRS := -L$(LIBPS4)
MAPFILE := Plutana-HEN.map
CFLAGS := $(IDIRS) -Os -std=c11 -ffunction-sections -fdata-sections -fno-builtin -nostartfiles -nostdlib -Wall -Wextra -Werror -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie -fPIC
LFLAGS := $(LDIRS) -Xlinker -T $(LIBPS4)/linker.x -Xlinker -Map=$(MAPFILE) -Wl,--build-id=none -Wl,--gc-sections
CFILES := $(wildcard $(SDIR)/*.c)
SFILES := $(wildcard $(SDIR)/*.s)
OBJS := $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(CFILES)) $(patsubst $(SDIR)/%.s,$(ODIR)/%.o,$(SFILES))
TARGET := Plutana-HEN.bin

$(TARGET): $(ODIR) $(OBJS)
	$(CC) $(LIBPS4)/crt0.s $(OBJS) -o temp.elf $(CFLAGS) $(LFLAGS) -lPS4
	$(OBJCOPY) -O binary temp.elf $@
	rm -f temp.elf
	test -s $@
	@echo "Built $@"

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.s
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(TARGET) $(MAPFILE) $(ODIR) temp.elf
