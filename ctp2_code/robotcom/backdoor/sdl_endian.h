#ifndef _SDL_endian_h
#define _SDL_endian_h




#define SDL_LIL_ENDIAN	1234
#define SDL_BIG_ENDIAN	4321






#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SDL_BYTEORDER	SDL_LIL_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_BYTEORDER	SDL_BIG_ENDIAN
#elif defined(i386) || defined(WIN32) || defined(__alpha__) || defined(__x86_64__) || defined(__aarch64__)
#define SDL_BYTEORDER	SDL_LIL_ENDIAN
#else
#define SDL_BYTEORDER	SDL_BIG_ENDIAN
#endif

#define SDL_Swap16(X)  ((X<<8)|(X>>8))
#define SDL_Swap32(X)  ((X<<24)|((X<<8)&0x00FF0000)|((X>>8)&0x0000FF00)|(X>>24))
#define SDL_Swap64(X)  (((uint64)(X)<<56)|(((uint64)(X)<<40)&0x00FF000000000000ULL)|(((uint64)(X)<<24)&0x0000FF0000000000ULL)|(((uint64)(X)<<8)&0x000000FF00000000ULL)|(((uint64)(X)>>8)&0x00000000FF000000ULL)|(((uint64)(X)>>24)&0x0000000000FF0000ULL)|(((uint64)(X)>>40)&0x000000000000FF00ULL)|((uint64)(X)>>56))

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(X)	(X)
#define SDL_SwapLE32(X)	(X)
#define SDL_SwapLE64(X)	(X)
#define SDL_SwapBE16(X)	SDL_Swap16(X)
#define SDL_SwapBE32(X)	SDL_Swap32(X)
#define SDL_SwapBE64(X)	SDL_Swap64(X)
#else
#define SDL_SwapLE16(X)	SDL_Swap16(X)
#define SDL_SwapLE32(X)	SDL_Swap32(X)
#define SDL_SwapLE64(X)	SDL_Swap64(X)
#define SDL_SwapBE16(X)	(X)
#define SDL_SwapBE32(X)	(X)
#define SDL_SwapBE64(X)	(X)
#endif

#endif
