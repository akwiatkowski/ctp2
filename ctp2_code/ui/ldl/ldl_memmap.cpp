#include "ctp/c3.h"

#include "ui/ldl/ldl_memmap.h"

#include <memory>


unsigned char *ldl_MemMap::GetFileBits( char *filename, unsigned long *junk )
{
	unsigned int filesize;
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

	// Caller owns the returned buffer and frees it via ReleaseFileBits.
	auto bits = std::make_unique<unsigned char[]>(filesize);

	if ( fread( bits.get(), 1, filesize, f ) != filesize ) {
		fclose(f);
		return nullptr;
	}

	fclose(f);

	return bits.release();

}


void ldl_MemMap::ReleaseFileBits( unsigned char *&bits )
{
	if ( bits ) {
		std::unique_ptr<unsigned char[]> deleter(bits);
		bits = nullptr;
	}
}
