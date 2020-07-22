#include <cmath>
#include <mutex>
#include <sys/time.h>   // gettimeofday function

#include "led-loop.h"
#include "led-matrix.h"
#include "sprite.h"

namespace led_loop {

tmillis_t getTimeInMillis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
void sleepMillis(tmillis_t milli_seconds) {
  if (milli_seconds <= 0) return;
  struct timespec ts;
  ts.tv_sec = milli_seconds / 1000;
  ts.tv_nsec = (milli_seconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}


SpriteAnimationLoop::SpriteAnimationLoop(rgb_matrix::RGBMatrix* matrix,
                                         Sprites::SpriteList* sprites,
                                         std::mutex* sprites_mutex,
                                         LoopOptions* options) {
  this->matrix = matrix;
  this->canvas = this->matrix->CreateFrameCanvas();
  this->sprites = sprites;
  if (options == nullptr) {
    this->frame_time_ms = 50;
  } else {
    this->frame_time_ms = options->frame_time_ms;
  }
  this->sprites_mutex = sprites_mutex;
}

void SpriteAnimationLoop::prepareFrame() {
  this->canvas->Clear();
  std::lock_guard<std::mutex> guard(*(this->sprites_mutex));
  for (auto &sprite_pair : *(this->sprites)) {
    Sprites::Sprite* sprite = sprite_pair.second;
    sprite->doStep();
    this->drawSprite(sprite);
  }
}

void SpriteAnimationLoop::drawSprite(Sprites::Sprite* sprite) {
  int x0 = std::round(sprite->getPosition().x);
  int y0 = std::round(sprite->getPosition().y);
  for (size_t img_y = 0; img_y < sprite->getHeight(); ++img_y) {
    for (size_t img_x = 0; img_x < sprite->getWidth(); ++img_x) {
      const Sprites::Pixel* p = sprite->getPixel(img_x, img_y);
      // if (p->red == 0 and p->green == 0 and p->blue == 0) continue;
      int x = img_x + x0;
      int y = img_y + y0;
      if (sprite->getWrapped()) {
        if (x > this->canvas->width())  x -= this->canvas->width();
        if (y > this->canvas->height()) y -= canvas->height();
      }
      this->canvas->SetPixel(x, y, p->red, p->green, p->blue);
    }
  }
}

void SpriteAnimationLoop::run() {
  const tmillis_t start_ms = getTimeInMillis();
  SpriteAnimationLoop::prepareFrame();
  this->canvas = this->matrix->SwapOnVSync(this->canvas, 1);
  const tmillis_t time_already_spent = getTimeInMillis() - start_ms;
  sleepMillis(this->frame_time_ms - time_already_spent);
}

std::mutex* SpriteAnimationLoop::getMutex() {
  return this->sprites_mutex;
}
void SpriteAnimationLoop::setMutex(std::mutex* sprites_mutex) {
  this->sprites_mutex = sprites_mutex;
}

} // end namespace led_loop
