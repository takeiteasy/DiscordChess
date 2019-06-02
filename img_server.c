//
//  main.c
//  img_server
//
//  Created by Rory B. Bellows on 28/05/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <png.h>
#include <uuid/uuid.h>
#include <sys/time.h>
#include <stdarg.h>

void LOG(FILE* out, const char *fmt, ...) {
#if !DEBUG
  if (out == stdout)
    return;
#endif
  
  char date[20];
  struct timeval tv;
  gettimeofday(&tv, NULL);
  strftime(date, sizeof(date) / sizeof(*date), "%Y-%m-%dT%H:%M:%S", gmtime(&tv.tv_sec));
  fprintf(out, "[%s.%03dZ] ", date, tv.tv_usec / 1000);
  
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

typedef struct {
  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_byte** rows;
} image_t;

#if DEBUG
# define RES_PATH "/Users/roryb/git/chess_discord_bot/assets/"
#else
# if !defined(RES_PATH)
#   define RES_PATH "assets/"
# endif
#endif
#define RES_JOIN(X,Y) (X Y ".png")
#define RES(X) (RES_JOIN(RES_PATH, X))

static const char* assets_paths[13] = {
  RES("white_pawn"),   RES("black_pawn"),
  RES("white_rook"),   RES("black_rook"),
  RES("white_knight"), RES("black_knight"),
  RES("white_bishop"), RES("black_bishop"),
  RES("white_queen"),  RES("black_queen"),
  RES("white_king"),   RES("black_king"),
  RES("board")
};
static const char* piece_map = "PpRrNnBbQqKk";
static image_t assets[13];

int load_asset(image_t* out, const char* path) {
  int ret = 1, y;
  
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    LOG(stderr, "ERROR! Failed to load '%s'\n", path);
    goto END3;
  }
  
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END2;
  }
  
  png_infop info = png_create_info_struct(png);
  if (!info) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END2;
  }
  
  if (setjmp(png_jmpbuf(png))) {
    LOG(stderr, "ERROR: Failed to create PNG!\n");
    goto END1;
  }
  
  png_init_io(png, fp);
  png_read_info(png, info);
  
  out->width      = png_get_image_width(png, info);
  out->height     = png_get_image_height(png, info);
  out->color_type = png_get_color_type(png, info);
  out->bit_depth  = png_get_bit_depth(png, info);
  
  // Read PNG data to image_t
  out->rows = malloc(sizeof(png_byte*) * out->height);
  if (!out->rows) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END1;
  }
  for (y = 0; y < out->height; y++) {
    out->rows[y] = malloc(png_get_rowbytes(png, info));
    if (!out->rows[y]) {
      LOG(stderr, "ERROR: Out of memory!\n");
      out->rows[y] = NULL;
      goto END1;
    }
  }
  png_read_image(png, out->rows);
  
  // Clean up
  ret = 0;
  goto END2;
END1:
  for (y = 0; y < out->height; y++) {
    if (!out->rows[y])
      break;
    png_free(png, out->rows[y]);
  }
  if (out->rows)
    png_free(png, out->rows);
END2:
  png_destroy_read_struct(&png, &info, NULL);
END3:
  fclose(fp);
  return ret;
}

void blit_piece(image_t* dst, image_t* src, int _x, int _y) {
  int x = 40 + (22 * _x) + ((22 - src->width)  / 2);
  int y = 40 + (22 * _y) + ((22 - src->height) / 2);
  int i, j;
  png_byte *dst_row = NULL, *src_row = NULL;
  for (i = 0; i < src->height; ++i) {
    dst_row = dst->rows[y + i];
    src_row = src->rows[i];
    for (j = 0; j < src->width; ++j)
      if (src_row[j * 4 + 3])
        memcpy(dst_row + (x + j) * 3, src_row + j * 4, 3 * sizeof(png_byte));
  }
}

