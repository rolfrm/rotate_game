typedef struct{
  char ** name;
  vec3 * offset;
  int cnt;
}level_desc;

void level_load(level_desc * lv, const char * file);
void level_clear(level_desc * lv);
