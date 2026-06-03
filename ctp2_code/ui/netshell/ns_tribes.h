#ifndef __NS_TRIBES_H__
#define __NS_TRIBES_H__

class AllinoneWindow;

class ns_Tribes;
// g_nsTribes demoted to file-scope `static` in ns_tribes.cpp.  External
// callers go through nstribes_Get() (returns NULL before the ns_Tribes
// singleton has been constructed).
ns_Tribes * nstribes_Get();

class ns_HPlayerItem;


#define k_TRIBES_MAX 100

#include "ui/netshell/ns_item.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_common/aui_stringtable.h"


class ns_Tribes
{
public:
	ns_Tribes();
	virtual ~ns_Tribes();

	sint32 GetNumTribes( ) const { return m_stringtable->GetNumStrings(); }
	aui_StringTable *GetStrings( ) const { return m_stringtable; }

private:
	aui_StringTable *m_stringtable;
};


class ns_TribesDropDown : public c3_DropDown
{
public:
	ns_TribesDropDown(
		AUI_ERRCODE *retval,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~ns_TribesDropDown() override;
};

#endif
