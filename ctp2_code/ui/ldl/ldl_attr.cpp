//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/ldl/ldl_file.hpp"
#include "ldl_attr.hpp"

ldl_attribute *ldl_attribute::GetCopy()
{
	ldl_attribute *newattr = nullptr;
	switch(m_type) {
		case ATTRIBUTE_TYPE_BOOL: newattr = new ldl_attributeValue<bool>(this); break;
		case ATTRIBUTE_TYPE_INT:  newattr = new ldl_attributeValue<int>(this); break;
		case ATTRIBUTE_TYPE_DOUBLE: newattr = new ldl_attributeValue<double>(this); break;
		case ATTRIBUTE_TYPE_STRING: newattr = new ldl_attributeValue<char const *>(this); break;
		case ATTRIBUTE_TYPE_UNKNOWN: break;
	}
	return newattr;
}

bool ldl_attribute::GetBoolValue()
{
	Assert(m_type == ATTRIBUTE_TYPE_BOOL);
	return ((ldl_attributeValue<bool> *)this)->GetValue();
}

int ldl_attribute::GetIntValue()
{
	Assert(m_type == ATTRIBUTE_TYPE_INT);
	return ((ldl_attributeValue<int> *)this)->GetValue();
}

double ldl_attribute::GetFloatValue()
{
	Assert(m_type == ATTRIBUTE_TYPE_DOUBLE);
	return ((ldl_attributeValue<double> *)this)->GetValue();
}

char const *ldl_attribute::GetStringValue()
{
	Assert(m_type == ATTRIBUTE_TYPE_STRING);
	return ((ldl_attributeValue<char const *> *)this)->GetValue();
}

char *ldl_attribute::GetValueText()
{
	static char buf[1024];
	switch(m_type) {
		case ATTRIBUTE_TYPE_BOOL:
			snprintf(buf, sizeof(buf), "%s", GetBoolValue() ? "true" : "false");
			break;
		case ATTRIBUTE_TYPE_INT:
			snprintf(buf, sizeof(buf), "%d", GetIntValue());
			break;
		case ATTRIBUTE_TYPE_DOUBLE:
			snprintf(buf, sizeof(buf), "%lf", GetFloatValue());
			break;
		case ATTRIBUTE_TYPE_STRING:
			snprintf(buf, sizeof(buf), "%s", GetStringValue());
			break;
		case ATTRIBUTE_TYPE_UNKNOWN:
			break;
	}
	return buf;
}
