#ifndef __AUI_DIRTYLIST_H__
#define __AUI_DIRTYLIST_H__

#include <vector>

#include "ui/aui_common/aui_base.h"
#include "ui/aui_common/tech_wllist.h"

struct aui_Span
{
	sint16 run;
	sint16 length;
};







#define k_DIRTYLIST_MAXSPANS	15

struct aui_SpanList
{
	sint32 num;
	aui_Span spans[ k_DIRTYLIST_MAXSPANS ];
};


class aui_DirtyList : public aui_Base, public tech_WLList<RECT *>
{
public:

	aui_DirtyList(
		BOOL useSpans = FALSE,
		sint32 width = 0,
		sint32 height = 0 );
	~aui_DirtyList() override;

	AUI_ERRCODE	AddRect(
		sint32 left,
		sint32 top,
		sint32 right,
		sint32 bottom );
	AUI_ERRCODE	AddRect(
		RECT *rect );

	AUI_ERRCODE	SubtractRect(
		sint32 left,
		sint32 top,
		sint32 right,
		sint32 bottom );
	AUI_ERRCODE	SubtractRect( RECT *sub );

	AUI_ERRCODE	Minimize( );
	void		Flush( );

	sint32 GetWidth() const { return m_width; }
	sint32 GetHeight() const { return m_height; }

	aui_SpanList *GetSpans() { return m_spanListArray.empty() ? nullptr : m_spanListArray.data(); }
	AUI_ERRCODE SetSpans( aui_DirtyList *newDirtyList );

	BOOL IsEmpty( ) const { return m_isEmpty; }

	AUI_ERRCODE ComputeSpans( RECT *newRect );

protected:
	// Held by value: created with the list, destroyed with it, never replaced.
	tech_Memory<RECT> m_rectMemory;

	sint32 m_width;
	sint32 m_height;

	std::vector<aui_SpanList> m_spanListArray;
	sint32 m_isEmpty;
};

#endif
