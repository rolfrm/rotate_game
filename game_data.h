
typedef enum{
  mesh_static,
  mesh_dynamic,
  mesh_scenery
}mesh_type;

typedef struct{
  int ** connections;
  int * connections_cnt;
  mesh_type * mesh_types;
  renderable * r;

}game_data;

void game_data_update(game_data * d, mat4 projview);
void game_data_load(game_data * gd, renderable * r);
