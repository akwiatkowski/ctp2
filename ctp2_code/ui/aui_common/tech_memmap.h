#ifndef __TECH_MEMMAP_H__
#define __TECH_MEMMAP_H__


class tech_MemMap
{
public:
	tech_MemMap() = default;
	virtual ~tech_MemMap() = default;

	virtual unsigned char *GetFileBits
	(
		const char *   filename,
		size_t *       filesize = nullptr
	);
	virtual void ReleaseFileBits( unsigned char *&bits );

	static bool GetFileExtension(char const * filename, char * extension);
};

#endif
