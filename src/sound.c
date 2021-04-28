#include "sound.h"

static bool is_enabled = false;

static u8 volume_master;
static u8 volumes[NUM_NOTES];
static u8 notes[NUM_NOTES];
static u8 waves[NUM_NOTES];

bool sound_enabled() {
    return is_enabled;
}

void sound_set_enabled(bool enabled) {
    is_enabled = enabled;
}

void sound_note(u8 index, u8 octave, u8 note) {
    notes[index] = (octave << 4) | note;
}

u8 sound_get_note(u8 index) {
    return notes[index];
}

void sound_volume(u8 index, u8 v) {
    volumes[index] = v;
}

u8 sound_get_volume(u8 index) {
    return volumes[index];
}

void sound_master(u8 v) {
    volume_master = v;
}

u8 sound_get_master() {
    return volume_master;
}

void sound_wave(u8 index, u8 wave) {
    waves[index] = wave;
}

u8 sound_get_wave(u8 index) {
    return waves[index];
}

void sound_init() {
    extern void sound_init_device();
    sound_init_device();

    memset(&notes, NOTE_NONE, sizeof(notes));
    memset(&waves, WAVE_SIN, sizeof(waves));
}

void sound_tick() {
    extern void sound_tick_device();
    sound_tick_device();
}
