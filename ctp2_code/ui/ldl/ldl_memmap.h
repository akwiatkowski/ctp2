#ifndef __LDL_MEMMAP_H__
#define __LDL_MEMMAP_H__

#include "ui/aui_common/tech_memmap.h"

class tech_MemMap;

class ldl_MemMap : public tech_MemMap
{
public:
	ldl_MemMap() {;}
	~ldl_MemMap() override = default;

	// Un-hide the base overload: this signature differs (char*, unsigned long*).
	using tech_MemMap::GetFileBits;

	virtual unsigned char *GetFileBits(
		char *filename,
		unsigned long *filesize = nullptr );
	void ReleaseFileBits( unsigned char *&bits ) override;
};

#endif
