#include "led-matrix.h"
#include "rapidjson/document.h"
#include "sprite.h"

// standard library:
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <fcntl.h>      // file descriptors
#include <poll.h>
#include <mutex>
#include <thread>

// POSIX/UNIX specific:
#include <sys/types.h>  // some types (also size_t, mutex)
#include <sys/stat.h>   // get file information on POSIX
#include <sys/time.h>   // gettimeofday function
#include <unistd.h>     // some POSIX API calls like pipe, fork


using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::ParseOptionsFromFlags;

using Sprites::Sprite;
using Sprites::SpriteList;
using Sprites::spriteID;
using Sprites::Point;
using Sprites::Pixel;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

typedef int64_t tmillis_t;
static const tmillis_t distant_future = (1LL<<40); // that is a while.
static tmillis_t GetTimeInMillis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
static void SleepMillis(tmillis_t milli_seconds) {
  if (milli_seconds <= 0) return;
  struct timespec ts;
  ts.tv_sec = milli_seconds / 1000;
  ts.tv_nsec = (milli_seconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

struct SpriteMoverOptions {
  SpriteMoverOptions() : frame_time_ms(10), verbosity(0) {}
  tmillis_t frame_time_ms;
  uint verbosity;
};


std::mutex sprites_mutex;

void drawSprite(FrameCanvas *canvas, Sprite *img) {
  int x0 = std::round(img->getPosition()->x);
  int y0 = std::round(img->getPosition()->y);
  for (size_t img_y = 0; img_y < img->height(); ++img_y) {
    for (size_t img_x = 0; img_x < img->width(); ++img_x) {
      const int x = img_x + x0;
      const int y = img_y + y0;
      Pixel * p = img->getPixel(img_x, img_y);
      if (p->red == 0 and p->green == 0 and p->blue == 0) continue;
      canvas->SetPixel(x, y, p->red, p->green, p->blue);
    }
  }
}

void nextFrame(FrameCanvas *canvas, SpriteList *sprites) {
  canvas->Clear();
  std::lock_guard<std::mutex> guard(sprites_mutex);
  // for(auto it = sprites->begin(); it != sprites->end(); ++it) {
  for (auto & sprite_pair : *sprites) {
    // Sprite * sprite = *it;
    Sprite * sprite = sprite_pair.second;
    sprite->doStep();
    drawSprite(canvas, sprite);
  }
}

void runAnimation(RGBMatrix *matrix, FrameCanvas *offscreen_canvas,
                  SpriteList *sprites, SpriteMoverOptions *options) {
  while (!interrupt_received) {
    const tmillis_t frame_delay_ms = options->frame_time_ms;
    const tmillis_t start_wait_ms = GetTimeInMillis();
    nextFrame(offscreen_canvas, sprites);
    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas, 1);
    const tmillis_t time_already_spent = GetTimeInMillis() - start_wait_ms;
    SleepMillis(frame_delay_ms - time_already_spent);
  }
}

enum Command {
  INVALID_COMMAND,
  UNKNOWN_COMMAND,
  NO_COMMAND,

  ADD_SPRITE,
  REMOVE_SPRITE,

  SET,
  ADD,
  TARGET
};
struct Message {
  Message() : command(NO_COMMAND), command_string(""), id(""), filename(""),
              position(0, 0), speed(nan("")), direction(nan("")), rotation(0),
              duration(0) {};
  void print() {
    fprintf(stderr, "Command %s:\n"
            "\tID=%s,\n"
            "\tfilename=%s,\n"
            "\tposition=(%f, %f)"
            "\tspeed=%f, direction=%f, rotation=%f\n"
            "\tduration=%llu\n",
            command_string.c_str(), id.c_str(), filename.c_str(),
            position.x, position.y, speed, direction, rotation, duration);
  } //TODO handle nan!
  Command command;
  std::string command_string;
  spriteID id;
  std::string filename;
  Point position;
  double speed;
  double direction;
  double rotation;
  tmillis_t duration;
};
Command resolveCommand(std::string str) {
  if (str == "") return NO_COMMAND;
  if (str == "addSprite") return ADD_SPRITE;
  if (str == "removeSprite") return REMOVE_SPRITE;
  if (str == "set") return SET;
  if (str == "add") return ADD;
  if (str == "target") return TARGET;
  fprintf(stderr, "UnkownCommand: %s\n", str.c_str());
  return UNKNOWN_COMMAND;
}

int applyCommand(Message *msg, SpriteList *sprites) {
  switch (msg->command) {
    case ADD : {
      if (msg->id == "") {
        std::lock_guard<std::mutex> guard(sprites_mutex);
        for (auto& sprite_pair : *sprites) {
          Sprite * sprite = sprite_pair.second;
          if (!std::isnan(msg->speed)) sprite->addSpeed(msg->speed);
          if (!std::isnan(msg->direction)) sprite->addDirection(msg->direction);
          if (!std::isnan(msg->position.x)) sprite->addPosition(msg->position);
        }
      } else {
        auto sprite_pair = sprites->find(msg->id);
        if (sprite_pair == sprites->end()) return 1;
        Sprite * sprite = sprite_pair->second;
        std::lock_guard<std::mutex> guard(sprites_mutex);
        if (!std::isnan(msg->speed)) sprite->addSpeed(msg->speed);
        if (!std::isnan(msg->direction)) sprite->addDirection(msg->direction);
        if (!std::isnan(msg->position.x)) sprite->addPosition(msg->position);
      }
      break;
    }
    case SET : {
      if (msg->id == "") {
        std::lock_guard<std::mutex> guard(sprites_mutex);
        for (auto& sprite_pair : *sprites) {
          Sprite * sprite = sprite_pair.second;
          if (!std::isnan(msg->speed)) sprite->setSpeed(msg->speed);
          if (!std::isnan(msg->direction)) sprite->setDirection(msg->direction);
          if (!std::isnan(msg->position.x)) sprite->setPosition(msg->position);
        }
      } else {
        auto sprite_pair = sprites->find(msg->id);
        if (sprite_pair == sprites->end()) return 1;
        Sprite * sprite = sprite_pair->second;
        std::lock_guard<std::mutex> guard(sprites_mutex);
        if (!std::isnan(msg->speed)) sprite->setSpeed(msg->speed);
        if (!std::isnan(msg->direction)) sprite->setDirection(msg->direction);
        if (!std::isnan(msg->position.x)) sprite->setPosition(msg->position);
      }
      break;
    }
    case TARGET : {
      if (msg->id == "") {
        std::lock_guard<std::mutex> guard(sprites_mutex);
        for (auto& sprite_pair : *sprites) {
          Sprite * sprite = sprite_pair.second;
          // ...
        }
      } else {
        auto sprite_pair = sprites->find(msg->id);
        if (sprite_pair == sprites->end()) return 1;
        Sprite * sprite = sprite_pair->second;
        std::lock_guard<std::mutex> guard(sprites_mutex);
        // ...
      }
      break;
    }
    case ADD_SPRITE : {
      if (msg->filename == "") break;
      if (msg->id == "") break;
      Sprite * sprite = new Sprite(msg->filename.c_str());
      std::lock_guard<std::mutex> guard(sprites_mutex);
      (*sprites)[msg->id] = sprite;
      if (!std::isnan(msg->speed)) sprite->setSpeed(msg->speed);
      if (!std::isnan(msg->direction)) sprite->setDirection(msg->direction);
      if (!std::isnan(msg->position.x)) sprite->setPosition(msg->position);
      break;
    }
    case REMOVE_SPRITE : {
      if (sprites->find(msg->id) == sprites->end()) return 1;
      std::lock_guard<std::mutex> guard(sprites_mutex);
      sprites->erase(msg->id);
      break;
    }
    case NO_COMMAND : {
      fprintf(stderr, "No command given\n");
      break;
    }
    default : {
      // ...
    }
  }
  return 0;
}

Message * parseJSON(char * buffer) {
  rapidjson::Document doc;
  doc.Parse(buffer);
  Message * msg = new Message();
  if (! doc.IsObject()) {
    fprintf(stdout, "no object parsed: %s\n", buffer);
    msg->command = INVALID_COMMAND;
    msg->command_string = "invalid";
    return msg;
  }
  if (doc.HasMember("command") && doc["command"].IsString()) {
    std::string command_string = doc["command"].GetString();
    msg->command_string = command_string;
    msg->command = resolveCommand(command_string);
  }
  if (doc.HasMember("ID") && doc["ID"].IsString()) {
    msg->id = doc["ID"].GetString();
  }
  if (doc.HasMember("filename") && doc["filename"].IsString()) {
    msg->filename = doc["filename"].GetString();
  }
  if (doc.HasMember("position") && doc["position"].IsObject()
      && doc["position"].HasMember("x") && doc["position"]["x"].IsNumber()
      && doc["position"].HasMember("y") && doc["position"]["y"].IsNumber()) {
    double x = doc["position"]["x"].GetDouble();
    double y = doc["position"]["y"].GetDouble();
    msg->position = Point(x, y);
  }
  if (doc.HasMember("speed") && doc["speed"].IsNumber()) {
    msg->speed = doc["speed"].GetDouble();
  }
  if (doc.HasMember("direction") && doc["direction"].IsNumber()) {
    msg->direction = doc["direction"].GetDouble();
  }
  if (doc.HasMember("rotation") && doc["rotation"].IsNumber()) {
    msg->rotation = doc["rotation"].GetDouble();
  }
  if (doc.HasMember("duration") && doc["duration"].IsUint()) {
    msg->duration = doc["duration"].GetUint();
  }
  return msg;
}

void listenFIFO(char const * fifo_path, SpriteList *sprites,
                SpriteMoverOptions *options) {
  struct pollfd pollfd;
  pollfd.fd = open(fifo_path, O_RDONLY|O_NONBLOCK);
  pollfd.events = POLLIN;
  while (!interrupt_received) {
    char buffer[PIPE_BUF] = "";
    pollfd.revents = 0;
    if (poll(&pollfd, 1, 1) < 0 && errno != EINTR) {
      fprintf(stderr, "Error polling the FIFO\n");
      return;
    }
    if (pollfd.revents & POLLIN) {
      read(pollfd.fd, &buffer, sizeof(buffer));
      Message * msg = parseJSON(buffer);
      if (options->verbosity > 4) {
        msg->print();
      }
      msg->duration /= options->frame_time_ms;
      if (!applyCommand(msg, sprites) == 0) {
        fprintf(stderr, "Command failed:\n");
        msg->print();
      }
      delete msg;
    }
  }
  close(pollfd.fd);
}


static int usage(const char *progname, const char *msg = nullptr);
bool parseOptions(int argc, char *argv[], SpriteMoverOptions *options);
SpriteList createSprites();

int main(int argc, char *argv[]) {
  fprintf(stdout, "Starting sprite mover\n");

  // Initialize Magick stuff
  Magick::InitializeMagick(*argv);

  // Parse options
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  SpriteMoverOptions spritemover_options;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 3;
  matrix_options.pwm_dither_bits = 1;  // helps with tearing in the middle row
  // matrix_options.limit_refresh = 100;  // might also help
  runtime_opt.drop_privileges = 1;
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }
  if (!parseOptions(argc, argv, &spritemover_options)) {
    return usage(argv[0]);
  }

  // Create canvas
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL) return 1;
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();

  // Create sprites
  SpriteList sprites; // = createSprites();

  // Open FIFO
  char const * fifo_path = "/tmp/led-fifo";   // user daemon needs access
  umask(0000);    // if not done, file permissions of the fifo are incorrect
  if (mkfifo(fifo_path, 0666) != 0 && errno != EEXIST) {
    fprintf(stderr, "Cannot create FIFO: %s\nExiting.\n", strerror(errno));
    return 1;
  }

  // Run!
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);
  // without signal handling, the read(fifo) thread does not block
  std::thread animation_loop(runAnimation,
                             matrix, offscreen_canvas, &sprites,
                             &spritemover_options);
  std::thread fifo_loop(listenFIFO,
                        fifo_path, &sprites, &spritemover_options);
  // runAnimation(matrix, offscreen_canvas, &sprites, &spritemover_options);
  animation_loop.join();
  fifo_loop.join();
  if (interrupt_received) {
    fprintf(stderr, "Caught signal. Exiting.\n");
  }
  matrix->Clear();
  delete matrix;
  return 0;
}

