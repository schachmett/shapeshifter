#include "led-matrix.h"
#include "sprite.h"

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>


using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}
static int usage(const char *progname);

typedef int64_t tmillis_t;
static const tmillis_t distant_future = (1LL<<40); // that is a while.
static tmillis_t GetTimeInMillis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
static void SleepMillis(tmillis_t milli_seconds) {
  if (milli_seconds <= 0) return;
  struct timespec ts;
  ts.tv_sec = milli_seconds / 1000;
  ts.tv_nsec = (milli_seconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

struct SpriteMoverOptions {
  SpriteMoverOptions() : resize(1.0), filename(NULL), ypos(0) {}
  double resize;
  char * filename;
  int ypos;
};


static void drawImage(Sprite *img, rgb_matrix::FrameCanvas *canvas) {
  int x0 = std::round(img->getPosition()->x);
  int y0 = std::round(img->getPosition()->y);
  for (size_t img_y = 0; img_y < img->height(); ++img_y) {
    for (size_t img_x = 0; img_x < img->width(); ++img_x) {
      const int x = img_x + x0;
      const int y = img_y + y0;
      Pixel * p = img->getPixel(img_x, img_y);
      if (p->red == 0 and p->green == 0 and p->blue == 0) continue;
      canvas->SetPixel(x, y, p->red, p->green, p->blue);
    }
  }
}


static bool nextFrame(FrameCanvas *canvas, SpriteList *sprites) {
  canvas->Clear();
  for(auto it = sprites->begin(); it != sprites->end(); ++it) {
    Sprite * sprite = *it;
    sprite->doStep();
    drawImage(sprite, canvas);
  }
  return true;
}

//Reminder: keep img out of parameter list, do sth more mutable
void runAnimation(RGBMatrix *matrix, FrameCanvas *offscreen_canvas,
                  SpriteList *sprites) {
  while (!interrupt_received && nextFrame(offscreen_canvas, sprites)) {
    const tmillis_t frame_delay_ms = 10;
    const tmillis_t start_wait_ms = GetTimeInMillis();
    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas, 1);
    const tmillis_t time_already_spent = GetTimeInMillis() - start_wait_ms;
    SleepMillis(frame_delay_ms - time_already_spent);
  }
}


int main(int argc, char *argv[]) {
  fprintf(stdout, "Starting sprite mover\n");
  Magick::InitializeMagick(*argv);

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 3;
  // matrix_options.
  // matrix_options.show_refresh_rate = true;
  // matrix_options.pwm_bits = 11;
  runtime_opt.drop_privileges = 1;

  SpriteMoverOptions spritemover_options;
  int opt;
  while ((opt = getopt(argc, argv, "s:f:")) != -1) {
    switch (opt) {
      case 's':
        spritemover_options.resize = atof(optarg);
        break;
      case 'f':
        spritemover_options.filename = optarg;
        break;
      default:
        return usage(argv[0]);
    }
  }

  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL) return 1;
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();

  char const *filename;
  if (spritemover_options.filename) {
    filename = spritemover_options.filename;
  } else {
    filename = "sprites/clubs/bremen42.png";
  }

  Sprite * img = new Sprite(filename);
  img->setDirection(30.0);
  img->setSpeed(0.3);
  Sprite * img2 = new Sprite("sprites/clubs/dortmund42.png");
  img2->setDirection(80.0);
  img2->setSpeed(0.5);
  img2->setPosition(Point(50, 2));
  Sprite * img3 = new Sprite("sprites/clubs/mainz42.png");
  img3->setDirection(170.0);
  img3->setSpeed(0.9);
  img3->setPosition(Point(170, 55));
  Sprite * img4 = new Sprite("sprites/clubs/koeln42.png");
  img4->setDirection(240.0);
  img4->setSpeed(0.2);
  img4->setPosition(Point(80, 12));
  Sprite * img5 = new Sprite("sprites/clubs/leverkusen42.png");
  img5->setDirection(0.0);
  img5->setSpeed(3);
  img5->setPosition(Point(80, 12));


  SpriteList sprites;
  sprites.push_back(img);
  sprites.push_back(img2);
  sprites.push_back(img3);
  sprites.push_back(img4);
  sprites.push_back(img5);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {
    runAnimation(matrix, offscreen_canvas, &sprites);
  }

  if (interrupt_received) {
    fprintf(stderr, "Caught signal. Exiting.\n");
  }

  matrix->Clear();
  delete matrix;
  return 0;
}

static int usage(const char *progname) {
  fprintf(stderr, "usage of %s: wrong\n", progname);
  return 1;
}
