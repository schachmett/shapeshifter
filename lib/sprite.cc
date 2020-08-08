#include <cmath>
#include <cstring>

#include <Magick++.h>
#include <magick/image.h>

#include "sprite.h"


namespace {

std::string generateID() {
    static int idx = 0;
    idx++;
    return std::to_string(idx);
}

} // end anonymous namespace


namespace Sprites {

// Some simple structs
PanelSize::PanelSize(size_t x, size_t y) :
  x(x), y(y) { };
Pixel::Pixel(char red, char green, char blue) :
  red(red), green(green), blue(blue) { };
Point::Point(double x, double y) :
  x(x), y(y) {};

// Image loading via Magick
Magick::Image loadImage(const std::string filename, const double resize_factor) {
  // Magick::Image img;
  // std::vector<Magick::Image> frames;
  // try {
  //   readImages(&frames, filename);
  // } catch (std::exception& e) {
  //   if (e.what()) fprintf(stderr, "Magickimage error: %s\n", e.what());
  // }
  // if (frames.size() == 0) fprintf(stderr, "No image found.\n");
  // if (frames.size() > 1) fprintf(stderr, "Discarded additional frames.\n");
  //
  // img = frames[0];    // tweak here for gifs!
  Magick::Image img = Magick::Image(filename);
  if (resize_factor != 1.0) {
    const double target_width = (double) img.columns() * resize_factor;
    const double target_height = (double) img.rows() * resize_factor;
    img.scale(Magick::Geometry(target_width, target_height));
  }
  return img;
}
PixelMatrix loadMatrix(const std::string filename, const double resize_factor) {
  Magick::Image img = loadImage(filename, resize_factor);
  PixelMatrix mat;
  for (size_t img_y = 0; img_y < img.rows(); ++img_y) {
    PixelColumn column;
    for (size_t img_x = 0; img_x < img.columns(); ++img_x) {
      const Magick::Color &c = img.pixelColor(img_x, img_y);
      char red = (char) ScaleQuantumToChar(c.redQuantum());
      char green = (char) ScaleQuantumToChar(c.greenQuantum());
      char blue = (char) ScaleQuantumToChar(c.blueQuantum());
      Pixel* pixel = new Pixel({red, green, blue});
      column.push_back(*pixel);
    }
    mat.push_back(column);
  }
  return mat;
}

// Sprite constructor and Image loading / initialization
Sprite::Sprite() :
    matrix(), resize_factor(1.0), width(0), height(0),
    max_dimensions(), edge_behavior(LOOP_INDIRECT),
    visible(true), out_of_bounds(false), wrapped(false),
    position(),  direction(0), speed(0),
    position_goal(nan(""), nan("")), goal_steps(-1) {
  this->id = generateID();
};
Sprite::Sprite(const char* filename) : Sprite() {
  this->loadMatrix(filename);
}
Sprite::Sprite(const char* filename, const double resize_factor) {
  this->loadMatrix(filename);
}
// Sprite::Sprite(SpriteID id) : Sprite() {
//   this->id = id;
// }
// Sprite::Sprite(const char* filename, SpriteID id) : Sprite() {
//   this->id = id;
//   this->loadMatrix(filename);
// }

void Sprite::setID(SpriteID id) {
  this->id = id;
}
const SpriteID& Sprite::getID() const {
  return this->id;
}
void Sprite::setFilename(const std::string filename) {
  this->loadMatrix(filename, 1.0);
}
const std::string& Sprite::getFilename() const {
  return this->filename;
}
void Sprite::setWidth(int width) {
  double new_resize = width / (this->width / this->resize_factor);
  this->loadMatrix(this->filename, new_resize);
}
const size_t& Sprite::getWidth() const {
  return this->width;
}
void Sprite::setHeight(const int height) {
  double new_resize = height / (this->height / this->resize_factor);
  this->loadMatrix(this->filename, new_resize);
}
const size_t& Sprite::getHeight() const {
  return this->height;
}

// Let the Sprite go in a direction
void Sprite::doStep() {
  this->direction = std::fmod(this->direction + 360, 360);
  double x = this->position.x + cos(this->direction * M_PI / 180) * this->speed;
  double y = this->position.y + sin(this->direction * M_PI / 180) * this->speed;
  this->out_of_bounds = false;
  this->wrapped = false;
  this->position = wrap_edge(x, y);
  if (this->goal_steps == 0) this->speed = 0;
  if (this->goal_steps >= 0) --this->goal_steps;
}
void Sprite::setVisible(bool visible) {
  this->visible = visible;
}
bool Sprite::getVisible() const {
  if (this->out_of_bounds) return false;
  return this->visible;
}

// Get drawing data
const Pixel& Sprite::getPixel(const int x, const int y) const {
  static Pixel EMPTY_PIXEL = {0, 0, 0};
  if (!this->visible) return EMPTY_PIXEL;
  if (x < 0) return EMPTY_PIXEL;
  if (y < 0) return EMPTY_PIXEL;
  if ((size_t) x > this->width) return EMPTY_PIXEL;
  if ((size_t) y > this->height) return EMPTY_PIXEL;
  const Pixel& pixel = this->matrix[y][x];
  return pixel;
}

// The sprite's position on the panel
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
const Point& Sprite::getPosition() const {
  return this->position;
}

// The sprite's direction and speed
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
const double& Sprite::getDirection() const {
  return this->direction;
}
void Sprite::setSpeed(double speed) {
  this->speed = speed;
}
void Sprite::addSpeed(double speed_inc) {
  this->speed += speed_inc;
}
const double& Sprite::getSpeed() const {
  return this->speed;
}

// Data regarding the sprite's behavior and status at the edge
void Sprite::setEdgeBehavior(EdgeBehavior edge_behavior) {
  this->edge_behavior = edge_behavior;
}
const EdgeBehavior& Sprite::getEdgeBehavior() const {
  return this->edge_behavior;
}
bool Sprite::getWrapped() {
  return this->wrapped;
}


// Private methods
void Sprite::loadMatrix(PixelMatrix mat) {
  this->matrix.swap(mat);
  this->height = this->matrix.size();
  this->height < 1 ? this->width = 0 : this->width = this->matrix[0].size();
}
void Sprite::loadMatrix(const std::string filename, double resize_factor) {
  PixelMatrix mat = Sprites::loadMatrix(filename, resize_factor);
  this->loadMatrix(mat);
  this->resize_factor = resize_factor;
  this->filename = filename;
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
  } //else if (this->edge_behavior == DISAPPEAR) {
  if (x + this->width < 0 || std::round(x) > xmax
      || y + this->height < 0 || std::round(y) > ymax) {
    this->out_of_bounds = true;
  } else {
    this->out_of_bounds = false;
  }
  // }
  return Point(x, y);
}

} // end namespace Sprites
