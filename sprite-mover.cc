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
  SpriteMoverOptions() : frame_time_ms(10) {}
  tmillis_t frame_time_ms;
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
  DELETE_SPRITE,

  INCREASE_SPEED,
  INCREASE_DIRECTION
};
struct Message {
  Message() : command(NO_COMMAND), command_string(""), id(""), arg(0),
              filename("") {};
  void print() {
    fprintf(stderr, "Command %s:\n"
            "\tID=%s,\n"
            "\tfilename=%s,\n"
            "\targ=%f\n",
            command_string.c_str(), id.c_str(), filename.c_str(), arg);
  }
  Command command;
  std::string command_string;
  spriteID id;
  double arg;
  std::string filename;
};
Command resolveCommand(std::string str) {
  if (str == "") return NO_COMMAND;
  if (str == "addSpeed") return INCREASE_SPEED;
  if (str == "addSprite") return ADD_SPRITE;
  fprintf(stderr, "UnkownCommand: %s\n", str.c_str());
  return UNKNOWN_COMMAND;
}

int applyCommand(Message * msg, SpriteList * sprites) {
  msg->print();
  switch (msg->command) {
    case INCREASE_SPEED : {
      std::lock_guard<std::mutex> guard(sprites_mutex);
      if (msg->id == "") {
        for (auto& sprite_pair : *sprites) {
          Sprite * sprite = sprite_pair.second;
          sprite->addSpeed(msg->arg);
        }
      } else {
        auto sprite_pair = sprites->find(msg->id);
        if (sprite_pair == sprites->end()) return 1;
        Sprite * sprite = sprite_pair->second;
        sprite->addSpeed(msg->arg);
      }
      break;
    }
    case ADD_SPRITE : {
      if (msg->filename == "") break;
      if (msg->id == "") break;
      Sprite * img = new Sprite(msg->filename.c_str());
      std::lock_guard<std::mutex> guard(sprites_mutex);
      (*sprites)[msg->id] = img;
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
  delete msg;
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
    Command command = resolveCommand(command_string);
    msg->command = command;
    msg->command_string = command_string;
  }
  if (doc.HasMember("argument") && doc["argument"].IsNumber()) {
    msg->arg = doc["argument"].GetDouble();
  }
  if (doc.HasMember("ID") && doc["ID"].IsString()) {
    msg->id = doc["ID"].GetString();
  }
  if (doc.HasMember("filename") && doc["filename"].IsString()) {
    msg->filename = doc["filename"].GetString();
  }
  return msg;
}

void listenFIFO(char const * fifo_path, SpriteList *sprites) {
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
      applyCommand(msg, sprites);
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
  SpriteList sprites = createSprites();

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
  std::thread fifo_loop(listenFIFO, fifo_path, &sprites);
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
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
      case 'f':
        options->frame_time_ms = strtoul(optarg, NULL, 0);
        break;
      default:
        return false;
    }
  }
  return true;
}
