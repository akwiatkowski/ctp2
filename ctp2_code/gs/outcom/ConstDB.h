#ifndef __CONST_DB_H__
#define __CONST_DB_H__

#include "ConstRecord.h"

// Compatibility typedef: old code used ConstDB, modern code uses CTPDatabase<ConstRecord>
typedef CTPDatabase<ConstRecord> ConstDB;

#endif
