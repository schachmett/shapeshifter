#ifndef LED_LOOP_H
#define LED_LOOP_H

#include <mutex>

#include "led-matrix.h"
#include "sprite.h"


namespace led_loop {

  typedef long long int tmillis_t;
  const tmillis_t DISTANT_FUTURE = (1LL<<40);
  tmillis_t getTimeInMillis();
  void sleepMillis(tmillis_t milli_seconds);

  struct LoopOptions {
    tmillis_t frame_time_ms;
  };

  class SpriteAnimationLoop {
    public:
      SpriteAnimationLoop(rgb_matrix::RGBMatrix* matrix,
                          Sprites::SpriteList* sprites,
                          std::mutex* sprites_mutex,
                          LoopOptions* options = nullptr);
      void prepareFrame();
      void drawSprite(Sprites::Sprite* sprite);
      void run();
      std::mutex* getMutex();
      void setMutex(std::mutex* sprites_mutex);
      rgb_matrix::FrameCanvas* getCanvas();
    private:
      std::mutex* sprites_mutex;
      rgb_matrix::RGBMatrix* matrix;
      rgb_matrix::FrameCanvas* canvas;
      Sprites::SpriteList* sprites;
      tmillis_t frame_time_ms;
  };

} // end namespace led_loop

#endif
