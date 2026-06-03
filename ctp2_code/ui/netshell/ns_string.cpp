#include "ctp/c3.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/ldl/ldl_data.hpp"
#include "ui/netshell/ns_string.h"

#include "gs/database/StrDB.h"



ns_String::ns_String( char *ldlBlock )
:
    m_string    (nullptr)
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return;

	const char *string;

	if ( block->GetBool(k_NS_STRING_LDL_NODATABASE) || (!block->GetString("text"))) {

		string = block->GetString( "text" );
	}
	else {
		string = stringdb_Get()->GetNameStr( block->GetString("text") );
	}

	m_string = new char[strlen(string) + 1];
    strcpy(m_string, string);
}

ns_String::~ns_String( )
{
	delete [] m_string;
}

char *ns_String::GetString( ) {
	return m_string;
}
