#ifndef __NS_UNITS_H__
#define __NS_UNITS_H__

#include <vector>

class aui_StringTable;

class ns_Units;
// g_nsUnits demoted to file-scope `static` in ns_units.cpp.  External
// callers go through nsunits_Get() / nsunits_Set() — the second is
// needed because allinonewindow.cpp recreates the singleton across
// option changes (delete-then-new).
ns_Units * nsunits_Get();
void       nsunits_Set(ns_Units *p);


#define k_UNITS_MAX 110


class ns_Units
{
public:
	ns_Units();
	virtual ~ns_Units();

	aui_StringTable *GetStrings( ) const { return m_stringtable; }

	std::vector<sint32>		m_noIndex;

private:
	aui_StringTable *m_stringtable;
};

#endif
