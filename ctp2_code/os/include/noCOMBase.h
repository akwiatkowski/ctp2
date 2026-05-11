/**
 * $Id$
 */
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __os__include__noCOMBase_h__
#define __os__include__noCOMBase_h__ 1

// If objbase.h was already included, it provides a full IUnknown with QueryInterface.
// Skip our minimal definition to avoid redefinition errors.
#ifndef __OBJBASE_H__
#define __NOCOMBASE_IUNKNOWN_DEFINED__
// Minimal IUnknown replacement that matches the Windows vtable layout:
// slot 0 = QueryInterface, slot 1 = AddRef, slot 2 = Release.
// This MUST match objbase.h's IUnknown to avoid ODR violations.
#include "os/include/ctp2_inttypes.h"  // sint32
struct _GUID;
typedef struct _GUID GUID;
#ifndef E_NOINTERFACE
#define E_NOINTERFACE ((sint32)0x80004002L)
#endif
typedef sint32 HRESULT;
class IUnknown {
public:
	virtual HRESULT QueryInterface(const GUID &riid, void **obj)
		{ (void)riid; *obj = nullptr; return E_NOINTERFACE; }
	virtual uint32 AddRef() = 0;
	virtual uint32 Release() = 0;
};
#endif

#endif
