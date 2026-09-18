#include "ctp/c3.h"
#include "net/io/net_debug.h"
#include "net/io/net_types.h"

#ifdef _DEBUG
char* netdebug_NetErrToString(NET_ERR err)
{
	switch(err) {
	case NET_ERR_OK:	return const_cast<char*>("NET_ERR_OK");
	case NET_ERR_INVALIDADDR: return const_cast<char*>("NET_ERR_INVALIDADDR");
	case NET_ERR_INVALIDPORT: return const_cast<char*>("NET_ERR_INVALIDPORT");
	case NET_ERR_CONNCLOSED: return const_cast<char*>("NET_ERR_CONNCLOSED");
	case NET_ERR_NODATA: return const_cast<char*>("NET_ERR_NODATA");
	case NET_ERR_WRITEERR: return const_cast<char*>("NET_ERR_WEITEERR");
	case NET_ERR_NOTIMPLEMENTED: return const_cast<char*>("NET_ERR_NOTIMPLEMENTED");
	case NET_ERR_TRANSPORTERROR: return const_cast<char*>("NET_ERR_TRANSPORTERROR");
	case NET_ERR_ALREADYOPEN: return const_cast<char*>("NET_ERR_ALREADYOPEN");
	case NET_ERR_NOMORESESSIONS: return const_cast<char*>("NET_ERR_NOMORESESSIONS");
	case NET_ERR_UNKNOWN: return const_cast<char*>("NET_ERR_UNKNOWN");
	default:
	{
		static char str[80];
		snprintf(str, sizeof(str), "NET_ERR_IHADSOMEBADCLAMS(%d)\n", err);
		return str;
	}
	}
}
#endif
