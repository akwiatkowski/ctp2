#ifndef __NS_STRING_H__
#define __NS_STRING_H__

#include <string>

#define k_NS_STRING_LDL_NODATABASE		"nodatabase"

class ns_String
{
	std::string m_string;
public:

	ns_String(char *ldlblock);

	virtual ~ns_String();

	char *GetString();
};

#endif
