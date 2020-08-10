#ifndef LED_LOOP_H
#define LED_LOOP_H

#include <mutex>
#include <thread>

#include "led-matrix.h"
#include "sprite.h"


namespace led_loop {

  typedef long long int tmillis_t;
  const tmillis_t DISTANT_FUTURE = (1LL<<40);
  tmillis_t getTimeInMillis();
  void sleepMillis(tmillis_t milli_seconds);

  struct LoopOptions {
    LoopOptions();
    tmillis_t frame_time_ms;
  };

  class AnimationLoop {
    public:
      AnimationLoop();
      AnimationLoop(rgb_matrix::RGBMatrix* matrix,
                    Sprites::CanvasObjectList* canvas_objects,
                    LoopOptions* options = nullptr,
                    std::mutex* data_mutex = nullptr);
      ~AnimationLoop();
      void startLoop();
      const std::thread& getThread() const;
      void endLoop();

      void prepareFrame();
      void doFrame();

      void lock_canvas_objects();
      void unlock_canvas_objects();
      void setMutex(std::mutex* data_mutex);
      std::mutex* getMutex() const;
      rgb_matrix::FrameCanvas* getCanvas();
    private:
      void animation_loop();

      std::mutex* data_mutex;
      volatile bool is_running;
      std::thread animation_thread;
      rgb_matrix::RGBMatrix* matrix;
      rgb_matrix::FrameCanvas* canvas;
      Sprites::CanvasObjectList* canvas_objects;
      tmillis_t frame_time_ms;
  };

} // end namespace led_loop

#endif
