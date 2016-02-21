#define list_h
#include <corange.h>
#include <stdint.h>
#include <stdbool.h>
#include <iron/types.h>
#include <iron/mem.h>
#include <iron/array.h>
#include <iron/utils.h>
#include "level_loader.h"
char * fmtstr(const char * fmt, ...);

static int SDL_RWreadline(SDL_RWops* file, char* buffer, int buffersize) {
  
  char c;
  int status = 0;
  int i = 0;
  while(1) {
    
    status = SDL_RWread(file, &c, 1, 1);
    
    if (status == -1) return -1;
    if (i == buffersize-1) return -1;
    if (status == 0) break;
    
    buffer[i] = c;
    i++;
    
    if (c == '\n') {
      buffer[i] = '\0';
      return i;
    }
  }
  
  if(i > 0) {
    buffer[i] = '\0';
    return i;
  } else {
    return 0;
  }  
}

void level_load(level_desc * lv, const char * filename){
  SDL_RWops* file = SDL_RWFromFile(filename, "r");
  if(file == NULL) 
    error("Could not load file %s", filename);
  char line[128];
  memset(line, 0, sizeof(line));
  while(SDL_RWreadline(file, line, 128)) {
    char * hashidx = strchr(line, '#');
    if(hashidx != NULL)
      *hashidx = 0;
    char asset_name[128];
    float x = 0.0, y= 0.0, z= 0.0;
    if(0 >= sscanf(line, "%s %f %f %f", &asset_name, &x, &y, &z))
      continue;
    char * name = fmtstr("%s", asset_name);
    vec3 offset = vec3_new(x, y, z);
    list_push(lv->name, lv->cnt, name);
    list_push(lv->offset, lv->cnt, offset);
    lv->cnt += 1;
    memset(line, 0, sizeof(line));
  }
}

void level_clear(level_desc * lv){
  free(lv->name);
  free(lv->offset);
  memset(lv, 0, sizeof(level_desc));
}
