#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_NAMED_SYMBOL_H__
#define __SLIC_NAMED_SYMBOL_H__

#include "gs/slic/SlicSymbol.h"
#include "gs/slic/SlicBuiltinEnum.h"
#include <nlohmann/json.hpp>

class SlicNamedSymbol : public SlicSymbolData
{
protected:

	sint32 m_index;

	uint8 m_fromFile;


	std::string m_name;






public:
	SlicNamedSymbol(const char *name, SLIC_SYM type);
	SlicNamedSymbol(const char *name);
	SlicNamedSymbol(const char *name, SlicArray *array);
	SlicNamedSymbol(const char *name, SlicStructDescription *structDesc);
	SlicNamedSymbol() = default;
	~SlicNamedSymbol() override = default;

	bool IsParameter() const override { return false; }
	virtual bool IsBuiltin() const { return false; }

	void Init(const char *name);

	void PostSerialize();
	SLIC_SYM_SERIAL_TYPE GetSerializeType() override { return SLIC_SYM_SERIAL_NAMED; }

	const char *GetName() const override;
	void DelName() { m_name.clear(); }

	sint32 GetIndex() const { return m_index; }
	void SetIndex(sint32 index) { m_index = index; }

	friend void to_json(nlohmann::json &j, SlicNamedSymbol const &s);
	friend void from_json(nlohmann::json const &j, SlicNamedSymbol &s);
};

class SlicParameterSymbol : public SlicNamedSymbol
{
private:
	sint32 m_parameterIndex;

public:
	SlicParameterSymbol(const char *name, sint32 index);
	SlicParameterSymbol() = default;

	SLIC_SYM_SERIAL_TYPE GetSerializeType() override { return SLIC_SYM_SERIAL_PARAMETER; }

	BOOL GetIntValue(sint32 &value) const override;
	BOOL GetPlayer(sint32 &value) const override;
	BOOL GetPos(MapPoint &pos) const override;
	BOOL GetUnit(Unit &u) const override;
	BOOL GetArmy(Army &a) const override;
	BOOL GetCity(Unit &c) const override;

	bool IsParameter() const override { return TRUE; }

	friend void to_json(nlohmann::json &j, SlicParameterSymbol const &s);
	friend void from_json(nlohmann::json const &j, SlicParameterSymbol &s);
};

class SlicBuiltinNamedSymbol : public SlicNamedSymbol
{
	SLIC_BUILTIN m_builtin;
public:
	SlicBuiltinNamedSymbol(SLIC_BUILTIN which, const char *name, SlicArray *array) :
		m_builtin(which),
		SlicNamedSymbol(name, array)
	{}
	SlicBuiltinNamedSymbol(SLIC_BUILTIN which, const char *name, SlicStructDescription *structDesc) :
		m_builtin(which),
		SlicNamedSymbol(name, structDesc)
	{}
	SlicBuiltinNamedSymbol() = default;

	SLIC_SYM_SERIAL_TYPE GetSerializeType() override { return SLIC_SYM_SERIAL_BUILTIN; }

	bool IsBuiltin() const override { return true; }
	SLIC_BUILTIN GetBuiltin() { return m_builtin; }

	friend void to_json(nlohmann::json &j, SlicBuiltinNamedSymbol const &s);
	friend void from_json(nlohmann::json const &j, SlicBuiltinNamedSymbol &s);
};

#endif
