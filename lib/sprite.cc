#include <cmath>
#include <cstring>

#include <Magick++.h>
// #include <magick/image.h>

#include "led-matrix.h"
#include "graphics.h"

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
PanelSize::PanelSize(size_t x, size_t y) : x(x), y(y) { };
Point::Point(double x, double y) : x(x), y(y) { };
Pixel::Pixel(char red, char green, char blue) : red(red), green(green), blue(blue) { };
bool operator==(const Pixel& lhs, const Pixel& rhs) {
  if (lhs.red == rhs.red && lhs.green && rhs.green && lhs.blue == rhs.blue) return true;
  return false;
}

std::string& cython_abstract() { throw std::logic_error("CanvasObject is abstract!"); }


CanvasObject::CanvasObject() :
    width(0), height(0), max_dimensions(), edge_behavior(LOOP_INDIRECT),
    visible(true), out_of_bounds(false), wrapped(false),
    position(0, 0), direction(0), speed(0),
    position_goal(nan(""), nan("")), goal_steps(-1) { this->id = generateID(); }
CanvasObject::CanvasObject(const std::string source) : CanvasObject() { this->setContent(source); }
CanvasObject::~CanvasObject() { }

void CanvasObject::setID(CanvasObjectID id)          {        this->id = id; }
const CanvasObjectID& CanvasObject::getID() const    { return this->id; }
size_t CanvasObject::getWidth() const         { return this->width; }
void CanvasObject::setWidth(int width)               {        cython_abstract(); }
size_t CanvasObject::getHeight() const        { return this->height; }
void CanvasObject::setHeight(int height)             {        cython_abstract(); }
void CanvasObject::setContent(const std::string filename) {   cython_abstract(); }
const std::string& CanvasObject::getContent() const  { return cython_abstract(); }

// Data regarding the sprite's behavior and status at the edge
void CanvasObject::setVisible(bool visible) {
  this->visible = visible;
}
bool CanvasObject::getVisible() const {
  if (this->out_of_bounds) return false;
  return this->visible;
}
void CanvasObject::setEdgeBehavior(EdgeBehavior edge_behavior) {
  this->edge_behavior = edge_behavior;
}
const EdgeBehavior& CanvasObject::getEdgeBehavior() const {
  return this->edge_behavior;
}

// Position, speed and direction
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
void CanvasObject::setPosition(const Point p)     {        this->position = p; }
const Point& CanvasObject::getPosition() const    { return this->position; }
void CanvasObject::setDirection(double ang)       {        this->direction = ang; }
const double& CanvasObject::getDirection() const  { return this->direction; }
void CanvasObject::setSpeed(double speed)         {        this->speed = speed; }
const double& CanvasObject::getSpeed() const      { return this->speed; }

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
void CanvasObject::draw(rgb_matrix::FrameCanvas* canvas) const { cython_abstract(); }
Point CanvasObject::wrap_edge(double x, double y) {
  size_t xmax = this->max_dimensions.x;
  size_t ymax = this->max_dimensions.y;
  if (this->edge_behavior == LOOP_INDIRECT) {
    if (x + this->getWidth() < 0)   x += xmax + this->getWidth();
    if (std::round(x) > xmax)       x -= xmax + this->getWidth();
    if (y + this->getHeight() < 0)  y += ymax + this->getHeight();
    if (std::round(y) > ymax)       y -= ymax + this->getHeight();
  } else if (this->edge_behavior == LOOP_DIRECT) {
    if (x < 0)                      x += xmax;
    if (std::round(x) > xmax)       x -= xmax;
    if (y < 0)                      y += ymax;
    if (std::round(y) > ymax)       y -= ymax;
    this->wrapped = true;
  } else if (this->edge_behavior == BOUNCE) {
    if (x < 0 || x + this->getWidth()  > xmax) setDirection(180 - this->direction);
    if (y < 0 || y + this->getHeight() > ymax) setDirection(360 - this->direction);
  } else if (this->edge_behavior == STOP) {
    if (   (                    x < 0    && std::fmod(this->direction + 270, 360) < 180)
        || ( x + this->getWidth() > xmax && std::fmod(this->direction + 90, 360)  < 180)
        || (                    y < 0    && std::fmod(this->direction + 180, 360) < 180)
        || (y + this->getHeight() > ymax &&                       this->direction < 180)) {
      this->speed = 0;
    }
  }
  if (    x + this->getWidth() < 0 || std::round(x) > xmax
      || y + this->getHeight() < 0 || std::round(y) > ymax) {
    this->out_of_bounds = true;
  } else {
    this->out_of_bounds = false;
  }
  return Point(x, y);
}



// Sprite constructor and Image loading / initialization
Sprite::Sprite() : CanvasObject::CanvasObject(), img(), resize_factor(1.0),
                   rotation(0) { }
Sprite::Sprite(const std::string filename) : Sprite() { this->setContent(filename); }
Sprite::~Sprite() { }

