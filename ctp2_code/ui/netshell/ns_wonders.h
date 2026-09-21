#ifndef __NS_WONDERS_H__
#define __NS_WONDERS_H__

class aui_StringTable;

#include <memory>

class ns_Wonders;
// g_nsWonders demoted to file-scope `static` in ns_wonders.cpp.
// External callers go through nswonders_Get() / nswonders_Set().
ns_Wonders * nswonders_Get();
void         nswonders_Set(ns_Wonders *p);


#define k_WONDERS_MAX 50


class ns_Wonders
{
public:
	ns_Wonders();
	virtual ~ns_Wonders();

	aui_StringTable *GetStrings( ) const { return m_stringtable.get(); }

private:
	std::unique_ptr<aui_StringTable>	m_stringtable;
};

#endif
