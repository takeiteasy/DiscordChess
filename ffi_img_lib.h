#pragma once
#include <png.h>
#include <stdlib.h>
#include <string.h>

struct image_t {
  int width, height, channels;
  png_byte** rows;
};

int load_image(struct image_t* out, const char* path);
int copy_image(struct image_t* src, struct image_t* dst);
int blit_image(struct image_t* src, struct image_t* dst, int dx, int dy);
int save_image(struct image_t* src, const char* path);
void free_image(struct image_t* ptr);
