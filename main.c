// test main

#include <corange.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iron/types.h>
#include <iron/log.h>
#include <iron/utils.h>

#include <iron/log.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include "level_loader.h"
#include "game_data.h"


void _error(const char * file, int line, const char * msg, ...){
  char buffer[1000];  
  va_list arglist;
  va_start (arglist, msg);
  vsprintf(buffer,msg,arglist);
  va_end(arglist);
  loge("%s\n", buffer);
  loge("Got error at %s line %i\n", file,line);
  iron_log_stacktrace();
  raise(SIGINT);
  exit(10);
}

void test(){
  level_desc lv = {0};
  level_load(&lv, "assets/level1.data");
  printf("LV: %i \n",lv.cnt);
  for(int i = 0; i < lv.cnt; i++){
    printf("> %s ", lv.name[i]);
    vec3_print(lv.offset[i]);
    printf("\n");    
  }
  game_data gd = {0};
  game_data_load(&gd, &lv);
    
  camera* cam = entity_new((char *) "camera", camera);
  cam->orthographic = true;
  mat4 proj = camera_proj_matrix(cam);
  mat4 view = camera_view_matrix(cam);
  mat4 projview = mat4_mul_mat4(proj, view);
  game_data_update(&gd, projview);
  
  printf("gd cnt: %i\n", gd.movable_cnt);
  for(int i = 0; i < gd.movable_cnt; i++){
    movable_object * mov = gd.movable + i;
    printf("mov item cnt: %i\n", mov->positions_cnt);
    for(int j = 0; j < mov->positions_cnt; j++){
      printf("vert cnt: %i\n", mov->vertex_cnt[j]);
      for(int k = 0; k < mov->vertex_cnt[j]; k++){
	vec3_print(mov->positions[j][k]);
	vec3_print(mov->positions_cache[j][k]);
	printf("\n");
      }
      printf("\n");
    }
  }
  level_clear(&lv);
}

int main(int argc, char ** argv){
  
  corange_init("assets");

  //test();
  
  //return 0;

  void _at_error(const char * str){
    _error("corange", -1, str);
  }

  graphics_viewport_set_title("Rotatör!");
  graphics_viewport_set_size(800, 800);
  renderer * renderer = renderer_new(asset_hndl_new_load(P("./assets/graphics.cfg")));
  renderer->exposure = 5;
  renderer->no_skydome_end = vec4_new(1.0,1.0,1.0,1.0);
  renderer->no_skydome_start = vec4_new(0.7,0.7,1.0,1.0);
  camera* cam = entity_new((char *) "camera", camera);
  cam->position = vec3_new(5, 5, 5);
  cam->target =  vec3_new(0, 0, 0);
  cam->orthographic = true;
  cam->ortho_width = 40;
  cam->ortho_height = 40;
  cam->near_clip = 0.1;
  cam->far_clip = 50.0;

  renderer_set_camera(renderer, cam);
  at_error(_at_error);
  const char * level = argc > 1 ? argv[1] : "./assets/4cubes.obj";
  //asset_hndl level_material = asset_hndl_new_load(P("./assets/level1.mat"));
  //printf("material: %p\n", level_material);
  
  int running = 1;
  SDL_Event e = {0};
  
  //static_object* level_entity = entity_new("level", static_object);

  //level_entity->renderable = asset_hndl_new_load(P(level));
  void (* on_win)() = NULL;
  
  game_data gd = {0};
  level_desc lv = {0};
  {
    level_load(&lv, "assets/level1.data");
    game_data_load(&gd, &lv);
  }
  
  {// Load UI
  
    ui_button* framerate = ui_elem_new("framerate", ui_button);
    ui_button_move(framerate, vec2_new(10,10));
    ui_button_resize(framerate, vec2_new(30,25));
    ui_button_set_label(framerate, "FRAMERATE");
    ui_button_disable(framerate);

    ui_button* reset = ui_elem_new("reset", ui_button);
    ui_button_move(reset, vec2_new(140, graphics_viewport_height() - 70));
    ui_button_resize(reset, vec2_new(50,25));
    ui_button_set_label(reset, "Reset");

    ui_rectangle * win_rect = ui_elem_new("winbox", ui_rectangle);

    ui_rectangle_resize(win_rect, vec2_new(100,50));
    ui_rectangle_move(win_rect, vec2_new(graphics_viewport_width() * 0.5 - 20,
					 graphics_viewport_height() * 0.5 - 15));
    win_rect->active = false;
    ui_rectangle_set_color(win_rect, vec4_new(0.5,1.0,0.5,0.9));
    
    ui_text * win_text = ui_elem_new("wintext", ui_text);
    ui_text_move(win_text, vec2_new(graphics_viewport_width() * 0.5,
				    graphics_viewport_height() * 0.5));
    win_text->active = false;
    ui_text_draw(win_text);
    ui_text_set_color(win_text, vec4_new(0,0,0,1));

    void clicked_reset(){
      file_reload(P(level));
      //gd.r = asset_hndl_ptr(&level_object);
      ui_text * win_text = ui_elem_get("wintext");
      ui_rectangle * win_rect = ui_elem_get("winbox");      
      gd.win_cond_met = false;
      win_text->active = false;
      win_rect->active = false;
    }

    void win_handler(){
      ui_text * win_text = ui_elem_get("wintext");
      ui_rectangle * win_rect = ui_elem_get("winbox");
      win_text->active = true;
      win_rect->active = true;
      ui_text_draw_string(win_text, "You Win!");
      
    }

    ui_button_set_onclick(reset, clicked_reset);
    
    on_win = win_handler;
  }
  renderer_set_skydome_enabled(renderer, false);

  float t = 0;
  while(running) {

    renderer_set_tod(renderer, sin((t++) * 0.01)* 0.12 + 0.25, 0);
    frame_begin();
    
    
    camera* cam = entity_get("camera");

    while(SDL_PollEvent(&e)) {
      switch(e.type){
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if (e.key.keysym.sym == SDLK_ESCAPE) { running = 0; }
        if (e.key.keysym.sym == SDLK_PRINTSCREEN) { graphics_viewport_screenshot(); }
        if (e.key.keysym.sym == SDLK_r &&
            e.key.keysym.mod == KMOD_LCTRL) {
            asset_reload_all();
        }
        break;
      case SDL_QUIT:
        running = 0;
        break;
      }
      camera_control_orbit(cam, e);
      ui_event(e);
    }
    camera_control_joyorbit(cam, frame_time());
    ui_button* framerate = ui_elem_get("framerate");
    ui_button_set_label(framerate, frame_rate_string());
    ui_update();
    
    glClearColor(0.25, 0.25, 0.25, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    
    mat4 view = camera_view_matrix(cam);
    mat4 proj = camera_proj_matrix(cam);
    mat4 projview = mat4_mul_mat4(proj, view);
    bool prev_win = gd.win_cond_met;
    game_data_update(&gd, projview);
    
    //level_entity->renderable.ptr = (void *) gd.r;
    if(gd.win_cond_met && !prev_win && on_win != NULL){
      // show winning GUI.
      printf("WIN!\n");
      on_win();
    }
    for(int i = 0; i < gd.movable_cnt;i++){
      static_object * obj = gd.movable[i].object;
      printf("%p\n", obj->renderable.ptr);
      renderer_add(renderer, render_object_static(obj));
    }

    renderer_render(renderer);
    glDisable(GL_DEPTH_TEST);
    ui_render();
    
    graphics_swap();
    
    frame_end();
    }


  return 0;
}
