#include "music.h"
#include "timer.h"
#include "sound.h"
#include "math.h"

struct Note {
    u8 octave;
    u8 note;
    u16 duration;
};

struct NoteActive {
    struct Note note;
    double ticks;
};

#define TRACK_BPM 150
#define TRACK_BPS (TRACK_BPM / 60.0)
#define TICKS_PER_BEAT (TIMER_TPS / TRACK_BPS)
#define TICKS_PER_SIXTEENTH (TICKS_PER_BEAT / 16.0)

#define CHORUS_MELODY_LENGTH (sizeof(CHORUS_MELODY) / sizeof(CHORUS_MELODY[0]))
static const struct Note CHORUS_MELODY[] = {
    // CHORUS MELODY
    { OCTAVE_5, NOTE_E,     16 },
    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_5, NOTE_D,     16 },

    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_4, NOTE_A,     16 },

    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_5, NOTE_E,     16 },

    { OCTAVE_5, NOTE_D,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_4, NOTE_B,     16 },

    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_5, NOTE_D,     16 },
    { OCTAVE_5, NOTE_E,     16 },
    { OCTAVE_5, NOTE_C,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_NONE,  24 },

    { OCTAVE_5, NOTE_D,     16 },
    { OCTAVE_5, NOTE_F,      8 },
    { OCTAVE_5, NOTE_A,      8 },
    { OCTAVE_5, NOTE_A,      4 },
    { OCTAVE_5, NOTE_A,      4 },
    { OCTAVE_5, NOTE_G,      8 },
    { OCTAVE_5, NOTE_F,      8 },
    { OCTAVE_5, NOTE_E,     16 },
    { OCTAVE_5, NOTE_NONE,   8 },

    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_5, NOTE_E,      8 },
    { OCTAVE_5, NOTE_E,      4 },
    { OCTAVE_5, NOTE_E,      4 },
    { OCTAVE_5, NOTE_D,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_4, NOTE_B,     16 },

    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_5, NOTE_C,      8 },
    { OCTAVE_5, NOTE_D,     16 },
    { OCTAVE_5, NOTE_E,     16 },
    { OCTAVE_5, NOTE_C,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_NONE,  16 },
};

#define BRIDGE_MELODY_LENGTH (sizeof(BRIDGE_MELODY) / sizeof(BRIDGE_MELODY[0]))
static const struct Note BRIDGE_MELODY[] = {
    // BRIDGE
    { OCTAVE_4, NOTE_E,     32 },
    { OCTAVE_4, NOTE_C,     32 },
    { OCTAVE_4, NOTE_D,     32 },
    { OCTAVE_3, NOTE_B,     32 },
    { OCTAVE_4, NOTE_C,     32 },
    { OCTAVE_3, NOTE_A,     32 },
    { OCTAVE_3, NOTE_AF,    48 },
    { OCTAVE_3, NOTE_NONE,  16 },

    { OCTAVE_4, NOTE_E,     32 },
    { OCTAVE_4, NOTE_C,     32 },
    { OCTAVE_4, NOTE_D,     32 },
    { OCTAVE_3, NOTE_B,     32 },
    { OCTAVE_3, NOTE_A,     16 },
    { OCTAVE_4, NOTE_E,     16 },
    { OCTAVE_4, NOTE_A,     32 },
    { OCTAVE_4, NOTE_AF,    48 },
    { OCTAVE_4, NOTE_NONE,  16 },
};

#define BASS_NOTE(_octave, _note)\
    { _octave,          _note,  8 },\
    { (_octave + 1),    _note,  8 }

#define BASS_HALF_MEASURE(_octave, _note)\
    BASS_NOTE(_octave, _note),\
    BASS_NOTE(_octave, _note)

#define BASS_MEASURE(_octave, _note)\
    BASS_HALF_MEASURE(_octave, _note),\
    BASS_HALF_MEASURE(_octave, _note)

#define CHORUS_BASS_LENGTH (sizeof(CHORUS_BASS) / sizeof(CHORUS_BASS[0]))
static const struct Note CHORUS_BASS[] = {
    // CHORUS BASS
    BASS_MEASURE(OCTAVE_2, NOTE_E),
    BASS_MEASURE(OCTAVE_2, NOTE_A),
    BASS_HALF_MEASURE(OCTAVE_2, NOTE_AF),
    BASS_HALF_MEASURE(OCTAVE_2, NOTE_E),
    BASS_HALF_MEASURE(OCTAVE_2, NOTE_A),
    { OCTAVE_2, NOTE_A,  8 },
    { OCTAVE_2, NOTE_A,  8 },
    { OCTAVE_2, NOTE_B,  8 },
    { OCTAVE_3, NOTE_C,  8 },

    BASS_MEASURE(OCTAVE_2, NOTE_D),
    BASS_MEASURE(OCTAVE_2, NOTE_C),
    BASS_HALF_MEASURE(OCTAVE_2, NOTE_AF),
    BASS_HALF_MEASURE(OCTAVE_2, NOTE_E),
    BASS_MEASURE(OCTAVE_2, NOTE_A),
};

