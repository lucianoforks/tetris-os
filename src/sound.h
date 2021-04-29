#ifndef SOUND_H
#define SOUND_H

#include "util.h"

// SOUND BACKENDS, enable one
#define SOUND_SB16
// #define SOUND_PCSPK

#define NUM_NOTES 8

#define NUM_OCTAVES 7
#define OCTAVE_SIZE 12

#define OCTAVE_1 0
#define OCTAVE_2 1
#define OCTAVE_3 2
#define OCTAVE_4 3
#define OCTAVE_5 4
#define OCTAVE_6 5
#define OCTAVE_7 6

#define NOTE_C      0
#define NOTE_CS     1
#define NOTE_DF     NOTE_CS
#define NOTE_D      2
#define NOTE_DS     3
#define NOTE_EF     NOTE_DS
#define NOTE_E      4
#define NOTE_F      5
#define NOTE_FS     6
#define NOTE_GF     NOTE_FS
#define NOTE_G      7
#define NOTE_GS     8
#define NOTE_AF     NOTE_GS
#define NOTE_A      9
#define NOTE_AS     10
#define NOTE_BF     NOTE_AS
#define NOTE_B      11

#define NOTE_NONE   12

#define WAVE_SIN        0
#define WAVE_SQUARE     1
#define WAVE_NOISE      2
#define WAVE_TRIANGLE   3

bool sound_enabled();
void sound_set_enabled(bool enabled);
void sound_init();
void sound_tick();
void sound_note(u8 index, u8 octave, u8 note);
u8 sound_get_note(u8 index);
void sound_master(u8 v);
u8 sound_get_master();
void sound_volume(u8 index, u8 v);
u8 sound_get_volume(u8 index);
void sound_wave(u8 index, u8 wave);
u8 sound_get_wave(u8 index);

#endif
