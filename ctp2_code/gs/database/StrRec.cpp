#include "ctp/c3.h"

#include "gs/database/StrRec.h"

StringRecord::StringRecord()

{
    m_index = -1;
    m_lesser = nullptr;
	m_greater = nullptr;
}

StringRecord::~StringRecord()

{
}
