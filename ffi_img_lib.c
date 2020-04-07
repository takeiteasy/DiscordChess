#include "ffi_img_lib.h"

int load_image(struct image_t* out, const char* path) {
  FILE* fp = NULL;
  png_struct* png = NULL;
  png_info* info = NULL;
  
  if (!(fp = fopen(path, "rb"))) {
    free(out);
    return 0;
  }
  
  if (!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
    abort();
  
  if (!(info = png_create_info_struct(png)))
    abort();
  
  if (setjmp(png_jmpbuf(png)))
    abort();
  
  png_init_io(png, fp);
  png_read_info(png, info);
  
  out->width    = png_get_image_width(png, info);
  out->height   = png_get_image_height(png, info);
  out->channels = png_get_channels(png, info);
  
  if (!(out->rows = malloc(sizeof(png_byte*) * out->height)))
    abort();
  for (int y = 0; y < out->height; ++y)
    if (!(out->rows[y] = malloc(png_get_rowbytes(png, info))))
      abort();
  png_read_image(png, out->rows);
  
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
  
  return 1;
}

int copy_image(struct image_t* src, struct image_t* dst) {
  dst->width    = src->width;
  dst->height   = src->height;
  dst->channels = src->channels;
  if (!(dst->rows = malloc(sizeof(png_byte*) * dst->height)))
    abort();
  
  size_t row_sz = sizeof(png_byte) * dst->width * dst->channels;
  for (int y = 0; y < dst->height; ++y) {
    if (!(dst->rows[y] = malloc(row_sz)))
      abort();
    memcpy(dst->rows[y], src->rows[y], row_sz);
  }
  return 1;
}

int blit_image(struct image_t* src, struct image_t* dst, int dx, int dy) {
  if (!src || !dst)
    return 0;
  for (int y = 0; y < src->height; ++y) {
    if (dy + y >= dst->height)
      break;
    if (dy + y < 0)
      continue;
    for (int x = 0; x < src->width; ++x) {
      if (dx + x >= dst->width)
        break;
      if (dx + x < 0)
        continue;
      if (src->rows[y][x * 4 + 3])
        memcpy(dst->rows[dy + y] + (dx + x) * 4, src->rows[y] + x * 4, 3 * sizeof(png_byte));
    }
  }
  return 1;
}

int save_image(struct image_t* src, const char* path) {
  FILE* fp = NULL;
  png_struct* png = NULL;
  png_info* info = NULL;
  
  if (!(fp = fopen(path, "wb")))
    return 0;
  
  if (!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
    abort();
  
  if (!(info = png_create_info_struct(png)))
    abort();
  
  if (setjmp(png_jmpbuf(png)))
    abort();
  
  png_set_IHDR(png,
               info,
               src->width,
               src->height,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  
  png_byte** rows = malloc(sizeof(png_byte*) * src->height);
  if (!rows)
    abort();
  
  for (int y = 0; y < src->height; ++y) {
    if (!(rows[y] = malloc(sizeof(png_byte) * src->height * 3)))
      abort();
    for (int x = 0; x < src->width; ++x)
      memcpy(rows[y] + x * 3, src->rows[y] + x * 4, 3 * sizeof(png_byte));
  }
  
  png_init_io(png, fp);
  png_set_rows(png, info, rows);
  png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
  
  fclose(fp);
  return 1;
}

void free_image(struct image_t* ptr) {
  if (!ptr || !ptr->rows)
    return;
  for (int i = 0; i < ptr->height; ++i)
    if (ptr->rows[i])
      free(ptr->rows[i]);
  free(ptr->rows);
}
