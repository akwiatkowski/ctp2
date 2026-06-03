#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_EYE_POINT_H__
#define __SLIC_EYE_POINT_H__

#include <nlohmann/json.hpp>
#include <string>

#include "gs/world/MapPoint.h"
#include "gs/gameobj/Unit.h"

class CivArchive;
class Message;
class SlicSegment;

enum EYE_POINT_TYPE {
	EYE_POINT_TYPE_GENERIC,
	EYE_POINT_TYPE_CITY,
	EYE_POINT_TYPE_UNIT,
	EYE_POINT_TYPE_ADVANCE,
	EYE_POINT_TYPE_NOTHING
};

class SlicEyePoint
{
private:
	friend void to_json(nlohmann::json &j, SlicEyePoint const &e);
	friend void from_json(nlohmann::json const &j, SlicEyePoint &e);

public:
	SlicEyePoint();
	SlicEyePoint(const MapPoint &point, const MBCHAR *name,
				 sint32 data, EYE_POINT_TYPE type,
				 const Unit &unit,
				 sint32 recipient,
				 SlicSegment *segment);

	SlicEyePoint(SlicEyePoint *copy);
	SlicEyePoint(CivArchive &archive);
	~SlicEyePoint();
	void Serialize(CivArchive &archive);

	void GetPoint(MapPoint &point) { point = m_point; }
	const char *GetName() { return m_name.empty() ? nullptr : m_name.c_str(); }

	void SetMessage(const Message &message);
	Message GetMessage() const;

	void Callback();

private:
	MapPoint m_point;
	std::string m_name;
	Message *m_message;
	sint32 m_data;
	Unit m_unit;
	sint32 m_recipient;
	SlicSegment *m_segment;

	EYE_POINT_TYPE m_type;

};

// std::string overloads of optStringToJson / jsonToOptString live in
// gs/fileio/json_save.cpp's anonymous namespace — all callers of these
// helpers for std::string-typed fields (SlicEyePoint::m_name,
// MessageData::m_text, etc.) are inside json_save.cpp.

#endif  // __SLIC_EYE_POINT_H__
