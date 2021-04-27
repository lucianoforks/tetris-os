#ifndef SPEAKER_H
#define SPEAKER_H

#include "util.h"

void speaker_note(u8 octave, u8 note);
void speaker_play(u16 d);
void speaker_pause();

#endif