void Sprite::setContent(const std::string filename, size_t index) {
  std::vector<Magick::Image> frames;
  try {
    Magick::readImages(&frames, filename);
  } catch (std::exception& e) {
    if (e.what()) fprintf(stderr, "Magickimage error: %s\n", e.what());
  }
  if (frames.size() == 0) fprintf(stderr, "No image found.\n");
  this->img = frames[0];
  Magick::ColorRGB black = Magick::ColorRGB(0, 0, 0);
  this->img.backgroundColor(black);
  // this->width = this->img.columns();
  // this->height = this->img.rows();
}
const std::string& Sprite::getContent() const {
  return this->filename;
}
void Sprite::setResize(double resize_factor) {
  double abs_resize_factor = resize_factor / this->resize_factor;
  const double target_width = (double) img.columns() * abs_resize_factor;
  const double target_height = (double) img.rows() * abs_resize_factor;
  this->img.scale(Magick::Geometry(target_width, target_height));
  this->resize_factor = resize_factor;
  // this->width = this->img.columns();
  // this->height = this->img.rows();
}
const double& Sprite::getResize() const {
  return this->resize_factor;
}
size_t Sprite::getWidth() const  { return this->img.columns(); }
void Sprite::setWidth(int width) {
  double new_resize = width / (this->getWidth() / this->resize_factor);
  this->setResize(new_resize);
}
size_t Sprite::getHeight() const { return this->img.rows(); }
void Sprite::setHeight(const int height) {
  double new_resize = height / (this->getHeight() / this->resize_factor);
  this->setResize(new_resize);
}
void Sprite::setRotation(double rotation) {
  double rotation_diff = rotation - this->rotation;
  this->img.rotate(rotation_diff);
}
const double& Sprite::getRotation() const {
  return this->rotation;
}


// Non-interface methods
void Sprite::setPixel(const Point point, const Pixel pixel) {
  this->setPixel(point.x, point.y, pixel.red, pixel.green, pixel.blue);
}
void Sprite::setPixel(const size_t x, const size_t y, const char r, const char g, const char b) {
  if (x > this->getWidth() || y > this->getHeight()) return;
  Magick::ColorRGB c = Magick::ColorRGB(r / 255, g / 255, b / 255);
  this->img.pixelColor(x, y, c);
}
static Pixel EMPTY_PIXEL = {0, 0, 0};
const Pixel Sprite::getPixel(const size_t x, const size_t y) const {
  if (!this->visible) return EMPTY_PIXEL;
  if ((size_t) x > this->getWidth()) return EMPTY_PIXEL;
  if ((size_t) y > this->getHeight()) return EMPTY_PIXEL;

  const Magick::Color &c = this->img.pixelColor(x, y);
  char red   = (char) ScaleQuantumToChar(c.redQuantum());
  char green = (char) ScaleQuantumToChar(c.greenQuantum());
  char blue  = (char) ScaleQuantumToChar(c.blueQuantum());
  Pixel pixel = Pixel({red, green, blue});
  return pixel;
}
const Points Sprite::getOverlap(const Sprite* other) const {
  Points points;
  if (!this->getVisible() or !other->getVisible()) return points;
  double dx = this->getPosition().x - other->getPosition().x;
  double dy = this->getPosition().y - other->getPosition().y;
  for (size_t img_y = 0; img_y < this->getHeight(); ++img_y) {
    for (size_t img_x = 0; img_x < this->getWidth(); ++img_x) {
      const Pixel px = this->getPixel(img_x, img_y);
      const Pixel px_o = other->getPixel(img_x - dx, img_y - dy);
      if (px == EMPTY_PIXEL and px_o == EMPTY_PIXEL) continue;
      Point p = Point(img_x, img_y);
      points.push_back(p);
    }
  }
  return points;
}
void Sprite::draw(rgb_matrix::FrameCanvas* canvas) const {
  if (!this->getVisible()) return;
  int x0 = std::round(this->getPosition().x);
  int y0 = std::round(this->getPosition().y);
  for (size_t img_y = 0; img_y < this->getHeight(); ++img_y) {
    for (size_t img_x = 0; img_x < this->getWidth(); ++img_x) {
      const Pixel pix = this->getPixel(img_x, img_y);
      if (pix == EMPTY_PIXEL) continue;
      int x = img_x + x0;
      int y = img_y + y0;
      if (this->wrapped) {
        if (x > canvas->width())  x -= canvas->width();
        if (y > canvas->height()) y -= canvas->height();
      }
      canvas->SetPixel(x, y, pix.red, pix.green, pix.blue);
    }
  }
}



Text::Text() : CanvasObject::CanvasObject(), color(255, 255, 255),
               fontfilename(""), text(""), kerning(0) { }
Text::Text(const std::string fontfilename, const std::string content) : Text() {
  this->setContent(content);
  this->setFont(fontfilename);
}
Text::~Text() { }

void Text::setContent(const std::string content) {
  this->text = content;
}
const std::string& Text::getContent() const {
  return this->text;
}

// Non-interface methods
void Text::setFont(const std::string fontfilename) {
  if (!this->font.LoadFont(fontfilename.c_str())) {
    fprintf(stderr, "Couldn't load font '%s'\n", fontfilename.c_str());
    return;
  }
  this->fontfilename = fontfilename;
}
const std::string& Text::getFont() const {
  return this->fontfilename;
}

void Text::setKerning(const float kerning) {
  this->kerning = (int)kerning;
}
const int& Text::getKerning() const {
  return this->kerning;
}
void Text::draw(rgb_matrix::FrameCanvas* canvas) const {
  if (this->fontfilename.empty()) { return; }
  rgb_matrix::DrawText(canvas, this->font, this->position.x,
                       this->position.y + this->font.baseline(),
                       this->color, NULL, this->text.c_str(),
                       this->kerning);
}


} // end namespace Sprites
