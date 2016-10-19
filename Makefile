TARGET = main
DEVICE = atmega328
FREQUENCE = 16000000UL
OPTIMIZE = -Os
WARNINGS = -Wall -Wextra -Wpedantic -Wshadow -Wpointer-arith -Wcast-align \
           -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
           -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
           -Wuninitialized -Wconversion -Wstrict-prototypes
CFLAGS = -g -DF_CPU=$(FREQUENCE) $(OPTIMIZE) -mmcu=$(DEVICE) $(WARNINGS)
LDFLAGS = -Wl,-Map,bin/$(TARGET).map


SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/*.c, bin/%.o, $(SOURCES))


all: bin/$(TARGET)


bin/$(TARGET): $(OBJECTS)
	@echo
	@echo "Compiling..." $<
	mkdir -p bin
	avr-gcc $(CFLAGS) $(LDFLAGS) -o $@ $^
	avr-objcopy -j .text -j .data -O ihex $@ $@.hex
	avr-objcopy -j .text -j .data -O srec $@ $@.srec
	avr-objcopy -j .text -j .data -O binary $@ $@.bin
	avr-size --mcu=$(DEVICE) --format avr bin/$(TARGET)

clean:
	rm -rf bin
