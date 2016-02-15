#include <corange.h>
// test main
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

float compare_triangles(vec3 v11, vec3 v12, vec3 v13,vec3 v21, vec3 v22, vec3 v23, int * m1){
  float d1 = vec3_dist(v11,v21);
  float d2 = vec3_dist(v11,v22);
  float d3 = vec3_dist(v11,v23);
  //printf("%f %f %f   ", d1, d2 ,d3);
  v11 = v12;
  v12 = v13;
  if(d1 < d2 && d1 < d3){
    v21 = v23;
    *m1 = 0;
  }else if( d2 < d1 && d2 < d3){
    v22 = v23;
    d1 = d2;
    *m1 = 1;
  }else{
    d1 = d3;
    *m1 = 2;
  }
  d2 = vec3_dist(v11,v21);
  d3 = vec3_dist(v11,v22);
  v11 = v12;
  if(d2 < d3){
    v21 = v22;
  }else{
    d2 = d3;
  }
  d3 = vec3_dist(v11,v21);
  //printf("%f\n", d3);
  //printf("D: %f %f %f\n", d1, d2, d3);
  return max(d1,max(d2,d3));
}

bool starts_with(const char *pre, const char *str)
{
  size_t lenpre = strlen(pre),
    lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void mesh_meld(mesh * m1, mesh * m2, vec3 offset){

  //printf("num triangles: %i %i\n", m1->num_triangles, m2->num_triangles);
  //for(int i = 0; i < m1->num_triangles * 3; i++){
  //  printf("%i %i\n", i, m1->triangles[i]);
  //}
  
  int m1_num_verts = m1->num_verts;
  int m1_num_triangles = m1->num_triangles;
  
  m1->num_verts += m2->num_verts;
  m1->num_triangles += m2->num_triangles;
  m1->verticies = realloc(m1->verticies, sizeof(vertex) * m1->num_verts);
  m1->triangles = realloc(m1->triangles, sizeof(uint32_t) * m1->num_triangles * 3);
  memcpy(m1->verticies + m1_num_verts, m2->verticies, m2->num_verts * sizeof(vertex));
  memcpy(m1->triangles + m1_num_triangles * 3, m2->triangles, m2->num_triangles * sizeof(m1->triangles[0]) * 3);
  for(int i = m1_num_triangles * 3; i < m1->num_triangles * 3; i++)
    m1->triangles[i] += m1_num_verts;
  //printf("num triangles: %i %i %i\n", m1_num_triangles, m1->num_triangles, m2->num_triangles);
  //for(int i = 0; i < m1->num_triangles * 3; i++){
  //  printf("%i %i\n", i, m1->triangles[i]);
  //}
  for(int i = m1_num_verts; i < m1->num_verts; i++)
    m1->verticies[i].position = vec3_add(m1->verticies[i].position, offset);
}

int find_mesh_merge_points(mesh * mesh, int * faces){
  vertex * verts = mesh->verticies;
  int cnt = 0;
  for(int it = 0; it < mesh->num_triangles; it++){

    int t11 = mesh->triangles[it * 3];
    int t12 = mesh->triangles[it * 3 + 1];
    int t13 = mesh->triangles[it * 3 + 2];
    vec3 v11 = verts[t11].position, v12 = verts[t12].position, v13 = verts[t13].position;
    for(int jt = it + 1; jt < mesh->num_triangles; jt++){
      int t21 = mesh->triangles[jt * 3];
      int t22 = mesh->triangles[jt * 3 + 1];
      int t23 = mesh->triangles[jt * 3 + 2];
      vec3 v21 = verts[t21].position, v22 = verts[t22].position, v23 = verts[t23].position;
      int m1 = 0;
      float match = compare_triangles(v11,v12,v13,v21,v22,v23, &m1);
      if(match < 0.0001){
	faces[cnt++] = it;
	faces[cnt++] = jt;

	break;
      }
    }
  }
  return cnt;
}

void mesh_remove_triangle(mesh * mesh, int face){
  int startidx = face * 3;
  int stopidx = (face + 1) * 3;
  int movecnt = (mesh->num_triangles * 3 - stopidx);
  memmove(mesh->triangles + startidx, mesh->triangles + stopidx, movecnt * sizeof(mesh->triangles[0]));
  mesh->num_triangles -= 1;
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
  int matched[100];
  for(int i = 0; i < array_count(matched); i++)
    matched[i] = -1;
  vec3 offsets[100] = {0};
  renderable * r = asset_hndl_ptr(&teapot_object);
  
  static_object* level_entity = entity_new("level", static_object);
  level_entity->renderable = asset_hndl_new_load(P(level));
  //level_entity->renderable = (asset_hndl)  r;
  
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
      //renderable_delete(r);
      r = asset_hndl_ptr(&teapot_object);
    }
    ui_button_set_onclick(reset, clicked_reset);

    renderer_set_skydome_enabled(renderer, false);


  }
  light * l1 = light_new_type(vec3_new(0,10,0), LIGHT_TYPE_SUN);
  /*l1->enabled = true;
  l1->power = 10;
  l1->falloff = 0.1;
  l1->diffuse_color = vec3_new(1,1,1);
  l1->ambient_color = vec3_new(1,1,1);
  l1->specular_color = vec3_new(1,1,1);
  l1->target = vec3_new(0,0,0);
  l1->cast_shadows = true;
  l1->position = vec3_new(0,10,0);*/
  float t = 0;
  while(running) {
    //renderer_set_tod(renderer, 0.25, 0);
    //memset(offsets,0,sizeof(offsets));
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

    model* model = renderable_to_model(r);
    vec3 ** positions = calloc(1,sizeof(vec3) * model->num_meshes);;
    mat4 view = camera_view_matrix(cam);
    shader_program_set_mat4(shader, "view", view);
    shader_program_set_mat4(shader, "proj", ortho);
    
    mat4 projview = mat4_mul_mat4(ortho, view);


    //logd("\n");

    for(int i = 0; i < model->num_meshes; i++){
      mat4 mod1 = mat4_translation(vec3_new(0,0,0));//offsets[i]);
      mat4 projviewmod1 = mat4_mul_mat4(projview, mod1);
      positions[i] = calloc(1,model->meshes[i]->num_verts * sizeof(vec3) );
      vec3 * pos = positions[i];
      //printf("mesh name: %i %s\n", model->meshes[i]->name, model->meshes[i]->name);
      //printf("num verts: %i\n", model->meshes[i]->num_verts);
      for(int j = 0; j < model->meshes[i]->num_verts; j++){

	pos[j] = mat4_mul_vec3(projviewmod1, model->meshes[i]->verticies[j].position);
	positions[i][j].z = 0;
	//vec3_print(pos[j]);
      }
      //printf("\n");
    }
    
    for(int i=0; i < r->num_surfaces; i++) {
      mesh * mesh1 = model->meshes[i];
      bool mesh1_static = starts_with("static", mesh1->name);
      for(int j=i + 1; j < r->num_surfaces; j++) {
	mesh * mesh2 = model->meshes[j];
	bool mesh2_static = starts_with("static", mesh2->name);
	if(mesh1_static && mesh2_static)
	  continue;
	for(int it = 0; it < mesh1->num_triangles; it++){

	  int t11 = mesh1->triangles[it * 3];
	  int t12 = mesh1->triangles[it * 3 + 1];
	  int t13 = mesh1->triangles[it * 3 + 2];
	  vec3 v11 = positions[i][t11], v12 = positions[i][t12], v13 = positions[i][t13];
	  for(int jt = 0; jt < mesh2->num_triangles; jt++){
	    int t21 = mesh2->triangles[jt * 3];
	    int t22 = mesh2->triangles[jt * 3 + 1];
	    int t23 = mesh2->triangles[jt * 3 + 2];
	    vec3 v21 = positions[j][t21], v22 = positions[j][t22], v23 = positions[j][t23];
	    int m1 = 0;
	    float match = compare_triangles(v11,v12,v13,v21,v22,v23, &m1);
	    if(match < 0.005){
	      vec3 v1 = mesh1->verticies[mesh1->triangles[it * 3]].position;
	      vec3 v2 = mesh2->verticies[mesh2->triangles[jt * 3 + m1]].position;
	      if(mesh2_static){
		matched[i] = j;
		offsets[i] = vec3_sub(v2,v1);
	      }else{
		matched[j] = i;
		offsets[j] = vec3_sub(v1,v2);
	      }
	      goto next_surface;
	    }
	  }
	}

      }
    
    }
  next_surface:;
    


    
    for(int i = 0; i < array_count(matched); i++){
      int j = matched[i];
      if(j < 0) continue;
      if(starts_with("static", model->meshes[i]))
	error("Cannot move static mesh");
      mesh * m1 = model->meshes[j];
      mesh * m2 = model->meshes[i];
      vec3 * p = positions[i];
       for(int k = 0; k < m2->num_verts; k++){
	 p[k] = vec3_add(p[k], offsets[i]);
      }
       
      mesh_meld(model->meshes[j], model->meshes[i], offsets[i]);
      int hits[100];
      int deleted_faces = find_mesh_merge_points(m1, hits);
      int compare(const int * _a, const int * _b){
	return *_a - *_b;
      }
	  
      qsort(hits, deleted_faces, sizeof(hits[0]), (int (*)(const void*,const void*)) compare);
      for(int k = deleted_faces-1; k >= 0; k--)
	mesh_remove_triangle(m1, hits[k]);
      m1->triangles = realloc(m1->triangles, m1->num_triangles * 3 * sizeof(m1->triangles[0]));
      //return 0;
    }
    /*for(int i = 0; i < array_count(matched); i++){
      printf(" %i", matched[i]);
      }*/
    //printf("\n");
    bool replace_renderable = false;
    for(int i = array_count(matched)-1; i >=0; i--){
      if(matched[i] < 0) continue;
      matched[i] = -1;
      replace_renderable = true;
      printf("Deleting %i %i\n", i, matched[i]);
      mesh_delete(model_remove_mesh(model, i)); 
    }

    if(replace_renderable){
      renderable_delete(r);
      r = renderable_new();
      renderable_add_model(r, model);
      level_entity->renderable.ptr = (void *) r;
    }
    for(int i = 0; i < model->num_meshes; i++)
      free(positions[i]);
    free(positions);
    model_delete(model);
    renderer_add(renderer, render_object_static(level_entity));
    //renderer_add_dyn_light(renderer, l1);
    renderer_render(renderer);
    //shader_program_set_vec3(shader, "camera_direction", camera_direction(cam));

    /*for(int i=0; i < r->num_surfaces; i++) {
      shader_program_set_mat4(shader, "world", mat4_translation(vec3_new(0,0,0)));//offsets[i]));
      renderable_surface* s = r->surfaces[i];
      
      //int mentry_id = min(i, ((material*)asset_hndl_ptr(&r->material))->num_entries-1);
      //return 1;
      //material_entry* me = material_get_entry(asset_hndl_ptr(&r->material), mentry_id);
      glBindBuffer(GL_ARRAY_BUFFER, s->vertex_vbo);

      shader_program_enable_attribute(shader, "vPosition",  3, 18, (void*)0);
      shader_program_enable_attribute(shader, "vNormal",    3, 18, (void*)(sizeof(float) * 3));
      //shader_program_enable_attribute(shader, "vTangent",   3, 18, (void*)(sizeof(float) * 6));
      //shader_program_enable_attribute(shader, "vBinormal",  3, 18, (void*)(sizeof(float) * 9));
      //shader_program_enable_attribute(shader, "vTexcoord",  2, 18, (void*)(sizeof(float) * 12));
      
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->triangle_vbo);
      glDrawElements(GL_TRIANGLES, s->num_triangles * 3, GL_UNSIGNED_INT, (void*)0);
      
      shader_program_disable_attribute(shader, "vPosition");
      shader_program_disable_attribute(shader, "vNormal");
      //shader_program_disable_attribute(shader, "vTangent");
      //shader_program_disable_attribute(shader, "vBinormal");
      //shader_program_disable_attribute(shader, "vTexcoord");
      
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

    }
  
    shader_program_disable(shader);
    */
    glDisable(GL_DEPTH_TEST);
    ui_render();
    
    graphics_swap();
    
    frame_end();
    }

    
  return 0;
}
