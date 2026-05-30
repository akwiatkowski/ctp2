# Easy Ticket: SCOUT-7-006

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ctp-ctp2_utils-tracklen-cpp--01.md

## Change Required
tracklen.cpp uses C-style arrays and VLAs for track buffers and path strings

## Current State
`tracklen.cpp` contains numerous C-style fixed-size arrays that should be replaced with safer C++ containers or `std::array`. Notably, `newbuf[tracklen_MAXTRACKS]` is declared as a local variable in `tracklen_CheckTrackLengths2()` (a VLA-like pattern in C++), and `trackLenBuf` is passed around as a raw `DWORD*` with implicit size assumptions.

## Code Evidence
```cpp
// File: ctp2_code/ctp/ctp2_utils/tracklen.cpp
// Lines: 336
		DWORD newbuf[tracklen_MAXTRACKS];
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/tracklen.cpp
// Lines: 371
	char szTemp[ MAX_PATH ];
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/tracklen.cpp
// Lines: 314-315
		char szDrive[ MAX_PATH ];
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/tracklen.cpp
// Lines: 505
	DWORD trackLenBuf[ DWVERSIONINFOLEN + tracklen_MAXTRACKS ];
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/tracklen.cpp
// Lines: 57
char tracklen_buf[512];
```

## Suggested Direction
1. Replace `DWORD newbuf[tracklen_MAXTRACKS]` with `std::vector<DWORD>` or `std::array<DWORD, tracklen_MAXTRACKS>`.
2. Replace path buffers (`szTemp`, `szDrive`) with `std::string` or `std::vector<char>`.
3. Pass buffers with their sizes explicitly, or return `std::vector<DWORD>` from `tracklen_GetTrackLengths`.
4. Replace `tracklen_buf[512]` with a thread-local or dynamically allocated buffer.

## Files You May Edit
ctp2_code/ctp/ctp2_utils/tracklen.cpp, ctp2_code/ctp/ctp2_utils/tracklen.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ctp/ctp2_utils/tracklen.cpp ctp2_code/ctp/ctp2_utils/tracklen.h
git commit -m "refactor(ctp): replace C arrays with std::array"
```
