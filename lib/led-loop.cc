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

LoopOptions::LoopOptions() : frame_time_ms(50) {}

SpriteAnimationLoop::SpriteAnimationLoop() {
  this->frame_time_ms = 50;
  this->is_running = false;
}
SpriteAnimationLoop::SpriteAnimationLoop(rgb_matrix::RGBMatrix* matrix,
                                         Sprites::SpriteList* sprites,
                                         LoopOptions* options) :
                     SpriteAnimationLoop() {
  this->matrix = matrix;
  this->canvas = this->matrix->CreateFrameCanvas();
  this->sprites = sprites;
  this->sprites_mutex = new std::mutex;   // do i need to del?
  if (options != nullptr) {
    this->frame_time_ms = options->frame_time_ms;
  }
}
SpriteAnimationLoop::SpriteAnimationLoop(rgb_matrix::RGBMatrix* matrix,
                                         Sprites::SpriteList* sprites,
                                         std::mutex* sprites_mutex,
                                         LoopOptions* options) :
                     SpriteAnimationLoop(matrix, sprites, options) {
    this->sprites_mutex = sprites_mutex;
}
SpriteAnimationLoop::~SpriteAnimationLoop() {
  this->is_running = false;
  if(this->animation_thread.joinable()) {
    this->animation_thread.join();
  }
}

void SpriteAnimationLoop::startLoop() {
  this->is_running = true;
  this->animation_thread = std::thread(&SpriteAnimationLoop::animation_loop, this);
}
const std::thread& SpriteAnimationLoop::getThread() const {
  return this->animation_thread;
}
void SpriteAnimationLoop::animation_loop() {
  while (this->is_running) {
    this->doFrame();
  }
}
void SpriteAnimationLoop::endLoop() {
  this->is_running = false;
  this->animation_thread.join();
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
void SpriteAnimationLoop::doFrame() {
  const tmillis_t start_ms = getTimeInMillis();
  SpriteAnimationLoop::prepareFrame();
  this->canvas = this->matrix->SwapOnVSync(this->canvas, 1);
  const tmillis_t time_already_spent = getTimeInMillis() - start_ms;
  sleepMillis(this->frame_time_ms - time_already_spent);
}
void SpriteAnimationLoop::drawSprite(Sprites::Sprite* sprite) {
  int x0 = std::round(sprite->getPosition().x);
  int y0 = std::round(sprite->getPosition().y);
  for (size_t img_y = 0; img_y < sprite->getHeight(); ++img_y) {
    for (size_t img_x = 0; img_x < sprite->getWidth(); ++img_x) {
      const Sprites::Pixel p = sprite->getPixel(img_x, img_y);
      // if (p->red == 0 and p->green == 0 and p->blue == 0) continue;
      int x = img_x + x0;
      int y = img_y + y0;
      if (sprite->getWrapped()) {
        if (x > this->canvas->width())  x -= this->canvas->width();
        if (y > this->canvas->height()) y -= canvas->height();
      }
      this->canvas->SetPixel(x, y, p.red, p.green, p.blue);
    }
  }
}

void SpriteAnimationLoop::lock_sprites() {
  this->sprites_mutex->lock();
}
void SpriteAnimationLoop::unlock_sprites() {
  this->sprites_mutex->unlock();
}
std::mutex* SpriteAnimationLoop::getMutex() const {
  return this->sprites_mutex;
}
void SpriteAnimationLoop::setMutex(std::mutex* sprites_mutex) {
  this->sprites_mutex = sprites_mutex;
}

} // end namespace led_loop
