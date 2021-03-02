#include <time.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "sound.h"

#define KILO 1024
#define MEGA 1048576
#define GIGA 1073741824

#define BARLEN 256

Display* dpy;

void error(char* msg, int abort)
{
    FILE* log = fopen("/home/chris/.local/log/bar", "a");
    if (log == NULL)
        exit(errno);

    fprintf(log, "%s: %s\n", msg, strerror(errno));
    fclose(log);
    if (abort)
    {
        XStoreName(dpy, DefaultRootWindow(dpy), "ABORTED");
        exit(errno);
    }
}

int getDiskSize(char* mount, char* buf, int remaining)
{
    struct statvfs root;
    if (statvfs(mount, &root) < 0)
        error("Failed to get disk size", 0);
    unsigned long space = root.f_bfree * root.f_bsize;

    char unit;
    if (space < KILO)
    {
        unit = 'B';
    }
    else if (space < MEGA)
    {
        space /= KILO;
        unit = 'K';
    }
    else if (space < GIGA)
    {
        space /= MEGA;
        unit = 'M';
    }
    else
    {
        space /= GIGA;
        unit = 'G';
    }

    //return snprintf(buf, remaining, "%s: %ld%c", mount, space, unit);
    return snprintf(buf, remaining, ": %ld%c", space, unit);
}

int getDisk(char* bar, int remaining)
{
    int offset = snprintf(bar, remaining, "%s", " ");
    return offset + getDiskSize("/", bar + offset, BARLEN - offset);
    //offset += snprintf(bar + offset, BARLEN - offset, "%s", ", ");
    //return offset + getDiskSize("/home", bar + offset, BARLEN - offset);
}

int getIP(char* buf, int remaining)
{
    struct sockaddr_in* ip4 = NULL;
    struct sockaddr_in6* ip6 = NULL;
    struct ifaddrs* ip;
    if (getifaddrs(&ip) < 0)
        error("Failed to get interface information", 0);

    for (struct ifaddrs* tmp = ip; tmp != NULL; tmp = tmp->ifa_next)
    {
        if (strcmp(tmp->ifa_name, "enp24s0") == 0 && tmp->ifa_addr->sa_family == AF_INET)
        {
            ip4 = (struct sockaddr_in*) tmp->ifa_addr;
        }
        if (strcmp(tmp->ifa_name, "enp24s0") == 0 && tmp->ifa_addr->sa_family == AF_INET6)
        {
            ip6 = (struct sockaddr_in6*) tmp->ifa_addr;
        }
    }

    //" %s"
    if (ip6)
    {
        inet_ntop(AF_INET6, &ip6->sin6_addr, buf, remaining);
    }
    else if (ip4)
    {
        inet_ntop(AF_INET, &ip4->sin_addr, buf, remaining);
    }
    else
    {
        snprintf(buf, remaining, "No IP");
    }

    freeifaddrs(ip);
    return strnlen(buf, remaining);
}

int getSleep(char* bar, int remaining)
{
    int timeout, interval, prefer_blank, allow_exp;
    XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank, &allow_exp);
    if (timeout)
    {
        return snprintf(bar, remaining, "");
    }
    else
    {
        return snprintf(bar, remaining, "");
    }
}

int getVolume(char* bar, int remaining, snd_mixer_selem_id_t* sid)
{
    // Setup handle
    snd_mixer_t* handle;
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    long vol, min, max;
    int on;

    // setup mixer and get values
    snd_mixer_elem_t* mixer = snd_mixer_find_selem(handle, sid);
    snd_mixer_selem_get_playback_switch(mixer, SND_MIXER_SCHN_MONO, &on);
    snd_mixer_selem_get_playback_volume(mixer, SND_MIXER_SCHN_MONO, &vol);
    snd_mixer_selem_get_playback_volume_range(mixer, &min, &max);

    // Calculate the current volume percentage
    long vol_tmp = (vol * 1000 / max) % 10;
    vol = vol * 100 / max;
    vol = vol_tmp >= 5 ? vol + 1 : vol;

    // Decide what to write to bar
    int written;
    if (on)
    {
        if (vol == 0)
        {
            written = snprintf(bar, remaining, " %ld%%", vol);
        }
        else if (vol < 50)
        {
            written = snprintf(bar, remaining, " %ld%%", vol);
        }
        else
        {
            written = snprintf(bar, remaining, " %ld%%", vol);
        }
    }
    else
    {
        written = snprintf(bar, remaining, " Muted");
    }

    snd_mixer_close(handle);

    return written;
}

//Get time
size_t getTime(char* timebuf, int remaining)
{
    time_t t = time(NULL);
    return strftime(timebuf, remaining, "%F %T", localtime(&t));
}

void newdifftimespec(struct timespec *res, struct timespec *a, struct timespec *b)
{
    res->tv_sec = 0;
    res->tv_nsec = 1E9 - (a->tv_nsec - b->tv_nsec);
}

int separator(char* bar, int remaining)
{
    return snprintf(bar, remaining, " | ");
}

int main()
{
    char* bar = malloc(BARLEN);

    struct timespec start;
    struct timespec end;
    struct timespec wait;

    snd_mixer_selem_id_t* sid;
    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");

    if (!(dpy = XOpenDisplay(NULL)))
        error("XOpenDisplay failed", 1);

    while (1)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
            error("Failed to get starting time", 0);

        bar[0] = ' ';
        size_t offset = 1;

        offset += getDisk(bar + offset, BARLEN - offset);

        offset += separator(bar + offset, BARLEN - offset);

        offset += getIP(bar + offset, BARLEN - offset);

        offset += separator(bar + offset, BARLEN - offset);

        offset += getSleep(bar + offset, BARLEN - offset);

        offset += separator(bar + offset, BARLEN - offset);

        offset += getVolume(bar + offset, BARLEN - offset, sid);

        offset += separator(bar + offset, BARLEN - offset);

        offset += getTime(bar + offset, BARLEN - offset);

        if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
            error("Failed to get ending time", 0);

        newdifftimespec(&wait, &end, &start);

        puts(bar);
        XStoreName(dpy, DefaultRootWindow(dpy), bar);
        XFlush(dpy);

        if (nanosleep(&wait, NULL) < 0)
            error("Failed to sleep", 0);
    }

    XCloseDisplay(dpy);
    snd_mixer_selem_id_free(sid);
}
