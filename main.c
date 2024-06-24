/**
 * Hammer: A Reversi Minimax AI
 * Copyright (C) 2024 marcus8448
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include <stdbool.h>
#include <threads.h>
#ifdef _MSC_VER
#include <intrin.h>
#include <Windows.h>
#endif

#define BOARD_SIZE 8
#define MOVE_CUTOFF 15000000

#ifdef DEBUG_MODE
#define DEBUG_LOG
#undef NDEBUG
#include <assert.h>
#else
#define puts(s) ;
#define printf(s, ...) ;
#define putchar(c) ;
#define fflush(f) ;
#endif
#define max(a, b) (a > b ? a : b)
#include <stdatomic.h>

static int placedTiles = 0;

static void print_board(uint64_t player, uint64_t opponent) {
  uint8_t index = 0;
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++, index++) {
      printf("%c ", ((player >> index) & 0b1) ? 'X' : opponent >> index & 0b1 ? 'O' : '-');
    }
    putchar('\n');
  }
  assert(!(player & opponent));
}

typedef struct BoardState {
  struct BoardState *nextStates;
  uint64_t player;
  uint64_t opponent;
  int16_t value;
  int16_t worstBranch;
  uint8_t x;
  uint8_t y;
  int8_t lenStates;
} BoardState;

static int8_t BOARD_VALUES[BOARD_SIZE * BOARD_SIZE] = {
    1,   -30,  1,   -1,   -1,    1,   -30,  1,
    -30, -30,  0,    0,    0,    0,   -30,  -30,
    1,   0,    0,    0,    0,    0,   0,    1,
    -1,  0,    0,    0,    0,    0,   0,   -1,
    -1,  0,    0,    0,    0,    0,   0,   -1,
    1,   0,    0,    0,    0,    0,   0,    1,
    -30, -30,  0,    0,    0,    0,   -30,  -30,
    1,   -30,  1,   -1,   -1,    1,   -30,  1,
};

static BoardState head = {.nextStates = NULL,
    .player = 0,
    .opponent = 0,
    .value = INT16_MIN,
    .worstBranch = 0,
    .x = 0,
    .y = 0,
    .lenStates = -1,
};

static int eval_left(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_right(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_down(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_up(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_up_left(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_up_right(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_down_left(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int eval_down_right(uint64_t p, uint64_t o, uint8_t x, uint8_t y, int16_t changed) {
  return changed + BOARD_VALUES[y * BOARD_SIZE + x];
}

static int maxDepth = 0;
static uint64_t visited = 0;

void generate_child_moves(BoardState *state) {
  assert(state->lenStates == -1);

  // swap
  const uint64_t player = state->opponent;
  const uint64_t opponent = state->player;

  BoardState *boards = malloc(sizeof(BoardState) * 16);

  int8_t move = 0;

  boards[move].nextStates = NULL;
  boards[move].x = 0;
  boards[move].y = 0;
  boards[move].player = player;
  boards[move].opponent = opponent;
  boards[move].value = 0;
  boards[move].worstBranch = 0;
  boards[move].lenStates = -1;

  // board (byte format)
  // << 63 62 61 60 59 58 57 56
  //    55 54 53 52 51 50 49 48
  //    47 46 45 44 43 42 41 40
  //    39 38 37 36 35 34 33 32
  //    31 30 29 28 27 26 25 24
  //    23 22 21 20 19 18 17 16
  //    15 14 13 12 11 10 09 08
  //    07 06 05 04 03 02 01 00 >>

  // 1 = empty, 0 = taken
  const uint64_t empty = ~(player | opponent);

  // 1 = empty spot adjacent to opponent on the right, 0 = not
  const uint64_t right = empty & (opponent << 1); // RIGHT EDGE INVALID

  // 1 = empty spot adjacent to opponent on the left, 0 = not
  const uint64_t left = empty & (opponent >> 1); // LEFT EDGE INVALID

  // 1 = empty spot adjacent to opponent above, 0 = not
  const uint64_t up = empty & (opponent >> BOARD_SIZE); // TOP ROW DEAD

  // 1 = empty spot adjacent to opponent below, 0 = not
  const uint64_t down = empty & (opponent << BOARD_SIZE); // BOTTOM ROW DEAD

  // 1 = empty spot adjacent to opponent on the diagonal top-right, 0 = not
  const uint64_t upRight = empty & (opponent >> (BOARD_SIZE - 1)); // TOP + RIGHT EDGE INVALID

  // 1 = empty spot adjacent to opponent on the diagonal bottom-left, 0 = not
  const uint64_t downLeft = empty & (opponent << (BOARD_SIZE - 1)); // BOTTOM + LEFT EDGE INVALID

  // 1 = empty spot adjacent to opponent on the diagonal top-left, 0 = not
  const uint64_t upLeft = empty & (opponent >> (BOARD_SIZE + 1)); // TOP + LEFT EDGE INVALID

  // 1 = empty spot adjacent to opponent on the diagonal bottom-right, 0 = not
  const uint64_t downRight = empty & (opponent << (BOARD_SIZE + 1)); // BOTTOM + RIGHT EDGE INVALID

  // index where tile will be placed
  uint8_t index = 0;

  // iterate over entire board
  for (int8_t y = 0; y < BOARD_SIZE; ++y) {
    for (int8_t x = 0; x < BOARD_SIZE; ++x) {
      // true if a move is possible, false otherwise
      bool success = false;
      // the value of the move (for the current player). Set to 1 to account for the placed tile.
      int16_t value = 1;

      // start at rightmost (min X) index, move left (+X)
      if (x + 2 < BOARD_SIZE && left >> index & 0b1) {
        // testX - x = number of tiles to flip (includes placed tile)
        // testX - x = offset from index
        for (int8_t testX = x + 2; testX < BOARD_SIZE; testX++) {
          const int8_t toFlip = testX - x;

          // if it's an opponent tile, just continue
          if (!(opponent >> (index + toFlip) & 0b1)) {
            // empty tile, so fail
            if (empty >> (index + toFlip) & 0b1) {
              break;
            }

            // found self (not opponent or empty), success
            assert((player >> (index + toFlip)) & 0b1);
            success = true;

            // take full strip, reduce to flipped length, shift to index
            const uint64_t mask = ((0b11111111ULL >> (BOARD_SIZE - (toFlip + 1))) << index);
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_left(boards[move].player, boards[move].opponent, x, y, toFlip - 1);
            break;
          }
        }
      }

      // start at leftmost (max X) index, move right (-X)
      if (x >= 2 && right >> index & 0b1) {
        // x - testX = number of tiles to flip (includes placed tile)
        // testX - x = offset from index
        for (int8_t testX = x - 2; testX >= 0; testX--) {
          // if it's an opponent tile, just continue
          if (!(opponent >> (index + (testX - x)) & 0b1)) {
            // empty tile, so fail
            if (empty >> (index + (testX - x)) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testX - x))) & 0b1);
            success = true;

            // take full strip, reduce to flipped length, shift to right of index
            const uint64_t mask =
                ((0b11111111ULL >> (BOARD_SIZE - ((x - testX)))) << (index - ((x - testX - 1))));
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_right(boards[move].player, boards[move].opponent, x, y, x - testX - 1);
            break;
          }
        }
      }

      // start at lowest (min y) index, move up (+Y)
      if (y + 2 < BOARD_SIZE && up >> index & 0b1) {
        // testY - y = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE = offset from index
        for (int8_t testY = y + 2; testY < BOARD_SIZE; testY++) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE)) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE)) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE)) & 0b1);
            success = true;

            // take full '\' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b0000000100000001000000010000000100000001000000010000000100000001ULL >>
                  ((BOARD_SIZE - ((testY - y)))) * BOARD_SIZE)
                 << index);
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_up(boards[move].player, boards[move].opponent, x, y, testY - y - 1);
            break;
          }
        }
      }

      // start at highest (max y) index, move down (-Y)
      if (y >= 2 && down >> index & 0b1) {
        // y - testY = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE = offset from index
        for (int8_t testY = y - 1; testY >= 0; testY--) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE)) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE)) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE)) & 0b1);
            success = true;

            // take full '\' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b0000000100000001000000010000000100000001000000010000000100000001ULL >>
                  ((BOARD_SIZE - ((y - testY))) * BOARD_SIZE))
                 << (index - (y - testY - 1) * BOARD_SIZE));
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_down(boards[move].player, boards[move].opponent, x, y, y - testY - 1);
            break;
          }
        }
      }

      // TOP + RIGHT EDGE INVALID / bottom-left to top-right
      if (y + 2 < BOARD_SIZE && x >= 2 && upRight >> index & 0b1) {
        // testY - y, x - testX = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE, testX - x = offset from index
        for (int8_t testY = y + 2, testX = x - 2; testY < BOARD_SIZE && testX >= 0;
             testY++, testX--) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1);
            success = true;

            // take mask of '/' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b0000000100000010000001000000100000010000001000000100000010000000ULL >>
                  (((BOARD_SIZE - ((testY - y))) * BOARD_SIZE)))
                 << ((y)*BOARD_SIZE + (x - (testY - y - 1))));
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_up_right(boards[move].player, boards[move].opponent, x, y, testY - y - 1);
            break;
          }
        }
      }

      // BOTTOM + LEFT EDGE INVALID / top-right to bottom-left
      if (y >= 2 && x + 2 < BOARD_SIZE && downLeft >> index & 0b1) {
        // y - testY, testX - x = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE, testX - x = offset from index
        for (int8_t testY = y - 2, testX = x + 2; testY >= 0 && testX < BOARD_SIZE;
             testY--, testX++) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1);
            success = true;

            // take mask of '/' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b0000000100000010000001000000100000010000001000000100000010000000ULL
                  << ((BOARD_SIZE - ((y - testY))) * BOARD_SIZE)) >>
                 ((BOARD_SIZE - y - 1) * BOARD_SIZE + BOARD_SIZE - testX));
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value +=
                eval_down_left(boards[move].player, boards[move].opponent, x, y, y - testY - 1);
            break;
          }
        }
      }

      // TOP + LEFT EDGE INVALID / bottom-right to top-left
      if (y + 2 < BOARD_SIZE && x + 2 < BOARD_SIZE && upLeft >> index & 0b1) {
        // testY - y, testX - x = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE, testX - x = offset from index
        for (int8_t testY = y + 2, testX = x + 2; testY < BOARD_SIZE && testX < BOARD_SIZE;
             testY++, testX++) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1);
            success = true;

            // take mask of '\' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b1000000001000000001000000001000000001000000001000000001000000001ULL >>
                  (((BOARD_SIZE - ((testY - y))) * BOARD_SIZE) + (BOARD_SIZE - ((testY - y)))))
                 << index);
            assert(mask);

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            // add value of move
            value += eval_up_left(boards[move].player, boards[move].opponent, x, y, testY - y - 1);
            break;
          }
        }
      }

      // BOTTOM + RIGHT EDGE INVALID / top-left to bottom-right
      if (y >= 2 && x >= 2 && downRight >> index & 0b1) {
        // y - testY, x - testX = number of tiles to flip (includes placed tile)
        // (testY - y) * BOARD_SIZE, testX - x = offset from index
        for (int8_t testY = y - 2, testX = x - 2; testY >= 0 && testX >= 0; testY--, testX--) {
          // if it's an opponent tile, just continue
          if (!((opponent >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1)) {
            // empty tile, so fail
            if ((empty >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1) {
              break;
            }
            // found self (not opponent or empty), success
            assert((player >> (index + (testY - y) * BOARD_SIZE + (testX - x))) & 0b1);
            success = true;

            // take mask of '\' diagonal, reduce to flipped height, shift to index
            const uint64_t mask =
                ((0b1000000001000000001000000001000000001000000001000000001000000001ULL
                  << (((BOARD_SIZE - ((y - testY))) * BOARD_SIZE) + (BOARD_SIZE - (y - testY)))) >>
                 ((BOARD_SIZE - y - 1) * BOARD_SIZE + (BOARD_SIZE - x - 1)));

            // set player tiles
            boards[move].player |= mask;
            assert(boards[move].player & boards[move].opponent);

            // clear opponent tiles
            boards[move].opponent &= ~mask;
            assert(!(boards[move].player & boards[move].opponent));

            value +=
                eval_down_right(boards[move].player, boards[move].opponent, x, y, y - testY - 1);
            break;
          }
        }
      }

      // if the move was successful, add it to the list of moves
      if (success) {
        printf("^^^^^^ %i, %i (d=%i) ^^^^^^\n", x, y, depth);
        print_board(boards[move].player, boards[move].opponent);

        // set the position + value of the move
        boards[move].x = x;
        boards[move].y = y;
        boards[move].value = value;

        // prepare next board state
        if (++move == 16) {
          // https://jxiv.jst.go.jp/index.php/jxiv/preprint/download/480/1498/1315
          boards = (BoardState*)realloc(boards, sizeof(BoardState) * 33);
          assert(boards != NULL);
        }

        boards[move].nextStates = NULL;
        boards[move].x = 0;
        boards[move].y = 0;
        boards[move].player = player;
        boards[move].opponent = opponent;
        boards[move].value = 0;
        boards[move].worstBranch = 0;
        boards[move].lenStates = -1;
      }

      index++;
    }
  }

  // check if moves were found
  if (move != 0) {
    boards = (BoardState*)realloc(boards, sizeof(BoardState) * move);
    assert(boards != NULL);
  } else {
    free(boards);
    boards = NULL;
  }

  state->nextStates = boards;
  state->lenStates = move;
  assert(state->lenStates != -1);
}

void recalculate_move_values(BoardState *state, const uint8_t depth) {
  assert(!(state->player & state->opponent));

  if (state->lenStates > 0) {
    if (depth < maxDepth) {
      int16_t max = INT16_MIN;

      // serially iterate over all possible moves
      for (int i = 0; i < state->lenStates; ++i) {
        recalculate_move_values(&state->nextStates[i], depth + 1);
        const int16_t realVal = state->nextStates[i].value - state->nextStates[i].worstBranch;
        max = realVal > max ? realVal : max;
      }
      state->worstBranch = max;
    }
  } else {
    // check if ending the game is beneficial
    if (_popcnt64(state->player) < _popcnt64(state->opponent)) {
      // we lose, so set the worst branch to the worst possible value
      state->value = 0;
      state->worstBranch = INT16_MAX / 8;
    } else if (_popcnt64(state->player) > _popcnt64(state->opponent)) {
      // we win, so set the worst branch to the best possible value
      state->value = 0;
      state->worstBranch = INT16_MIN / 8;
    } else {
      // tie, so set the worst branch to 0
      state->value = 0;
      state->worstBranch = -10;
    }
  }
}

void search_for_moves_serial(BoardState *state, int16_t alpha, int16_t beta, const uint8_t depth) {
  assert(!(state->player & state->opponent));

  if (state->lenStates == -1) {
    generate_child_moves(state);
  }

  if (state->lenStates > 0) {
    if (depth < maxDepth) {
      int16_t max = INT16_MIN;

      visited += state->lenStates;
      if (visited > MOVE_CUTOFF) {
        maxDepth = depth;
        return;
      }
      // serially iterate over all possible moves
      for (int i = 0; i < state->lenStates; ++i) {
        search_for_moves_serial(&state->nextStates[i], beta, alpha, depth + 1);
        const int16_t realVal = state->nextStates[i].value - state->nextStates[i].worstBranch;
        max = realVal > max ? realVal : max;
        alpha = alpha > max ? alpha : max;

        if (beta != INT16_MIN && max > beta) {
          break;
        }
      }
      state->worstBranch = max;
    }
  } else {
    // check if ending the game is beneficial
    if (_popcnt64(state->player) < _popcnt64(state->opponent)) {
      // we lose, so set the worst branch to the worst possible value
      state->value = 0;
      state->worstBranch = INT16_MAX / 8;
    } else if (_popcnt64(state->player) > _popcnt64(state->opponent)) {
      // we win, so set the worst branch to the best possible value
      state->value = 0;
      state->worstBranch = INT16_MIN / 8;
    } else {
      // tie, so set the worst branch to 0
      state->value = 0;
      state->worstBranch = -10;
    }
  }
}

struct ThreadArgs {
  BoardState *state;
  int16_t alpha;
  int16_t beta;
  uint8_t depth;
};

int search_for_move_thread_cb(void *args) {
  const struct ThreadArgs *thread_args = args;
  search_for_moves_serial(thread_args->state, thread_args->alpha, thread_args->beta, thread_args->depth);
  return 0;
}

void search_for_moves_paralell(BoardState *state, int16_t alpha, int16_t beta, const uint8_t depth, const int8_t parDepth) {
  assert(!(state->player & state->opponent));

  if (state->lenStates == -1) {
    generate_child_moves(state);
  }

  if (state->lenStates > 0) {
    if (depth < maxDepth) {
      int16_t worst = INT16_MIN;

      if (parDepth == depth) {
        thrd_t *threads = malloc(sizeof(thrd_t) * state->lenStates);

        // paralelly iterate over all possible moves
        visited += state->lenStates;
        if (visited > MOVE_CUTOFF) {
          maxDepth = depth;
          return;
        }
        for (int i = 0; i < state->lenStates; ++i) {
          thrd_create(&threads[i], search_for_move_thread_cb, &(struct ThreadArgs) {.state = &state->nextStates[i], .beta = beta, .alpha = alpha, .depth = (uint8_t)depth + 1 });
        }

        for (int i = 0; i < state->lenStates; ++i) {
          if (thrd_join(threads[i], NULL) != thrd_success) {
            fputs("FAIL\n", stdout);
          }
        }

        free(threads);

        for (int i = 0; i < state->lenStates; i++) {
          const int16_t realVal = state->nextStates[i].value - state->nextStates[i].worstBranch;
          worst = max(realVal, worst);
          alpha = max(alpha, worst);
        }
      } else {
        // serially iterate over all possible moves
        visited += state->lenStates;
        if (visited > MOVE_CUTOFF) {
          maxDepth = depth;
          return;
        }
        for (int i = 0; i < state->lenStates; ++i) {
          search_for_moves_paralell(&state->nextStates[i], beta, alpha, depth + 1, parDepth);
          const int16_t realVal = state->nextStates[i].value - state->nextStates[i].worstBranch;
          worst = realVal > worst ? realVal : worst;
          alpha = alpha > worst ? alpha : worst;

          if (beta != INT16_MIN && worst > beta) {
            break;
          }
        }
      }

      state->worstBranch = worst;
    }
  } else {
    // check if ending the game is beneficial
    if (_popcnt64(state->player) < _popcnt64(state->opponent)) {
      // we lose, so set the worst branch to the worst possible value
      state->value = 0;
      state->worstBranch = INT16_MAX / 8;
    } else if (_popcnt64(state->player) > _popcnt64(state->opponent)) {
      // we win, so set the worst branch to the best possible value
      state->value = 0;
      state->worstBranch = INT16_MIN / 8;
    } else {
      // tie, so set the worst branch to 0
      state->value = 0;
      state->worstBranch = -10;
    }
  }
#ifdef DEBUG_LOG
  if (depth == 0) {
    fprintf(stdout,
            "%i:%i [n=%i,t=%i,d=%i]: %i\n",
            static_cast<int>(__popcntq(head.player)),
            static_cast<int>(__popcntq(head.opponent)),
            head.lenStates,
            placedTiles,
            maxDepth,
            state->worstBranch);
  }
#endif
}

/**
 * \brief frees all the children of a board state
 * \param state the board state to free
 * \note DOES NOT free the state itself (assumes its a child in an array)
 */
