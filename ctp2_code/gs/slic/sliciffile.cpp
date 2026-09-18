#include "ctp/c3.h"
#include "gs/slic/slicif.h"
#include "gs/fileio/CivPaths.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "gs/database/StrDB.h"




int slicif_find_file(char *filename, char *fullpath)
{
	if(!civpaths_Get()->FindFile(C3DIR_GAMEDATA, filename, fullpath))
		return 0;
	return 1;
}

void slicif_report_error(char *s)
{
	// "%s": s is caller-supplied text, not a template.
	c3errors_ErrorDialog("SLIC", "%s", s);
}

int slicif_is_valid_string(char *s)
{
	return stringdb_Get()->GetNameStr(s) != nullptr;
}
