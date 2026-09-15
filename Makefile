FQBN := esp32:esp32:esp32c3:CDCOnBoot=cdc
PORT := COM7

all: upload monitor

check-board:
	arduino-cli board list
	arduino-cli board details --fqbn $(FQBN)

setup:
	arduino-cli core install esp32:esp32

build:
	arduino-cli compile --fqbn $(FQBN) main/ #--verbose

upload: build
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) main/

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=115200 -c dtr=off -c rts=off