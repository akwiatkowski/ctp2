# Easy Ticket: SCOUT-6-011

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-build-WonderRecord-cpp--01.md

## Change Required
WonderRecord manages 15+ dynamic arrays with raw new[]/delete[]

## Current State
`WonderRecord` contains over 15 dynamically allocated `sint32*` arrays, each paired with a count variable. Manual allocation, copying, and deallocation is repeated boilerplate that is error-prone and should be replaced with `std::vector`.

## Code Evidence
```cpp
// File: ctp2_code/build/WonderRecord.h
// Lines: 231-305 (excerpt)
sint32          * m_PrerequisiteBuilding;
sint32            m_numPrerequisiteBuilding;
sint32          * m_GovernmentsModified;
sint32            m_numGovernmentsModified;
sint32          * m_CultureOnly;
sint32            m_numCultureOnly;
sint32          * m_GovernmentType;
sint32            m_numGovernmentType;
sint32          * m_ObsoleteGovernmentType;
sint32            m_numObsoleteGovernmentType;
sint32          * m_CityStyleOnly;
sint32            m_numCityStyleOnly;
sint32          * m_NeedsCityGood;
sint32            m_numNeedsCityGood;
// ... (continues for BuildingAnywhere, ExcludedByBuilding, ExcludedByWonder, etc.)
```

The destructor manually deletes each array:

```cpp
// File: ctp2_code/build/WonderRecord.cpp
// Lines: 409-428
WonderRecord::~WonderRecord()
{
    delete [] m_PrerequisiteBuilding;
    delete [] m_GovernmentsModified;
    delete [] m_CultureOnly;
    delete [] m_GovernmentType;
    delete [] m_ObsoleteGovernmentType;
    delete [] m_CityStyleOnly;
    delete [] m_NeedsCityGood;
    delete [] m_NeedsCityGoodAll;
    delete [] m_NeedsCityGoodAnyCity;
    delete [] m_EnablesGood;
    delete [] m_ShowOnMap;
    delete [] m_ShowOnMapRadius;
    delete [] m_CivilisationOnly;
    delete [] m_NeedsFeatToBuild;
    delete [] m_NeedsAnyPlayerFeatToBuild;
    delete [] m_BuildingAnywhere;
    delete [] m_ExcludedByBuilding;
    delete [] m_ExcludedByWonder;
}
```

The assignment operator repeats the same `delete[] / new[] / std::copy` pattern for every field (~300 lines of boilerplate).

## Suggested Direction
Replace each `sint32* / m_num*` pair with `std::vector<sint32>`. This eliminates manual memory management, provides correct copy/assignment semantics, and removes the need for paired count variables. Serialization can use `.data()` and `.size()`.

## Files You May Edit
ctp2_code/build/WonderRecord.cpp, ctp2_code/build/WonderRecord.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/build/WonderRecord.cpp ctp2_code/build/WonderRecord.h
git commit -m "refactor(build): replace C arrays with std::array"
```
