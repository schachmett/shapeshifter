// standard library:
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <mutex>
#include <thread>

// POSIX/UNIX specific:
#include <sys/types.h>  // some types (also size_t, mutex)
#include <unistd.h>     // some POSIX API calls like pipe, fork

// external libraries:
#include <Magick++.h>
#include <magick/image.h>

// project headers
#include "led-matrix.h"
#include "sprite.h"
#include "led-loop.h"

using Sprites::SpriteList;
using Sprites::Sprite;


static volatile bool INTERRUPT_RECEIVED = false;


void animate(led_loop::SpriteAnimationLoop* animation) {
  animation->startLoop();
}
void dostuff(SpriteList* sprites, std::mutex* sprites_mutex) {
  Sprite* werder = (*sprites)["werder"];
  while (!INTERRUPT_RECEIVED) {
    led_loop::sleepMillis(1000);
    sprites_mutex->lock();    // is it even necessary?
    werder->addSpeed(0.1);
    sprites_mutex->unlock();
  }
}


struct Options {
  rgb_matrix::RuntimeOptions rgb_runtime;
  rgb_matrix::RGBMatrix::Options matrix;
  led_loop::LoopOptions loop;
  int verbosity;
};
static void handleInterrupt(int signo) { INTERRUPT_RECEIVED = true; }
static bool parseOptions(int argc, char* argv[], Options* options);
static int usage(const char *progrname, const char *msg = nullptr);

int main(int argc, char *argv[]) {
  fprintf(stdout, "Starting shapeshifter...\n");
  signal(SIGTERM, handleInterrupt);
  signal(SIGINT, handleInterrupt);

  Options* options = new Options();
  if (!parseOptions(argc, argv, options)) {
    return usage(argv[0], "Failed parsing options");
  }

  Magick::InitializeMagick(*argv);

  rgb_matrix::RGBMatrix *matrix = rgb_matrix::CreateMatrixFromOptions(
    (options->matrix), (options->rgb_runtime));
  if (matrix == NULL) {
    return usage(argv[0], "Matrix creation failed");
  }
  SpriteList* sprites = new SpriteList();

  std::mutex sprites_mutex;
  led_loop::SpriteAnimationLoop animation(
      matrix, sprites, &sprites_mutex, &(options->loop));

  Sprite* werder = new Sprite("sprites/clubs/bremen42.png");
  werder->setID("werder");
  werder->setPosition(Sprites::Point(10, 10));
  werder->setDirection(33.8);
  werder->setSpeed(0.5);
  (*sprites)["werder"] = werder;

  std::thread animation_thread(animate, &animation);
  std::thread worker_thread(dostuff, sprites, &sprites_mutex);

  animation_thread.join();
  worker_thread.join();

  if (INTERRUPT_RECEIVED) {
    fprintf(stderr, "Caught interrupt signal. Exiting.\n");
  } else {
    fprintf(stdout, "Exiting without any reason\n");
  }

  matrix->Clear();
  delete matrix;
  delete sprites;
  return 0;
}



static bool parseOptions(int argc, char* argv[], Options* options) {
  options->matrix.hardware_mapping = "regular";
  options->matrix.rows = 64;
  options->matrix.cols = 64;
  options->matrix.chain_length = 3;
  // helps with tearing in the middle row:
  options->matrix.pwm_dither_bits = 1;
  // might also help with tearing?:
  // matrix_options.limit_refresh = 100;
  options->rgb_runtime.drop_privileges = 1;
  if (!rgb_matrix::ParseOptionsFromFlags(
      &argc, &argv, &(options->matrix), &(options->rgb_runtime))) {
    return false;
  }

  int opt;
  while ((opt = getopt(argc, argv, "f:v")) != -1) {
    switch (opt) {
      case 'f': options->loop.frame_time_ms = strtoul(optarg, NULL, 0); break;
      case 'v': options->verbosity = 5; break;
      default:  return false;
    }
  }
  return true;
}

static int usage(const char *progname, const char *msg) {
  if (msg) {
    fprintf(stderr, "%s\n", msg);
  }
  rgb_matrix::PrintMatrixFlags(stderr);
  fprintf(stderr, "Server application that pushes a few sprites on a panel\n");
  fprintf(stderr, "usage: %s [options] <video> [<video>...]\n", progname);
  fprintf(stderr, "Options:\n"
          "\t-f                 : Frame duration in ms.\n"
          "\t-v                 : Verbose mode.\n");
  return 1;
}
