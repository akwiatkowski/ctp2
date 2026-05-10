//
//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header file
// Description  : Minimal COM compatibility layer for non-Windows systems
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project.
//
//----------------------------------------------------------------------------

#ifndef __OBJBASE_H__
#define __OBJBASE_H__

#include "windows.h"
#include <string.h>

// 'interface' is a Microsoft extension for COM interfaces
#ifndef interface
#define interface struct
#endif

// Calling convention (empty on non-Windows)
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE
#endif

#ifndef STDAPICALLTYPE
#define STDAPICALLTYPE
#endif

// Pure virtual method
#ifndef PURE
#define PURE = 0
#endif

// this pointer macros
// In C++ mode, 'this' is implicit. In C mode, COM interfaces need explicit This.
#ifndef THIS_
#ifdef __cplusplus
#define THIS_
#else
#define THIS_                   INTERFACE *This,
#endif
#endif
#ifndef THIS
#ifdef __cplusplus
#define THIS
#else
#define THIS                    INTERFACE *This
#endif
#endif

// HRESULT values (already defined in windows.h as sint32)
#ifndef S_OK
#define S_OK           ((HRESULT)0L)
#endif
#ifndef S_FALSE
#define S_FALSE        ((HRESULT)1L)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE  ((HRESULT)0x80004002L)
#endif
#ifndef E_FAIL
#define E_FAIL         ((HRESULT)0x80004005L)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL      ((HRESULT)0x80004001L)
#endif
#ifndef E_UNEXPECTED
#define E_UNEXPECTED   ((HRESULT)0x8000FFFFL)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY  ((HRESULT)0x8007000EL)
#endif

// Method declaration macros
#ifndef STDMETHOD
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#endif
#ifndef STDMETHOD_
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#endif
#ifndef STDMETHODIMP
#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE
#endif

#ifndef STDAPI
#define STDAPI                  HRESULT STDAPICALLTYPE
#endif
#ifndef STDAPI_
#define STDAPI_(type)           type STDAPICALLTYPE
#endif

// Reference types
typedef const GUID& REFGUID;
typedef const GUID& REFIID;
typedef const GUID& REFCLSID;
typedef GUID IID;
typedef GUID CLSID;

// Interface declaration
#ifndef DECLARE_INTERFACE_
#define DECLARE_INTERFACE_(iface, base) interface iface : public base
#endif

// IUnknown interface
#ifndef __NOCOMBASE_IUNKNOWN_DEFINED__
#undef INTERFACE
#define INTERFACE IUnknown
interface IUnknown
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};
#endif

// GUID comparison
inline int IsEqualGUID(const GUID& rguid1, const GUID& rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#ifndef IsEqualIID
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#endif
#ifndef IsEqualCLSID
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
#endif

// GUID definition macro
// In C++, const at namespace scope has internal linkage, so it's safe to use in headers
#ifdef __cplusplus
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

// IID_IUnknown - standard IID
#ifndef IID_IUnknown_DEFINED
#define IID_IUnknown_DEFINED
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
#endif

#endif // __OBJBASE_H__
