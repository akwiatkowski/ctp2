#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_RECORD_H__
#define __SLIC_RECORD_H__

#include <nlohmann/json.hpp>

class CivArchive;

class SlicSegment;

class SlicRecord {
private:
	sint32 m_owner;
	MBCHAR *m_title;
	MBCHAR *m_text;
	SlicSegment *m_segment;

public:
	SlicRecord(sint32 owner, MBCHAR *title, MBCHAR *text,
			   SlicSegment *segment);
	SlicRecord(CivArchive &archive);
	~SlicRecord();
	void Serialize(CivArchive &archive);

	const MBCHAR *GetTitle() { return m_title; }

	MBCHAR *AccessTitle() { return m_title; }
	const MBCHAR *GetText() { return m_text; }
	SlicSegment *GetSegment() { return m_segment; }
	void Reconstitute();

	// JSON bridge — mirrors SlicRecord::Serialize.  Persists owner +
	// title + text strings + the segment's name (resolved via
	// g_slicEngine on load).  Implementation in json_save.cpp.
	friend void to_json(nlohmann::json &j, SlicRecord const &r);
	friend void from_json(nlohmann::json const &j, SlicRecord &r);
};

#endif
