#ifndef SPEAKER_H
#define SPEAKER_H

void initSpeakerTimer();
void toggleSpeaker();

void speakerOn();
void speakerOff();

void changeWaveLen(int by);

typedef struct {
  char letter; // 1-12, where C=1 and B=12
  char octave; // 0-8
} Note;

void setNote(Note note);


#endif // SPEAKER_H