#ifndef SPRITE_H
#define SPRITE_H

#include <vector>

#include <Magick++.h>
#include <magick/image.h>

namespace Sprites {
  typedef std::string SpriteID;

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
  struct ColoredPixel {
    Pixel pixel;
    Point point;
  };
  typedef std::vector<Pixel> PixelColumn;
  typedef std::vector<std::vector<Pixel>> PixelMatrix;
  typedef std::vector<ColoredPixel> ColoredPixelList;

  Magick::Image loadImage(const char* filename, const double resize_factor = 1);
  PixelMatrix loadMatrix(const char* filename, const double resize_factor = 1);

  class Sprite {
    public:
      Sprite();
      Sprite(const char* filename);
      void loadMatrix(const PixelMatrix mat);
      void loadMatrix(const char* filename, double resize_factor = 1.0);
      size_t getHeight();
      size_t getWidth();

      void doStep();
      void flip();
      void flop();
      void rotate();

      void setPixel(const int x, const int y,
                    const char r, const char g, const char b);
      void setPixel(const int x, const int y, const Pixel* pixel);
      void setPixel(const Point* point, const Pixel* pixel);
      void setPixel(const ColoredPixel* cpixel);
      Pixel* getPixel(const int x, const int y);

      void setPosition(const Point p);
      void addToPosition(const Point p);
      void reachPosition(const Point p, const uint steps);
      Point* getPosition();

      void setDirection(const double ang);
      void setDirection(const char dir);
      void addDirection(const double ang);
      double* getDirection();

      void setSpeed(const double speed);
      void addSpeed(const double speed);
      double* getSpeed();

      void setEdgeBehavior(const EdgeBehavior edge_behavior);
      EdgeBehavior getEdgeBehavior();

      bool getWrapped();

    private:
      Point wrap_edge(double x, double y);

      PixelMatrix matrix;
      size_t width;
      size_t height;

      PanelSize max_dimensions;
      EdgeBehavior edge_behavior;

      bool invisible;
      bool wrapped;

      Point position;
      double direction;
      double speed;
      Point position_goal;
      int goal_steps;
  };

  typedef std::map<SpriteID, Sprite*> SpriteList;

} // namespace Sprites

#endif
