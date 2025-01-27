# rp2040_pwm_mp3_player
Project aim is to create mp3 player using Raspberry pi Pico(RP2040). Music is stored on sd card. Interface is realised with I2C display SSD1306.

![alt text](https://github.com/mztulip/rp2040_pwm_mp3_player/blob/main/hardware_img/main.png?raw=true)

Project uses Arduino enviroment. It was tested and compiled using arduino ide 2.3.4.
Sdcard is used in SPI mode for reading files, formatted with Fat32.
Application search for mp3 files in main directory. 
Display SSD1306 is connected using I2C interface. It shows volume control, current song index
with file name. Encoder is used to control volume. Pressing encoder button will go to the next song.

Application is written with two core usage. First core reads data from SDcard to buffer in RAM,
then DMA is used to transfer it to PWM peripheral. 
Second core realises user interface. It checks button pressing, encoder reading and updating display.
Arduino Wire and SSd1306 library files are copied locally to project.
It was modified to use I2C in polling mode without DMA.(probably this is not necessary, but I had problems
during development and this modyfication was created to simplify project, maybe it not necessary now. I was also interested how
I2C was implemented, then modification was good case for analysing code).

GP0 is used as PWM audio output. 
* Small speaker could be connected directly beetwen GP0 and GND.
It works without DC block and sound not bad, but it is not loud enought to be comfortable to listen music.
* RC filter with DC block and external amplifier can be used
* Simple Class D with PWM presented here. I connected inverter created from NMOS and PMOS transistors(AO3400, AO3407), driven directly from GP0. At the inverter output LC filter is connected as presented in simulations. Inverter is supplied from 3.3V. At the end 0.5W speaker is connected(it works without DC block). Sound quality is good enought to have fun with player. Output power is big enough to oversteer 0.5W speaker.


# External libraries used in project

* Arduino core for rp2040 https://github.com/earlephilhower/arduino-pico
* BackgroundAudio https://github.com/earlephilhower/BackgroundAudio/tree/master
* encoder library https://github.com/gbr1/rp2040-encoder-library
* Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306/tree/master
* SdFat library https://github.com/greiman/SdFat