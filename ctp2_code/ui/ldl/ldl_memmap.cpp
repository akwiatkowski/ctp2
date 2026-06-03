#include "ctp/c3.h"

#include "ui/ldl/ldl_memmap.h"


unsigned char *ldl_MemMap::GetFileBits( char *filename, unsigned long *junk )
{
	unsigned int filesize;
	unsigned char *bits;
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

	bits = new unsigned char[filesize];

	if (!bits) {
		fclose(f);
		return nullptr;
	}

	if ( fread( bits, 1, filesize, f ) != filesize ) {
		delete[] bits;
		fclose(f);
		return nullptr;
	}

	fclose(f);

	return bits;

}


void ldl_MemMap::ReleaseFileBits( unsigned char *&bits )
{
	if ( bits ) {
		delete[] bits;
		bits = nullptr;
	}
}
