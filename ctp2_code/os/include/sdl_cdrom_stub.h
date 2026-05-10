// SDL CD-ROM stubs for SDL2 compatibility
// SDL2 removed CD-ROM support. These stubs allow compilation without CD audio.

#ifndef SDL_CDROM_STUB_H
#define SDL_CDROM_STUB_H

#ifdef USE_SDL

#include <stdint.h>

// Stub types
typedef struct SDL_CDTrack {
    uint8_t id;
    uint8_t type;
    uint32_t unused;
    uint32_t length;
    uint32_t offset;
} SDL_CDTrack;

typedef struct SDL_CD {
    int dummy;
    int numtracks;
    SDL_CDTrack track[99];
} SDL_CD;

typedef enum { CD_TRAYEMPTY, CD_STOPPED, CD_PLAYING, CD_PAUSED, CD_ERROR } CDstatus;

#define CD_INDRIVE(status) ((status) > 0)
#define CD_FPS 75
#define SDL_MAX_TRACKS 99

// Stub functions
static inline int SDL_CDNumDrives(void) { return 0; }
static inline const char* SDL_CDName(int drive) { return NULL; }
static inline SDL_CD* SDL_CDOpen(int drive) { return NULL; }
static inline CDstatus SDL_CDStatus(SDL_CD *cdrom) { return CD_TRAYEMPTY; }
static inline void SDL_CDClose(SDL_CD *cdrom) {}
static inline int SDL_CDPlayTracks(SDL_CD *cdrom, int start_track, int start_frame, int ntracks, int nframes) { return -1; }
static inline int SDL_CDStop(SDL_CD *cdrom) { return -1; }

#endif // USE_SDL

#endif // SDL_CDROM_STUB_H
