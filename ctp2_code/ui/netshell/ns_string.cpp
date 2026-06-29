#include "ctp/c3.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/ldl/ldl_data.hpp"
#include "ui/netshell/ns_string.h"

#include "gs/database/StrDB.h"



ns_String::ns_String( char *ldlBlock )
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

	if ( string ) {
		m_string = string;
	}
}

ns_String::~ns_String( )
{
}

char *ns_String::GetString( ) {
	return const_cast<char *>(m_string.c_str());
}
