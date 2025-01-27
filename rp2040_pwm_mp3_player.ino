/*
    MP3 PWM Player for RP2040 with SSD1306 display

    Copyleft (c) 2025 mztulip <mail@tulip.lol>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <BackgroundAudio.h>
#include <PWMAudio.h>
#include <I2S.h>
#include <vector>
#include <string.h>

#include <Adafruit_GFX.h>
#include "wire.h"
#include "Adafruit_SSD1306.h"
#include "SdFat.h"
#include "pio_encoder.h"

PioEncoder encoder(21); 

const uint8_t SCREEN_WIDTH = 128; // OLED display width, in pixels
const uint8_t SCREEN_HEIGHT = 64; // OLED display height, in pixels

ModAdafruit_SSD1306 *display = 0;

PWMAudio audio(0);
// We will make a larger buffer because SD cards can sometime take a long time to read
BackgroundAudioMP3Class<RawDataBuffer<16 * 1024>> BMP(audio);

// List of all MP3 files in the root directory
std::vector<String> mp3list;
//Index of above buffer for currently playing song
int song_index = 0;

// Read buffer that's better off not in the stack due to its size
uint8_t filebuff[512];

const uint8_t SD_CS_PIN = 5;
const uint8_t SPI_SCK = 6;
const uint8_t SPI_MOSI = 7; //TX
const uint8_t SPI_MISO = 4; //RX
//Measured with oscilloscope clock is 11.76MHz and this is maximum.
const uint32_t SPI_CLOCK = 12000000;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK, &SPI)

SdFat32 sd;

// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))

  char file_name[255]; 

void scanDirectory() 
{
  File32 file;
  File32 dir;
  
  String dirname = "/";
  if (!dir.open(dirname.c_str())) 
  {
    error("dir.open failed\n\r");
  }

  while (true) 
  {
    Serial.printf("Openning next file\n\r");
    file.openNext(&dir, O_RDONLY);
    if (!file)
    {
      Serial.printf("No new file to open\n\r");
      break;
    }

    file.getName(file_name, 255);
    String path = dirname;
    path += String(file_name);

    if (file.isDir()) 
    {
      Serial.printf("Skipped directory: %s\n\r", path.c_str());
      if (file_name[0] == '.') 
      {
        goto close_file;
      }
    } 

    else if (strstr(file_name, ".mp3")) 
    {
      mp3list.push_back(path);
      Serial.printf("Added to playlist: %s\n\r", path.c_str());
    } else 
    {
      Serial.printf("Skipped: %s\n\r", path.c_str());
    }

    close_file:
    file.close();
  }
}

void init_sdcard_spi()
{
  SPI.setSCK(SPI_SCK);
  SPI.setTX(SPI_MOSI);
  SPI.setRX(SPI_MISO);

  //https://github.com/greiman/SdFat/blob/master/src/SdCard/SdSpiCard.cpp
  if (!sd.begin(SD_CONFIG)) 
  {
    sd.initErrorHalt(&Serial);
  }

  Serial.println("Card successfully initialized.");

}

float volume = 1.0;
File32 file;
bool next_song = true;

void setup() 
{
  multicore_launch_core1(main2);
  srand(rp2040.hwrand32());

  gpio_init(20);
  gpio_set_dir(20,GPIO_IN);
  gpio_pull_up(20);

  encoder.begin();
  encoder.reset(100);

 
  AsyncWire1.setSDA(10);
  AsyncWire1.setSCL(11);
  AsyncWire1.setClock(1000);
  display = new  ModAdafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &AsyncWire1, -1);

  if(!display->begin(SSD1306_SWITCHCAPVCC, 0x3c)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (1) {
      delay(1000);
      rp2040.reboot();
    }
  }
  else
  {
    Serial.println(F("SSD1306 display initialisation success"));
  }

  display->clearDisplay();
  display->setTextSize(1);      // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0, 0);     // Start at top-left corner
  display->setCursor(4,4);
  display->print("hello");
  display->setCursor(4,24);
  display->print("wtf display works!");
  display->display();

  delay(5000);
  Serial.println("Sysclk frequency[Hz]:");
  Serial.println(clock_get_hz(clk_sys));
  Serial.println("\n\r");

  init_sdcard_spi();

  scanDirectory();

  song_index = random(mp3list.size());

  //By default is 49kHz, I want more
  audio.setPWMFrequency(160000);
  BMP.setGain(0.5); 

  BMP.begin();
  while(true)
  {
    
    if(next_song)
    {
      if(file.isOpen())
      {
        // BMP.end();
        file.close();
      }
      open_next_song(file);
      // BMP.begin();
      next_song = false;
    }

    set_volume();
    read_sd_fill_mp3_buffer(file);
  }
}

void set_volume()
{
  static uint32_t previous_ms = millis();
  if (millis() - previous_ms >= 100) 
  {
    previous_ms = millis();
    int32_t encoder_val = encoder.getCount();
    if(encoder_val < 0) {encoder_val = 0;}
    uint32_t volume_encoder = (encoder_val%101);
    volume = (float)volume_encoder/100.0;
    Serial.println(volume);
    BMP.setGain(volume); 
  }

}

bool check_button()
{
  static bool previous_button_state = gpio_get(20);
  bool button_state = gpio_get(20);
  bool next_song = false;
  if (previous_button_state == true && button_state == false)
  {
    next_song = true;
  }
  previous_button_state = button_state;
  return next_song;
}

void read_sd_fill_mp3_buffer(File32 &file)
{
  while (file.isOpen() && BMP.availableForWrite() > 512) 
  {
    int len = file.read(filebuff, 512);
    BMP.write(filebuff, len);
    if (len != 512)
    {
      file.close();
      next_song = true;
    }
  }
}

void open_next_song(File32 &file)
{
  song_index++;
  if (song_index >= (int)mp3list.size())
  {
    song_index = 0;
  }

  file = sd.open(mp3list[song_index]);
  Serial.printf("\r\n\r\nNow playing: %s\r\n", mp3list[song_index].c_str());
}

void update_display()
{
  static uint32_t previous_ms = millis();
  if (millis() - previous_ms >= 100)
  {
    display->clearDisplay();
    display->setCursor(4,0);
    display->print("Volume:");
    display->print(volume);
    display->print(" [");
    display->print(song_index);
    display->print("]");
    display->setCursor(4,24);
    display->print("Playing:");
    display->print(mp3list[song_index].c_str());
    display->display();
  }
}

void loop() 
{
}

void main2()
{
  gpio_init(25);
  gpio_set_dir(25,GPIO_OUT);
  gpio_put(25,true);

  while(true)
  {
    static uint32_t previous_ms = millis();
    if (millis() - previous_ms >= 500) 
    {
      previous_ms = millis();
      gpio_xor_mask(1<<25);
    }

    if(check_button())
    {
      next_song = true;
    }
    update_display();
  }
}

