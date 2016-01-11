PITYPE := $(shell grep "Hardware" /proc/cpuinfo)

all:
ifneq (,$(findstring BCM2709,$(PITYPE)))
	@echo Compiling for Raspberry Pi2
	gcc -DBCM2835_PERI_BASE=0x3f000000 -o gps bcm2835.c bcm2835_i2cbb.c gps.c
else ifneq (,$(findstring BCM2708,$(PITYPE)))
	@echo Compiling for Raspberry Pi1
	gcc -DBCM2835_PERI_BASE=0x20000000 -o gps bcm2835.c bcm2835_i2cbb.c gps.c
else
	@echo Neither Raspberry Pi1 nor Pi2
	$(error /proc/cpuinfo has neither BCM2708 nor BCM2709)
endif

clean:
	rm -f *.o gps
