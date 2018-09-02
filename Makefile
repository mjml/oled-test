CC=avr-gcc
CCPP=avr-g++
OBJCOPY=avr-objcopy
ARDUINOHOME=/usr/share/arduino/hardware/arduino/avr
CORE=$(ARDUINOHOME)/cores/arduino
LIBRARIES=$(ARDUINOHOME)/libraries
VARIANT=$(ARDUINOHOME)/variants/standard
U8G2INCL=$(shell pwd)/u8g2/cppsrc
CCPARAMS=-O1 -DF_CPU=8000000L -mmcu=atmega168p -c -Iu8g2/csrc -Iarduino
DUDE=avrdude
RM=rm

# The 3v3 pro minis are atmega168pa but the device codes on them suggest that they are atmega168
MCU=atmega168
PROGRAMMER_BAUD=19200
PROGRAMMER_TYPE=arduino #This works for my little Baite-009 ch340 device

CONSOLE_BAUD=9600

arduino_modules    = wiring_digital.c Wire.cpp utility/twi.c
arduino_headers    = Wire.h utility/twi.h Stream.h Print.h WString.h Printable.h wiring_private.h Arduino.h binary.h WCharacter.h HardwareSerial.h USBAPI.h pins_arduino.h
arduino_objects    = $(patsubst %.c,arduino/%.o,$(patsubst %.cpp,arduino/%.obj,$(arduino_modules)))
arduino_local_headers = $(patsubst %,arduino/%,$(arduino_headers))

u8x8cpp_modules  = 
u8x8_modules     = $(patsubst %.c,%,$(shell mkdir -p arduino && ls u8g2/csrc | grep '^u8x8.*\.c' | grep -v '^u8x8_d_.*')) u8x8_d_ssd1306_128x64_noname u8log_u8x8
u8x8cpp_objects  = $(patsubst %,u8g2/cppsrc/%.o,$(u8x8cpp_modules))
u8x8_objects     = $(patsubst %,u8g2/csrc/%.o,$(u8x8_modules))

default: test-oled

.PHONY: clean upload run arduino_headers

clean:
	$(RM) -f test-oled.hex
	find -name '*.o' -delete
	find -name '*.obj' -delete
	rm -rf arduino

twi.o: twi.c
	avr-gcc $(CCPARAMS) -o $@ $^

arduino:
	mkdir -p arduino

arduino_headers: $(arduino_local_headers)

arduino/%.cpp: arduino arduino_headers
	mkdir -p arduino/$(*D) && cp $(shell find /usr/share/arduino/hardware/arduino/avr -name $(@F)) arduino/$(*D)/$(*F).cpp

arduino/%.c: arduino arduino_headers
	mkdir -p arduino/$(*D) && cp $(shell find /usr/share/arduino/hardware/arduino/avr -name $(@F)) arduino/$(*D)/$(*F).c

arduino/%.h: arduino
	mkdir -p arduino/$(*D) && cp $(shell find /usr/share/arduino/hardware/arduino/avr -name $(@F)) arduino/$(*D)/$(*F).h

arduino/%.obj: arduino/%.cpp
	$(CCPP) $(CCPARAMS) -o $@ $^

arduino/%.o: arduino/%.c
	$(CC) $(CCPARAMS) -o $@ $^

arduino/pins_arduino.h: arduino
	cp /usr/share/arduino/hardware/arduino/avr/variants/standard/pins_arduino.h arduino/pins_arduino.h

u8g2/cppsrc/%.obj: u8g2/cppsrc/%.cpp
	$(CCPP) $(CCPARAMS) -o $@ $^

u8g2/csrc/%.o: u8g2/csrc/%.c
	$(CC) $(CCPARAMS) -o $@ $^

test-oled.obj: test-oled.cpp | arduino_headers
	$(CCPP) $(CCPARAMS) -o $@ $^

test-oled: test-oled.obj $(u8x8_objects) $(u8x8cpp_objects) $(arduino_objects)
	$(CCPP) -mmcu=$(MCU) $^ -o $@ 

test-oled.hex: test-oled
	$(OBJCOPY) -O ihex -R .eeprom $< $@

upload: test-oled.hex
	$(DUDE) -F -V -c $(PROGRAMMER_TYPE) -p $(MCU) -P /dev/ttyUSB0 -b $(PROGRAMMER_BAUD) -U flash:w:test-oled.hex

run: @upload
	sleep 2
	minicom -8 -b $(CONSOLE_BAUD) -D /dev/ttyUSB0 
