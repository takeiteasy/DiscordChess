#include <stdio.h>
#include "../ffi_img_lib.h"

int main(int argc, const char* argv[]) {
	struct image_t* test = load_image("assets/board.png");
	if (!test)
		return 1;
		
	struct image_t* test2 = load_image("assets/black_knight.png");
	if (!test2)
		return 2;
		
	struct image_t* test3 = copy_image(test2);
	if (!test3)
		return 3;
	
	blit_image(test2, test, 245, 245);
	blit_image(test3, test, 10, 10);
	
	if (!save_image(test, "test.png"))
		return 4;
	
	free_image(test);
	free_image(test2);
	free_image(test3);
	return 0;
}