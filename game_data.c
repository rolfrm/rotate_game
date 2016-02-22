#define list_h
#include <corange.h>
#include <stdint.h>
#include <stdbool.h>
#include <iron/types.h>
#include <iron/mem.h>
#include <iron/array.h>
#include <iron/utils.h>
#include "level_loader.h"
#include "game_data.h"

float compare_triangles(vec3 v11, vec3 v12, vec3 v13,vec3 v21, vec3 v22, vec3 v23, int * m1){
  float d1 = vec3_dist(v11,v21);
  float d2 = vec3_dist(v11,v22);
  float d3 = vec3_dist(v11,v23);
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
  return max(d1,max(d2,d3));
}

bool starts_with(const char *pre, const char *str)
{
  size_t lenpre = strlen(pre),
    lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void mesh_meld(mesh * m1, mesh * m2, vec3 offset){
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

void game_data_load(game_data * gd, const level_desc * lv){
  for(int i = 0; i < lv->cnt; i++){
    static_object * level_entity = entity_new("item%i", static_object, i);
    movable_object mov = {0};
    mov.object = level_entity;

    asset * hndl = asset_get_load(P(lv->name[i]));
    renderable * r = hndl;
    //printf(">>>>> %s", r->path.ptr);
    if(r == NULL)
      error("Unable to load level data for '%s'\n", lv->name[i]);
    model* model = renderable_to_model(r);
    level_entity->position = lv->offset[i];
    int con[10];
    int con_cnt = 0;
    for(int j = 0; j < model->num_meshes; j++){
      mesh * mesh = model->meshes[j];
      if(starts_with("connection", mesh->name))
	con[con_cnt++] = j;

    }
    for(int j = con_cnt - 1; j >= 0; j--){
      mesh * m = model_remove_mesh(model, con[j]);
      mov.positions = realloc(mov.positions, (1 + mov.positions_cnt) * sizeof(vec3*));
      mov.positions_cache = realloc(mov.positions_cache, (1 + mov.positions_cnt) * sizeof(vec3*));
      mov.vertex_cnt = realloc(mov.vertex_cnt, (1 + mov.positions_cnt) * sizeof(int));
      mov.vertex_cnt[mov.positions_cnt] = m->num_verts;
      mov.positions[mov.positions_cnt] = malloc(m->num_verts * sizeof(vec3));
      mov.positions_cache[mov.positions_cnt] = malloc(m->num_verts * sizeof(vec3));
      for(int i = 0; i < m->num_verts; i++){
	mov.positions[mov.positions_cnt][i] = m->verticies[i].position;
      }
      mov.positions_cnt += 1;
      mesh_delete(m);
    }
    mov.ignore = calloc(1, mov.positions_cnt * sizeof(bool));
    level_entity->renderable = asset_hndl_new_load(P(lv->name[i]));
    
    list_push(gd->movable, gd->movable_cnt, mov);
    gd->movable_cnt += 1;
    //renderable_delete(r);
    //r = renderable_new();
    //renderable_add_model(r, model);
    
  }
}

void game_data_update(game_data * gd, mat4 projview){
  if(gd->win_cond_met)
    return;

  for(int i = 0; i < gd->movable_cnt; i++){
    movable_object * mov = gd->movable + i;
    mat4 world = static_object_world(mov->object);
    mat4 projviewworld = mat4_mul_mat4(projview, world);
    for(int j = 0; j < mov->positions_cnt; j++){
      for(int k = 0; k < mov->vertex_cnt[j]; k++){
	mov->positions_cache[j][k] = mat4_mul_vec3(projviewworld, mov->positions[j][k]);
	mov->positions_cache[j][k].z = 0;
      }
    }
  }
  int matches[gd->movable_cnt];
  int match1_item[gd->movable_cnt];
  int match2_item[gd->movable_cnt];
  vec3 offset[gd->movable_cnt];
  for(size_t i = 0; i < array_count(matches); i++)
    matches[i] = -1;
  for(int i = 0; i < gd->movable_cnt; i++){
    movable_object * movi = gd->movable + i;
    for(int j = i + 1; j < gd->movable_cnt; j++){
      movable_object * movj = gd->movable + j;

      for(int it = 0; it < movi->positions_cnt; it++){
	if(movi->ignore[it])
	  continue;
	for(int jt = 0; jt < movj->positions_cnt; jt++){
	  if(movj->ignore[it])
	    continue;
	  int v1cnt = movi->vertex_cnt[it];
	  int v2cnt = movj->vertex_cnt[jt];
	  if(v1cnt != v2cnt)
	    continue;

	  vec3 * v1 = movi->positions_cache[it];
	  vec3 * v2 = movj->positions_cache[jt];
	  bool does_match = true;
	  for(int k = 0; k < v1cnt; k++){
	    float match = vec3_dist_manhattan(v1[k], v2[v1cnt - 1 - k]);
	    
	    if( match < 0.01){
	      
	      vec3 * v1 = movi->positions[it];
	      vec3 * v2 = movj->positions[jt];
	      int k2 = v1cnt - 1 - k;
	      offset[i] = vec3_sub(vec3_add(v1[k], movi->object->position), vec3_add(v2[k2],movj->object->position));

	      
	    }else{
	      does_match = false;
	      break;
	    }
	  }
	  if(does_match){
	    matches[i] = j;
	    match1_item[i] = it;
	    match2_item[j] = jt;
	    i = j;
	    goto next;
	  }  
	}
      }
    }
  next:;
  }

  for(size_t i = 0; i < array_count(matches); i++){
    int j = matches[i];
    if(j < 0) continue;
    int item1 = match1_item[i];
    int item2 = match2_item[j];

    movable_object * m1 = gd->movable + i;
    movable_object * m2 = gd->movable + j;
    if(m1->ignore[item1] || m2->ignore[item2])
      error("This should not happen!");
    vec3 off = offset[i];
    int to_move[gd->movable_cnt];
    memset(to_move, 0, sizeof(to_move));
    to_move[j] = 1;
    do{
      bool one_exists = false;
      for(size_t i = 0; i < array_count(to_move); i++){
	if(to_move[i] == 1){
	  one_exists = true;
	  int cnt = gd->movable[i].linked_object_cnt;
	  for(int j = 0; j < cnt; j++){
	    int k = gd->movable[i].linked_object[j];
	    if(to_move[k] == 0)
	      to_move[k] = 1;
	    else
	      to_move[k] = 2;
	  }
	  to_move[i] = 2;
	}
      }
      if(!one_exists)
	break;
    }while(true);
    
    m1->ignore[item1] = true;
    m2->ignore[item2] = true;

    if(to_move[i] == 0){
      for(size_t i = 0; i < array_count(to_move); i++){
	if(to_move[i] > 0)
	  gd->movable[i].object->position = vec3_add(gd->movable[i].object->position, off);
      }
    }
    list_push(m1->linked_object, m1->linked_object_cnt, j);
    list_push(m2->linked_object, m2->linked_object_cnt, i);
    m1->linked_object_cnt += 1;
    m2->linked_object_cnt += 1;
  }
}