static void node_free(BoardState *state, int exclude) {
  for (int i = 0; i < state->lenStates; ++i) {
    if (i == exclude) continue;
    node_free(&state->nextStates[i], -1);
  }

  if (state->nextStates != NULL) {
    free(state->nextStates);
    state->nextStates = NULL;
  }
  state->lenStates = -1;
}

/**
 * \brief generates a move for the current board state
 * \param self python module instance
 * \param args function arguments from python: board, player, time
 * \return the move to make in a python dict -> list -> tuple
 */
static PyObject *revai_ai(PyObject *self, PyObject *args) {
  puts("Start");

  // parse arguments
  PyObject *pyBoard;
  PyObject *pyPlayer;
  int64_t time_s = 0;
  PyArg_ParseTuple(args, "OOk", &pyBoard, &pyPlayer, &time_s);

  // check if the board is empty (first move)
  if ((head.player | head.opponent) == 0) {
    // get the board state from the python game
    placedTiles = 0;
    uint8_t index = 0;
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
      PyObject *pyRow = PyList_GetItem(pyBoard, y);
      for (uint8_t x = 0; x < BOARD_SIZE; x++, index++) {
        PyObject *pyItem = PyList_GetItem(pyRow, x);
        // if the tile is not a tuple it is a player tile
        if (!PyTuple_Check(pyItem)) {
          // check if the tile is ours
          // we're generating the previous move's state, so we swap player/opponent
          if (PyObject_RichCompareBool(pyItem, pyPlayer, Py_EQ)) {
            head.opponent |= 1ULL << index;
          } else {
            head.player |= 1ULL << index;
          }

          placedTiles++;
        }
      }
    }

    head.value = 0;
  } else {
    // find the move that was made
    for (int i = 0; i < head.lenStates; ++i) {
      const BoardState nxt = head.nextStates[i];

      // unpack the position of the move
      const uint8_t x = nxt.x;
      const uint8_t y = nxt.y;

      // check if the tile at the position (python) is not empty (tuple)
      if (!PyObject_IsInstance(PyList_GetItem(PyList_GetItem(pyBoard, y), x), (PyObject *)&PyTuple_Type)) {
        printf("Opponent: %i, %i\n", x, y);

        // free all the other moves
        node_free(&head, i);

        puts("OPP BEFORE");
        print_board(head.player, head.opponent);
        puts("OPP AFTER");
        print_board(nxt.opponent, nxt.player);

        // set the new board state and increment the move count
        head = nxt;
        placedTiles++;

#ifdef DEBUG_LOG
        fprintf(stdout, "%i:%i Opponent [t=%i]: %i\n",
                static_cast<int>(__popcntq(head.player)),
                static_cast<int>(__popcntq(head.opponent)), placedTiles, head.value - head.worstBranch);
#endif
        break;
      }
    }
  }

  // calculate the maximum depth to search based on the number of tiles placed
  placedTiles++;
  maxDepth = placedTiles < 25 ? 3
             : placedTiles < 30 ? 5
             : placedTiles < 35 ? 6
             : placedTiles < 40 ? 7
             : placedTiles < 45 ? 7
             : placedTiles < 50 ? 7
                                : 8;

  SYSTEMTIME time;
  GetSystemTime(&time);
  visited = 0;
  const int rmd = maxDepth;
  // calculate the best possible move
  if (maxDepth == 5) {
    search_for_moves_paralell(&head, INT16_MIN, INT16_MIN, 0, 1);
  } else if (maxDepth == 6) {
    search_for_moves_paralell(&head, INT16_MIN, INT16_MIN, 0, 3);
  } else if (maxDepth == 7) {
    search_for_moves_paralell(&head, INT16_MIN, INT16_MIN, 0, 3);
  }  else if (maxDepth == 8) {
    search_for_moves_paralell(&head, INT16_MIN, INT16_MIN, 0, 4);
  } else {
    // otherwise search serially
    search_for_moves_serial(&head, INT16_MIN, INT16_MIN, 0);
  }

  SYSTEMTIME time2;
  GetSystemTime(&time2);
  uint64_t duration =
      (time2.wDay - time.wDay) * 24 * 60 * 1000ULL + (time2.wMinute - time.wMinute) * 60 * 1000ULL +
      (time2.wSecond - time.wSecond) * 1000ULL + time2.wMilliseconds - time.wMilliseconds;
  fprintf(stdout, "Move took: %llums\n", duration);
  // if (duration > 1000) {
  //   for (int i = 0; i < maxDepth; i++) {
  //     fprintf(stdout, "D%i: %i\n", i, depthSize[i]);
  //   }
  // }

  if (rmd != maxDepth) {
    fputs("rmv\n", stdout);
    recalculate_move_values(&head, 0);
    fputs("Prmv\n", stdout);
  }

  // create the python dict to return
  PyObject *output = PyDict_New();
  // create the python list of moves
  PyObject *moves = PyList_New(0);

  // find the best move from all the possible moves
  int8_t *bestMoves = malloc(sizeof(int8_t) * head.lenStates);
  int8_t idx = 0;
  // the value of the best move
  int16_t best = INT16_MIN;

  // iterate over all possible moves
  for (int8_t i = 0; i < head.lenStates; ++i) {
    const int16_t realVal = head.nextStates[i].value - head.nextStates[i].worstBranch;
    // if the move is better than the current best, reset the list of best moves
    if (realVal > best) {
      best = realVal;
      idx = 0;
      bestMoves[idx++] = i;
    } else if (realVal == best) {
      // otherwise append the move to the list of best moves
      bestMoves[idx++] = i;
    }
  }

  // check if there are any moves
  if (idx > 0) {
    // if there are multiple best moves, pick one at random
    const int8_t best_move = bestMoves[rand() % idx];

    // unpack the move value
    const BoardState next_state = head.nextStates[best_move];
    const uint8_t x = next_state.x;
    const uint8_t y = next_state.y;

    // add the move to the python list of moves
    PyObject *pyX = PyLong_FromLong(x);
    PyObject *pyY = PyLong_FromLong(y);
    PyObject *pyTup = PyTuple_Pack(2, pyX, pyY);
    PyList_Append(moves, pyTup);

    // decrement the reference count of the python objects
    Py_DECREF(pyX);
    Py_DECREF(pyY);
    Py_DECREF(pyTup);

    assert(!(head.player & head.opponent));
    assert(!(next_state.player & next_state.opponent));
    printf("before (%i, %i) - Possbile moves %i/%i (max %i)\n",
           x,
           y,
           idx,
           head.lenStates,
           head.worstBranch);
    print_board(head.player, head.opponent);
    puts("after");
    print_board(next_state.opponent, next_state.player);

    node_free(&head, best_move);

    // set the new board state
    head = next_state;
  }

  // free the list of best moves
  free(bestMoves);

  // set the python dict values
  PyDict_SetItemString(output, "moves", moves);
  Py_DECREF(moves);
  puts("End");
  return output;
}

static PyObject *revai_reset(PyObject *self, PyObject *args) {
  placedTiles = 0;
  node_free(&head, -1);
  head.nextStates = NULL;
  head.value = 0;
  head.player = 0;
  head.opponent = 0;
  head.worstBranch = 0;
  head.x = 0;
  head.y = 0;
  head.lenStates = -1;

  Py_RETURN_NONE;
}

static PyMethodDef RevaiMethods[] = {
    {"ai_moves", revai_ai, METH_VARARGS, "AI."},
    {"reset", revai_reset, METH_NOARGS, "Reset."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef revaimodule = {PyModuleDef_HEAD_INIT, "revai", NULL, -1, RevaiMethods};

PyMODINIT_FUNC PyInit_revai(void) {
  srand(time(NULL));
  return PyModule_Create(&revaimodule);
}
