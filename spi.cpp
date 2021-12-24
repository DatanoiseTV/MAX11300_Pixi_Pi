#include "Pixi.h"
#include <cstdio>

#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <math.h>
#include <time.h>

Pixi pixi;

float voltageScaler = (1 << 12) / 10;

static int tick_thread(void *data);

int main(int argc, char **argv) {
  uint32_t Pixi_ID = 0;
  float Temp = 0;
  uint32_t test = 0;

  Pixi_ID = pixi.config();

  if (Pixi_ID == 0x0424) {

    test = pixi.readRawTemperature(TEMP_CHANNEL_INT);
    uint32_t temperature = pixi.readTemperature(TEMP_CHANNEL_INT);
    printf("Internal Temperature: %i (Raw: %08x)\n", temperature, test);

    // configure channels for functialnal test
    // Channel 0 DAC
    pixi.configChannel(CHANNEL_0, CH_MODE_DAC, 0, CH_0_TO_10P, 0);
    pixi.configChannel(CHANNEL_1, CH_MODE_ADC_P, 0, CH_0_TO_10P, 0);

  } else {
    printf("No PIXI module found!\n");
  };

  // pixi.writeAnalog ( CHANNEL_0, 1024 );

  SDL_Thread *thread = SDL_CreateThread(tick_thread, "tick", NULL);
  while (1) {
    usleep(50);
  }
}

void PIXI_random() {
  std::srand(time(NULL));

  while (1) {
    // uint16_t rand = std::rand() % 24;

    uint16_t ch1 = pixi.readAnalog(CHANNEL_1);

    pixi.writeAnalog(CHANNEL_0, ch1);
    // usleep(10);
  }
}

static int tick_thread(void *data) {
  (void)data;

  printf("Starting Tick thread.\n");

  while (1) {
    PIXI_random();
  }

  return 0;
}
