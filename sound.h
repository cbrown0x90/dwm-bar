#ifndef SOUND_H
#define SOUND_H
#include <alsa/asoundlib.h>

typedef struct sound {
    long volume;
    int status;
    char percent[4];
} sound;

void soundInit(snd_mixer_selem_id_t**);
void soundDestroy(snd_mixer_selem_id_t*);
sound* getMasterStatus(sound*, snd_mixer_t*, snd_mixer_selem_id_t*, snd_mixer_elem_t*);
char* volIcon(sound*);

#endif
