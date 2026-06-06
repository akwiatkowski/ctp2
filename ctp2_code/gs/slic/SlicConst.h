#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_CONST_H__
#define __SLIC_CONST_H__

#include <nlohmann/json.hpp>


class SlicConst {
public:
	SlicConst(const MBCHAR *name, sint32 value)
		: m_name(name), m_value(value) {}

	~SlicConst() = default;

	const MBCHAR *GetName() { return m_name.c_str(); }
	const sint32 GetValue() { return m_value; }

	// JSON bridge — mirrors SlicConst::Serialize.  Persists the
	// length-prefixed name string + integer value.  Implementation in
	// gs/fileio/json_save.cpp.
	friend void to_json(nlohmann::json &j, SlicConst const &c);
	friend void from_json(nlohmann::json const &j, SlicConst &c);

private:
	std::string m_name;
	sint32 m_value;
};

enum SLIC_CONSTANTS {
	SLIC_CONST_CONTINUE,
	SLIC_CONST_GETINPUT,
	SLIC_CONST_STOP,

	SLIC_CONST_MAX
};

void slicconst_Initialize();

#endif
