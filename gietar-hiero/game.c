#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lib/include/keyboard.h"
#include "lib/include/lcd.h"
#include "lib/include/dma_uart.h"
#include "game.h"

static void helperPrintInt64(char* start, int64_t to_print, int space) {
  bool minus = false;
  if (to_print < 0) {
    minus = true;
    to_print = -to_print;
  }
  char* next_space = start + space - 1;
  if (!to_print) {
    *next_space = '0';
  }
  for (; to_print && next_space >= start; next_space--) {
    *next_space = '0' + to_print % 10;
    to_print /= 10;
  }
  if (next_space >= start && minus) {
    *next_space = '-';
  }
} 

// this module internally uses column numbers [0..3] for space efficiency
// both LCD and client module use [1..4] and this is the macro to convert 
// to the internal format when indexing GameState arrays
#define COL (col - 1)

#define MAX_NOTES_IN_COL 16
#define NOTE_INIT_Y (-30)

// concrete note that is already spawned
typedef struct {
  int pos_y;
} Note;

typedef uint64_t tick_t;

struct GameState {
  Note notes[N_COLS][MAX_NOTES_IN_COL];
  unsigned int note_buf_state[N_COLS];
  uint64_t score;
  int spawned;
  tick_t ticks;
} state;

#define GET_LOWEST_FREE(_col) ({ \
    unsigned int st = state.note_buf_state[_col]; \
    st == 0 ? 0 : 32 - __builtin_clz(st); \
  })

void updateScore() {
  const int offset = sizeof("Score: ") - 1;
  int width = LCDgetTextWidth() - offset; // space for digits
  int size = width + offset;
  
  const char score[] = "Score: ";
  char buf[size];
  memcpy(buf, score, offset);
  char* digits = buf + offset;

  memset(digits, ' ', width);
  helperPrintInt64(digits, state.score, width);

  LCDgoto(0, 0);
  for (int i = 0; i < size; ++i) {
    LCDputchar(buf[i]);
  }
}

void changeScoreBy(int delta) {
  if (delta < 0 && state.score < (uint64_t)-delta) {
    state.score = 0;
  } else {
    state.score += delta;
  }
  updateScore();
}

// information on how notes should be spawned and played
typedef struct {
  int column; // the column of the note
  tick_t start_time; // the number of ticks since start
  // TODO: melodic 
} NoteInfo;

const int to_spawn = 6;

NoteInfo song[256] = {
  {.column = 1, .start_time =  30},
  {.column = 2, .start_time =  50},
  {.column = 1, .start_time =  70},
  {.column = 3, .start_time =  90},
  {.column = 1, .start_time = 110},
  {.column = 4, .start_time = 130},
};

// spawns all notes which haven't been spawned since last tick
void spawnNotesForTick(tick_t tick) {
  while (song[state.spawned].start_time <= tick && state.spawned < to_spawn) {
    int64_t noteY = song[state.spawned].start_time - tick - 100;
    char msg[128] = "Spawning note with start time ............ during tick ............ at y = ............\n";
    helperPrintInt64(msg + sizeof("Spawning note with start time ") - 1, song[state.spawned].start_time, 12);
    helperPrintInt64(msg + sizeof("Spawning note with start time ............ during tick ") - 1, tick, 12);
    helperPrintInt64(msg + sizeof("Spawning note with start time ............ during tick ............ at y = ") - 1, noteY, 12);
    
    dmaSendWithCopy(msg, 128);
    
    spawnNoteY(song[state.spawned].column, noteY);
    state.spawned++;
    
  }
}


void spawnNote(int col) {
  spawnNoteY(col, NOTE_INIT_Y);
}

void spawnNoteY(int col, int y) {
  unsigned int lowest_free = GET_LOWEST_FREE(COL);
  if (lowest_free == 32) {
    for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
      if (!(state.note_buf_state[COL] & (1 << i))) {
        lowest_free = i;
        break;
      }
    }
    if (lowest_free == 32) {
      return;
    }
  }
  state.notes[COL][lowest_free].pos_y = y;
  state.note_buf_state[COL] |= 1 << lowest_free;
  
  char msg[] = "Note __ spawned in column _\n";
  msg[sizeof("Note _") - 1] = '0' + lowest_free % 10;
  msg[sizeof("Note ") - 1] = '0' + lowest_free / 10 % 10;
  msg[sizeof("Note __ spawned in column ") - 1] = '0' + col;
  
  dmaSendWithCopy(msg, sizeof(msg));
}

typedef void (*NoteHandler)(int col, int i);

void noteHelper(NoteHandler handler) {
  for (int col = 0; col < N_COLS; ++col) {
    for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
      handler(col + 1, i);
    }
  }
}

void moveNotes(int how_many) {
  void lambda(int col, int i) {
    if (state.note_buf_state[COL] & (1 << i)) {
      int y = state.notes[COL][i].pos_y;
      if (y > LCD_PIXEL_HEIGHT + 1) {
        deleteNote(col, i);
        changeScoreBy(-100);
      } else {
        LCDmoveNoteVertical(col, y, how_many);
        state.notes[COL][i].pos_y += how_many;
      }
    }
  } // lambda
  noteHelper(lambda);
}

void deleteNote(int col, int i) {
  LCDremoveNote(col, state.notes[COL][i].pos_y);
  state.note_buf_state[COL] &= ~(1 << i);

  char msg[] = "Note __ deleted in column _\n";
  msg[sizeof("Note _") - 1] = '0' + i % 10;
  msg[sizeof("Note ") - 1] = '0' + i / 10 % 10;
  msg[sizeof("Note __ deleted in column ") - 1] = '0' + col;
  
  dmaSendWithCopy(msg, sizeof(msg));
}

#define IABS(x) ({ \
  int val = x; \
  val < 0 ? -val : val; \
})

static int hit_window = 23; // determined by trial and error

void handleFretPress(int col) {
  LCDpressFret(col);
  for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
    if (state.note_buf_state[COL] & (1 << i)) {
      int difference = IABS(state.notes[COL][i].pos_y - FRET_PRESS_Y);
      if (difference < hit_window) {
        DMA_DBG("Detected fret/note collision!\n");
        deleteNote(col, i);
        changeScoreBy(1000 - difference);
      }
    }
  }
}

void handleFretRelease(int col) {
  // todo hard: provide overlapping notes
  LCDreleaseFret(col);
}

void increaseHitWindow() {
  hit_window++;
  char msg[] = "Hit window increased to __\n";
  msg[sizeof("Hit window increased to _") - 1] = '0' + hit_window % 10;
  msg[sizeof("Hit window increased to ") - 1] = '0' + hit_window / 10 % 10;

  dmaSendWithCopy(msg, sizeof(msg));
}

void decreaseHitWindow() {
  hit_window--;
  char msg[] = "Hit window decreased to __\n";
  msg[sizeof("Hit window decreased to _") - 1] = '0' + hit_window % 10;
  msg[sizeof("Hit window decreased to ") - 1] = '0' + hit_window / 10 % 10;

  dmaSendWithCopy(msg, sizeof(msg));
}

void handleTicks(uint32_t how_many_ticks) {
  state.ticks += how_many_ticks;
  spawnNotesForTick(state.ticks);
  moveNotes(how_many_ticks);
}

void resetGame() {
  // let the existing notes just fall
  state.ticks = 0;
  state.spawned = 0;
  state.score = 0;
  updateScore();
}