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

LoopOptions::LoopOptions() : frame_time_ms(50) { }

AnimationLoop::AnimationLoop() {
  this->frame_time_ms = 50;
  this->is_running = false;
}
AnimationLoop::AnimationLoop(rgb_matrix::RGBMatrix* matrix,
                             Sprites::CanvasObjectList* canvas_objects,
                             LoopOptions* options,
                             std::mutex* data_mutex) :
               AnimationLoop() {
  this->matrix = matrix;
  this->canvas = this->matrix->CreateFrameCanvas();
  this->canvas_objects = canvas_objects;
  // std::string d = "dorie";
  // fprintf(stderr, "%f\n", this->canvas_objects->at(d)->getPosition().x);
  if (data_mutex != nullptr) {
    this->data_mutex = data_mutex;
  } else {
    this->data_mutex = new std::mutex;   // do i need to del?
  }
  if (options != nullptr) {
    this->frame_time_ms = options->frame_time_ms;
  }
}
AnimationLoop::~AnimationLoop() {
  this->is_running = false;
  if(this->animation_thread.joinable()) {
    this->animation_thread.join();
  }
}

void AnimationLoop::startLoop() {
  this->is_running = true;
  this->animation_thread = std::thread(&AnimationLoop::animation_loop, this);
}
void AnimationLoop::endLoop() {
  this->is_running = false;
  if(this->animation_thread.joinable()) {
    this->animation_thread.join();
  }
}
const std::thread& AnimationLoop::getThread() const {
  return this->animation_thread;
}
void AnimationLoop::animation_loop() {
  while (this->is_running) {
    this->doFrame();
  }
}

void AnimationLoop::prepareFrame() {
  this->canvas->Clear();
  std::lock_guard<std::mutex> guard(*(this->data_mutex));
  for (auto &sprite_pair : *(this->canvas_objects)) {
    Sprites::CanvasObject* sprite = sprite_pair.second;
    sprite->doStep();
    sprite->draw(this->canvas);
  }
}
void AnimationLoop::doFrame() {
  const tmillis_t start_ms = getTimeInMillis();
  this->prepareFrame();
  this->canvas = this->matrix->SwapOnVSync(this->canvas, 1);
  const tmillis_t time_already_spent = getTimeInMillis() - start_ms;
  sleepMillis(this->frame_time_ms - time_already_spent);
}

void AnimationLoop::lock_canvas_objects() {
  this->data_mutex->lock();
}
void AnimationLoop::unlock_canvas_objects() {
  this->data_mutex->unlock();
}
std::mutex* AnimationLoop::getMutex() const {
  return this->data_mutex;
}
void AnimationLoop::setMutex(std::mutex* data_mutex) {
  this->data_mutex = data_mutex;
}

} // end namespace led_loop
