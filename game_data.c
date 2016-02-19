#define list_h
#include <corange.h>
#include <stdint.h>
#include <stdbool.h>
#include <iron/types.h>
#include <iron/mem.h>
#include <iron/array.h>
#include <iron/utils.h>

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

void game_data_load(game_data * gd, renderable * r){
  gd->r = r;
  gd->connection = calloc(1, r->num_surfaces * sizeof(gd->connection[0]));
  gd->connection_cnt = calloc(1, r->num_surfaces * sizeof(gd->connection_cnt[0]));
  // things are connected to themselves.
  for(int i = 0; i < r->num_surfaces; i++){
    gd->connection[i] = malloc(sizeof(int));
    gd->connection[i][0] = i;
  }
  
  gd->mesh_types = calloc(1, r->num_surfaces * sizeof(mesh_type));
}

void game_data_update(game_data * gd, mat4 projview){
  if(gd->win_cond_met)
    return;
  model* model = renderable_to_model(gd->r);
  for(int i = 0; i < model->num_meshes; i++){
    const char * name = model->meshes[i]->name;
    if(startswith("static", name)){
      gd->mesh_types[i] = mesh_static;
    }else if(startswith(scenery, name)){
      gd->mesh_types[i] = mesh_scenery;
    }else
      gd->mesh_types[i] = mesh_dynamic;
  }
  renderable * r = gd->r;
  vec3 ** positions = calloc(1,sizeof(vec3) * model->num_meshes);;
  int matched[100];
  for(size_t i = 0; i < array_count(matched); i++)
    matched[i] = -1;
  vec3 offsets[model->num_meshes];
  for(int i = 0; i < model->num_meshes; i++){
      mat4 mod1 = mat4_translation(vec3_new(0,0,0));
      mat4 projviewmod1 = mat4_mul_mat4(projview, mod1);
      positions[i] = calloc(1,model->meshes[i]->num_verts * sizeof(vec3) );
      vec3 * pos = positions[i];
      for(int j = 0; j < model->meshes[i]->num_verts; j++){
	pos[j] = mat4_mul_vec3(projviewmod1, model->meshes[i]->verticies[j].position);
	pos[j].z = 0;
      }
    }
    
    for(int i=0; i < r->num_surfaces; i++) {
      mesh * mesh1 = model->meshes[i];
      mesh_type mesh1_type = gd->mesh_types[i];
      if(mesh1_type == mesh_scenery)
	continue;
      for(int j=i + 1; j < r->num_surfaces; j++) {
	mesh * mesh2 = model->meshes[j];
	mesh_type mesh2_type = gd->mesh_types[j];
	if(mesh2_type == mesh_scenery)
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
	      if(mesh2_type == mesh_static && mesh1_type == mesh_static){
		matched[i] = j;
		offsets[i] = vec3_new(0,0,0);
	      }else if(mesh2_type == mesh_static){
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
            
    for(size_t i = 0; i < array_count(matched); i++){
      int j = matched[i];
      if(j < 0) continue;
      mesh_type meshitype = gd->mesh_types[i];
      mesh_type meshjtype = gd->mesh_types[j];
      if(meshitype == mesh_static){
	if(meshjtype == mesh_static){
	  
	}else{
	  error("Cannot move static mesh");
	}
      }
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
      int jcnt = gd->connection_cnt[i];
      for(int k = 0; k < jcnt; k++){
	list_push2(gd->connection[j], gd->connection_cnt[j], gd->connection[i][k]);
      }

      printf("connection: %i", gd->connection_cnt[j]);
      for(int k = 0 ; k < gd->connection_cnt[j]; k++){
	printf("%i ", gd->connection[j][k]);
      }
      printf("\n");
      gd->connection_cnt[i] = 0;
      int wincheck[model->num_meshes];
      for(int i = 0; i < model->num_meshes; i++){
	mesh_type type = gd->mesh_types[i];
	wincheck[i] = (type == mesh_dynamic || type == mesh_static) ? 1 : 0;
      }
      for(int j = 0; j < gd->connection_cnt[i]; j++)
	wincheck[gd->connection[j]] += 1;
      bool win = true;
      for(int i = 0; i < model->num_meshes;i++){
	if(false == (wincheck[i] == 0 || wincheck[i] == 2)){
	  win = false;
	  break;
	}
      }
      if(win)
	gd->win_cond_met = true;
    }

    bool replace_renderable = false;
    for(int i = (int)(array_count(matched)-1); i >=0; i--){
      if(matched[i] < 0) continue;
      matched[i] = -1;
      replace_renderable = true;
      mesh_delete(model_remove_mesh(model, i)); 
    }

    if(replace_renderable){
      renderable_delete(r);
      r = renderable_new();
      renderable_add_model(r, model);
      gd->r = r;
    }
    for(int i = 0; i < model->num_meshes; i++)
      free(positions[i]);
    free(positions);
    model_delete(model);
}

