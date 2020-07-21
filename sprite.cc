#include <cmath>

#include <Magick++.h>
#include <magick/image.h>

#include "sprite.h"


namespace Sprites {

  PanelSize::PanelSize(size_t x, size_t y) : x(x), y(y) { };
  Pixel::Pixel(char red, char green, char blue) : red(red), green(green), blue(blue) { };
  Point::Point(double x, double y) : x(x), y(y) {};


  Magick::Image loadImage(const char* filename,
                                 const double resize_factor) {
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

  PixelMatrix loadMatrix(const char *filename, double resize_factor) {
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

  Sprite::Sprite() :
    matrix(), width(0), height(0),
    max_dimensions(), edge_behavior(LOOP_INDIRECT),
    invisible(false), wrapped(false),
    position(),  direction(0), speed(0),
    position_goal(nan(""), nan("")), goal_steps(-1)
    { };
  Sprite::Sprite(const char* filename) : Sprite() {
    this->loadMatrix(filename);
  }
  void Sprite::loadMatrix(PixelMatrix mat) {
    this->matrix.swap(mat);
    this->height = this->matrix.size();
    this->height < 1 ? this->width = 0 : this->width = this->matrix[0].size();
  }
  void Sprite::loadMatrix(const char *filename, double resize_factor) {
    PixelMatrix mat = Sprites::loadMatrix(filename, resize_factor);
    this->loadMatrix(mat);
  }
  size_t Sprite::getWidth() {
    return this->width;
  }
  size_t Sprite::getHeight() {
    return this->height;
  }

  void Sprite::doStep() {
    this->direction = std::fmod(this->direction + 360, 360);
    double x = this->position.x + cos(this->direction * M_PI / 180) * this->speed;
    double y = this->position.y + sin(this->direction * M_PI / 180) * this->speed;
    this->invisible = false;
    this->wrapped = false;
    this->position = wrap_edge(x, y);
    if (this->goal_steps == 0) this->speed = 0;
    if (this->goal_steps >= 0) --this->goal_steps;
  }

  Pixel EMPTY_PIXEL = {0, 0, 0};
  Pixel* Sprite::getPixel(const int x, const int y) {
    if (this->invisible) return &EMPTY_PIXEL;
    if (x < 0) return &EMPTY_PIXEL;
    if (y < 0) return &EMPTY_PIXEL;
    if ((size_t) x > this->width) return &EMPTY_PIXEL;
    if ((size_t) y > this->height) return &EMPTY_PIXEL;
    Pixel * pixel = &this->matrix[y][x];
    return pixel;
  }

  void Sprite::setPosition(const Point p) {
    this->position = p;
  }
  void Sprite::addToPosition(const Point p) {
    this->position.x += p.x;
    this->position.y += p.y;
  }
  void Sprite::reachPosition(const Point p, const uint steps) {
    this->position_goal = p;
    this->goal_steps = steps;
    double dx = this->position_goal.x - this->position.x;
    double dy = this->position_goal.y - this->position.y;
    double distance = sqrt(pow(dx, 2) + pow(dy, 2));
    double direction = atan2(dy, dx) * 180 / M_PI;
    setDirection(direction);
    setSpeed(distance / this->goal_steps);
  }
  Point* Sprite::getPosition() {
    return &this->position;
  }

  void Sprite::setDirection(double ang) {
    this->direction = ang;
  }
  void Sprite::setDirection(char dir) {
    switch (dir) {
      case 'u': this->direction = 270; break;
      case 'l': this->direction = 180; break;
      case 'd': this->direction = 90; break;
      case 'r': this->direction = 0; break;
      default: fprintf(stderr, "Invalid direction '%c'\n", dir);
    }
  }
  void Sprite::addDirection(double ang_inc) {
    this->direction += ang_inc;
  }
  double* Sprite::getDirection() {
    return &this->direction;
  }

  void Sprite::setSpeed(double speed) {
    this->speed = speed;
  }
  void Sprite::addSpeed(double speed_inc) {
    this->speed += speed_inc;
  }
  double* Sprite::getSpeed() {
    return &this->speed;
  }

  void Sprite::setEdgeBehavior(EdgeBehavior edge_behavior) {
    this->edge_behavior = edge_behavior;
  }
  EdgeBehavior Sprite::getEdgeBehavior() {
    return this->edge_behavior;
  }

  bool Sprite::getWrapped() {
    return this->wrapped;
  }

  Point Sprite::wrap_edge(double x, double y) {
    size_t xmax = this->max_dimensions.x;
    size_t ymax = this->max_dimensions.y;

    if (this->edge_behavior == LOOP_INDIRECT) {
      if (x + this->width < 0) x += xmax + this->width;
      if (std::round(x) > xmax) x -= xmax + this->width;
      if (y + this->height < 0) y += ymax + this->height;
      if (std::round(y) > ymax) y -= ymax + this->height;
    } else if (this->edge_behavior == LOOP_DIRECT) {
      if (x < 0) x += xmax;
      if (std::round(x) > xmax) x -= xmax;
      if (y < 0) y += ymax;
      if (std::round(y) > ymax)  y -= ymax;
      this->wrapped = true;
    } else if (this->edge_behavior == BOUNCE) {
      if (x < 0 || x + this->width > xmax) setDirection(180 - this->direction);
      if (y < 0 || y + this->height > ymax)  setDirection(360 - this->direction);
    } else if (this->edge_behavior == STOP) {
      if ((x < 0 && std::fmod(this->direction + 270, 360) < 180)
          || (x + this->width > xmax && std::fmod(this->direction + 90, 360) < 180)
          || (y < 0 && std::fmod(this->direction + 180, 360) < 180)
          || (y + this->height > ymax && this->direction < 180)) {
        this->speed = 0;
      }
    } else if (this->edge_behavior == DISAPPEAR) {
      if (x + this->width < 0 || std::round(x) > xmax
          || y + this->height < 0 || std::round(y) > ymax) {
        this->invisible = true;
      } else {
        this->invisible = false;
      }
    }
    return Point(x, y);
  }


// class Sprite {
// public:
//   Sprite() : matrix_(), position_(), direction_(0), speed_(0),
//              max_dimensions_(), width_(0), height_(0),
//              edge_behavior_(LOOP_INDIRECT), invisible_(false), wrap_(false),
//              position_goal_(nan(""), nan("")), goal_steps_(-1) {};
//   Sprite(const char *filename) : matrix_(), position_(), direction_(0),
//                                  speed_(0), max_dimensions_(), width_(0),
//                                  height_(0), edge_behavior_(LOOP_INDIRECT),
//                                  invisible_(false), wrap_(false),
//                                  position_goal_(nan(""), nan("")),
//                                  goal_steps_(-1) {
//     loadMatrix(filename);
//   };
//
//   void doStep() {
//     direction_ = std::fmod(direction_ + 360, 360);
//     double x = position_.x + cos(direction_ * M_PI / 180) * speed_;
//     double y = position_.y + sin(direction_ * M_PI / 180) * speed_;
//     invisible_ = false;
//     wrap_ = false;
//     position_ = wrap_edge(x, y);
//     if (goal_steps_ == 0) speed_ = 0;
//     if (goal_steps_ >= 0) --goal_steps_;
//   }
//
//   Point wrap_edge(double x, double y) {
//     size_t xmax = max_dimensions_.x;
//     size_t ymax = max_dimensions_.y;
//
//     if (edge_behavior_ == LOOP_INDIRECT) {
//       if (x + width_ < 0) x += xmax + width_;
//       if (std::round(x) > xmax) x -= xmax + width_;
//       if (y + height_ < 0) y += ymax + height_;
//       if (std::round(y) > ymax) y -= ymax + height_;
//     } else if (edge_behavior_ == LOOP_DIRECT) {
//       if (x < 0) x += xmax;
//       if (std::round(x) > xmax) x -= xmax;
//       if (y < 0) y += ymax;
//       if (std::round(y) > ymax)  y -= ymax;
//       wrap_ = true;
//     } else if (edge_behavior_ == BOUNCE) {
//       if (x < 0 || x + width_ > xmax) setDirection(180 - direction_);
//       if (y < 0 || y + height_ > ymax)  setDirection(360 - direction_);
//     } else if (edge_behavior_ == STOP) {
//       if ((x < 0 && std::fmod(direction_ + 270, 360) < 180)
//           || (x + width_ > xmax && std::fmod(direction_ + 90, 360) < 180)
//           || (y < 0 && std::fmod(direction_ + 180, 360) < 180)
//           || (y + height_ > ymax && direction_ < 180)) {
//         speed_ = 0;
//       }
//     } else if (edge_behavior_ == DISAPPEAR) {
//       if (x + width_ < 0 || std::round(x) > xmax
//           || y + height_ < 0 || std::round(y) > ymax) {
//         invisible_ = true;
//       } else {
//         invisible_ = false;
//       }
//     }
//     return Point(x, y);
//   }
//
//   void loadMatrix(PixelMatrix mat) {
//     matrix_.swap(mat);
//     height_ = matrix_.size();
//     height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
//   }
//   void loadMatrix(const char *filename) {
//     PixelMatrix mat = Sprites::loadMatrix(filename, 1.0);
//     matrix_.swap(mat);
//     height_ = matrix_.size();
//     height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
//   }
//   void loadMatrix(const char *filename, double resize_factor) {
//     PixelMatrix mat = Sprites::loadMatrix(filename, resize_factor);
//     matrix_.swap(mat);
//     height_ = matrix_.size();
//     height_ < 1 ? width_ = 0 : width_ = matrix_[0].size();
//   }
//
//   size_t height() {
//     return height_;
//   }
//   size_t width() {
//     return width_;
//   }
//
//   void setPixel(int x, int y, Pixel * pixel);
//   void setPixel(int x, int y, char r, char g, char b);
//   Pixel * getPixel(int x, int y) {
//     if (invisible_) return &empty_pixel;
//     if (x < 0) return &empty_pixel;
//     if (y < 0) return &empty_pixel;
//     if ((size_t) x > width_) return &empty_pixel;
//     if ((size_t) y > height_) return &empty_pixel;
//     Pixel * pixel = &matrix_[y][x];
//     return pixel;
//   }
//
//   void setPosition(Point p) {
//     position_ = p;
//   }
//   void addToPosition(Point p) {
//     position_.x += p.x;
//     position_.y += p.y;
//   }
//   void reachPosition(Point p, uint steps) {
//     position_goal_ = p;
//     goal_steps_ = steps;
//     double dx = position_goal_.x - position_.x;
//     double dy = position_goal_.y - position_.y;
//     double distance = sqrt(pow(dx, 2) + pow(dy, 2));
//     double direction = atan2(dy, dx) * 180 / M_PI;
//     setDirection(direction);
//     setSpeed(distance / goal_steps_);
//   }
//   Point * getPosition() {
//     return &position_;
//   }
//
//   void setDirection(double ang) {
//     direction_ = ang;
//   }
//   void setDirection(char dir) {
//     if (dir == 'u') {
//       direction_ = 270;
//     } else if (dir == 'l') {
//       direction_ = 180;
//     } else if (dir == 'd') {
//       direction_ = 90;
//     } else {
//       direction_ = 0;
//     }
//   }
//   void addDirection(double ang_inc) {
//     direction_ += ang_inc;
//   }
//   double * getDirection() {
//     return &direction_;
//   }
//
//   void setSpeed(double speed) {
//     speed_ = speed;
//   }
//   void addSpeed(double speed_inc) {
//     speed_ += speed_inc;
//   }
//   double * getSpeed() {
//     return &speed_;
//   }
//
//   EdgeBehavior getEdgeBehavior() {
//     return edge_behavior_;
//   }
//   void setEdgeBehavior(EdgeBehavior edge_behavior) {
//     edge_behavior_ = edge_behavior;
//   }
//
//   bool isWrapped() {
//     return wrap_;
//   }
//
//   void flip();
//   void flop();
//   void rotate();
// private:
//   PixelMatrix matrix_;
//   Point position_;
//   double direction_;
//   double speed_;
//   PanelSize max_dimensions_;
//   size_t width_;
//   size_t height_;
//   EdgeBehavior edge_behavior_;
//   bool invisible_;
//   bool wrap_;
//   Point position_goal_;
//   int goal_steps_;
// };
//
// typedef std::map<spriteID, Sprite*> SpriteList;

} // namespace Sprites
