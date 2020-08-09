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
  x(x), y(y) { };

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


CanvasObject::CanvasObject() :
    width(0), height(0), max_dimensions(), edge_behavior(LOOP_INDIRECT),
    visible(true), out_of_bounds(false), wrapped(false),
    position(0, 0), direction(0), speed(0),
    position_goal(nan(""), nan("")), goal_steps(-1) {
  this->id = generateID();
}
CanvasObject::~CanvasObject() { }

void CanvasObject::setID(CanvasObjectID id) { this->id = id; }
const CanvasObjectID& CanvasObject::getID() const { return this->id; }
const size_t& CanvasObject::getHeight() const { return this->height; }
const size_t& CanvasObject::getWidth() const { return this->width; }

// Data regarding the sprite's behavior and status at the edge
void CanvasObject::setVisible(bool visible) { this->visible = visible; }
bool CanvasObject::getVisible() const {
  if (this->out_of_bounds) return false;
  return this->visible;
}
bool CanvasObject::getWrapped() { return this->wrapped; }
void CanvasObject::setEdgeBehavior(EdgeBehavior edge_behavior) {
  this->edge_behavior = edge_behavior;
}
const EdgeBehavior& CanvasObject::getEdgeBehavior() const {
  return this->edge_behavior;
}

void CanvasObject::setPixel(const ColoredPixel* cpixel) {
  this->setPixel(&cpixel->point, &cpixel->pixel);
}
void CanvasObject::setPixel(const Point* point, const Pixel* pixel) {
  this->setPixel(point->x, point->y, pixel);
}
void CanvasObject::setPixel(const int x, const int y,
                            const char r, const char g, const char b) {
  Pixel* pixel = new Pixel(r, g, b);
  this->setPixel(x, y, pixel);
  delete pixel;   // is this correct?
}

// The position on the panel
void CanvasObject::setPosition(const Point p) { this->position = p; }
void CanvasObject::addToPosition(const Point p) {
  this->position.x += p.x;
  this->position.y += p.y;
}
void CanvasObject::reachPosition(const Point p, const uint steps) {
  this->position_goal = p;
  this->goal_steps = steps;
  double dx = this->position_goal.x - this->position.x;
  double dy = this->position_goal.y - this->position.y;
  double distance = sqrt(pow(dx, 2) + pow(dy, 2));
  double direction = atan2(dy, dx) * 180 / M_PI;
  setDirection(direction);
  setSpeed(distance / this->goal_steps);
}
const Point& CanvasObject::getPosition() const { return this->position; }
// The direction and speed
void CanvasObject::setDirection(double ang) { this->direction = ang; }
void CanvasObject::setDirection(char dir) {
  switch (dir) {
    case 'u': this->direction = 270; break;
    case 'l': this->direction = 180; break;
    case 'd': this->direction = 90; break;
    case 'r': this->direction = 0; break;
    default: fprintf(stderr, "Invalid direction '%c'\n", dir);
  }
}
void CanvasObject::addDirection(double ang_inc) { this->direction += ang_inc; }
const double& CanvasObject::getDirection() const { return this->direction; }
void CanvasObject::setSpeed(double speed) { this->speed = speed; }
void CanvasObject::addSpeed(double speed_inc) { this->speed += speed_inc; }
const double& CanvasObject::getSpeed() const { return this->speed; }

// Let the Sprite go in a direction
void CanvasObject::doStep() {
  this->direction = std::fmod(this->direction + 360, 360);
  double x = this->position.x + cos(this->direction * M_PI / 180) * this->speed;
  double y = this->position.y + sin(this->direction * M_PI / 180) * this->speed;
  this->out_of_bounds = false;
  this->wrapped = false;
  this->position = wrap_edge(x, y);
  if (this->goal_steps == 0) this->speed = 0;
  if (this->goal_steps >= 0) --this->goal_steps;
}
Point CanvasObject::wrap_edge(double x, double y) {
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




// Sprite constructor and Image loading / initialization
Sprite::Sprite() : CanvasObject::CanvasObject(), matrix(), resize_factor(1.0) { }
Sprite::Sprite(const char* filename) : Sprite() {
  this->loadMatrix(filename);
}
Sprite::Sprite(const char* filename, const double resize_factor) {
  this->loadMatrix(filename);
}
Sprite::~Sprite() { }

void Sprite::setSource(const std::string filename) {
  this->loadMatrix(filename, 1.0);
}
const std::string& Sprite::getSource() const {
  return this->filename;
}
void Sprite::setWidth(int width) {
  double new_resize = width / (this->width / this->resize_factor);
  this->loadMatrix(this->filename, new_resize);
}
void Sprite::setHeight(const int height) {
  double new_resize = height / (this->height / this->resize_factor);
  this->loadMatrix(this->filename, new_resize);
}

// Get drawing data
void Sprite::setPixel(const int x, const int y, const Pixel* pixel) {
  this->matrix[y][x] = *pixel;
}
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

// Non-interface methods
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


} // end namespace Sprites
