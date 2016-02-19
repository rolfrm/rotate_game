
typedef enum{
  mesh_static,
  mesh_dynamic,
  mesh_scenery
}mesh_type;

typedef struct{
  int ** connection;
  int * connection_cnt;
  mesh_type * mesh_types;
  renderable * r;
  bool win_cond_met;

}game_data;

void game_data_update(game_data * d, mat4 projview);
void game_data_load(game_data * gd, renderable * r);