#define BRIDGE_BASS_NOTE(_octave_0, _note_0, _octave_1, _note_1)\
    { _octave_0, _note_0, 8 },\
    { _octave_1, _note_1, 8 }

#define BRIDGE_BASS_HALF_MEASURE(_octave_0, _note_0, _octave_1, _note_1)\
    BRIDGE_BASS_NOTE(_octave_0, _note_0, _octave_1, _note_1),\
    BRIDGE_BASS_NOTE(_octave_0, _note_0, _octave_1, _note_1)

#define BRIDGE_BASS_MEASURE(_octave_0, _note_0, _octave_1, _note_1)\
    BRIDGE_BASS_HALF_MEASURE(_octave_0, _note_0, _octave_1, _note_1),\
    BRIDGE_BASS_HALF_MEASURE(_octave_0, _note_0, _octave_1, _note_1)

#define BRIDGE_BASS_LENGTH (sizeof(BRIDGE_BASS) / sizeof(BRIDGE_BASS[0]))
static const struct Note BRIDGE_BASS[] = {
    BRIDGE_BASS_MEASURE(OCTAVE_2, NOTE_A, OCTAVE_3, NOTE_E),
    BRIDGE_BASS_MEASURE(OCTAVE_2, NOTE_AF, OCTAVE_3, NOTE_E),

    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_A, OCTAVE_3, NOTE_E),
    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_A),

    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_AF),
    BRIDGE_BASS_NOTE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_AF),
    { OCTAVE_2, NOTE_E,      8 },
    { OCTAVE_2, NOTE_NONE,   8 },

    BRIDGE_BASS_MEASURE(OCTAVE_2, NOTE_A, OCTAVE_3, NOTE_E),
    BRIDGE_BASS_MEASURE(OCTAVE_2, NOTE_AF, OCTAVE_3, NOTE_E),

    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_A, OCTAVE_3, NOTE_E),
    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_A),

    BRIDGE_BASS_HALF_MEASURE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_AF),
    BRIDGE_BASS_NOTE(OCTAVE_2, NOTE_E, OCTAVE_2, NOTE_AF),
    { OCTAVE_2, NOTE_E,      8 },
    { OCTAVE_2, NOTE_NONE,   8 }
};

#define CHORUS_HARMONY_LENGTH (sizeof(CHORUS_HARMONY) / sizeof(CHORUS_HARMONY[0]))
static const struct Note CHORUS_HARMONY[] = {
    // CHORUS HARMONY
    { OCTAVE_5, NOTE_NONE,  16 },
    { OCTAVE_4, NOTE_AF,     8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_B,     16 },

    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_AF,     8 },
    { OCTAVE_4, NOTE_E,     16 },

    { OCTAVE_4, NOTE_E,      8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_A,     16 },

    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_AF,    16 },

    { OCTAVE_4, NOTE_AF,     8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_B,     16 },
    { OCTAVE_5, NOTE_C,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_E,     16 },
    { OCTAVE_4, NOTE_E,     16 },
    { OCTAVE_4, NOTE_NONE,  24 },

    { OCTAVE_4, NOTE_F,     16 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_5, NOTE_C,     16 },
    { OCTAVE_4, NOTE_B,      8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_G,     16 },
    { OCTAVE_4, NOTE_NONE,   8 },

    { OCTAVE_4, NOTE_E,      8 },
    { OCTAVE_4, NOTE_G,     16 },
    { OCTAVE_4, NOTE_F,      8 },
    { OCTAVE_4, NOTE_E,      8 },
    { OCTAVE_4, NOTE_AF,    16 },

    { OCTAVE_4, NOTE_AF,     8 },
    { OCTAVE_4, NOTE_A,      8 },
    { OCTAVE_4, NOTE_B,     16 },
    { OCTAVE_5, NOTE_C,     16 },
    { OCTAVE_4, NOTE_A,     16 },
    { OCTAVE_4, NOTE_E,     16 },
    { OCTAVE_4, NOTE_E,     16 },
    { OCTAVE_4, NOTE_NONE,  16 },
};

#define BRIDGE_HARMONY_LENGTH (sizeof(BRIDGE_HARMONY) / sizeof(BRIDGE_HARMONY[0]))
static const struct Note BRIDGE_HARMONY[] = {
    { OCTAVE_4, NOTE_C,     32 },
    { OCTAVE_3, NOTE_A,     32 },
    { OCTAVE_3, NOTE_B,     32 },
    { OCTAVE_3, NOTE_AF,    32 },
    { OCTAVE_3, NOTE_A,     32 },
    { OCTAVE_3, NOTE_E,     32 },
    { OCTAVE_3, NOTE_E,     48 },
    { OCTAVE_3, NOTE_NONE,  16 },

    { OCTAVE_4, NOTE_C,     32 },
    { OCTAVE_3, NOTE_A,     32 },
    { OCTAVE_3, NOTE_B,     32 },
    { OCTAVE_3, NOTE_AF,    32 },
    { OCTAVE_3, NOTE_E,     16 },
    { OCTAVE_3, NOTE_A,     16 },
    { OCTAVE_4, NOTE_E,     32 },
    { OCTAVE_4, NOTE_E,     48 },
    { OCTAVE_4, NOTE_NONE,  16 },
};