SpriteList createSprites() {
  char const *filename = "sprites/clubs/bremen42.png";
  Sprite * img = new Sprite(filename);
  img->setDirection(30.0);
  img->setSpeed(0.3);
  Sprite * img2 = new Sprite("sprites/clubs/dortmund42.png");
  img2->setDirection(80.0);
  img2->setSpeed(0.5);
  img2->setPosition(Point(50, 2));
  Sprite * img3 = new Sprite("sprites/clubs/mainz42.png");
  img3->setDirection(170.0);
  img3->setSpeed(0.9);
  img3->setPosition(Point(170, 55));
  Sprite * img4 = new Sprite("sprites/clubs/koeln42.png");
  img4->setDirection(240.0);
  img4->setSpeed(0.2);
  img4->setPosition(Point(80, 12));
  Sprite * img5 = new Sprite("sprites/clubs/leverkusen42.png");
  img5->setDirection(0.0);
  img5->setSpeed(3);
  img5->setPosition(Point(80, 12));

  SpriteList sprites;
  sprites["bremen"] = img;
  sprites["dortmund"] = img2;
  sprites["mainz"] = img3;
  sprites["koeln"] = img4;
  sprites["leverkusen"] = img5;
  // sprites.push_back(img);
  // sprites.push_back(img2);
  // sprites.push_back(img3);
  // sprites.push_back(img4);
  // sprites.push_back(img5);
  return sprites;
}

static int usage(const char *progname, const char *msg) {
  if (msg) {
    fprintf(stderr, "%s\n", msg);
  }
  fprintf(stderr, "Server application that pushes a few sprites on a panel\n");
  fprintf(stderr, "usage: %s [options] <video> [<video>...]\n", progname);
  fprintf(stderr, "Options:\n"
          "\t-f                 : Frame duration in ms.\n"
          "\t-x                 : Example option.\n");
  return 1;
}

bool parseOptions(int argc, char *argv[], SpriteMoverOptions *options) {
  int opt;
  while ((opt = getopt(argc, argv, "f:v:")) != -1) {
    switch (opt) {
      case 'f':
        options->frame_time_ms = strtoul(optarg, NULL, 0);
        break;
      case 'v':
        options->verbosity = 5;
        break;
      default:
        return false;
    }
  }
  return true;
}
