#include "ctp/c3.h"

#include "ui/aui_common/tech_memmap.h"
#include <memory>




bool tech_MemMap::GetFileExtension(char const * filename, char * extension, size_t size)
{
	if (filename && extension)
	{
		char const * lastDot = strrchr(filename, '.');
		if (lastDot)
		{
			strlcpy(extension, ++lastDot, size);
			return true;
		}
	}

	return false;
}


unsigned char *tech_MemMap::GetFileBits
(
	const char *    filename,
	size_t *        outfilesize
)
{

	if ( outfilesize ) *outfilesize = 0;

	size_t        filesize;
	FILE *f = fopen( filename, "rb" );

	if ( !f )
		return nullptr;

	if (fseek(f, 0, SEEK_END) == 0) {
		filesize = ftell(f);
	} else {
		fclose(f);
		return nullptr;
	}

	if (fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return nullptr;
	}

	auto bits = std::make_unique<unsigned char[]>(filesize);

	if (!bits) {
		fclose(f);
		return nullptr;
	}

	if ( fread( bits.get(), 1, filesize, f ) != filesize ) {
		fclose(f);
		return nullptr;
	}

	fclose(f);

	if ( outfilesize ) *outfilesize = filesize;

	return bits.release();
}


void tech_MemMap::ReleaseFileBits( unsigned char *&bits )
{
	if (bits) {
		std::unique_ptr<unsigned char[]> deleter(bits);
		bits = nullptr;
	}
}
