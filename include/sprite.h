#ifndef SPRITE_H
#define SPRITE_H

#include <vector>
#include <cstring>

#include <Magick++.h>
// #include <magick/image.h>

#include "led-matrix.h"
#include "graphics.h"

namespace Sprites {

  enum EdgeBehavior {
    UNDEFINED_EDGE_BEHAVIOR,
    LOOP_DIRECT,
    LOOP_INDIRECT,
    BOUNCE,
    STOP,
    DISAPPEAR
  };
  struct PanelSize {
    PanelSize(size_t x = 192, size_t y = 64);
    size_t x;
    size_t y;
  };

  struct Pixel {
    Pixel(char red = 0, char green = 0, char blue = 0);
    char red;
    char green;
    char blue;
  };
  struct Point {
    Point(double x = 0, double y = 0);
    double x;
    double y;
  };
  // struct ColoredPixel {
  //   Pixel pixel;
  //   Point point;
  // };
  // typedef std::vector<Pixel> PixelColumn;
  // typedef std::vector<std::vector<Pixel>> PixelMatrix;
  // typedef std::vector<ColoredPixel> ColoredPixelList;
  typedef std::vector<Point> Points;

  // Magick::Image loadImage(const char* filename, const double resize_factor = 1);
  // PixelMatrix loadMatrix(const char* filename, const double resize_factor = 1);

  // typedef std::string SpriteID;
  typedef std::string CanvasObjectID;

  class CanvasObject {
  public:
    CanvasObject();
    CanvasObject(const std::string source);
    virtual ~CanvasObject();

    virtual void setID(CanvasObjectID);
    virtual const CanvasObjectID& getID() const;
    virtual void setVisible(bool visible);
    virtual bool getVisible() const;
    // virtual bool getWrapped() const;
    virtual void setEdgeBehavior(const EdgeBehavior edge_behavior);
    virtual const EdgeBehavior& getEdgeBehavior() const;

    virtual void setContent(const std::string filename); // = 0;
    virtual const std::string& getContent() const; // = 0;
    virtual void setWidth(int width); // = 0;
    virtual size_t getWidth() const;
    virtual void setHeight(int height); // = 0;
    virtual size_t getHeight() const;

    virtual void doStep();
    virtual void draw(rgb_matrix::FrameCanvas* canvas) const; // = 0;

    virtual void setPosition(const Point p);
    virtual void reachPosition(const Point p, const uint steps);
    virtual const Point& getPosition() const;
    virtual void setDirection(const double ang);
    virtual const double& getDirection() const;
    virtual void setSpeed(const double speed);
    virtual const double& getSpeed() const;

  protected:
    Point wrap_edge(double x, double y);

    CanvasObjectID id;
    // std::string filename;
    // PixelMatrix matrix;

    double resize_factor;
    size_t width;
    size_t height;

    PanelSize max_dimensions;
    EdgeBehavior edge_behavior;

    bool visible;
    bool out_of_bounds;
    bool wrapped;

    Point position;
    double direction;
    double speed;
    Point position_goal;
    int goal_steps;
  };


  class Sprite : public CanvasObject {
  public:
    Sprite();
    Sprite(const std::string filename);
    ~Sprite();

    void setContent(const std::string filename, size_t index = 0);
    const std::string& getContent() const;
    void setWidth(int width);
    size_t getWidth() const;
    void setHeight(int height);
    size_t getHeight() const;
    void setResize(double resize_factor);
    const double& getResize() const;
    void setRotation(double rotation);
    const double& getRotation() const;

    void setPixel(const size_t x, const size_t y, const char r, const char g, const char b);
    void setPixel(const Point point, const Pixel pixel);
    const Pixel getPixel(const size_t x, const size_t y) const;
    const Points getOverlap(const Sprite*) const;
    void draw(rgb_matrix::FrameCanvas* canvas) const;

  protected:
    std::string filename;
    Magick::Image img;
    double resize_factor;
    double rotation;
  };


  class Text : public CanvasObject {
  public:
    Text();
    Text(const std::string fontfilename, const std::string content = "");
    ~Text();

    void setContent(const std::string filename);
    const std::string& getContent() const;
    // void setWidth(int width);   // not implemented
    // void setHeight(int height); // not implemented

    void setFont(std::string content);
    const std::string& getFont() const;
    void setKerning(const float kerning);
    const int& getKerning() const;
    void draw(rgb_matrix::FrameCanvas* canvas) const;

  protected:
    void loadFont(const std::string fontfilename);

    rgb_matrix::Font font;
    rgb_matrix::Color color;
    std::string fontfilename;
    std::string text;
    int kerning;
  };

  typedef std::map<CanvasObjectID, CanvasObject*> CanvasObjectList;
  typedef CanvasObjectList::iterator CanvasObjectListIterator;

} // end namespace Sprites

#endif