#define SNARE_OCTAVE OCTAVE_4
#define SNARE_NOTE NOTE_E
#define SNARE_AND\
    { SNARE_OCTAVE, NOTE_NONE,  8},\
    { SNARE_OCTAVE, SNARE_NOTE, 2},\
    { SNARE_OCTAVE, NOTE_NONE,  6}
#define SNARE_EIGTH\
    { SNARE_OCTAVE, SNARE_NOTE, 2},\
    { SNARE_OCTAVE, NOTE_NONE,  6}

#define SNARE_MEASURE\
    SNARE_AND,\
    SNARE_AND,\
    SNARE_AND,\
    SNARE_AND

#define CHORUS_SNARE_LENGTH (sizeof(CHORUS_SNARE) / sizeof(CHORUS_SNARE[0]))
static const struct Note CHORUS_SNARE[] = {
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_AND,
    SNARE_AND,
    SNARE_AND,
    SNARE_EIGTH,
    SNARE_EIGTH,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_AND,
    SNARE_AND,
    SNARE_AND,
    SNARE_EIGTH,
    SNARE_EIGTH,
};

#define BRIDGE_SNARE_LENGTH (sizeof(BRIDGE_SNARE) / sizeof(BRIDGE_SNARE[0]))
static const struct Note BRIDGE_SNARE[] = {
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_MEASURE,
    SNARE_AND,
    SNARE_AND,
    SNARE_AND,
    SNARE_EIGTH,
    SNARE_EIGTH,
};

#define TRACK_MAX_LENGTH (4 * (CHORUS_MELODY_LENGTH + BRIDGE_MELODY_LENGTH))
#define TRACK_PARTS 4
static struct Note TRACK[TRACK_PARTS][TRACK_MAX_LENGTH];
static size_t PART_LENGTHS[TRACK_PARTS];

static i32 indices[TRACK_PARTS];
static struct NoteActive current[NUM_NOTES];

void music_tick() {
    for (size_t i = 0; i < TRACK_PARTS; i++) {
        if (indices[i] == -1 || (current[i].ticks -= 1) <= 0) {
            indices[i] = (indices[i] + 1) % PART_LENGTHS[i];

            double remainder = fabs(current[i].ticks);

            struct Note note = TRACK[i][indices[i]];
            current[i].note = note;
            current[i].ticks = TICKS_PER_SIXTEENTH * note.duration - remainder;

            sound_note(i, note.octave, note.note);
        }

        // remove last tick to give each note an attack
        if (current[i].ticks == 1) {
            sound_note(i, OCTAVE_1, NOTE_NONE);
        }
    }
}

void music_init() {
    sound_wave(0, WAVE_TRIANGLE);
    sound_volume(0, 255);

    sound_wave(1, WAVE_NOISE);
    sound_volume(1, 0);

    sound_wave(2, WAVE_TRIANGLE);
    sound_volume(2, 0);

    sound_wave(3, WAVE_TRIANGLE);
    sound_volume(3, 0);

    memcpy(&TRACK[0][0], CHORUS_MELODY, sizeof(CHORUS_MELODY));
    memcpy(&TRACK[0][CHORUS_MELODY_LENGTH], BRIDGE_MELODY, sizeof(BRIDGE_MELODY));
    PART_LENGTHS[0] = CHORUS_MELODY_LENGTH + BRIDGE_MELODY_LENGTH;

    memcpy(&TRACK[1][0], CHORUS_SNARE, sizeof(CHORUS_SNARE));
    memcpy(&TRACK[1][CHORUS_SNARE_LENGTH], BRIDGE_SNARE, sizeof(BRIDGE_SNARE));
    PART_LENGTHS[1] = CHORUS_SNARE_LENGTH + BRIDGE_SNARE_LENGTH;

    memcpy(&TRACK[2][0], CHORUS_BASS, sizeof(CHORUS_BASS));
    memcpy(&TRACK[2][CHORUS_BASS_LENGTH], BRIDGE_BASS, sizeof(BRIDGE_BASS));
    PART_LENGTHS[2] = CHORUS_BASS_LENGTH + BRIDGE_BASS_LENGTH;

    memcpy(&TRACK[3][0], CHORUS_HARMONY, sizeof(CHORUS_HARMONY));
    memcpy(&TRACK[3][CHORUS_HARMONY_LENGTH], BRIDGE_HARMONY, sizeof(BRIDGE_HARMONY));
    PART_LENGTHS[3] = CHORUS_HARMONY_LENGTH + BRIDGE_HARMONY_LENGTH;

    for (size_t i = 0; i < TRACK_PARTS; i++) {
        indices[i] = -1;
    }
}
