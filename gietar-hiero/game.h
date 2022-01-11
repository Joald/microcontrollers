#ifndef GAME_H
#define GAME_H

void spawnNote(int col);
void spawnNoteY(int col, int y);
void deleteNote(int col, int i);
void moveNotes(int how_many);
void handleFretPress(int col);
void handleFretRelease(int col);

void increaseHitWindow();
void decreaseHitWindow();

#endif  // GAME_H