void* client_handler(void* _sock) {
  int sock = *(int*)_sock;
  LOG(stdout, "AWAITING CLIENT REQUEST\n");
  
  image_t out = {
    .width = 256,
    .height = 256,
    .color_type = PNG_COLOR_TYPE_RGB,
    .bit_depth = 8,
    .rows = NULL
  };
  
  // Recieve message from client
  char buf[65];
  size_t msg_sz = recv(sock, buf, 65, 0);
  if (!msg_sz) {
    LOG(stderr, "ERROR: Failed to recv client request\n");
    goto END4;
  }
  buf[64] = '\0';
  
  LOG(stdout, "CLIENT REQUEST PROCESSING\n");
  
  int i, j, x, y;
#if DEBUG
  LOG(stdout, "BOARD:\n  ");
  for (i = 0; i < 8; ++i)
    fprintf(stdout, "%d ", i);
  fprintf(stdout, "\n");
  for (i = 0; i < 8; ++i) {
    fprintf(stdout, "%d ", 8 - i);
    for (j = 0; j < 8; ++j)
      fprintf(stdout, "%c ", buf[8 * i + j]);
    fprintf(stdout, "%d\n", i);
  }
  fprintf(stdout, "  ");
  for (i = 0; i < 8; ++i)
    fprintf(stdout, "%c ", 65 + i);
  fprintf(stdout, "\n--------------------\n");
#endif
  
  // Create UUID for filename
  uuid_t _uuid;
  uuid_generate_random(_uuid);
  char* uuid = malloc(37 * sizeof(char));
  uuid_unparse_lower(_uuid, uuid);
  char filename[46];
  sprintf(filename, "/tmp/%s.png", uuid);
  free(uuid);
  
  FILE* fp = fopen(filename, "wb");
  if (!fp) {
    LOG(stderr, "ERROR: Failed to create '%s'\n", filename);
    goto END4;
  }
  
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END3;
  }
  png_infop info = png_create_info_struct(png);
  if (!info) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END2;
  }
  
  if (setjmp(png_jmpbuf(png))) {
    LOG(stderr, "ERROR: Failed to create PNG!\n");
    goto END1;
  }
  
  png_set_IHDR(png,
               info,
               256,
               256,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  
  // Copy board asset to new image_t
  png_byte *row = NULL, *src_row = NULL;
  out.rows = malloc(sizeof(png_byte*) * 256);
  if (!out.rows) {
    LOG(stderr, "ERROR: Out of memory!\n");
    goto END1;
  }
  for (y = 0; y < 256; ++y) {
    out.rows[y] = malloc(sizeof(png_byte) * 256 * 3);
    if (!out.rows[y]) {
      LOG(stderr, "ERROR: Out of memory!\n");
      out.rows[y] = NULL;
      goto END1;
    }
    
    // Convert RGBA to RGB
    row = out.rows[y];
    src_row = assets[12].rows[y];
    for (x = 0; x < 256; ++x)
      memcpy(row + x * 3, src_row + x * 4, 3 * sizeof(png_byte));
  }
  
  // Go through board and blit pieces
  char p;
  for (i = 0; i < 8; ++i) {
    for (j = 0; j < 8; ++j) {
      p = buf[8 * j + i];
      if (p == '.')
        continue;
      for (x = 0; x < 12; ++x) {
        if (piece_map[x] == p) {
          blit_piece(&out, &assets[x], i, j);
          break;
        }
      }
    }
  }
  
  // Finish writing PNG to disk
  png_init_io(png, fp);
  png_set_rows(png, info, out.rows);
  png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
  
  // Return file path to client
  LOG(stdout, "CLIENT REQUEST COMPLETED\n");
  write(sock, filename, 46 * sizeof(char));
  LOG(stdout, "CLIENT SERVED\n");
  
  // Clean up
END1:
  png_destroy_write_struct(&png, &info);
END2:
  fclose(fp);
END3:
  for (y = 0; i < 256; ++y) {
    if (!out.rows[y])
      break;
    png_free(png, out.rows[y]);
  }
  if (out.rows)
    png_free(png, out.rows);
END4:
  close(sock);
  return 0;
}

int main(int argc, const char* argv[]) {
  int ret = 1, i, j;
  
  // Load PNG assets to array
  for (i = 0; i < 13; ++i)
    if (load_asset(&assets[i], assets_paths[i]))
      return 1;
  LOG(stdout, "ASSETS LOADED\n");
  
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    fprintf(stderr, "ERROR! Failed to create socket\n");
    goto END;
  }
  LOG(stdout, "SOCKET CREATED\n");
  
  struct sockaddr_in sockaddr, client_sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = INADDR_ANY;
  sockaddr.sin_port = htons(8950);
  
#if DEBUG
  // Ignore port already in use error
  int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    fprintf(stderr,"ERROR: Failed to set socket opt\n");
    goto END;
  }
#endif
  
  if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0){
    fprintf(stderr, "ERROR: Failed to bind socket\n");
    goto END;
  }
  LOG(stdout, "SOCKET BOUND\n");
  
  // Listen for connections, create thread
  listen(sock, 10);
  int client_sock, c = sizeof(struct sockaddr_in);
  pthread_t tid;
  LOG(stdout, "ACCEPTING CLIENTS!\n");
  while ((client_sock = accept(sock, (struct sockaddr*)&client_sockaddr, (socklen_t*)&c))) {
    LOG(stdout, "CLIENT ACCEPTED\n");
    if (pthread_create(&tid , NULL,  client_handler, (void*)&client_sock) < 0) {
      fprintf(stderr, "ERROR: Failed to create thread\n");
      goto END;
    }
    LOG(stdout, "CLIENT HANDLER ASSIGNED\n");
  }
  
  if (client_sock < 0) {
    fprintf(stderr, "ERROR: Client accept failed\n");
    goto END;
  }
  
  // Clean up
  ret = 0;
END:
  close(sock);
  for (i = 0; i < 13; ++i) {
    if (!assets[i].rows)
      continue;
    for (j = 0; j < assets[i].height; ++j) {
      if (!assets[i].rows[j])
        break;
      free(assets[i].rows[j]);
    }
    free(assets[i].rows);
  }
  return ret;
}
