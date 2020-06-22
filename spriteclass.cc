#include "led-matrix.h"
#include "pixel-mapper.h"
#include "content-streamer.h"

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <Magick++.h>
#include <magick/image.h>

using rgb_matrix::GPIO;
using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamReader;

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

struct PanelSize {
  PanelSize() : x(192), y(64) {};
  PanelSize(size_t x_, size_t y_) : x(x_), y(y_) {};
  size_t x;
  size_t y;
};

struct Pixel {
  Pixel() : red(0), green(0), blue(0) {};
  Pixel(char red_, char green_, char blue_) :
    red(red_), green(green_), blue(blue_) {};
  char red;
  char green;
  char blue;
};
struct Point {
  Point() : x(0), y(0) {};
  Point(double x_, double y_) : x(x_), y(y_) {};
  double x;
  double y;
};
struct ColoredPixel {
  Pixel pixel;
  Point point;
};
typedef std::vector<Pixel> PixelColumn;
typedef std::vector<std::vector<Pixel>> PixelMatrix;
typedef std::vector<ColoredPixel> PixelList;


static Magick::Image loadImage(const char *filename, double resize_factor) {
  Magick::Image img;
  std::vector<Magick::Image> frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception& e) {
    if (e.what()) fprintf(stderr, "Magickimage error: %s\n", e.what());
  }
  if (frames.size() == 0) fprintf(stderr, "No image found.\n");
  if (frames.size() > 1) fprintf(stderr, "Discarded additional frames.\n");

  img = frames[0];

  if (resize_factor != 1.0) {
    const double target_width = (double) img.columns() * resize_factor;
    const double target_height = (double) img.rows() * resize_factor;
    img.scale(Magick::Geometry(target_width, target_height));
  }
  return img;
}

static PixelMatrix loadMatrix(const char *filename, double resize_factor) {
  PixelMatrix mat;
  Magick::Image img = loadImage(filename, resize_factor);
  for (size_t img_y = 0; img_y < img.rows(); ++img_y) {
    PixelColumn column;
    for (size_t img_x = 0; img_x < img.columns(); ++img_x) {
      const Magick::Color &c = img.pixelColor(img_x, img_y);
      char red = (char) ScaleQuantumToChar(c.redQuantum());
      char green = (char) ScaleQuantumToChar(c.greenQuantum());
      char blue = (char) ScaleQuantumToChar(c.blueQuantum());
      Pixel * pixel = new Pixel({red, green, blue});
      column.push_back(*pixel);
    }
    mat.push_back(column);
  }
  return mat;
}

class Sprite {
public:
  Sprite() : matrix_(), position_(), direction_angle_(0), speed_(0),
             max_dimensions_() {};
  Sprite(const char *filename) : matrix_(), position_(), direction_angle_(0),
                                 speed_(0), max_dimensions_() {
    loadMatrix(filename);
  };

  void doStep() {
    position_.x += cos(direction_angle_ * M_PI / 180) * speed_;
    position_.y += sin(direction_angle_ * M_PI / 180) * speed_;

    if (position_.x + width_ < 0) {
      position_.x += max_dimensions_.x + width_;
    } else if (std::round(position_.x) > max_dimensions_.x) {
      position_.x -= max_dimensions_.x + width_;
    }
    if (position_.y + height_ < 0) {
      position_.y += max_dimensions_.y + height_;
    } else if (std::round(position_.y) > max_dimensions_.y) {
      position_.y -= max_dimensions_.y + height_;
    }
  }

  void loadMatrix(PixelMatrix mat) {
    matrix_.swap(mat);
    height_ = matrix_.size();
    height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
  }
  void loadMatrix(const char *filename) {
    PixelMatrix mat = ::loadMatrix(filename, 1.0);
    matrix_.swap(mat);
    height_ = matrix_.size();
    height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
  }
  void loadMatrix(const char *filename, double resize_factor) {
    PixelMatrix mat = ::loadMatrix(filename, resize_factor);
    matrix_.swap(mat);
    height_ = matrix_.size();
    height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
  }
  size_t height() {
    return height_;
  }
  size_t width() {
    return width_;
  }
  void setPixel(int x, int y, Pixel * pixel);
  void setPixel(int x, int y, char r, char g, char b);
  Pixel * getPixel(int x, int y) {
    if (height() < (size_t) y) return new Pixel();
    if (width() < (size_t) x) return new Pixel();
    Pixel * pixel = &matrix_[y][x];
    return pixel;
  }
  void setPosition(Point p) {
    position_ = p;
  }
  Point * getPosition() {
    return &position_;
  }
  void setDirection(double ang) {
    direction_angle_ = ang;
  }
  void setDirection(char dir) {
    if (dir == 'u') {
      direction_angle_ = 270;
    } else if (dir == 'l') {
      direction_angle_ = 180;
    } else if (dir == 'd') {
      direction_angle_ = 90;
    } else {
      direction_angle_ = 0;
    }
  }
  double * getDirection() {
    return &direction_angle_;
  }
  void setSpeed(double speed) {
    speed_ = speed;
  }
  double * getSpeed() {
    return &speed_;
  }

  void flip();
  void flop();
  void rotate();
private:
  PixelMatrix matrix_;
  Point position_;
  double direction_angle_;
  double speed_;
  PanelSize max_dimensions_;
  size_t width_;
  size_t height_;
};


typedef std::vector<Sprite*> SpriteList;


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
