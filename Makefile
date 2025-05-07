##.PHONY: all
##all: SP_audio SP_mic SPtone_player SP_touchpad

# Default target to build the main program
all: SP_test

# Build the main program with touchpad functionality
SP_test: main.c src/mic.c src/audio_processing.c src/SPmouse_HID.c src/SPsound_generator.c src/SPtouchpad_reader.c
	gcc -I include -I /usr/include/libevdev-1.0 -I /usr/include \
	    -L /usr/lib/x86_64-linux-gnu \
	    -o $@ $^ -lasound -lm -levdev -ludev

# Clean target to remove built files
clean:
	rm -f SP_test
