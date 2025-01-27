# rp2040_pwm_mp3_player
Project aim is to create mp3 player using Raspberry pi Pico(RP2040). Music is stored on sd card. Interface is realised with I2C display SSD1306. Mono audio is played using PWM and amplified with external inverter.

![alt text](https://github.com/mztulip/rp2040_pwm_mp3_player/blob/main/hardware_img/main.png?raw=true)

Project uses Arduino environment. It was tested and compiled using arduino ide 2.3.4.
Sdcard is used in SPI mode for reading files, formatted with Fat32.
Application search for mp3 files in main directory. 
SSD1306 display is connected using I2C interface. It presents volume control, current song number,
file name. Encoder is used to control volume. Pressing encoder button will result in switching to the next song.

Application is written with two cores. First core reads data from SDcard to buffer in RAM, decodes MP3,
then DMA is used to transfer it to PWM peripheral. 
Second core realises user interface. It checks if  button is pressed, reads encoder position and updates display.
Arduino Wire and SSd1306 library files are copied locally to project.
It was modified to use I2C in polling mode without DMA.(I had problems
during development and this modyfication was created to simplify project, maybe it not necessary now. I was also interested how
I2C was implemented, then modification was good case for analysing code).

GP0 is used as PWM audio output. 
* Small speaker could be connected directly beetwen GP0 and GND.
It works with or without DC block and sound is not bad, but it is not loud enought to be comfortable to listen music.
* RC filter with DC block and external amplifier can be used
* Simple Class D using PWM presented here. I connected inverter created with NMOS and PMOS transistors(AO3400, AO3407).
Inverter is driven directly from GP0. LC filter is connected directly to the inverter output, as presented in simulations. 
![alt text](https://github.com/mztulip/rp2040_pwm_mp3_player/blob/main/filter_sim/class_d_sim.png?raw=true)
Inverter is supplied from 3.3V from Rpi Pico. Speaker(0.5W) is connected to filter out(it works without DC block). 
Sound quality is good enought to have fun with player. Output power is huge enough to oversteer 0.5W speaker.


# External libraries used in project

* Arduino core for rp2040 https://github.com/earlephilhower/arduino-pico
* BackgroundAudio https://github.com/earlephilhower/BackgroundAudio/tree/master
* encoder library https://github.com/gbr1/rp2040-encoder-library
* Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306/tree/master
* SdFat library https://github.com/greiman/SdFat