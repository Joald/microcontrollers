#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lib/include/keyboard.h"
#include "lib/include/lcd.h"
#include "lib/include/dma_uart.h"
#include "game.h"
#include "speaker.h"

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

#define MAX_NOTES_IN_COL 32
#define NOTE_INIT_Y (-30)

typedef uint64_t tick_t;

// information on how notes should be spawned and played
typedef struct {
  const int column; // the column of the note
  const tick_t start_time; // the number of ticks since start
  const Note note;
  const int duration;
} NoteInfo;

void spawnNoteY(const NoteInfo* info, int y);

// concrete note that is already spawned
typedef struct {
  int pos_y;
  const NoteInfo* info;  
} SpawnedNote;


struct GameState {
  SpawnedNote notes[N_COLS][MAX_NOTES_IN_COL];
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


const int to_spawn = 64;

// must be ordered by start_time
const NoteInfo song[256] = {
#include "song.txt"
  // {.column = 1, .start_time = 10, .note = {.letter = 2, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 40, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 70, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 100, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 130, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 160, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 190, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 220, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 1, .start_time = 250, .note = {.letter = 2, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 280, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 310, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 340, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 370, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 400, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 430, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 460, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 490, .note = {.letter = 4, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 520, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 550, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 580, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 610, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 640, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 670, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 700, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 730, .note = {.letter = 4, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 760, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 790, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 820, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 850, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 880, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 910, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 940, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 970, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1000, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 1030, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 1060, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1090, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1120, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 1150, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1180, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 1210, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1240, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 1270, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 1300, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1330, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1360, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 1390, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1420, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 1, .start_time = 1450, .note = {.letter = 2, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1480, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 1510, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 1540, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1570, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1600, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 1630, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1660, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 1, .start_time = 1690, .note = {.letter = 2, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1720, .note = {.letter = 2, .octave = 4}, .duration = 29},
  // {.column = 3, .start_time = 1750, .note = {.letter = 9, .octave = 3}, .duration = 29},
  // {.column = 2, .start_time = 1780, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 4, .start_time = 1810, .note = {.letter = 7, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1840, .note = {.letter = 7, .octave = 3}, .duration = 29},
  // {.column = 3, .start_time = 1870, .note = {.letter = 6, .octave = 4}, .duration = 29},
  // {.column = 2, .start_time = 1900, .note = {.letter = 7, .octave = 3}, .duration = 29},
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
    
    spawnNoteY(&song[state.spawned], noteY);
    state.spawned++;
    
  }
}

void spawnNoteY(const NoteInfo* info, int y) {
  int col = info->column;
  unsigned int lowest_free = GET_LOWEST_FREE(COL);
  if (lowest_free == 32) {
    for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
      if (!(state.note_buf_state[COL] & (1 << i))) {
        lowest_free = i;
        break;
      }
    }
    if (lowest_free == 32) {
      DMA_DBG("NOTE NOT SPAWNED!!!\n");
      return;
    }
  }
  state.notes[COL][lowest_free].pos_y = y;
  state.notes[COL][lowest_free].info = info;
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

const int hit_window = 23; // determined by trial and error

tick_t when_speaker_off = 0;

void handleFretPress(int col) {
  LCDpressFret(col);
  for (int i = 0; i < MAX_NOTES_IN_COL; ++i) {
    if (state.note_buf_state[COL] & (1 << i)) {
      int y = state.notes[COL][i].pos_y;
      int difference = IABS(y - FRET_PRESS_Y);
      if (difference < hit_window) {
        DMA_DBG("Detected fret/note collision!\n");
        deleteNote(col, i);
        changeScoreBy(1000 - difference);
        const NoteInfo* info = state.notes[COL][i].info;
        setNote(info->note);
        speakerOn();
        when_speaker_off = state.ticks + info->duration + FRET_PRESS_Y - y;
      }
    }
  }
}

void handleFretRelease(int col) {
  LCDreleaseFret(col);
}


void handleTicks(uint32_t how_many_ticks) {
  state.ticks += how_many_ticks;
  spawnNotesForTick(state.ticks);
  moveNotes(how_many_ticks);

  if (state.ticks > when_speaker_off) {
    speakerOff();
  }
}

void resetGame() {
  state.ticks = 0;
  state.spawned = 0;
  state.score = 0;
  updateScore();
  speakerOff();

  void deleterLambda(int col, int i) {
    if (state.note_buf_state[COL] & (1 << i)) {
      deleteNote(col, i);
    }
  }

  noteHelper(deleterLambda);
}