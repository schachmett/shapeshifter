
#include <cmath>
#include <vector>
#include <cstring>

#include <Magick++.h>
#include <magick/image.h>


namespace Sprites {

typedef std::string spriteID;

struct PanelSize {
  PanelSize() : x(192), y(64) {};
  PanelSize(size_t x_, size_t y_) : x(x_), y(y_) {};
  size_t x;
  size_t y;
};

enum EdgeBehavior {
  UNDEFINED_EDGE_BEHAVIOR;
  LOOP_DIRECT;
  LOOP_INDIRECT;
  BOUNCE;
  STOP;
}

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
  Sprite() : matrix_(), position_(), direction_(0), speed_(0),
             max_dimensions_(), width_(0), height_(0),
             edge_behavior_(LOOP_INDIRECT) {};
  Sprite(const char *filename) : matrix_(), position_(), direction_(0),
                                 speed_(0), max_dimensions_(), width_(0),
                                 height_(0), edge_behavior_(LOOP_INDIRECT) {
    loadMatrix(filename);
  };

  void doStep() {
    direction_ = (direction_ + 360) % 360;
    double x = position_.x + cos(direction_ * M_PI / 180) * speed_;
    double y = position_.y + sin(direction_ * M_PI / 180) * speed_;

    if (edge_behavior_ == LOOP_INDIRECT) {
      if (x + width_ < 0) {
        x += max_dimensions_.x + width_;
      } else if (std::round(x) > max_dimensions_.x) {
        x -= max_dimensions_.x + width_;
      }
      if (y + height_ < 0) {
        y += max_dimensions_.y + height_;
      } else if (std::round(y) > max_dimensions_.y) {
        y -= max_dimensions_.y + height_;
      }
    } else if (edge_behavior_ == LOOP_DIRECT) {
      if (x < 0) {
        x += max_dimensions_.x;
      } else if (std::round(x) > max_dimensions_.x) {
        x -= max_dimensions_.x;
      }
      if (y < 0) {
        y += max_dimensions_.y
      } else if (std::round(y) > max_dimensions_.y) {
        y -= max_dimensions_.y
      }
    } else if (edge_behavior_ == BOUNCE) {
      if (x < 0 || x + width_ > max_dimensions_.x) {
        setDirection(180 - direction_);
      }
      if (y < 0 || y + height_ > max_dimensions_.y) {
        setDirection(360 - direction_);
      }
    } else if (edge_behavior_ == STOP) {
      if ((x < 0 && (direction_ + 270) % 360 < 180)
          || (x + width_ > max_dimensions_.x && (direction_ + 90) % 360 < 180)
          || (y < 0 && (direction_ + 180) % 360 < 180)
          || (y + height_ > max_dimensions_.y && direction_ < 180)) {
        speed_ = 0;
      }
    }
    position_.x = x;
    position_.y = y;
  }

  void loadMatrix(PixelMatrix mat) {
    matrix_.swap(mat);
    height_ = matrix_.size();
    height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
  }
  void loadMatrix(const char *filename) {
    PixelMatrix mat = Sprites::loadMatrix(filename, 1.0);
    matrix_.swap(mat);
    height_ = matrix_.size();
    height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
  }
  void loadMatrix(const char *filename, double resize_factor) {
    PixelMatrix mat = Sprites::loadMatrix(filename, resize_factor);
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
  void addPosition(Point p) {
    position_.x += p.x;
    position_.y += p.y;
  }
  Point * getPosition() {
    return &position_;
  }
  void setDirection(double ang) {
    direction_ = ang;
  }
  void setDirection(char dir) {
    if (dir == 'u') {
      direction_ = 270;
    } else if (dir == 'l') {
      direction_ = 180;
    } else if (dir == 'd') {
      direction_ = 90;
    } else {
      direction_ = 0;
    }
  }
  void addDirection(double ang_inc) {
    direction_ += ang_inc;
  }
  double * getDirection() {
    return &direction_;
  }
  void setSpeed(double speed) {
    speed_ = speed;
  }
  void addSpeed(double speed_inc) {
    speed_ += speed_inc;
  }
  double * getSpeed() {
    return &speed_;
  }
  EdgeBehavior getEdgeBehavior() {
    return edge_behavior_;
  }
  void setEdgeBehavior(EdgeBehavior edge_behavior) {
    edge_behavior_ = edge_behavior;
  }

  void flip();
  void flop();
  void rotate();
private:
  PixelMatrix matrix_;
  Point position_;
  double direction_;
  double speed_;
  PanelSize max_dimensions_;
  size_t width_;
  size_t height_;
  EdgeBehavior edge_behavior_;
};

typedef std::map<spriteID, Sprite*> SpriteList;

} // namespace Sprites
