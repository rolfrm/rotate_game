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

int main(int argc, char ** argv){
  mat4 ortho = mat4_orthographic(-20,20,-20,20,0.01,100);
  corange_init("assets");
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

  mat4_print(camera_proj_matrix(cam));printf("\n");
  mat4_print(ortho);printf("\n");
  renderer_set_camera(renderer, cam);

  char * level = argc > 1 ? argv[1] : "./assets/4cubes.obj";
  asset_hndl teapot_shader = asset_hndl_new_load(P("./assets/teapot.mat"));
  asset_hndl teapot_object = asset_hndl_new_load(P(level));
  
  int running = 1;
  SDL_Event e = {0};
  
  static_object* level_entity = entity_new("level", static_object);
  level_entity->renderable = asset_hndl_new_load(P(level));
  
  
  game_data gd = {0};
  {
    renderable * r = asset_hndl_ptr(&teapot_object);
    game_data_load(&gd, r);
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
    void clicked_reset(){
      printf("Reset clicked!\n");
      file_reload(P(level));
      gd.r = asset_hndl_ptr(&teapot_object);
    }
    ui_button_set_onclick(reset, clicked_reset);

    ui_rectangle * win_rect = ui_rectangle_new();
    ui_rectangle_move(win_rect, vec2_new(graphics_viewport_width() * 0.5,
					 graphics_viewport_height() * 0.5));
    ui_rectangle_size(win_rect, vec2_new(100,50));
    win_rect->active = false;
    ui_rectangle_set_color(win_rect, vec4_new(0.5,1.0,0.5,0.9));
    
    ui_rect * win_rect = ui_rect_new();
    ui_text_move(win_rect, vec2_new(graphics_viewport_width() * 0.5,
					 graphics_viewport_height() * 0.5));
    win_rect->active = false;
    ui_rect_set_color(win_rect, vec4_new(0.5,1.0,0.5,0.9));
    
    

    
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
    
    shader_program* shader = material_first_program(asset_hndl_ptr(&teapot_shader));

    shader_program_enable(shader);

    mat4 view = camera_view_matrix(cam);
    shader_program_set_mat4(shader, "view", view);
    shader_program_set_mat4(shader, "proj", ortho);
    
    mat4 projview = mat4_mul_mat4(ortho, view);
    game_data_update(&gd, projview);
    
    level_entity->renderable.ptr = (void *) gd.r;
    if(gd.win_cond_met){
      // show winning GUI.
    }
    renderer_add(renderer, render_object_static(level_entity));
    renderer_render(renderer);
 
    glDisable(GL_DEPTH_TEST);
    ui_render();
    
    graphics_swap();
    
    frame_end();
    }

    
  return 0;
}
