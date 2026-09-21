#ifndef __NS_IMPROVEMENTS_H__
#define __NS_IMPROVEMENTS_H__

class aui_StringTable;

#include <memory>

class ns_Improvements;
// g_nsImprovements demoted to file-scope `static` in ns_improvements.cpp.
// External callers go through nsimprovements_Get() / nsimprovements_Set().
ns_Improvements * nsimprovements_Get();
void              nsimprovements_Set(ns_Improvements *p);


#define k_IMPROVEMENTS_MAX 70


class ns_Improvements
{
public:
	ns_Improvements();
	virtual ~ns_Improvements();

	aui_StringTable *GetStrings( ) const { return m_stringtable.get(); }

private:
	std::unique_ptr<aui_StringTable>	m_stringtable;
};

#endif
