#include <stdbool.h>

#include "lib/include/keyboard.h"
#include "lib/include/lcd.h"
#include "lib/include/dma_uart.h"
#include "game.h"

// this module internally uses column numbers [0..3] for space efficiency
// both LCD and client module use [1..4] and this is the macro to convert 
// to the internal format when indexing GameState arrays
#define COL (col - 1)

#define MAX_NOTES_IN_COL 16
#define NOTE_INIT_Y (-30)
typedef struct {
  int pos_y;

} Note;


struct GameState {
  Note notes[N_COLS][MAX_NOTES_IN_COL];
  unsigned int note_buf_state[N_COLS];
} state;

#define GET_LOWEST_FREE(col) (__builtin_clz(~state.note_buf_state[col]))

void spawnNote(int col) {
  unsigned int lowest_free = GET_LOWEST_FREE(COL);
  state.notes[COL][lowest_free].pos_y = NOTE_INIT_Y;
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
        // TODO: remove points
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

static int hit_window = 9; // determined by trial and error

void handleFretPress(int col) {
  LCDpressFret(col);
  for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
    if (state.note_buf_state[COL] & (1 << i) &&
        IABS(state.notes[COL][i].pos_y - FRET_PRESS_Y) < hit_window) {
      DMA_DBG("Detected fret/note collision!");
      deleteNote(col, i);
      // TODO: add points
    }
  }
}

void handleFretRelease(int col) {
  // todo: provide overlapping notes
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