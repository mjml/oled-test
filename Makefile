# Standard parameters.
# Notice:
#   the mcu gcc string
#   the F_CPU clock
#   the -DARDUINO define which helps emulate the Processing environment.
#   the optimization level
#   the include directories for whatever submodules / imports you may have
CC=avr-gcc
CCPP=avr-g++
OBJCOPY=avr-objcopy
ARDUINOHOME=/usr/share/arduino/hardware/arduino/avr
CORE=$(ARDUINOHOME)/cores/arduino
LIBRARIES=$(ARDUINOHOME)/libraries
VARIANT=standard
VARIANTPATH=$(ARDUINOHOME)/variants/$(VARIANT)
CCPARAMS=-O1 -DF_CPU=8000000L -DARDUINO -fno-fat-lto-objects -flto -mmcu=atmega168 -c -I u8g2/cppsrc -Iu8g2/csrc -Iarduino
#CCPARAMS=-O1 -DF_CPU=8000000L -fno-fat-lto-objects -flto -mmcu=atmega168 -c
#LDPARAMS=-mmcu=atmega168 -flto
LDPARAMS=-mmcu=atmega168
DUDE=avrdude
RM=rm


# The 3v3 pro minis are atmega168pa but the device codes on them suggest that they are atmega168
MCU=atmega168
PROGRAMMER_BAUD=19200
PROGRAMMER_TYPE=arduino
CONSOLE_BAUD=19200


## Emulate an Arduino Processing environment using these variables.
## The modules are picked out of the /usr/share/arduino/hardware/arduino/avr tree
## You can narrow/widen this path to include files as you see fit, but this default covers all of
##   cores/ and libraries/ where most of the builtin header files are found.
arduino_modules    = hooks.c wiring.c wiring_digital.c Print.cpp abi.cpp Wire.cpp utility/twi.c SPI.cpp HardwareSerial.cpp HardwareSerial0.cpp
arduino_headers    = Wire.h utility/twi.h Stream.h Print.h WString.h Printable.h wiring_private.h Arduino.h binary.h WCharacter.h HardwareSerial.h HardwareSerial_private.h USBAPI.h pins_arduino.h SPI.h
arduino_objects    = $(patsubst %.c,arduino/%.o,$(patsubst %.cpp,arduino/%.obj,$(arduino_modules)))
arduino_local_headers = $(patsubst %,arduino/%,$(arduino_headers))


## Additional modules and includes are underneath the local u8x8 git submodule folder.
u8x8cpp_modules  = U8x8lib.cpp
u8x8_modules     = u8x8_8x8.c u8x8_cad.c u8x8_display.c u8x8_gpio.c u8x8_byte.c u8x8_setup.c u8x8_fonts.c u8x8_d_ssd1306_128x64_noname.c
u8x8cpp_objects  = $(patsubst %.cpp,u8g2/cppsrc/%.obj,$(u8x8cpp_modules))
u8x8_objects     = $(patsubst %.c,u8g2/csrc/%.o,$(u8x8_modules))


## By default, just build but don't upload.
default: test-oled2

.PHONY: clean upload run arduino_headers

# Upload and run are separate steps
upload: test-oled2.hex
	$(DUDE) -F -V -c $(PROGRAMMER_TYPE) -p $(MCU) -P /dev/ttyUSB0 -b $(PROGRAMMER_BAUD) -U flash:w:$<

run: upload
	sleep 2
	minicom -8 -b $(CONSOLE_BAUD) -D /dev/ttyUSB0 

clean:
	$(RM) -f test-oled.hex
	find -name '*.o' -delete
	find -name '*.obj' -delete
	rm -rf arduino

twi.o: twi.c
	avr-gcc $(CCPARAMS) -o $@ $^


# This directory and the following rules provide copies of the Arduino processing environment files to the build.
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

arduino/pins_arduino.h: arduino # important: select the right variant or the pin configurations will be wrong!
	cp $(VARIANTPATH)/pins_arduino.h arduino/pins_arduino.h


# u8g2 and u8x8 files are in a submodule folder
u8g2/cppsrc/%.obj: u8g2/cppsrc/%.cpp $(arduino_local_headers)
	$(CCPP) $(CCPARAMS) -o $@ $(filter-out %.h,$^)

u8g2/csrc/%.o: u8g2/csrc/%.c
	$(CC) $(CCPARAMS) -o $@ $(filter-out %.h,$^)


%.obj: %.cpp
	$(CCPP) $(CCPARAMS) -o $@ $(filter-out %.h,$^)

%.o: %.c
	$(CC) $(CCPARAMS) -o $@ $(filter-out %.h,$^)

# Finally, our actual program
test-oled.obj: test-oled.cpp $(arduino_local_headers)
	$(CCPP) $(CCPARAMS) -o $@ $(filter-out %.h,$^)

test-oled: test-oled.obj $(u8x8_objects) $(u8x8cpp_objects) $(arduino_objects)
	$(CCPP) $(LDPARAMS) $^ -o $@ 

test-oled.hex: test-oled
	$(OBJCOPY) -O ihex -R .eeprom $< $@


test-oled2.o: test-oled2.c $(arduino_local_headers)
	$(CC) $(CCPARAMS) -o $@ $(filter-out %.h,$^)

test-oled2: test-oled2.o async-uart.o i2c.o $(u8x8_objects) $(u8x8cpp_objects)
	$(CC) $(LDPARAMS) $^ -o $@ 

test-oled2.hex: test-oled2
	$(OBJCOPY) -O ihex -R .eeprom $< $@


test3.o: test3.c
	$(CC) $(CCPARAMS) -o $@ $^

test3: test3.o async-uart.o
	$(CC) $(LDPARAMS) $^ -o $@ 

test3.hex: test3
	$(OBJCOPY) -O ihex -R .eeprom $< $@

