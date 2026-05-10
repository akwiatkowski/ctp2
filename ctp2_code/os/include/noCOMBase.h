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
class IUnknown {
public:
	virtual uint32 AddRef() = 0;
	virtual uint32 Release() = 0;
};
#endif

#endif
