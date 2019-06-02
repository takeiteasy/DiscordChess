//
//  main.c
//  chess_bitboard
//
//  Created by Rory B. Bellows on 25/05/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITE    0
#define BLACK    1
#define FULL     2

#define PAWNS    3
#define ROOKS    5
#define KNIGHTS  7
#define BISHOPS  9
#define QUEENS   11
#define KINGS    13

#define BOARD_SZ 15
#define BOARD_STR_SZ 65

typedef unsigned long long u64;
typedef u64* bitboard_t;
typedef u64 board_t[BOARD_SZ];
typedef u64 (*board_ptr_t)[BOARD_SZ];
typedef char board_str_t[BOARD_STR_SZ];
typedef char (*board_str_ptr_t)[BOARD_STR_SZ];

static board_t DEFAULT_BOARD = {
  // WHITE            BLACK               FULL
  0xf7ff000000000000, 0x000000000000ffff, 0xf7ff00000000ffff,
  // PAWNS
  0x00ff000000000000, 0x000000000000ff00,
  // ROOKS
  0x8100000000000000, 0x0000000000000081,
  // KNIGHTS
  0x4200000000000000, 0x0000000000000042,
  // BISHOPS
  0x2400000000000000, 0x0000000000000024,
  // QUEENS
  0x0800000000000000, 0x0000000000000008,
  // KINGS
  0x1000000000000000, 0x0000000000000010
};

typedef struct {
  unsigned short side;
  unsigned short castling;
  unsigned short en_passant;
  unsigned int   halfmove_clock;
  unsigned short fullmove_number;
  board_t        board;
  board_str_t    str;
} game_t;

static const char* pieces = "PpRrNnBbQqKk";

void update_board_str(board_ptr_t _b, board_str_ptr_t out) {
  int i, j, k, l, n;
  // Reset board string
  memset(out, '.', sizeof(*out));
  (*out)[64] = '\0';
  
  // Loop through all pieces
  // White pieces are even, black are odd
  for (i = 0; i < 12; i += 2) {
    // Array of black and white pieces (white = 0, black = 1)
    bitboard_t _p[2] = { *_b + 3 + i, *_b + 3 + i + 1};
    // Loop through black and white
    for (j = 0; j < 2; ++j) {
      // Pointer to current pieces
      bitboard_t p = *(_p + j);
      // Loop through rank, top to bottom
      for (k = 7; k >= 0; --k) {
        // Loop though file, left to right
        for (l = 0; l < 8; ++l) {
          n = 8 * k + l;
          // If occupied set the piece in the string
          if (*p & (1ULL << n))
            (*out)[n] = pieces[i + j];
        }
      }
    }
  }
}

void print_board_str(board_str_ptr_t b, FILE* out) {
  int i, j;
  for (i = 0; i < 8; ++i) {
    for (j = 0; j < 8; ++j)
      fprintf(out, "%c", (*b)[i * 8 + j]);
    printf("\n");
  }
}

void init_game(game_t* g) {
  g->side            = WHITE;
  g->castling        = 0x1111;
  g->en_passant      = -1;
  g->halfmove_clock  =  0;
  g->fullmove_number =  1;
  memcpy(&g->board, &DEFAULT_BOARD, BOARD_SZ * sizeof(unsigned long long));
  update_board_str(&g->board, &g->str);
}

int main(int argc, const char* argv[]) {
  game_t game;
  init_game(&game);
  print_board_str(&game.str, stdout);
  return 0;
}
