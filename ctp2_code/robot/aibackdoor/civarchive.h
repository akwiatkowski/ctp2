//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Civilization archive for storing and loading the information
//                to/from savegames
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _DEBUG
// - Generate debug version when set.
//
// USE_COM_REPLACEMENT
// - Use COM replacement (for Linux)
//
// HUNT_SERIALIZE
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added put and get methods for MBCHAR* (Aug 24th 2005 Martin G�hmann)
// - Removed DoubleUp method. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __CIVARCHIVE_H__
#define __CIVARCHIVE_H__

class CivArchive;

// G-4b: Ic3CivArchive base removed; CivArchive is a plain concrete
// class.  The COM-style interface (DECLARE_INTERFACE_, GUIDs, IUnknown
// base) was never used outside CivArchive itself.
#include "os/include/ctp2_inttypes.h"
// #include "SDL2/SDL_endian.h"

#define k_ARCHIVE_MAGIC_VALUE_1	'OTAK'
#define k_ARCHIVE_MAGIC_VALUE_2	'U-98'

class GameFile ;
class DataCheck ;

// G-4b/c: REFCOUNT_TYPE and INTERFACE_RESULT_TYPE macros removed —
// they were COM-only typedefs (ULONG / STDMETHODIMP_) and have no
// callers now that the IUnknown layer is gone.

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

  /*
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_SwapLE64(x) (x)
*/

class CivArchive
{
private:
		bool	m_bIsStoring ;

		uint32	m_ulAllocated,
				m_ulLength ;

		uint8	*m_pbBaseMemory,
				*m_pbInsert ;

		void DoubleExpand(uint32 ulAmount) ;

public:
		void SetSize(uint32 ulSize) ;
		void SetStore(void) { m_bIsStoring = true ; }
		void SetLoad(void) { m_bIsStoring = false ; }
		void ResetForLoad(void);
		uint8 *GetStream(void) { return (m_pbBaseMemory) ; }
		uint32 StreamLen(void) { return (m_ulLength) ; }

		friend class GameFile ;
		friend class GameMapFile ;
		friend class DataCheck ;
		friend class BuildQueue ;
		friend class NetCRC;
		friend class MapFile;

		CivArchive() ;
		CivArchive(uint32 ulSize) ;
		virtual ~CivArchive() ;

    // G-4b/c: COM scaffolding removed.  CivArchive used to implement
    // IUnknown (QueryInterface / AddRef / Release).  No external
    // callers ever invoked those.  Lifetime is now plain new/delete.
    void Load(uint8 *pbData, uint32 ulLen);
    void Store(uint8 *pbData, uint32 ulLen);

#ifndef HUNT_SERIALIZE
    void StoreChunk(uint8 *start, uint8 *end);
#endif

        void TypeCheck(const uint8 tc);

