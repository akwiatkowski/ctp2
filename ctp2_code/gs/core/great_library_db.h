//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Great-library database id (engine-side mirror)
//
//----------------------------------------------------------------------------
//
// The full DATABASE enum lives in ui/interface/GreatLibraryTypes.h because
// the great-library widget is UI.  Game logic only ever needs to *name*
// which sub-DB a SLIC built-in addresses — LibraryUnit, LibraryWonder, ...
//
// These constants mirror that enum's integer values so non-UI code (slicfunc
// and friends) can pass a database id through observer notifications
// without including the UI header.  Keep this in sync with GreatLibraryTypes.h.
//
//----------------------------------------------------------------------------

#pragma once

enum GreatLibraryDB {
    GL_DB_DEFAULT            = 0,
    GL_DB_UNITS              = 1,
    GL_DB_BUILDINGS          = 2,
    GL_DB_WONDERS            = 3,
    GL_DB_ADVANCES           = 4,
    GL_DB_TERRAIN            = 5,
    GL_DB_CONCEPTS           = 6,
    GL_DB_GOVERNMENTS        = 7,
    GL_DB_TILE_IMPROVEMENTS  = 8,
    GL_DB_RESOURCE           = 9,
    GL_DB_ORDERS             = 10,
    GL_DB_SEARCH             = 11
};
