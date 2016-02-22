
typedef enum{
  mesh_static,
  mesh_dynamic,
  mesh_scenery
}mesh_type;

typedef struct{
  static_object * object;
  vec3 ** positions;
  vec3 ** positions_cache;
  int * vertex_cnt;
  bool * ignore;
  int positions_cnt;

  int * linked_object;
  int linked_object_cnt;
}movable_object;

typedef struct{
  bool win_cond_met;
  movable_object * movable;
  int movable_cnt;
}game_data;

void game_data_update(game_data * d, mat4 projview);
void game_data_load(game_data * gd, const level_desc * lv);
