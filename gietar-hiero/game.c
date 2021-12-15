#include <stdbool.h>

#include "keyboard.h"
#include "lcd.h"

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
  state.notes[COL][1 << lowest_free].pos_y = NOTE_INIT_Y;
  state.note_buf_state[COL]++;
}

typedef void (*NoteHandler)(int col, int i);

void noteHelper(NoteHandler handler) {
  for (int col = 0; col < N_COLS; ++col) {
    for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
      handler(col + 1, i);
    }
  }
}

void moveNotes() {
  void lambda(int col, int i) {  
    if (state.note_buf_state[COL] & (1 << i)){
      int y = state.notes[COL][i].pos_y;
      if (y > LCD_PIXEL_HEIGHT + 1) {
        LCDremoveNote(col, y);
        // TODO: remove points
      } else {
        LCDmoveNoteVertical(col, y, false);
        state.notes[COL][i].pos_y++;
      }
    }
  } // lambda
  noteHelper(lambda);
}

void deleteNote(int col, int i) {
  LCDremoveNote(col, state.notes[COL][i].pos_y);
  state.note_buf_state[COL] &= ~(1 << i);
}

// call without side effects, and the optimizing compiler
// will take care of the rest
#define IABS(x) ((x) < 0 ? -(x) : (x))

void handleFretPress(int col) {
  LCDpressFret(col);
  for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
    if (state.note_buf_state[COL] & (1 << i) &&
      IABS(state.notes[COL][i].pos_y - FRET_PRESS_Y) < 5) {
        deleteNote(col, i);
        // TODO: add points
    }
  }  
}

void handleFretRelease(int col) {
  // todo: provide overlapping notes
  LCDreleaseFret(col);
}