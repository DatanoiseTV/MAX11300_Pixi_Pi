#include "Pixi.h"
#include <cstdio>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "lo/lo.h"

Pixi pixi;

SDL_mutex *spi_mutex;

static int tick_thread(void *data);

int done = 0;

void error(int num, const char *msg, const char *path) {
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
  fflush(stdout);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data) {
  int i;

  printf("path: <%s>\n", path);
  for (i = 0; i < argc; i++) {
    printf("arg %d '%c' ", i, types[i]);
    lo_arg_pp((lo_type)types[i], argv[i]);
    printf("\n");
  }
  printf("\n");
  fflush(stdout);

  return 1;
}

int cv_handler(const char *path, const char *types, lo_arg **argv, int argc,
               void *data, void *user_data) {
  /* example showing pulling the argument values out of the argv array */

  int chn = int(argv[0]->i);

  if (chn < 0 || chn > 10) {
    return 0;
  }

  int val = int(argv[1]->f * 4095.0);

  if (val > 4095) {
    val = 4095;
  }

  printf("(%s): Setting channel %i to %i.\n", path, chn, val);

  // We have to use a mutex to avoid two threads writing to SPI
  if (SDL_LockMutex(spi_mutex) == 0) {
    pixi.writeAnalog(CHANNEL_0 + chn, val);
    SDL_UnlockMutex(spi_mutex);
  }
  fflush(stdout);

  return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data) {
  done = 1;
  printf("quiting\n\n");
  fflush(stdout);

  return 0;
}

int main(int argc, char **argv) {

  int lo_fd;
  fd_set rfds;
#ifndef WIN32
  struct timeval tv;
#endif
  int retval;

  spi_mutex = SDL_CreateMutex();
  if (!spi_mutex) {
    fprintf(stderr, "Couldn't create mutex\n");
  }

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

  lo_server_thread st = lo_server_thread_new("7770", error);

  /* add method that will match any path and args */
  //lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);
  lo_server_thread_add_method(st, "/cv", "if", cv_handler, NULL);
  lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

  lo_server_thread_start(st);

  while (!done) {
    usleep(1000);
  }

  SDL_DestroyMutex(spi_mutex);
  pixi.~Pixi();
}

void PIXI_random() {
  std::srand(time(NULL));

  while (1) {
    // uint16_t rand = std::rand() % 24;

    //

    // pixi.writeAnalog(CHANNEL_0, ch1);

    if (SDL_LockMutex(spi_mutex) == 0) {
      uint16_t ch1 = pixi.readAnalog(CHANNEL_1);
      SDL_UnlockMutex(spi_mutex);
    }

    usleep(10);
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
