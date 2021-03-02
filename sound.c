#include <math.h>
#include "sound.h"

void soundInit(snd_mixer_selem_id_t** sid) {
    snd_mixer_selem_id_malloc(sid);
    snd_mixer_selem_id_set_index(*sid, 0);
    snd_mixer_selem_id_set_name(*sid, "Master");
}

void soundDestroy(snd_mixer_selem_id_t* sid) {
    snd_mixer_selem_id_free(sid);
}

sound* getMasterStatus(sound* s, snd_mixer_t* handle, snd_mixer_selem_id_t* sid, snd_mixer_elem_t* elem) {
    long volume_tmp;
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &(s->status));
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &volume_tmp);

    s->volume = rint((double)volume_tmp/74 * 100);
    sprintf(s->percent, "%ld%%", s->volume);
    snd_mixer_close(handle);
    return s;
}

char* volIcon(sound* s) {
    if (s->status == 0 || s->volume == 0) {
        return " ";
    } else if (s->volume < 50) {
        return " ";
    } else {
        return " ";
    }
}
