#include <stdio.h>
#include "../ffi_img_lib.h"

int main(int argc, const char* argv[]) {
  struct image_t test, test2, test3;
  
  if (!load_image(&test, "assets/board.png"))
    return 1;
  
  if (!load_image(&test2, "assets/black_knight.png"))
    return 2;
  
  if (!copy_image(&test2, &test3))
    return 3;
  
  blit_image(&test2, &test, 245, 245);
  blit_image(&test3, &test, 10, 10);
  
  if (!save_image(&test, "test.png"))
    return 4;
  
  free_image(&test);
  free_image(&test2);
  free_image(&test3);
  return 0;
}
