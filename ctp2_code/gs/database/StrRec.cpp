#include "ctp/c3.h"

#include "gs/database/StrRec.h"

StringRecord::StringRecord()

{
    m_index = -1;
    m_lesser = NULL;
	m_greater = NULL;
}

StringRecord::~StringRecord()

{
}
