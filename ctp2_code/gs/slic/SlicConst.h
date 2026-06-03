#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_CONST_H__
#define __SLIC_CONST_H__

#include <nlohmann/json.hpp>

class CivArchive;

class SlicConst {
public:
	SlicConst(const MBCHAR *name, sint32 value) {
		m_name = new MBCHAR[strlen(name) * sizeof(MBCHAR) + 1];
		strcpy(m_name, name);
		m_value = value;
	}
	SlicConst(CivArchive &archive) { Serialize(archive); }

	~SlicConst() {
		
			delete [] m_name;
	}

	void Serialize(CivArchive &archive);

	const MBCHAR *GetName() { return m_name; }
	const sint32 GetValue() { return m_value; }

	// JSON bridge — mirrors SlicConst::Serialize.  Persists the
	// length-prefixed name string + integer value.  Implementation in
	// gs/fileio/json_save.cpp.
	friend void to_json(nlohmann::json &j, SlicConst const &c);
	friend void from_json(nlohmann::json const &j, SlicConst &c);

private:
	char *m_name;
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
