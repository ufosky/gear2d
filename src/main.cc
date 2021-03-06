#include "gear2d.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
#define main SDL_main
#endif


void help() {
  printf("gear2d [options] [scene-file]\n");
  printf("Options:\n"
         "\t-v        : Prints Gear2D version\n"
         "\t-h        : Prints this help\n"
         "\t-l<level> : Verbosity level to the logging messages. 0 is the lowest,\n"
         "\t            4 is the highest.\n"
         "\t-f<filter>: Filter string to apply to the logging messages \n");
}

#ifdef __cplusplus
extern "C" 
#endif
int main(int argc, char ** argv, char ** env) {
  char * arg = 0;
  const char * scene = "gear2d.yaml";
  while (argc > 1) {
    arg = argv[argc-1];
    if (strlen(arg) == 2 && arg[0] == '-' && arg[1] == 'v') {
      printf("%s\n", gear2d::engine::version());
      exit(0);
    }
    else if (arg[0] == '-') {
      switch (arg[1]) {
        case 'l': {
          int level = atoi(arg+2);
          if (level <= gear2d::log::minimum) level = gear2d::log::minimum+1;
          if (level > gear2d::log::maximum) level = gear2d::log::maximum;
          gear2d::log::globalverb = (gear2d::log::verbosity)level;
          break;
        }
        
        case 'f': {
          gear2d::log::filter.insert(arg+2);
          break;
        }

        case 'i': {
          gear2d::log::ignore.insert(arg+2);
          break;
        }
        
        case 'h': {
          help();
          exit(0);
        }
        
        case 'o': {
          gear2d::log::open(arg+2);
          break;
        }
        
        default: {
          printf("Unknown argument %s.\n", arg);
          help();
          exit(0);
        }
      }
    }
    else scene = arg;
    argc--;
  }

// Harder to use commandline on android to control logging.
#if defined(LOGTRACE) && defined(ANDROID)
  gear2d::log::globalverb = gear2d::log::minimum;
#endif

  gear2d::engine::load(scene);
  int running = gear2d::engine::run();
  exit(running);
}