		CivArchive &operator<< (const double &val) {
			PutDOUBLE(val); return (*this);
		}
		CivArchive &operator<< (const sint8 &val) {
			PutSINT8(val); return (*this);
		}
		CivArchive &operator<< (const uint8 &val) {
			PutUINT8(val); return (*this);
		}
		CivArchive &operator<< (const sint16 &val) {
			PutSINT16(val); return (*this);
		}
		CivArchive &operator<< (const uint16 &val) {
			PutUINT16(val); return (*this);
		}
		CivArchive &operator<< (const sint32 &val) {
			PutSINT32(val); return (*this);
		}
		CivArchive &operator<< (const uint32 &val) {
			PutUINT32(val); return (*this);
		}
		CivArchive &operator<< (const sint64 &val) {
			PutUINT64(val); return (*this);
		}
		CivArchive &operator<< (const uint64 &val) {
			PutUINT64(val); return (*this);
		}
		CivArchive &operator<< (const MBCHAR *val) {
			PutMBCHAR(val); return (*this);
		}
		void PutDOUBLE(const double &val) {

			double temp = val;
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutDoubleString(const double &val);

		void PutSINT8(const sint8 &val) {
			Store((uint8 *)&val, sizeof(sint8));
		}
		void PutUINT8(const uint8 &val) {
			Store((uint8 *)&val, sizeof(uint8));
		}
		void PutSINT16(const sint16 &val) {
			sint16 temp = SDL_SwapLE16(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutUINT16(const uint16 &val) {
			uint16 temp = SDL_SwapLE16(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutSINT32(const sint32 &val) {
			sint32 temp = SDL_SwapLE32(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutUINT32(const uint32 &val) {
			uint32 temp = SDL_SwapLE32(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutSINT64(const sint64 &val) {
			sint64 temp = SDL_SwapLE64(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutUINT64(const uint64 &val) {
			uint64 temp = SDL_SwapLE64(val);
			Store((uint8 *)&temp, sizeof(temp));
		}
		void PutMBCHAR(const MBCHAR *val) {
			if(val){
				uint32 len = strlen(val) + 1;
				PutUINT32(len);
				Store((uint8 *)val, len * sizeof(MBCHAR));
			}
			else{
				PutUINT32(0);
			}
		}

#ifndef HUNT_SERIALIZE
		  void LoadChunk(uint8 *start, uint8 *end);
#endif
		CivArchive &operator>> (double &val) {
			val = GetDOUBLE(); return (*this);
		}
		CivArchive &operator>> (sint8 &val) {
			val = GetSINT8(); return (*this);
		}
		CivArchive &operator>> (uint8 &val) {
			val = GetUINT8(); return (*this);
		}
		CivArchive &operator>> (sint16 &val) {
			val = GetSINT16(); return (*this);
		}
		CivArchive &operator>> (uint16 &val) {
			val = GetUINT16(); return (*this);
		}
		CivArchive &operator>> (sint32 &val) {
			val = GetSINT32(); return (*this);
		}
		CivArchive &operator>> (uint32 &val) {
			val = GetUINT32(); return (*this);
		}
		CivArchive &operator>> (sint64 &val) {
			val = GetUINT64(); return (*this);
		}
		CivArchive &operator>> (uint64 &val) {
			val = GetUINT64(); return (*this);
		}
		CivArchive &operator>> (MBCHAR *val) {
			val = GetMBCHAR(); return (*this);
		}
		double GetDOUBLE(void) {
			double val;
			Load((uint8 *)&val, sizeof(val));

			return val;
		}
		sint8 GetSINT8(void) {
			sint8 val;
			Load((uint8 *)&val, sizeof(val));
			return val;
		}
		uint8 GetUINT8(void) {
			uint8 val;
			Load((uint8 *)&val, sizeof(val));
			return val;
		}
		sint16 GetSINT16(void) {
			sint16 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE16(val);
		}
		uint16 GetUINT16(void) {
			uint16 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE16(val);
		}
		sint32 GetSINT32(void) {
			sint32 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE32(val);
		}
		uint32 GetUINT32(void) {
			uint32 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE32(val);
		}
		sint64 GetSINT64(void) {
			sint64 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE64(val);
		}
		uint64 GetUINT64(void) {
			uint64 val;
			Load((uint8 *)&val, sizeof(val));
			return SDL_SwapLE64(val);
		}
		MBCHAR *GetMBCHAR(void) {
			uint32 len = GetUINT32();
			if(len > 0){
				MBCHAR *val = new MBCHAR[len];
				Load((uint8 *)val, len * sizeof(MBCHAR));
				return val;
			}
			else{
				return NULL;
			}
		}

		void PerformMagic(uint32 id) ;
		void TestMagic(uint32 id) ;

      BOOL IsStoring(void) { return m_bIsStoring; };

		void StoreArray( sint8 * dataarray, size_t size ) {
			Store((uint8 *)dataarray, size);
		}
		void StoreArray( uint8 * dataarray, size_t size ) {
			Store((uint8 *)dataarray, size);
		}
		void StoreArray( sint16 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutSINT16(dataarray[i]);
		}
		void StoreArray( uint16 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutUINT16(dataarray[i]);
		}
		void StoreArray( sint32 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutSINT32(dataarray[i]);
		}
		void StoreArray( uint32 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutUINT32(dataarray[i]);
		}
		void StoreArray( sint64 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutSINT64(dataarray[i]);
		}
		void StoreArray( uint64 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutUINT64(dataarray[i]);
		}
		void StoreArray( double * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutDOUBLE(dataarray[i]);
		}
		void StoreArrayString( double * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				PutDoubleString(dataarray[i]);
		}
		void LoadArray( sint8 * dataarray, size_t size ) {
			Load( (uint8 *)dataarray, size );
		}
		void LoadArray( uint8 * dataarray, size_t size ) {
			Load( (uint8 *)dataarray, size );
		}
		void LoadArray( sint16 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetSINT16();
		}
		void LoadArray( uint16 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetUINT16();
		}
		void LoadArray( sint32 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetSINT32();
		}
		void LoadArray( uint32 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetUINT32();
		}
		void LoadArray( sint64 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetSINT64();
		}
		void LoadArray( uint64 * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetUINT64();
		}
		void LoadArray( double * dataarray, size_t size ) {
			for ( int i=0; i<(int)size; ++i )
				dataarray[i] = GetDOUBLE();
		}
	} ;


#ifdef _DEBUG

#ifdef _USE_CHECKSERIALIZE
#define CHECKSERIALIZE \
{\
    char archive_filename_here[100]= {0}; \
    char archive_filename_tmp[100]= {0}; \
    sint32 archive_filename_len=0; \
	char *partialname = strrchr(__FILE__, '\\') + 1; \
	if(!partialname) \
		partialname = __FILE__; \
    sprintf(archive_filename_here, "%s%d", partialname, __LINE__);\
    archive_filename_len = strlen(archive_filename_here); \
    if (archive.IsStoring()) { \
        archive.Store((uint8*)archive_filename_here, archive_filename_len); \
    } else {\
        archive.Load((uint8*)archive_filename_tmp, archive_filename_len); \
        Assert(strncmp(archive_filename_tmp, archive_filename_here, archive_filename_len) == 0);\
    }\
}
#else
#define CHECKSERIALIZE
#endif

#else

#define CHECKSERIALIZE

#endif

#endif
