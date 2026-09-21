#include "ctp/c3.h"
#include <memory>
#include "ui/aui_ctp2/c3memmap.h"

#include "ui/aui_ctp2/c3imageformats.h" // TargaImageFormat, TiffImageFormat

/// Create an aui_FileFormat object
/// \param  filename The file to use
/// \return The created object (or NULL, when the file format is not supported).
/// \remarks The format is derived from the extension of the file.
/// \see aui_MemMap::GetFileFormat for more supported extensions.
aui_FileFormat * C3MemMap::GetFileFormat(MBCHAR const * filename)
{
	MBCHAR extension[ 8 ];
	if ( GetFileExtension( filename, extension, sizeof(extension) ) )
	{
		if ( !strnicmp( extension, "tga", 3 ) )
			return std::make_unique<TargaImageFormat>().release();
		if ( !strnicmp( extension, "tif", 3 ) )
			return std::make_unique<TiffImageFormat>().release();
	}

    return aui_MemMap::GetFileFormat(filename);
}
