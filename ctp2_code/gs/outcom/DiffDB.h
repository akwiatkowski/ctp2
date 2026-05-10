#ifndef __DIFF_DB_H__
#define __DIFF_DB_H__

#include "DifficultyRecord.h"

// Compatibility typedefs: old code used DiffDB/DifficultyDB, modern code uses CTPDatabase<DifficultyRecord>
typedef CTPDatabase<DifficultyRecord> DiffDB;
typedef CTPDatabase<DifficultyRecord> DifficultyDB;

#endif
