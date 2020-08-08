#ifndef SPRITE_H
#define SPRITE_H

#include <vector>
#include <cstring>

#include <Magick++.h>
#include <magick/image.h>

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
    // int xi();
    // int yi();
    double x;
    double y;
  };
  struct ColoredPixel {
    Pixel pixel;
    Point point;
  };
  typedef std::vector<Pixel> PixelColumn;
  typedef std::vector<std::vector<Pixel>> PixelMatrix;
  typedef std::vector<ColoredPixel> ColoredPixelList;

  // Magick::Image loadImage(const char* filename, const double resize_factor = 1);
  // PixelMatrix loadMatrix(const char* filename, const double resize_factor = 1);

  typedef std::string SpriteID;

  class Sprite {
    public:
      Sprite();
      Sprite(SpriteID id);
      Sprite(const char* filename);
      Sprite(const char* filename, const double resize_factor);
      // Sprite(const char* filename, SpriteID id);

      void setID(SpriteID);
      const SpriteID& getID() const;
      void setFilename(const std::string filename);
      const std::string& getFilename() const;
      void setWidth(int width);
      const size_t& getWidth() const;
      void setHeight(int height);
      const size_t& getHeight() const;

      void doStep();
      void setVisible(bool visible);
      bool getVisible() const;
      void flip();
      void flop();
      void rotate();

      void setPixel(const int x, const int y,
                    const char r, const char g, const char b);
      void setPixel(const int x, const int y, const Pixel* pixel);
      void setPixel(const Point* point, const Pixel* pixel);
      void setPixel(const ColoredPixel* cpixel);
      const Pixel& getPixel(const int x, const int y) const;

      void setPosition(const Point p);
      void addToPosition(const Point p);
      void reachPosition(const Point p, const uint steps);
      const Point& getPosition() const;

      void setDirection(const double ang);
      void setDirection(const char dir);
      void addDirection(const double ang);
      const double& getDirection() const;

      void setSpeed(const double speed);
      void addSpeed(const double speed);
      const double& getSpeed() const;

      void setEdgeBehavior(const EdgeBehavior edge_behavior);
      const EdgeBehavior& getEdgeBehavior() const;

      bool getWrapped();

    private:
      void loadMatrix(const PixelMatrix mat);
      void loadMatrix(const std::string filename, double resize_factor = 1.0);
      Point wrap_edge(double x, double y);

      SpriteID id;
      std::string filename;
      PixelMatrix matrix;

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

  typedef std::map<SpriteID, Sprite*> SpriteList;
  // typedef std::vector<Sprite*>::iterator SpriteIterator;

} // end namespace Sprites

#endif
