# CTP2 Bug Hunt Report

Generated from automated analysis of 456 suspicious source files across 31 parallel agents.

## Summary

**Total bugs found: 963** (after deduplication)

### By Severity

| Severity | Count |
|----------|-------|
| Critical | 54 |
| High | 441 |
| Medium | 449 |
| Low | 19 |

### By Category

| Category | Count |
|----------|-------|
| MISSING_BOUNDS_CHECK | 305 |
| NULL_DEREFERENCE | 166 |
| DIVISION_BY_ZERO | 122 |
| DANGEROUS_SHIFT | 107 |
| INTEGER_OVERFLOW | 99 |
| FORMAT_STRING_BUG | 70 |
| UNALIGNED_ACCESS | 33 |
| FLOAT_CAST_OVERFLOW | 25 |
| UNINITIALIZED_VARIABLE | 20 |
| BUFFER_OVERFLOW | 3 |
| INTEGER_OVERFLOW / MISSING_BOUNDS_CHECK | 3 |
| INTEGER_OVERFLOW (pointer truncation) | 2 |
| INTEGER_OVERFLOW / DANGEROUS_SHIFT | 2 |
| MEMCPY_OVERLAP | 2 |
| FORMAT_STRING_BUG/NULL_DEREFERENCE | 1 |
| LOGIC_ERROR | 1 |
| MISSING_BOUNDS_CHECK / NULL_DEREFERENCE | 1 |
| MISSING_BOUNDS_CHECK/NULL_DEREFERENCE | 1 |

---


## CRITICAL Severity

> **Resolution status — all 54 CRITICAL findings verified in code 2026-07-02.**
> Findings are static-analysis leads, not verdicts; each was checked against the
> current source. **Net result: 0 open.** Breakdown:
>
> **Fixed this pass (5 findings, 4 commits):**
> - `c3cmdline.cpp` `CommandLine::AddKey` / `Parse` buffer & argv overflow → commit `fbbe6965`
> - `spriteutils.cpp` `spriteutils_DecodeToBuffer` unbounded writes → commit `65406bc4` (function is currently uncalled)
> - `CityData.cpp` 64-bit `(uint32)&` pointer-truncation memcpy → commit `9aee4932` (dormant behind `NEW_RESOURCE_PROCESS`)
> - `net_unit.cpp` missing `size` check + `network.cpp` `ChunkList` write-before-grow → commit `2fe812d5`
>
> **Already fixed (verified present in current code, from the May 2026 batch or later):**
> - `GaiaController.cpp` — all 18 DANGEROUS_SHIFT via `safe_shift_left_u64()` (runtime-guarded, not just Assert)
> - `CityData.cpp` DIVISION_BY_ZERO (2585/2586/2588/3763/3771/3774) — all denominators guarded
> - `UnitActor.cpp:1550` — `totalHP > 0` guard
> - `Goal.cpp:3255` — refactored to `snprintf` into a sized `std::vector` (underflow loop gone)
> - `ArmyData.cpp:2963` — `m_nElements <= 0` and `data == nullptr` guards
> - `slicobject.cpp` (100/135/553) — refactored to `std::string`, null-safe
> - `sourcelist.cpp:133` — pointers `nullptr`-initialized in ctor
> - `thronedb.cpp:73` — bounds check returns `nullptr`
> - `spritefile.cpp` (91-98/376-380/452-456/634-641) — `GetNumFrames() > 800` guard with early return
> - `net_action.cpp` (278/320) — `m_action` range guard; `net_info.cpp:320` — `m_type` range guard
>
> **Obsolete (the flagged code path was removed/replaced, so the bug no longer exists):**
> - `SlicSegment.cpp` (441/443/456/471) — `malloc`/`Serialize` path gone; `m_code` is `nullptr`-initialized
> - `foreigner.cpp:1493` — `ComputeLandContinentShared` removed
> - `gameinit.cpp:1890` — `usedPositions[]` array removed
> - `MapFile.cpp:716` — binary chunk reader replaced by the JSON map loader
>
> Verified with `mise exec -- make test` and `mise exec -- make ubsan-smoke` (both green).


### DANGEROUS_SHIFT

- **GaiaController.cpp:154** — `sm_endgameImprovements |= uint64((uint64)1 << (uint64)type);` performs a left shift where `type` comes from `terr_rec->GetIndex()`. If the terrain improvement index is >= 64, the shift amount is >= the bit width of `uint64`, which is undefined behavior in C++.
  *Fix: Guard before shifting: `if (type >= 0 && type < 64) sm_endgameImprovements |= (uint64)1 << type;`*

- **GaiaController.cpp:161** — `sm_endgameBuildings |= uint64((uint64)1 << (uint64)type);` where `type` is a building index. If `type >= 64`, undefined behavior.
  *Fix: Same guard as above, or use a bitset class instead of uint64.*

- **GaiaController.cpp:168** — `sm_endgameWonders |= uint64((uint64)1 << (uint64)type);` where `type` is a wonder index. If `type >= 64`, undefined behavior.
  *Fix: Same guard as above.*

- **GaiaController.cpp:280** — `(city_buildings & ((uint64)0x1 << (uint64)type))` in `GaiaController_CaptureCity` shifts by `type` (building index). If `type >= 64`, UB.
  *Fix: Guard shift or use a larger bit container.*

- **GaiaController.cpp:280** — `(uint64)0x1 << (uint64)type` where `type` ranges over `g_theBuildingDB->NumRecords()`. If the database contains 64 or more records, shifting a 64-bit value by 64 or more is undefined behavior in C++.
  *Fix: Guard the shift with `if (type < 64)` or use a bitmask array instead of a single uint64.*

- **GaiaController.cpp:300** — `(city_wonders & ((uint64)0x1 << (uint64)type))` shifts by `type` (wonder index). If `type >= 64`, UB.
  *Fix: Guard shift or use a larger bit container.*

- **GaiaController.cpp:300** — Same pattern as line 280 for wonders: `(uint64)0x1 << (uint64)type` where `type` comes from `g_theWonderDB->NumRecords()`. Shift by >= 64 is undefined behavior.
  *Fix: Guard with `if (type < 64)` or use a larger bitmask type / array.*

- **GaiaController.cpp:352** — `GaiaController::sm_endgameImprovements & ((uint64)0x1 << (uint64)type)` shifts by `type` (terrain improvement index). If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:392** — `GaiaController::sm_endgameImprovements & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_ImprovementComplete`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:417** — `GaiaController::sm_endgameBuildings & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_SellBuilding`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:443** — `GaiaController::sm_endgameBuildings & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_CreateBuilding`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:520** — `GaiaController::sm_endgameWonders & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_CreateWonder`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:545** — `GaiaController::sm_endgameBuildings & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_BuildingRemoved`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:570** — `GaiaController::sm_endgameWonders & ((uint64)0x1 << (uint64)type)` shifts by `type` in `GaiaController_WonderRemoved`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:624** — `~((uint64)0x1 << (uint64)type)` in `HandleBuildingChange` shifts by `type`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:635** — `~((uint64)0x1 << (uint64)type)` in `HandleBuildingChange` (else branch). If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:656** — `~((uint64)0x1 << (uint64)type)` in `HandleWonderChange`. If `type >= 64`, UB.
  *Fix: Guard shift.*

- **GaiaController.cpp:670** — `~((uint64)0x1 << (uint64)type)` in `HandleTerrImprovementChange`. If `type >= 64`, UB.
  *Fix: Guard shift.*


### DIVISION_BY_ZERO

- **CityData.cpp:2585** — In `ComputeSpecialistsEffects`, `food_ratio = totalFood / totalFood` — if `totalFood` is 0, this is a division by zero. (Note: the code divides by `totalFood` in multiple places; if the total is 0, all ratios are undefined.)
  *Fix: Add a guard `if (totalFood == 0) food_ratio = 0; else food_ratio = totalFood / totalFood;` (or more generally, handle zero denominators for all three: totalFood, totalProd, totalGold).*

- **CityData.cpp:2586** — In `ComputeSpecialistsEffects`, `production_ratio = totalProduction / totalProduction` — if `totalProduction == 0`, division by zero.
  *Fix: Guard the division with a zero-check.*

- **CityData.cpp:2588** — In `ComputeSpecialistsEffects`, `gold_ratio = totalGold / totalGold` — if `totalGold == 0`, division by zero.
  *Fix: Guard the division with a zero-check.*

- **CityData.cpp:3763** — `double popRatio = ... / (maxPop - overcrowding);` If `maxPop == overcrowding`, the denominator is zero.
  *Fix: Add a guard `if (maxPop <= overcrowding) return 0.0;` before the division.*

- **CityData.cpp:3771** — `popScale = (PopCount() - minPop) / (maxPop - minPop);` If `maxPop == minPop`, this divides by zero.
  *Fix: Guard with `if (maxPop == minPop) return 0.0;` before the division.*

- **CityData.cpp:3774** — `return (... / maxSurplusFood) * growthRate;` If `maxSurplusFood == 0`, this divides by zero.
  *Fix: Add a guard `if (maxSurplusFood == 0) return 0.0;` before the division.*

- **UnitActor.cpp:1550** — `ratio = std::max(0.0, m_unitID.GetHP() / m_unitID->CalculateTotalHP());` performs integer division before the double conversion. If `CalculateTotalHP()` returns 0, this causes a division-by-zero crash before `std::max` is even evaluated.
  *Fix: Check denominator first: `sint32 totalHP = m_unitID->CalculateTotalHP(); ratio = (totalHP > 0) ? std::max(0.0, (double)m_unitID.GetHP() / totalHP) : 0.0;`.*


### FORMAT_STRING_BUG

- **c3cmdline.cpp:~6500 (CommandLine::AddKey)** — `m_buf[m_len++] = c` in the default case has no bounds check. Typing a very long command overflows `m_buf`.
  *Fix: Add `if (m_len >= sizeof(m_buf) - 1) { m_addingKey = false; return TRUE; }` before the assignment.*

- **c3cmdline.cpp:~6562 (CommandLine::Parse)** — `curarg[argpos++] = m_buf[p]` has no bounds check against `curarg[1024]`. A long command-line argument overflows the stack buffer. Additionally, `m_argv[m_argc]` is written without checking `m_argc` against the array size.
  *Fix: Add `if (argpos >= sizeof(curarg) - 1) continue;` and `if (m_argc >= MAX_ARGS) break;` before writing.*


### INTEGER_OVERFLOW (pointer truncation)

- **CityData.cpp:1149** — `memcpy(..., (uint32)&copy->m_science_lost_to_crime + sizeof(copy->m_science_lost_to_crime) - (uint32)&copy->m_max_processed_terrain_food)` has the same pointer-to-`uint32` truncation bug. On 64-bit systems the computed size is wrong, leading to memory corruption.
  *Fix: Use `size_t` instead of `uint32`.*

- **CityData.cpp:760** — `memset(&m_max_processed_terrain_food, 0, (uint32)&m_science_lost_to_crime + sizeof(m_science_lost_to_crime) - (uint32)&m_max_processed_terrain_food)` casts pointers to `uint32`. On 64-bit platforms the upper 32 bits of the pointer are discarded, producing a completely wrong size for `memset`. This can corrupt memory or zero out the wrong number of bytes.
  *Fix: Use `size_t` instead of `uint32` for the pointer arithmetic.*


### MISSING_BOUNDS_CHECK

- **Goal.cpp:3255** — `for (uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)` underflows when `strlen(myText) < 5` because `strlen` returns `size_t` (unsigned). The underflow produces a huge positive bound, causing the loop to write up to 255 bytes into `goalString` which is only `strlen(myText)+40` bytes (e.g., 44 bytes when strlen is 4). This is a stack buffer overflow.
  *Fix: Change the loop to check `strlen(myText) > 5` before the loop, or cast to a signed type: `for (sint32 myComp = 0; myComp < static_cast<sint32>(strlen(myText)) - 5; myComp++)`.*

- **MapFile.cpp:716** — `chunkSize` is read directly from the input file (`fread(&chunkSize, 1, sizeof(chunkSize), infile)`) and then used to allocate a buffer (`new uint8[chunkSize]`) without any validation. A malicious or corrupted file can specify a negative `chunkSize` (since `sint32` is signed), causing an underflow/overflow in the allocation, or an extremely large value causing memory exhaustion.
  *Fix: Validate `chunkSize` before allocation: assert it is non-negative and impose a reasonable upper bound based on expected map dimensions.*

- **ctp2_code/gs/database/thronedb.cpp:73** — `GetThroneInfo` computes `index = (type * m_nThroneLevels) + level` and returns `&m_throneInfo[index]` without validating that `type`, `level`, or the resulting `index` are within bounds. Malformed DB or save data leads to out-of-bounds memory access.
  *Fix: Add bounds checks: `Assert(type >= 0 && type < m_nThroneTypes); Assert(level >= 0 && level < m_nThroneLevels);` and return NULL on failure.*

- **gameinit.cpp:1890** — `usedPositions` is declared as `sint32 usedPositions[k_MAX_START_POINTS]`. The loop iterates up to `positionCount` (read from `g_theWorld->GetNumStartingPositions()`). If `positionCount` exceeds `k_MAX_START_POINTS`, the write `usedPositions[j] = 1` causes a stack buffer overflow.
  *Fix: Assert `positionCount <= k_MAX_START_POINTS` before the loop, or dynamically allocate the array.*

- **net_action.cpp:278** — In `NetAction::Unpacketize`, `m_action` is read from a network packet (`PULLSHORTTYPE(m_action, NET_ACTION)`) and immediately used to index the `m_args` array: `m_args[m_action]`. The `m_args` array has `NET_ACTION_NULL` elements. A malicious or corrupted packet with `m_action >= NET_ACTION_NULL` causes an out-of-bounds read and can lead to information disclosure or controlled memory access.
  *Fix: Add a bounds check before indexing: `Assert(m_action < NET_ACTION_NULL); if (m_action >= NET_ACTION_NULL) return;`*

- **net_action.cpp:320** — `m_action` is read from a network packet (`PULLSHORTTYPE(m_action, NET_ACTION)`) and immediately used as an array index into `m_args[m_action]`. There is no validation that `m_action` is within the valid range `[0, NET_ACTION_NULL)`. A malicious client can send an out-of-bounds action ID, causing an array out-of-bounds read.
  *Fix: Add a bounds check: `if (m_action < 0 || m_action >= NET_ACTION_NULL) { return; }` before indexing `m_args`.*

- **net_info.cpp:320** — In `NetInfo::Unpacketize`, `m_type` is read from the network packet (`m_type = (NET_INFO_CODE)getshort(&buf[2])`) and used to index `m_args[m_type]`. The `m_args` array has `NET_INFO_CODE_NULL` elements. A forged packet with `m_type >= NET_INFO_CODE_NULL` causes an out-of-bounds read.
  *Fix: Validate before indexing: `Assert(m_type < NET_INFO_CODE_NULL); if (m_type >= NET_INFO_CODE_NULL) return;`*

- **net_info.cpp:320** — `m_type` is read from the network packet via `getshort(&buf[2])` and immediately used to index the `m_args` array on line 323 (`m_args[m_type]`). There is no validation that `m_type` is within `[0, NET_INFO_CODE_NULL)`. A malicious or corrupted packet can set `m_type >= 290`, causing an out-of-bounds read and potential code execution.
  *Fix: Add a bounds check after reading `m_type`: `if (m_type < 0 || m_type >= NET_INFO_CODE_NULL) return;` or abort processing.*

- **net_unit.cpp:219-223** — Hardcoded offsets `&buf[10]`, `&buf[12]`, `&buf[16]`, `&buf[18]`, `&buf[20]` are accessed without bounds checking. A truncated packet causes out-of-bounds reads.
  *Fix: Validate `size >= 24` before accessing these offsets.*

- **network.cpp:3920-3938** — In `ChunkList`, `packet->Packetize(&mapBuf[size + 2], len)` writes to the buffer BEFORE the reallocation check. If `size + 2 + len > mapBufSize`, the write overflows the buffer. The reallocation at line 3929 happens only after the overflow.
  *Fix: Reallocate before writing, or check `size + 2 + len <= mapBufSize` before calling `Packetize`.*

- **spritefile.cpp:376-380** — `ReadSpriteDataBasic` declares `uint32 ssizes[800]` and reads `sizeof(uint32) * s->GetNumFrames()` directly into it. A corrupt sprite with `numFrames > 800` overflows the stack buffer.
  *Fix: Check `s->GetNumFrames() <= 800` before reading.*

- **spritefile.cpp:452-456** — `ReadSpriteDataFull` uses `uint32 ssizes[800]` and reads based on `GetNumFrames()` without a cap. Same stack overflow as above.
  *Fix: Validate `numFrames <= 800` before filling the stack arrays.*

- **spritefile.cpp:634-641** — `ReadFacedSpriteDataFull` declares `uint32 ssizes[k_MAX_FACINGS][800]` and reads `sizeof(uint32) * s->GetNumFrames()` per facing. A corrupt file with numFrames > 800 causes stack buffer overflow.
  *Fix: Validate `numFrames <= 800` before the read loops.*

- **spritefile.cpp:91-98** — `WriteSpriteData` declares `uint32 normal_ssizes[800]` and loops `for (i=0; i<s->GetNumFrames(); i++)`. A malformed sprite file could report `GetNumFrames() > 800`, causing a stack buffer overflow.
  *Fix: Assert or check `s->GetNumFrames() <= 800` before the loop; reject malformed sprites.*

- **spriteutils.cpp:496-557** — `spriteutils_DecodeToBuffer` performs no bounds checking on `table[j*2+1]` offsets, `rowData` pointer arithmetic, or `destPixel` writes. Malformed sprite data can cause arbitrary out-of-bounds reads and writes.
  *Fix: Validate that `j*2+1 < height*2`, compute `rowData` against `frameEnd`, and ensure `destPixel` stays within `outBuf + width*height`.*


### NULL_DEREFERENCE

- **ArmyData.cpp:2963** — In `CanSlaveRaid`, if `m_nElements == 0`, the loop `for(i = 0; i < m_nElements; i++)` never executes, leaving `data == NULL`. The next line dereferences `data->GetChance()`, causing a null pointer dereference.
  *Fix: Add an early return or explicit `if (m_nElements == 0) return false;` before the loop, or initialize `data` and check for NULL before dereferencing.*

- **SlicSegment.cpp:441** — `SlicSegment::Serialize` allocates `m_id` with `malloc(l)` and immediately calls `archive.Load((uint8*)m_id, l)` without checking the malloc return value. On OOM this writes to NULL.
  *Fix: Add `if (!m_id) return;` or equivalent error handling after each `malloc`.*

- **SlicSegment.cpp:443** — `m_code = (uint8*)malloc(m_codeSize);` is followed by `archive.Load((uint8*)m_code, m_codeSize);` with no NULL check.
  *Fix: Verify `m_code != NULL` before the archive load.*

- **SlicSegment.cpp:456** — `m_uiComponent = (char *)malloc(l);` followed by `archive.Load((uint8*)m_uiComponent, l);` without NULL validation.
  *Fix: Check `m_uiComponent` before loading.*

- **SlicSegment.cpp:471** — `m_filename = (char *)malloc(l + 1);` followed by `archive.Load((uint8*)m_filename, l);` without NULL check.
  *Fix: Verify `m_filename != NULL` before loading.*

- **foreigner.cpp:1493** — `ComputeLandContinentShared` checks `if (NULL == m_known_cities) { continents_shared = 0.0; }` but does **not** return. Execution falls through to line 1540 where `m_known_cities->First(id)` is called, dereferencing the null pointer and crashing.
  *Fix: Add `return;` after `continents_shared = 0.0;` inside the null check.*

- **slicobject.cpp:100** — `SlicObject::SlicObject(char const * id)` calls `strlen(id)` in the initializer list without checking if `id` is NULL. A NULL pointer passed from script loading or message pool code causes an immediate crash.
  *Fix: Add a NULL check before `strlen(id)`; if `id` is NULL, set `m_id = NULL` and handle gracefully.*

- **slicobject.cpp:135** — `SlicObject::SlicObject(SlicSegment *segment)` calls `strlen(segment->GetName())` without verifying `segment` or `segment->GetName()` is non-NULL.
  *Fix: Add NULL checks for `segment` and `segment->GetName()` before `strlen`.*

- **slicobject.cpp:553** — In `SlicObject::Serialize`, the storing branch calls `strlen(m_id)` but the default constructor sets `m_id = NULL`. Serializing a default-constructed object dereferences NULL.
  *Fix: Guard `strlen(m_id)` with `if (m_id)`; store `l = 0` when `m_id` is NULL.*


### UNINITIALIZED_VARIABLE

- **sourcelist.cpp:133** — `m_step`, `m_stepInto`, and `m_status` are never initialized in the `SourceList` constructor. If `Initialize()` returns early (which it can at multiple points), these pointers contain garbage. The destructor then unconditionally `delete`s them, causing undefined behavior or a crash.
  *Fix: Initialize `m_step = NULL; m_stepInto = NULL; m_status = NULL;` in the constructor body alongside `m_continue`, `m_list`, and `m_exit`.*


## HIGH Severity


### SAVE_LOAD_AI_DETERMINISM

- **gs/fileio/GameFile.cpp** — Save+load does not round-trip enough engine state to make load deterministic. A game saved at turn N and resumed for M more turns diverges from a continuous run of N+M turns: player scores, gold, num_cities, and the set of founded cities drift. `g_rand` is serialized (`GameFile.cpp:355`); the divergence appears to come from AI strategy / Goal / Plan / Strategist evaluation state that is rebuilt from scratch rather than restored. Symptom is newly visible after commit `169999b7` repaired the load path (which previously silently `exit(0)`-ed mid-init, masking this issue). The planned save format rework (SQLite-backed, structured rather than raw archive) is expected to address this; see refactoring plan. Until then, `test_save_load.cpp` reports drift via `MESSAGE("WARN: ...")` without failing.
  *Fix scope: identify which subsystems' `Serialize()` methods omit AI-decision-driving state, or replace the archive-based format with a structured representation that captures the full deterministic input set.*


### BUFFER_OVERFLOW

- **net_thread.cpp:265** — In `NetThread::Run`, a static stack buffer is used: `static uint8 buf[dp_MAXLEN_UNRELIABLE * 2]; memcpy(buf, packet->m_buf, packet->m_len);`. There is no check that `packet->m_len <= dp_MAXLEN_UNRELIABLE * 2`. A large or corrupted packet length causes a stack buffer overflow.
  *Fix: Add `Assert(packet->m_len <= dp_MAXLEN_UNRELIABLE * 2); if (packet->m_len > dp_MAXLEN_UNRELIABLE * 2) { delete packet; continue; }` before the `memcpy`.*

- **net_thread.cpp:499** — Compressed packet handling reads `uLongf uSize = getlong(&packet->m_buf[1]);` and calls `uncompress(uBuf, &uSize, &packet->m_buf[5], packet->m_len - 5);` without first verifying `packet->m_len >= 5`. If `packet->m_len` is smaller than 5, `packet->m_len - 5` underflows (signed subtraction of a positive sint32 producing a large negative value, then passed as `uLong`). The `getlong` call also reads beyond the buffer boundary.
  *Fix: Add `Assert(packet->m_len >= 5); if (packet->m_len < 5) { delete packet; continue; }` at the start of the compressed-packet branch.*


### DANGEROUS_SHIFT

> **Resolution status — all first-party HIGH DANGEROUS_SHIFT findings verified in code 2026-07-02. Net result: 0 open (first-party).**
> Findings are static-analysis leads, not verdicts; line numbers had drifted since May 2026. `safety.h` provides `safe_shift_left_u32/u64` (bounds-checked, returns 0 on out-of-range). `k_MAX_PLAYERS == 32`, so a player-index shift can reach `1 << 31`, which on a signed `int` is genuinely UB — fixed by making the literal unsigned (`1u << idx`), behaviour-identical for valid indices.
>
> **Already fixed before this pass (verified present):** `ArmyData.cpp` 1487/4107 (now `safe_shift_left_u64`, guarded); `CityData.cpp` 979/988/998/1007/1716/2162/2177/5007/5911 (converted to `safe_shift_left_u64`); `CityData.cpp:4793` wonder shift (guarded `type < 64`); `SlicFunc.cpp:6800` wonder shift (guarded `wonder < 64`); `UnitActor.cpp` 2397-2402/2432 (loops bounded `< 64`); `cellunitlist.cpp:287` (guarded `owner < 32`); `scheduler.cpp` 1726/1767/1805 (loop `< k_MAX_PLAYERS`, extra `< 32` guard, `uint32` cache, `1u<<i`).
>
> **Fixed this pass (commit pending):**
> - Player-index signed-shift UB → `1u << idx`: `cellunitlist.cpp` 319/390; `ArmyData.cpp` 17 visibility/owner masks; `armyevent.cpp:1284`; `CityData.cpp` 497/1261/6016/7683; `slicfunc.cpp:841`; `UnitActor.cpp` 2095/2130/2164; `c3cmdline.cpp` DebugMask stray `1<<bit`.
> - Unbounded DB-record / console-input shifts → `safe_shift_*`: `slicfunc.cpp` building-mask `(1 << NumRecords()) - 1` ×2 (→ `safe_shift_left_u64`, which yields all-ones at 64 — the correct mask); `c3cmdline.cpp` CreateImprovement (2961) and ChatMask (3055) shift by raw `atoi(argv)`.
>
> **Deferred (networking, per plan ground rules):** `net_cheat.cpp:150`, `net_info.cpp:1260`, `net_thread.cpp` buffer overflows.
> **Not bugs (skipped):** direction-flag shifts (`1 << NORTH … 1 << dd`, amount ≤ 7).
>
> Verified with `mise exec -- make build`, `make test` (ratchet unchanged), and `make ubsan-smoke` (all green).

- **ArmyData.cpp:1487** — `~(1 << m_array[0].GetOwner())` performs a left shift by `GetOwner()`. If the owner index is `>= 32`, the shift width exceeds the bit-width of `int`, causing undefined behavior.
  *Fix: Use a 32-bit mask and cap the shift: `uint32 mask = (m_array[0].GetOwner() < 32) ? (1u << m_array[0].GetOwner()) : 0;` or use `~((m_array[0].GetOwner() < 32) ? (1u << m_array[0].GetOwner()) : 0u)`.*

- **ArmyData.cpp:4107** — `((uint64)1 << (uint64)i)` where `i` iterates over `g_theWonderDB->NumRecords()`. If the wonder database contains 64 or more records, the shift amount is >= 64, causing undefined behavior.
  *Fix: Guard the shift with `if (i < 64)` or use a larger integer type (e.g., `__uint128_t` or a bitset) if more than 64 bits are needed.*

- **ArmyEvent.cpp:1228** — `(0x1 << army_owner)` shifts by `army_owner`. If `army_owner >= 32`, this is undefined behavior.
  *Fix: Guard the shift: `if (army_owner >= 0 && army_owner < 32)` before shifting.*

- **ArmyEvent.cpp:1228** — `(0x1 << army_owner)` where `army_owner` is a `PLAYER_INDEX` that could be >= 32. Shifting by 32 or more on a 32-bit integer is undefined behavior.
  *Fix: Use `((uint64)1 << army_owner)` if the value can exceed 31, or add a guard `if (army_owner < 32)`.*

- **CityData.cpp:1007** — `((uint64)1 << i)` where `i` may be >= 64.
  *Fix: Add a bounds check before shifting.*

- **CityData.cpp:1716** — `buildingCheck = (uint64)1 << (uint64)i;` shifts by `i`. If `g_theWonderDB->NumRecords()` returns `>= 64`, the shift width equals or exceeds the bit-width of `uint64`, which is undefined behavior.
  *Fix: Guard the shift: `if (i < 64) { buildingCheck = (uint64)1 << i; } else { buildingCheck = 0; }`*

- **CityData.cpp:1716** — `((uint64)1 << w)` where `w` iterates over wonders. If wonder count >= 64, shift is UB.
  *Fix: Guard with `if (w < 64)`.*

- **CityData.cpp:2162** — `m_built_improvements & ((uint64)1 << b)` — if `g_theBuildingDB->NumRecords() >= 64`, shift by `>= 64` is undefined behavior.
  *Fix: Ensure `b < 64` before shifting.*

- **CityData.cpp:2162** — `((uint64)1 << i)` where `i` iterates over building records. Can be >= 64.
  *Fix: Add a guard `if (i < 64)` before shifting.*

- **CityData.cpp:2177** — `m_builtWonders & ((uint64)1 << w)` — if `g_theWonderDB->NumRecords() >= 64`, shift by `>= 64` is undefined behavior.
  *Fix: Ensure `w < 64` before shifting.*

- **CityData.cpp:2177** — `((uint64)1 << i)` where `i` iterates over building records. Can be >= 64.
  *Fix: Guard the shift with `if (i < 64)`.*

- **CityData.cpp:5007** — `((uint64)1 << i)` where `i` may be >= 64.
  *Fix: Guard with `if (i < 64)`.*

- **CityData.cpp:5911** — `((uint64)1 << i)` where `i` may be >= 64.
  *Fix: Guard with `if (i < 64)`.*

- **CityData.cpp:979** — `((uint64)1 << i)` where `i` iterates up to `g_theBuildingDB->NumRecords()`. If the building database has >= 64 entries, the shift amount is >= 64, causing undefined behavior.
  *Fix: Guard with `if (i < 64)` or use a bitset/dynamic bitset for building tracking.*

- **CityData.cpp:988** — `((uint64)1 << i)` where `i` can be >= 64 if the building database is large.
  *Fix: Add a bounds check before shifting, or use a wider integer type / bitset.*

- **CityData.cpp:998** — `((uint64)1 << i)` where `i` may be >= 64.
  *Fix: Guard the shift with `if (i < 64)`.*

- **SlicFunc.cpp:6800** — `city.GetCityData()->GetBuiltWonders() & ((uint64)1 << wonder)` performs a shift without bounds-checking `wonder`. If `wonder` is >= 64 (or negative), the shift is undefined behavior and can crash or produce incorrect results.
  *Fix: Add `if (wonder < 0 || wonder >= 64) return SFN_ERROR_OUT_OF_RANGE;` before the shift.*

- **UnitActor.cpp:2397-2402** — `(uint64)1 << b` is performed where `b` loops up to `g_theBuildingDB->NumRecords()`. If the database contains 64 or more records, the shift amount equals or exceeds the 64-bit width, causing undefined behavior.
  *Fix: Add a guard `if (b < 64)` before the shift, or use a larger bitmask type / bitset.*

- **UnitActor.cpp:2432** — `(uint64)1 << (uint64)i` where `i` loops up to `g_theWonderDB->NumRecords()`. Same issue as above—shift amount can be >= 64.
  *Fix: Add `if (i < 64)` guard before shifting.*

- **c3cmdline.cpp:2984** — `((uint64)1 << atoi(argv[1]))` performs a shift without validating the shift amount. If `argv[1]` is >= 64, this is undefined behavior.
  *Fix: Validate `sint32 idx = atoi(argv[1]); if (idx < 0 || idx >= 64) return;` before shifting.*

- **c3cmdline.cpp:3002** — `(1 << bit)` where `bit = atoi(argv[1])`. If `bit` is >= 32 (the width of `int`), this is undefined behavior.
  *Fix: Validate `if (bit < 0 || bit >= 32) return;` before shifting.*

- **cellunitlist.cpp:287** — `uint32 b = (0x00000001 << owner);` performs a left shift on a signed `int` literal. If `owner == 31`, shifting `1` left by 31 is implementation-defined (signed overflow). If `owner >= 32`, it is undefined behavior. `owner` comes from `PLAYER_INDEX` which can be any integer value.
  *Fix: Use an unsigned literal: `uint32 b = (1u << owner);` and add a bounds check (`owner < 32`) before shifting.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:2401** — `unit.CD()->GetImprovements() & ((uint64)1 << b)` shifts a 64-bit integer by `b`, where `b` is a `sint32` loop index up to `g_theBuildingDB->NumRecords() - 1`. If the database has 64 or more records, the shift amount is >= 64, which is undefined behavior in C++.
  *Fix: Use a bounds check `if (b >= 64) continue;` or use a dynamically sized bitset instead of a single uint64.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:2432** — `unit.CD()->GetBuiltWonders() & (uint64)1 << (uint64)i` shifts by `i` (up to `g_theWonderDB->NumRecords() - 1`). Shifting by 64 or more bits is undefined behavior.
  *Fix: Guard with `if (i >= 64) continue;` or use a wider/variable bitset.*

- **net_cheat.cpp:150** — `((uint64)1 << m_data[1])` performs a shift by `m_data[1]` bits. If `m_data[1] >= 64`, this is undefined behavior.
  *Fix: Add a check `if (m_data[1] < 64)` before shifting, or mask the shift amount.*

- **net_info.cpp:1260** — `cd->SetWonders(cd->GetBuiltWonders() | ((uint64)1 << (uint64)m_data));` shifts a 64-bit value by `m_data` bits. `m_data` comes from the network packet. If `m_data >= 64`, this is undefined behavior and may crash or produce incorrect results.
  *Fix: Validate `m_data < 64` before performing the shift.*

- **scheduler.cpp:1726** — `m_contactCache |= (1<<i);` shifts a 32-bit `int` literal by `i`, where `i` iterates up to `k_MAX_PLAYERS`. If `k_MAX_PLAYERS` exceeds 31, the shift is undefined behavior in C++.
  *Fix: Use a 64-bit mask type and shift a `uint64_t` value: `m_contactCache |= (1ULL << i);`, or assert `i < 32` before shifting.*

- **scheduler.cpp:1767** — `m_neutralRegardCache |= (1<<i);` has the same undefined shift behavior when `i >= 32`.
  *Fix: Change to `m_neutralRegardCache |= (1ULL << i);` and ensure the cache type is at least 64 bits.*

- **scheduler.cpp:1805** — `m_allyRegardCache |= (1<<i);` is vulnerable to the same undefined shift when `i >= 32`.
  *Fix: Change to `m_allyRegardCache |= (1ULL << i);` with a 64-bit cache type.*


### DIVISION_BY_ZERO

> **Resolution status — all first-party HIGH DIVISION_BY_ZERO findings verified in code 2026-07-02. Net result: 0 open (first-party).**
> Line numbers had drifted since May 2026; `safety.h` provides `safe_divide`/`safe_divide_double`.
>
> **Fixed this pass (commit pending):** `SpriteLow565.cpp` `DrawScaledLow565`/`DrawFlashScaledLow565` — `(m_height - destHeight) / (double)destHeight` cast to `sint32` was UB when `destHeight == 0` (nan/inf → int). Guarded inline (`destHeight != 0 ? … : 0`); the surrounding scale loop already no-ops at `destHeight == 0` (`vend == vdestpos`), so the guard fully closes it.
>
> **Already fixed before this pass (verified present):** `CityData.cpp` ring food/prod/gold (all `totalX > 0 ? … : 0.0`); `FeatTracker.cpp` (`safe_divide`); `Readiness.cpp` (`safe_divide_double`); `SlicFunc.cpp` Pillage/Plunder (`if (modifier == 0) return SFN_ERROR_INTERNAL`); `UnitData.cpp` armor (`armor > 0.0 ? …`), attack/(attack+defense) (guarded), `a/(a+d)` (runtime guard replacing the stripped `Assert`); `aui_listbox.cpp` (`m_maxItemHeight > 0 ? …`, and `DragSelect` is unreachable at height 0 via the `y < maxY` test); `c3slider.cpp` (`if (m_maxY == m_minY) return`); `director.cpp` (`totalHP > 0 ? …`); `soundmanager.cpp` (`if (trackRange <= 0) …`); `UnitActor.cpp:1550` (per CRITICAL note).
>
> **False positives (verified not bugs):** all `Barbarians.cpp` `civrand().Next(maxHut - 1)` sites — `RandomGenerator::Next(sint32 r)` (RandGen.h:48) self-guards with `if (r <= 0) return 0;` before `Next() % r`, so a zero/negative range never reaches the modulo. `UnitData::FightOneLineDanceAssault` `a/(a+d)` is in an explicitly `// Not used.` dead function.
>
> Verified with `make build`, `make test` (ratchet unchanged), `make ubsan-smoke` (all green).

- **Barbarians.cpp:159** — `g_rand->Next(count - rankMax) + rankMax` calls `Next(0)` when `count == rankMax`. Most `RandomGenerator::Next(n)` implementations use modulo (`rand() % n`), which divides by zero when `n == 0`.
  *Fix: Ensure the argument is never zero: `sint32 range = count - rankMax; if (range <= 0) range = 1; sint32 whichbest = g_rand->Next(range) + rankMax;`*

- **Barbarians.cpp:193** — `g_rand->Next(g_theRiskDB->Get(...)->GetHutMaxBarbarians() - 1) + 1` calls `Next(-1)` or `Next(0)` when the database value is 0 or 1.
  *Fix: Clamp the argument: `sint32 max = g_theRiskDB->Get(...)->GetHutMaxBarbarians(); if (max < 1) max = 1; maxBarbarians = g_rand->Next(max - 1) + 1;`*

- **Barbarians.cpp:197** — Same pattern as above with `GetMaxSpontaniousBarbarians()`.
  *Fix: Clamp the argument to `Next()` to be `>= 1`.*

- **CityData.cpp:2585** — `static_cast<double>(m_ringFood[i]) / static_cast<double>(totalFood)` divides by `totalFood` without checking it is non-zero. `totalFood` can be 0 when a city produces no food, causing a division-by-zero crash.
  *Fix: Add `if (totalFood == 0) { m_farmersEff[i] = 0; } else { ... }`.*

- **CityData.cpp:2586** — Division by `totalProd` without a zero check. If the city produces no production, this crashes.
  *Fix: Guard with `if (totalProd != 0)`.*

- **CityData.cpp:2587** — Division by `totalGold` without a zero check. If the city produces no gold, this crashes.
  *Fix: Guard with `if (totalGold != 0)`.*

- **CityData.cpp:2588** — Division by `totalGold` (used for scientist efficiency) without a zero check.
  *Fix: Guard with `if (totalGold != 0)`.*

- **FeatTracker.cpp:548** — `sint32 havePercent = (numCities * 100) / g_player[city.GetOwner()]->m_all_cities->Num();` divides by the number of cities without checking for zero. A player with zero cities causes division by zero.
  *Fix: Check for zero before dividing: `sint32 num = g_player[...]->m_all_cities->Num(); if (num == 0) num = 1;`*

- **Readiness.cpp:266** — `m_delta = (GetReadyHP(gov, m_readinessLevel) - GetReadyHP(gov, oldLevel))/turns;` — `turns` is obtained from `GetTurnsToNewReadiness()`, which can return 0 if the database defines zero turns. This causes a divide-by-zero crash.
  *Fix: Guard with `if (turns > 0)` before the division.*

- **SlicFunc.cpp:7397** — In `Slic_Pillage::Call`, `amt = p / g_theGovernmentDB->Get(g_player[pl]->m_government_type)->GetBuildingRushModifier();` divides by the modifier value without checking if it is zero. A zero modifier causes a divide-by-zero crash.
  *Fix: Store the divisor in a local variable, check `if (divisor == 0) return SFN_ERROR_INTERNAL;` before dividing.*

- **SlicFunc.cpp:7457** — In `Slic_Plunder::Call`, the same unchecked division by `GetBuildingRushModifier()` is present. No guard against a zero return value.
  *Fix: Same as above — check the divisor before dividing.*

- **SpriteLow565.cpp:1340** — Same division by zero in `DrawFlashScaledLow565` when `destHeight == 0`.
  *Fix: Guard against `destHeight == 0` before the division.*

- **SpriteLow565.cpp:876** — `vpos2 = (sint32)((double)(m_height - destHeight) / (double)destHeight);` divides by zero when `destHeight == 0`.
  *Fix: Return early or skip scaling when `destHeight == 0`.*

- **UnitData.cpp:1403** — `sint32 f = (sint32)(rec->GetFirepower() / defender.GetDBRec()->GetArmor());` divides firepower by armor without checking if armor is zero. A unit with zero armor causes a divide-by-zero crash.
  *Fix: Add a zero check before dividing: `if (defender.GetDBRec()->GetArmor() == 0) f = 0; else f = (sint32)(rec->GetFirepower() / defender.GetDBRec()->GetArmor());`*

- **UnitData.cpp:1420** — `double prob = attack / (attack + defenseStrength);` computes a probability without validating that the denominator is non-zero. If both attack and defense strength are zero, this crashes.
  *Fix: Guard with `if (attack + defenseStrength <= 0.0) prob = 0.0; else prob = attack / (attack + defenseStrength);`*

- **UnitData.cpp:1567** — `sint32 p = sint32(double (0x0fff) * a/(a+d));` divides by `a+d` after only an `Assert(0.00001f < a+d)` which is stripped in release builds. If both values are zero in release, the game crashes.
  *Fix: Replace the assert with a runtime check: `if (a + d <= 0.00001f) p = 0; else p = sint32(double(0x0fff) * a / (a + d));`*

- **aui_listbox.cpp:1744** — `MouseRGrabInside` computes `sint32 itemIndex = y / m_maxItemHeight + ...` without checking whether `m_maxItemHeight` is zero. An empty listbox can leave this member at `0`, causing a division-by-zero crash.
  *Fix: Use the same guard as in `MouseLGrabInside`: `y / (m_maxItemHeight != 0 ? m_maxItemHeight : 1)`.*

- **aui_listbox.cpp:1982** — `MouseLDoubleClickInside` also computes `y / m_maxItemHeight` without a zero check, leading to the same division-by-zero vulnerability when the listbox is empty.
  *Fix: Use `y / (m_maxItemHeight != 0 ? m_maxItemHeight : 1)`.*

- **c3slider.cpp:256** — `sint32 spacing = m_height / ( m_maxY - m_minY ) - 1;` will divide by zero if `m_maxY == m_minY`.
  *Fix: Add a check `if (m_maxY == m_minY) return AUI_ERRCODE_INVALIDPARAM;` or handle the zero-range case explicitly.*

- **c3slider.cpp:273** — `sint32 spacing = m_width / ( m_maxX - m_minX ) - 1;` will divide by zero if `m_maxX == m_minX`.
  *Fix: Add a check `if (m_maxX == m_minX) return AUI_ERRCODE_INVALIDPARAM;` or handle the zero-range case explicitly.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1550** — `ratio = std::max(0.0, m_unitID.GetHP() / m_unitID->CalculateTotalHP());` divides by `CalculateTotalHP()` without checking for zero.
  *Fix: `sint32 totalHP = m_unitID->CalculateTotalHP(); ratio = (totalHP > 0) ? std::max(0.0, static_cast<double>(m_unitID.GetHP()) / totalHP) : 0.0;`.*

- **ctp2_code/gfx/spritesys/director.cpp:1457** — `attackerActor->SetHealthPercent(attacker.GetHP() / attacker->CalculateTotalHP())` performs integer division. If `CalculateTotalHP()` returns 0, this is a division-by-zero crash.
  *Fix: `sint32 totalHP = attacker->CalculateTotalHP(); double pct = (totalHP > 0) ? static_cast<double>(attacker.GetHP()) / totalHP : 0.0; attackerActor->SetHealthPercent(pct);`.*

- **ctp2_code/gfx/spritesys/director.cpp:1473** — `defenderActor->SetHealthPercent(defender.GetHP() / defender->CalculateTotalHP())` has the same unguarded division.
  *Fix: Guard against `CalculateTotalHP() == 0` before dividing.*

- **director.cpp:1457** — `attackerActor->SetHealthPercent(attacker.GetHP() / attacker->CalculateTotalHP());` divides by zero if the unit's total HP is zero.
  *Fix: Check `attacker->CalculateTotalHP() > 0` before dividing.*

- **director.cpp:1457-1458** — `attackerActor->SetHealthPercent(attacker.GetHP() / attacker->CalculateTotalHP());` divides by `CalculateTotalHP()` without checking for zero. If the calculation returns zero, the game crashes.
  *Fix: Check denominator before dividing: `sint32 totalHP = attacker->CalculateTotalHP(); attackerActor->SetHealthPercent(totalHP ? attacker.GetHP() / totalHP : 0);`*

- **director.cpp:1473** — `defenderActor->SetHealthPercent(defender.GetHP() / defender->CalculateTotalHP());` same zero-denominator risk.
  *Fix: Guard the division with a positive denominator check.*

- **director.cpp:1473-1474** — Same pattern as above: `defenderActor->SetHealthPercent(defender.GetHP() / defender->CalculateTotalHP());` with no zero check.
  *Fix: Guard the division with a zero check on `CalculateTotalHP()`.*

- **soundmanager.cpp:981** — In `PickNextTrack`, the expression `rand() % (m_numTracks - (1 + s_startTrack))` can divide by zero. `s_startTrack` is `1`, so when `m_numTracks == 2` the modulo operand becomes `0`.
  *Fix: Add a guard: `if (m_numTracks <= 1 + s_startTrack) { m_curTrack = s_startTrack; break; }` before the modulo.*


### FORMAT_STRING_BUG

- **AgreementData.cpp:1047** — `sprintf(s, "%s ... %s", s, ...)`: Overlapping memory regions in `sprintf` call.
  *Fix: Format into a temporary buffer first, then copy back to `s`.*

- **AgreementData.cpp:436** — `sprintf(s, "%s ... %s", s, ...)`: The destination buffer `s` is also passed as a source argument. `sprintf` does not guarantee safe in-place operation; this causes undefined behavior due to overlapping memory.
  *Fix: Use a temporary buffer for formatting, then `strcpy`/`strncpy` the result back into `s`.*

- **AgreementData.cpp:440** — `sprintf(s, "%s ... %s", s, ...)`: The destination buffer `s` is also passed as a source argument, causing potential buffer corruption/UB.
  *Fix: Use a temporary buffer for the formatted string, then copy it into `s`.*

- **AgreementData.cpp:444** — `sprintf(s, "%s ... %s", s, ...)`: Destination overlaps with source; `sprintf` may read from `s` while writing to it.
  *Fix: Format into a temporary buffer first, then copy back to `s`.*

- **AgreementData.cpp:448** — `sprintf(s, "%s ... %s", s, ...)`: Overlapping source/destination in `sprintf` causes undefined behavior.
  *Fix: Use a temporary buffer, then copy the result into `s`.*

- **AgreementData.cpp:452** — `sprintf(s, "%s ... %s", s, ...)`: The destination buffer `s` overlaps with the source argument.
  *Fix: Use a temporary buffer for formatting, then copy back.*

- **AgreementData.cpp:456** — `sprintf(s, "%s ... %s", s, ...)`: Overlapping memory in `sprintf` call causes undefined behavior.
  *Fix: Format into a temporary buffer first, then copy into `s`.*

- **AgreementData.cpp:460** — `sprintf(s, "%s ... %s", s, ...)`: Destination `s` overlaps with source argument `s`.
  *Fix: Use a temporary buffer for the formatted string, then copy back.*

- **AgreementData.cpp:464** — `sprintf(s, "%s ... %s", s, ...)`: Overlapping source and destination in `sprintf`.
  *Fix: Use a temporary buffer, then copy the result into `s`.*

- **GameEventManager.cpp:671** — `vsprintf(text, fmt, vl)` writes into a 1024-byte stack buffer (`char text[1024]`) with an unchecked format string and unchecked argument list. A malformed or unexpectedly long format/arguments will overflow the buffer.
  *Fix: Replace `vsprintf` with `vsnprintf(text, sizeof(text), fmt, vl)` and ensure `text` is null-terminated afterward.*

- **MessageData.cpp:1385** — `strcpy(m_caption, caption);` copies into fixed-size buffer `m_caption[k_MAX_MSG_LEN]` without length validation. An overlong `caption` causes buffer overflow.
  *Fix: Use `strncpy(m_caption, caption, k_MAX_MSG_LEN - 1); m_caption[k_MAX_MSG_LEN - 1] = '\0';`*

- **MessageData.cpp:283** — `strcpy(m_caption, copy->m_caption);` copies into fixed-size buffer without length check. If `copy->m_caption` is malformed (longer than expected), buffer overflow.
  *Fix: Use `strncpy` with explicit null termination.*

- **Player.cpp:5332** — `sprintf(s, "%s P%d, ", s, i)` uses buffer `s` as both the format argument (`%s`) and the destination. This is undefined behavior in C and can crash or corrupt the stack on some platforms.
  *Fix: Use a temporary buffer or `snprintf` with an offset: `snprintf(s + strlen(s), remaining, "P%d, ", i)`.*

- **c3debug.cpp:233** — `vsprintf(g_last_debug_text + strlen(g_last_debug_text), format, list)` appends to a 4096-byte buffer without length checking. A long format string can overflow the buffer.
  *Fix: Use `vsnprintf(g_last_debug_text + len, sizeof(g_last_debug_text) - len, format, list)`.*

- **c3errors.cpp:140** — `vsprintf(szFmtTmp, fmt, list)` writes into a computed pointer offset without length checking, potentially overflowing the allocated buffer.
  *Fix: Use `vsnprintf` with a calculated remaining size based on the allocation.*

- **c3errors.cpp:51** — `vsprintf(str, fmt, list)` writes into `str[_MAX_PATH]` without length checking. A caller-supplied format string with large arguments can overflow the buffer.
  *Fix: Use `vsnprintf(str, sizeof(str), fmt, list)`.*

- **c3errors.cpp:92** — `vsprintf(str, dbError, list)` into `str[_MAX_PATH]` can overflow if `dbError` is large.
  *Fix: Use `vsnprintf(str, sizeof(str), dbError, list)`.*

- **ctp2_code/gs/database/profileDB.cpp:609** — `c3errors_ErrorDialog` is called with format string `"Line %d: %s is an illegal value for %s"` (three specifiers) but only two arguments after the format: `value` and `var->m_name`. The `%d` consumes the `char*` pointer `value` as an int, and the third `%s` reads uninitialized/garbage from the stack.
  *Fix: Add the missing `linenum` argument: `c3errors_ErrorDialog("Profile", "Line %d: %s is an illegal value for %s", linenum, value, var->m_name);`*

- **netfunc.cpp:2573** — `strncpy(servername, s->GetName(), 64)` copies up to 64 bytes into `servername[64]`. If the source name is exactly 64 non-null characters, `servername` is left without a null terminator. Subsequent string operations on `servername` will read out of bounds.
  *Fix: Ensure null termination: `servername[63] = '\0';` after the `strncpy`, or use a smaller copy limit.*

- **netfunc.cpp:3312** — `PlayerCallBack` does `strcpy(((NETFunc *)context)->player.player.name, n);` without bounds checking. If the peer-provided name `n` is longer than `dp_PNAMELEN`, this overflows the `player.name` buffer.
  *Fix: Use `strncpy` with an explicit null-termination guarantee, or `snprintf`.*

- **ns_chatbox.cpp:105** — `sprintf(buf, "<%s%s:> %s\n", p->GetName(), suffix, m)` writes into a 256-byte stack buffer. The input message `m` is asserted to be <= 240 chars (debug only), and `p->GetName()` can be up to `dp_PNAMELEN` bytes (64+). The format string plus name, suffix, and message can easily exceed 256 bytes in release builds, causing a stack buffer overflow.
  *Fix: Use `snprintf(buf, sizeof(buf), ...)` instead of `sprintf`.*


### FORMAT_STRING_BUG/NULL_DEREFERENCE

- **civ3_main.cpp:1091** — `fopen("crash.txt", "r")` return value is passed directly to `c3debug_ExceptionStackTraceFromFile()` without a NULL check. If the file cannot be opened, the function will dereference NULL, crashing the process.
  *Fix: Check the return value of `fopen` before passing it to `c3debug_ExceptionStackTraceFromFile`.*


### INTEGER_OVERFLOW

- **Barbarians.cpp:193** — `g_rand->Next(g_theRiskDB->Get(...)->GetHutMaxBarbarians() - 1)`. If the database value is `0`, this evaluates to `g_rand->Next(-1)`. When `-1` is converted to an unsigned type inside `Next()`, it wraps to a very large number, causing an out-of-bounds random request.
  *Fix: Guard the calculation: `int maxBarbs = db->GetHutMaxBarbarians(); int count = (maxBarbs > 0) ? g_rand->Next(maxBarbs) : 0;`*

- **Barbarians.cpp:197** — `g_rand->Next(g_theRiskDB->Get(...)->GetHutMaxBarbarians() - 1)` can evaluate to `Next(-1)` when the database value is 0, causing unsigned integer wraparound and a massive out-of-bounds request.
  *Fix: Ensure the argument to `Next()` is non-negative before calling it.*

- **CityData.cpp:1992** — In `PayFederalProductionAbs`, `m_net_production -= mil_paid;` can underflow (signed integer overflow) when `mil_paid` is large and `m_net_production` is small. The sanity check `if (m_net_production < 0) m_net_production = 0;` happens after the subtraction.
  *Fix: Check before subtracting: `if (m_net_production < mil_paid) m_net_production = 0; else m_net_production -= mil_paid;`.*

- **CityData.cpp:760** — `memset` size is computed as `(uint32)&m_science_lost_to_crime + sizeof(...) - (uint32)&m_max_processed_terrain_food`. If pointer arithmetic yields a negative difference, the unsigned cast wraps to a huge value, causing `memset` to write far beyond the intended range.
  *Fix: Compute the size with `ptrdiff_t` and assert it is positive before casting to `size_t`.*

- **Gold.cpp:149** — `int newval = m_level + delta;` performs signed integer addition that can overflow before the overflow check on line 151. Signed integer overflow is undefined behavior in C++.
  *Fix: Check for overflow before addition using `if (delta > 0 && m_level > INT_MAX - delta)` or use unsigned arithmetic.*

- **MapFile.cpp:1181** — In `LoadHuts`, `w * h * sizeof(uint8)` is computed for size validation with `sint16` values. Multiplication can overflow `sint32`, causing the check to pass for a malformed buffer and leading to out-of-bounds reads.
  *Fix: Use safe multiplication with overflow checks or `size_t` arithmetic.*

- **MapFile.cpp:765** — In `LoadTerrain`, `w` and `h` are read as `sint16` from the buffer, then `w * h` is computed in the size check `if(size != (sint32)((w * h) + (sizeof(sint16) * 2)))`. If `w` and `h` are large (e.g., near 32767), `w * h` can overflow a 32-bit signed integer, causing the size check to pass incorrectly and subsequent out-of-bounds reads.
  *Fix: Use `size_t` for the multiplication or check each dimension against `sqrt(INT_MAX)` before multiplying.*

- **MapFile.cpp:833** — In `LoadTerrainEnv`, the size validation computes `w * h * sizeof(uint32)` where `w` and `h` are `sint16`. This multiplication can overflow a 32-bit `sint32`, bypassing the size check and allowing out-of-bounds buffer access.
  *Fix: Perform the size check using `size_t` arithmetic and validate each factor before multiplication.*

- **MaterialPool.cpp:62** — `if(m_level > 0 && amt > 0 && (m_level + amt) < 0)` relies on signed integer overflow to detect overflow. The expression `(m_level + amt)` is already UB if it overflows.
  *Fix: Use safe overflow check: `if (amt > 0 && m_level > INT_MAX - amt)`*

- **WrlEnv.cpp:556** — `array = new uint8[m_size.y * m_size.x];` multiplies two `sint32` values. If the map dimensions are large, the product can overflow, causing a heap buffer that is smaller than expected. Subsequent indexing into `array` then overflows the allocation.
  *Fix: Use `size_t` for the multiplication and check for overflow, e.g., with `if (m_size.x > 0 && m_size.y > SIZE_MAX / m_size.x)` before allocating.*

- **ctp2_code/gfx/gfx_utils/tiffutils.cpp:238** — `malloc(imageWidth * imageLength * 4)` multiplies three `uint32` values in 32-bit arithmetic. A large TIFF can cause the size to wrap around, leading to a heap allocation smaller than needed and subsequent buffer overflows.
  *Fix: `size_t allocSize = static_cast<size_t>(imageWidth) * static_cast<size_t>(imageLength) * 4; if (imageWidth && allocSize / imageWidth / 4 != imageLength) return NULL; char* outBuf = (char*)malloc(allocSize);`.*

- **ctp2_code/gfx/gfx_utils/tiffutils.cpp:45** — `size_t npixels = w * h;` where `w` and `h` are `uint32`. The multiplication is performed in 32-bit arithmetic before promotion to `size_t`; if `w * h` exceeds 2^32-1, it overflows.
  *Fix: Cast before multiplying: `size_t npixels = static_cast<size_t>(w) * static_cast<size_t>(h);`.*

- **ctp2_code/gfx/gfx_utils/tiffutils.cpp:51** — `char * destImage = (char *)malloc(npixels * sizeof(uint32));` uses the already-overflowed `npixels` value. Same root cause as the w*h overflow above.
  *Fix: Use the same safe 64-bit cast pattern before allocating.*

- **iparser.cpp:1049** — `strncpy(new_char_variable->string_value, &(string_symbols[5][1]), strlen(string_symbols[5])-2);` computes `strlen(...) - 2` using unsigned `size_t`. If the string length is 0 or 1, the subtraction underflows to a massive value (~SIZE_MAX), causing `strncpy` to copy far beyond the allocated buffer.
  *Fix: Check `strlen(string_symbols[5]) > 2` before calling `strncpy`, or use the allocated size as the count.*

- **iparser.cpp:1190** — `strncpy(assign_variable->string_value, &(string_symbols[1][1]), strlen(string_symbols[1])-2);` has the same unsigned underflow bug when `strlen(string_symbols[1]) < 2`.
  *Fix: Add a length check before computing `strlen(...) - 2`.*

- **iparser.cpp:1242** — `strncpy(new_variable->string_value, &(string_symbols[1][1]), strlen(string_symbols[1])-2);` in `Parse_Pound_Define` underflows when `strlen(string_symbols[1]) < 2`.
  *Fix: Guard the subtraction with a length check.*

- **iparser.cpp:1308** — `strncpy(file_name, &(string_symbols[0][1]), file_name_length-2);` where `file_name_length = strlen(string_symbols[0])`. If the string is 0 or 1 characters, `file_name_length - 2` underflows to a huge unsigned value.
  *Fix: Ensure `file_name_length >= 2` before subtracting.*

- **iparser.cpp:1564** — `symbol_len = strlen(string_symbols[(*current_symbol)])-2;` underflows to a huge `size_t` when the string is shorter than 2 characters. The subsequent `strncpy(buf, ..., symbol_len)` then copies an unbounded amount of data.
  *Fix: Check string length before subtracting 2.*

- **net_thread.cpp:479** — `getshort(&packet->m_buf[packet->m_len - 2])` can underflow if `packet->m_len < 2`, causing a read from a huge negative offset (or wrapping to a large positive offset).
  *Fix: Validate `packet->m_len >= 2` before computing the offset.*

- **net_thread.cpp:504** — `packet->m_len - 5` underflows if `packet->m_len < 5`, causing `uncompress` to receive a huge length value.
  *Fix: Validate `packet->m_len >= 5` before the subtraction.*

- **net_thread.cpp:504** — `uncompress(..., packet->m_len - 5, ...)` computes `packet->m_len - 5` where `m_len` is a `sint32`. If the compressed packet is malformed and `m_len < 5`, the subtraction underflows to a large positive value, passing a huge length to `uncompress`.
  *Fix: Add `if (packet->m_len < 5) { delete packet; continue; }` before the `uncompress` call.*

- **network.cpp:3913** — `ChunkList` computes `sint32 mapBufSize = a_List->GetCount() * 258 + 16384;`. `GetCount()` returns a `size_t` which can be very large. The multiplication by 258 can overflow a signed `sint32`, causing a negative or small positive `mapBufSize`. A subsequent `new uint8[mapBufSize]` with an overflowing value leads to heap corruption or a too-small buffer being used.
  *Fix: Use `size_t` for the calculation, check for overflow, or cap the list size: `size_t mapBufSize = static_cast<size_t>(a_List->GetCount()) * 258 + 16384;`*

- **network.cpp:3913** — `sint32 mapBufSize = a_List->GetCount() * 258 + 16384;` can overflow if `GetCount()` is large, leading to a much smaller buffer being allocated than needed. Subsequent writes overflow the heap buffer.
  *Fix: Use `size_t` for the calculation and check for overflow, or cap `GetCount()`.*

- **spriteutils.cpp:397** — `new Pixel16[(1 + height + width * height) * 8]` computes the allocation size with unchecked `uint16` multiplication. `width * height` can overflow, wrapping to a small value and causing a tiny allocation. Subsequent encoding writes far past the buffer.
  *Fix: Compute the size with `size_t` and check for overflow before `new`.*

- **spriteutils.cpp:593** — `malloc(destWidth * destHeight * sizeof(Pixel32))` multiplies untrusted dimensions without overflow checks. Overflow leads to a small allocation and subsequent out-of-bounds writes.
  *Fix: Use `size_t` math and validate `destWidth > 0 && destHeight <= SIZE_MAX / destWidth / sizeof(Pixel32)`.*

- **tiffutils.cpp:101** — `char * rasterPtr = raster + (bytesPerRow * (h-1));` when `h == 0`, `h-1` underflows (uint32 wrap) producing a gigantic offset and out-of-bounds pointer.
  *Fix: Explicitly check `h > 0` before computing `rasterPtr`.*

- **tiffutils.cpp:152** — `sint32 bytesPerRow = w * 4;` same signed-overflow issue as line 98 in `TIFLoadIntoBuffer16`.
  *Fix: Use `size_t` and validate `w`.*

- **tiffutils.cpp:161** — `char * rasterPtr = raster + (bytesPerRow * (h-1));` same underflow when `h == 0`.
  *Fix: Guard with `if (h == 0) return 0;` before pointer math.*

- **tiffutils.cpp:237-238** — `malloc(imageWidth * imageLength * 4)` multiplies three untrusted `uint32` values without overflow checks. Overflow produces a small allocation, and the subsequent strip decode loop writes out of bounds.
  *Fix: Use `size_t` math and check `if (imageWidth > 0 && imageLength > SIZE_MAX / imageWidth / 4) return NULL;`.*

- **tiffutils.cpp:238** — `malloc(imageWidth * imageLength * 4)` multiplies three attacker-controlled values without overflow checks, potentially allocating a smaller buffer than expected.
  *Fix: Use `uint64_t` to compute the size, check for overflow, and verify each dimension is reasonable.*

- **tiffutils.cpp:45** — `size_t npixels = w * h;` multiplies two `uint32` values from the TIFF. On 32-bit platforms the product can overflow `size_t`, causing a tiny allocation. The subsequent `memcpy(destImage, raster, npixels * sizeof(uint32))` then writes far past the buffer.
  *Fix: Check for overflow before allocating: `if (w > 0 && h > SIZE_MAX / w / sizeof(uint32)) return NULL;`*

- **tiffutils.cpp:85** — Same `w * h` overflow issue in `TIF2mem`.
  *Fix: Use 64-bit math and validate dimensions before allocation.*

- **tiffutils.cpp:98** — `sint32 bytesPerRow = w * 4;` multiplies an untrusted `uint32` image width by 4 and stores it in a signed 32-bit integer. For widths > 0x3FFFFFFF the multiplication overflows, producing a negative `bytesPerRow` that corrupts all pointer arithmetic in the row loop.
  *Fix: Use `size_t bytesPerRow = static_cast<size_t>(w) * 4;` and check for overflow.*

- **tiffutils.cpp:98** — `sint32 bytesPerRow = w * 4;` multiplies a `uint32` by 4 into a signed 32-bit variable, which can overflow or become negative.
  *Fix: Use `size_t bytesPerRow = static_cast<size_t>(w) * 4;` and validate `w`.*


### INTEGER_OVERFLOW / MISSING_BOUNDS_CHECK

- **targautils.cpp:172** — `data + ((Buffer_Width * height) - Buffer_Width)` performs a signed multiplication that can overflow, and the resulting pointer is not validated to lie within the caller-supplied buffer.
  *Fix: Use `size_t` arithmetic, check for overflow, and verify `height * Buffer_Width` does not exceed the buffer size.*

- **targautils.cpp:176** — `fread(dataPtr, width * bpp, 1, fp)` multiplies attacker-controlled `width` by `bpp` without overflow checks, and does not verify the write fits into the destination buffer.
  *Fix: Check `width` against `Buffer_Width` and use `size_t` math with overflow guards.*

- **targautils.cpp:293** — In `Load_TGA_File`, the same `Buffer_Width * height` multiplication and subsequent `fread(dataPtr, width * 2, ...)` lack overflow and bounds checks.
  *Fix: Validate dimensions and use `size_t` overflow-safe arithmetic.*


### LOGIC_ERROR

- **Player.cpp:9693** — `Player::MergeCivs()` iterates over `g_player[Mergee]->m_all_cities` but then calls `m_all_cities->Get(c)` (the Merger's city list) to get `CityData`. Since the city belongs to Mergee, not Merger, this looks up the wrong player's city list, likely returning invalid data or failing to find the city.
  *Fix: Change `m_all_cities->Get(c)` to `g_player[Mergee]->m_all_cities->Get(c)`.*


### MISSING_BOUNDS_CHECK

- **ArmyData.cpp:1029** — `m_array[0]` is accessed in `IsAsleep()` without first checking `m_nElements > 0`.
  *Fix: Add an early return `if (m_nElements == 0) return false;` before accessing `m_array[0]`.*

- **ArmyData.cpp:1062** — `m_array[0]` is accessed in `IsEntrenched()` without verifying `m_nElements > 0`.
  *Fix: Add a check `if (m_nElements == 0) return false;` before accessing `m_array[0]`.*

- **ArmyData.cpp:1068** — `m_array[0]` is accessed in `IsFortified()` without checking `m_nElements > 0`.
  *Fix: Add a guard `if (m_nElements == 0) return false;` before accessing `m_array[0]`.*

- **ArmyData.cpp:1413** — `m_array[0]` is accessed in `GetActiveDefenders()` without verifying `m_nElements > 0`.
  *Fix: Add a bounds check before accessing `m_array[0]`.*

- **ArmyData.cpp:7154** — `m_array[0]` is accessed in `MoveIntoForeigner()` without checking `m_nElements > 0`.
  *Fix: Add a guard `if (m_nElements == 0) return;` before accessing `m_array[0]`.*

- **ArmyData.cpp:7177** — `m_array[0]` is accessed in `MoveIntoForeigner()` without verifying `m_nElements > 0`.
  *Fix: Add a bounds check before accessing `m_array[0]`.*

- **ArmyEvent.cpp:1383** — In `LawsuitEvent`, `cell->AccessUnit(0)` is called before checking whether `cell->GetNumUnits() > 0`.
  *Fix: Add a check `if (cell->GetNumUnits() > 0)` before calling `cell->AccessUnit(0)`.*

- **C3GameState.cpp:548** — `return (*city.GetData()->GetCityData()->GetCollectingResources())[resource];` indexes into a resource collection array using the `resource` parameter with no bounds validation.
  *Fix: Validate `resource` against the known size of the collecting resources array before indexing.*

- **C3Player.cpp:730** — In `CancelCityRoutesToPlayer`, the second loop iterates over `dest` but uses `route_num = src->Num()` as the loop bound. If `src` has more elements than `dest`, `dest->Access(route_idx)` reads out of bounds.
  *Fix: Change `route_num = src->Num();` to `route_num = dest->Num();`*

- **C3Science.cpp:163** — `GetCanAskFor` copies `g_theAdvanceDB->NumRecords()` bytes into the caller-supplied `askable` buffer via `memcpy`. If the caller allocates a smaller buffer, this is a heap overflow.
  *Fix: Accept and validate a `maxSize` parameter, or return the required size to the caller.*

- **C3Science.cpp:173** — `GetCanOffer` copies `g_theAdvanceDB->NumRecords()` bytes into `offerable` without verifying the buffer is large enough.
  *Fix: Same as above—add size validation.*

- **C3Trade.cpp:111** — `g_player[m_aiPlayer]` is dereferenced without validating `m_aiPlayer` is within `[0, k_MAX_PLAYERS)`. An invalid trade offer index can index out of bounds.
  *Fix: Add `if (m_aiPlayer < 0 || m_aiPlayer >= k_MAX_PLAYERS) return;`.*

- **CityData.cpp:2193** — In `CollectResources`, `GetRing(it.Pos())` returns a ring index used to access `m_ringFood[ring]`, `m_ringProd[ring]`, etc. There is no validation that the returned ring is within `[0, maxRing)`.
  *Fix: Validate the ring index before using it to access the arrays, or ensure `GetRing` returns a safe default.*

- **Cont.cpp:419** — `m_land_armies[cont]++` and `m_water_armies[cont]++` use `cont` from `GetContinent()` without runtime bounds check in release builds. `GetContinent()` only validates in `_DEBUG`.
  *Fix: Add runtime bounds check before array access.*

- **Cont.cpp:443** — `m_land_cities[cont]++` and `m_water_cities[cont]++` use `cont` without runtime bounds validation.
  *Fix: Add runtime bounds check.*

- **DB.cpp:145-151** — `Database<T>::Access(const sint32 i)` only asserts bounds but never validates them in release builds, directly returning `&m_rec[i]`. Negative or oversized `i` causes out-of-bounds access.
  *Fix: Add the same guard as `Get()`: `if(i < 0 || i >= m_nRec) return NULL;`.*

- **DB.cpp:150** — `Database<T>::Access(sint32 i)` returns `&(m_rec[i])` without any bounds check, unlike `Get()` which validates `0 <= i < m_nRec`. Callers of `Access()` can trigger out-of-bounds memory access with any invalid index.
  *Fix: Add the same bounds check as `Get()`: `if (i < 0 || i >= m_nRec) return NULL;`.*

- **DiplomaticRequest.cpp:46** — `sint32 r = GetRecipient()` is used to index `g_player[r]` without validating that `r` is in the range `[0, k_MAX_PLAYERS)`. A malformed or corrupted request object could have an out-of-bounds recipient index, causing undefined behavior or memory corruption.
  *Fix: Add bounds check before indexing: `if (r >= 0 && r < k_MAX_PLAYERS && g_player[r])`*

- **DiplomaticRequest.cpp:52** — `sint32 o = GetOwner()` is used to index `g_player[o]` without validating that `o` is in the range `[0, k_MAX_PLAYERS)`. Same risk as above.
  *Fix: Add bounds check before indexing: `if (o >= 0 && o < k_MAX_PLAYERS && g_player[o])`*

- **DiplomaticRequestData.cpp:408** — `g_player[m_owner]` and `g_player[m_recipient]` are dereferenced after only a NULL check. If `m_owner` or `m_recipient` are negative or >= `k_MAX_PLAYERS`, the NULL check is itself undefined behavior (out-of-bounds array access).
  *Fix: Validate indices before access: `if (m_owner >= 0 && m_owner < k_MAX_PLAYERS && g_player[m_owner] && ...)`*

- **DiplomaticRequestData.cpp:408** — `m_owner` and `m_recipient` default to `PLAYER_INDEX_INVALID` (-1) in constructors. `Enact()` uses them directly as array indices (`g_player[m_owner]`, `g_player[m_recipient]`) without validating they are within `[0, k_MAX_PLAYERS)`. In release builds `Assert` is disabled, causing out-of-bounds reads.
  *Fix: Add explicit bounds checks before indexing: `if (m_owner < 0 || m_owner >= k_MAX_PLAYERS || m_recipient < 0 || m_recipient >= k_MAX_PLAYERS) return;`*

- **DiplomaticRequestData.cpp:692** — `GetAttitude()` indexes `g_player[p1]` and `g_player[p2]` after only `Assert` checks. In release builds, invalid player indices cause out-of-bounds memory access.
  *Fix: Replace `Assert` with runtime bounds checks and NULL checks before indexing.*

- **DiplomaticRequestData.cpp:693** — `GetAttitude()` accesses `g_player[p1]` and `g_player[p2]` with only `Assert()` macros for validation. In release builds, invalid player indices (e.g., from corrupted save data or network packets) cause out-of-bounds memory access.
  *Fix: Replace Asserts with runtime bounds checks: `if (p1 < 0 || p1 >= k_MAX_PLAYERS || !g_player[p1]) return ATTITUDE_TYPE_NEUTRAL;`*

- **DiplomaticRequestData.cpp:727** — `Reject()` accesses `g_player[m_owner]`, `g_player[m_recipient]`, and `g_player[m_thirdParty]` (lines 784, 797) without bounds checks. Invalid indices lead to UB.
  *Fix: Add bounds validation before all `g_player[index]` accesses in this function.*

- **DiplomaticRequestData.cpp:783** — `m_thirdParty` defaults to `PLAYER_INDEX_INVALID` (-1). In `Reject()`, case `REQUEST_TYPE_DEMAND_STOP_TRADE`, it is used as `g_player[m_thirdParty]` without bounds validation. This accesses `g_player[-1]`, an out-of-bounds read.
  *Fix: Check `m_thirdParty >= 0 && m_thirdParty < k_MAX_PLAYERS` before indexing `g_player`.*

- **DiplomaticRequestData.cpp:798** — Same issue as line 783 in case `REQUEST_TYPE_DEMAND_ATTACK_ENEMY`: `g_player[m_thirdParty]` is dereferenced without validating `m_thirdParty` is a valid player index.
  *Fix: Add the same bounds check before accessing `g_player[m_thirdParty]`.*

- **FeatTracker.cpp:314** — `m_achieved[type] = true` indexes the `m_achieved` array without validating `type` against `g_theFeatDB->NumRecords()`. A malicious or corrupted `type` value causes out-of-bounds write.
  *Fix: Add check: `if (type < 0 || type >= g_theFeatDB->NumRecords()) return;`*

- **FeatTracker.cpp:314** — `m_achieved[type] = true;` indexes the `m_achieved` array using the `type` parameter without verifying it is within `[0, g_theFeatDB->NumRecords())`.
  *Fix: Add `Assert(type >= 0 && type < g_theFeatDB->NumRecords())` or an explicit bounds check.*

- **FeatTracker.cpp:381** — `g_player[player]->m_score->AddFeat()` dereferences `g_player[player]` without validating `player` is in `[0, k_MAX_PLAYERS)`.
  *Fix: Add bounds check before indexing `g_player`.*

- **FeatTracker.cpp:390** — `g_player[player]->AddFeatHPBonus(hpBonus)` dereferences `g_player[player]` without bounds validation.
  *Fix: Add bounds check before indexing `g_player`.*

- **FeatTracker.cpp:527** — `g_player[city.GetOwner()]->m_all_cities->Num()` accesses `g_player` with `city.GetOwner()` which could be negative or >= `k_MAX_PLAYERS`.
  *Fix: Validate `city.GetOwner()` before indexing.*

- **FeatTracker.cpp:570** — `g_player[defeated]->GetMaxCityCount()` indexes `g_player` with the `defeated` parameter without bounds validation.
  *Fix: Add bounds check: `if (defeated >= 0 && defeated < k_MAX_PLAYERS && g_player[defeated])`*

- **GWCivRecord.cpp:267** — `strcpy(dataBuffer[counter].name, current->name);` copies into a fixed-size `GWUnitRecord` field without bounds checking. Same overflow risk as line 39.
  *Fix: Use `strncpy` with the destination field size.*

- **GWCivRecord.cpp:299** — `strcpy(unitRecord->name, unitName);` copies a user-provided `unitName` into a 128-byte field without length validation.
  *Fix: Use `strncpy(unitRecord->name, unitName, sizeof(unitRecord->name) - 1);` and null-terminate.*

- **GWCivRecord.cpp:305** — `strcpy(unitRecord->builderAIP, builderAIP);` copies into an 80-byte field without checking the source length.
  *Fix: Use `strncpy` with the field size minus one.*

- **GWCivRecord.cpp:336** — `strcpy(unitRecord->killerAIP, killerAIP);` copies into an 80-byte field without bounds checking.
  *Fix: Use `strncpy` with size limiting.*

- **GWCivRecord.cpp:341** — `strcpy(unitRecord->killedAIP, killedAIP);` copies into an 80-byte field without bounds checking.
  *Fix: Use `strncpy` with size limiting.*

- **GWCivRecord.cpp:39** — `strcpy(record->name, unitArray[index].name);` copies into a 128-byte field without length validation. If the source name is 128 non-null characters, `strcpy` writes 129 bytes (including null), overflowing by 1.
  *Fix: Use `strncpy(record->name, unitArray[index].name, sizeof(record->name) - 1); record->name[sizeof(record->name) - 1] = '\0';`.*

- **GaiaController.cpp:198** — `Player *player_ptr = g_player[m_playerId];` indexes `g_player` without validating `m_playerId`. If corrupted or invalid, causes out-of-bounds access.
  *Fix: Add bounds check: `if (m_playerId < 0 || m_playerId >= k_MAX_PLAYERS) return;`*

- **GaiaController.cpp:263** — `g_player[newOwner]` and `g_player[originalOwner]` are accessed without bounds validation. `newOwner` and `originalOwner` come from event arguments.
  *Fix: Validate both indices before indexing.*

- **GaiaController.cpp:343** — `g_player[owner]` is accessed after checking `owner == -1`, but there is no upper bounds check. If `owner >= k_MAX_PLAYERS`, UB.
  *Fix: Add upper bound check: `if (owner >= k_MAX_PLAYERS) return GEV_HD_Continue;`*

- **Goal.cpp:3255** — `ExecuteTask` contains the same underflow-prone loop pattern: `for (uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)`.
  *Fix: Same as above.*

- **Happy.cpp:599** — `Player *p = g_player[cd.m_owner];` indexes `g_player` without bounds validation. `cd.m_owner` could be out of bounds.
  *Fix: Add bounds check before indexing.*

- **MapFile.cpp:869** — `numCellsWithUnits` is read from the file buffer via `PULLLONG(numCellsWithUnits)` and used directly as a loop bound without validation. A corrupted file can set this to a huge value, causing the loop to read far past the end of `buf`.
  *Fix: Validate `numCellsWithUnits` against `chunkSize` and the expected maximum based on map dimensions before entering the loop.*

- **MapFile.cpp:929** — `numCities` is pulled from the buffer with `PULLLONG(numCities)` and used as a loop bound without validation. Malicious input can cause out-of-bounds reads past the allocated buffer.
  *Fix: Validate `numCities` against the remaining buffer size and a reasonable maximum before looping.*

- **MaterialPool.cpp:73** — `g_player[m_owner]->GetPoints()` indexes `g_player` without validating `m_owner` is in bounds.
  *Fix: Add bounds check before all `g_player[m_owner]` accesses.*

- **MessageData.cpp:329** — Destructor calls `g_player[m_owner]->StartResearching(m_advance)` without bounds check on `m_owner` or null check on `g_player[m_owner]`.
  *Fix: Validate index and pointer before access.*

- **MessagePool.cpp:106** — `g_player[owner]->AddMessage(newRequest)` accesses `g_player[owner]` without bounds or null check. Same issue at lines 132, 151, 165.
  *Fix: Validate `owner` before all accesses.*

- **Order.cpp:252** — `OrderToEvent` returns `s_orderToEventMap[order]` without validating that `order` is within `[0, UNIT_ORDER_MAX)`. An invalid order value causes an out-of-bounds array access.
  *Fix: Add `Assert(order >= 0 && order < UNIT_ORDER_MAX)` or a runtime bounds check.*

- **SelItem.cpp:1803** — `g_theUnitAstar->FindPath(a, start, player, m_waypoints[0], ...)` accesses `m_waypoints[0]` without first checking that `m_waypoints.Num() > 0`. If the waypoint array is empty, this is an out-of-bounds read.
  *Fix: Add `Assert(m_waypoints.Num() > 0);` or `if (m_waypoints.Num() <= 0) return;` before accessing index 0.*

- **SelItem.cpp:1851** — `isCircular = start == m_waypoints[m_waypoints.Num() - 1];` computes `Num() - 1` without checking `Num() > 0`. If the array is empty, this accesses index `-1`.
  *Fix: Add `if (m_waypoints.Num() <= 0) { isCircular = false; return; }` before the access.*

- **SlicRecord.cpp:99** — `archive.Load((uint8*)segmentName, l)` writes into `char segmentName[k_MAX_SLIC_STRING]` on the stack without verifying `l <= k_MAX_SLIC_STRING`. A malformed save file causes stack buffer overflow.
  *Fix: Reject or truncate if `l > k_MAX_SLIC_STRING - 1`.*

- **SlicSegment.cpp:125** — `SlicSegment::SlicSegment(sint32 slicifIndex)` only asserts `slicifIndex < g_slicNumEntries`. With asserts disabled, an invalid index accesses `g_slicObjectArray` out of bounds.
  *Fix: Add a runtime bounds check and early return if `slicifIndex >= g_slicNumEntries`.*

- **SpriteStateDB.cpp:193** — `SetName(count, "SPRITE_MYSTERY");` is called without confirming `count < m_size`. If the file contains fewer entries than expected, `count` may equal `m_size` and write past the allocated array.
  *Fix: Only call `SetName` and `SetVal` after verifying `count >= 0 && count < m_size`.*

- **UnitActor.cpp:1101** — `DrawCityWalls()` accesses `g_player[unit->GetOwner()]->m_age` without checking the owner index or null pointer. A unit with an invalid owner will cause an out-of-bounds read or null dereference.
  *Fix: Check `sint32 owner = unit->GetOwner(); if (owner < 0 || owner >= k_MAX_PLAYERS || !g_player[owner]) return;` before use.*

- **UnitActor.cpp:1206** — `DrawForceField()` accesses `g_player[unit->GetOwner()]->m_age` without bounds checking the owner index or null pointer.
  *Fix: Same as DrawCityWalls — validate owner index and null pointer before dereferencing.*

- **UnitActor.cpp:1913** — `DrawSpecialIndicators()` accesses `g_player[displayedOwner]` where `displayedOwner` is derived from `m_playerNum` (which could be out of bounds) or `PLAYER_INDEX_VANDALS`. No bounds check is performed.
  *Fix: Validate `displayedOwner` is in `[0, k_MAX_PLAYERS)` before indexing `g_player`.*

- **UnitActor.cpp:348** — `AddVision()` accesses `g_player[m_playerNum]` without validating that `m_playerNum` is within `[0, k_MAX_PLAYERS)`. A corrupted savegame or invalid deserialization could set `m_playerNum` out of bounds, causing an array overrun and crash.
  *Fix: Add bounds check before indexing: `if (m_playerNum < 0 || m_playerNum >= k_MAX_PLAYERS) return;`.*

- **UnitActor.cpp:358** — `RemoveVision()` accesses `g_player[m_playerNum]` without bounds validation. Same root cause and risk as the `AddVision()` issue.
  *Fix: Add bounds check before indexing: `if (m_playerNum < 0 || m_playerNum >= k_MAX_PLAYERS) return;`.*

- **UnitActor.cpp:384** — `GetIDAndType()` accesses `g_player[owner]` without validating `owner` is in range. The `owner` parameter comes from external input (savegame / network) and could be any value.
  *Fix: Add `if (owner < 0 || owner >= k_MAX_PLAYERS) { *spriteID = CTPRecord::INDEX_INVALID; return; }` at the top of the function.*

- **WrlEnv.cpp:142** — `m_map[x][y]->m_env` is accessed in `IsLand(const sint32 x, const sint32 y)` without validating that `x` and `y` are within the map bounds. Callers may pass out-of-bounds coordinates, leading to array out-of-bounds access.
  *Fix: Add bounds assertions or checks: `Assert(0 <= x && x < m_size.x && 0 <= y && y < m_size.y)` before indexing `m_map`.*

- **WrlEnv.cpp:562** — `array[pnt.y * m_size.x + pnt.x] = 1;` indexes into a dynamically allocated array using `pnt` coordinates that are never validated against map bounds. Out-of-bounds coordinates write outside the allocation.
  *Fix: Validate `pnt.x` and `pnt.y` against `m_size` before indexing.*

- **WrlEnv.cpp:621** — `array[pnt.y * m_size.x + pnt.x] = 1;` in `IsContinentSharedWithOthers` uses unvalidated `pnt` coordinates to index the flood-fill array.
  *Fix: Bounds-check `pnt` before indexing.*

- **WrlEnv.cpp:659** — `array[pnt.y * m_size.x + pnt.x] = 1;` in `IsContinentBiggerThan` uses `pnt` without bounds validation.
  *Fix: Validate `pnt.x` and `pnt.y` against map dimensions before indexing.*

- **WrlEnv.cpp:697** — `Cell *thisCell = m_map[point.x][point.y];` in `ChangeOwner` does not validate that `point.x` and `point.y` are within map bounds before array access.
  *Fix: Validate `point` against `m_size` before indexing `m_map`.*

- **WrlEnv.cpp:745** — `Cell *thisCell = m_map[point.x][point.y];` in `CutImprovements` lacks bounds validation on `point`, allowing out-of-bounds access.
  *Fix: Add bounds checks on `point.x` and `point.y` before the array dereference.*

- **agent.cpp:692** — `Group_Order` contains `for (uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)`. When `strlen(myText) < 5`, `strlen(myText) - 5` underflows (wraps to a very large `size_t`), causing the loop to write up to 256 bytes past the end of `goalString` (which is sized `strlen(myText) + 40`).
  *Fix: Replace the loop condition with an explicit bounds check: `for (size_t myComp = 0; myComp + 5 < strlen(myText); myComp++)`.*

- **agent.cpp:933** — `ClearOrders` contains the same underflow-prone loop pattern: `for(uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)`.
  *Fix: Same as above.*

- **agreementmatrix.cpp:140** — `return m_agreements[index];` returns a reference to an element without runtime bounds checking (only `Assert` in debug). A malformed packet could trigger an out-of-bounds access.
  *Fix: Add runtime check: `if (index >= m_agreements.size()) return s_badAgreement;`.*

- **appstrings.cpp:41** — `inStr[strlen(inStr)-1] = '\0';` subtracts 1 from `strlen()` without checking if the string is empty. If `fgets` reads a line starting with a null byte, `strlen()` returns 0, causing an unsigned integer underflow (`SIZE_MAX`) and an out-of-bounds write.
  *Fix: Check `strlen(inStr) > 0` before indexing `strlen(inStr)-1`, or use `strcspn` to strip newlines safely.*

- **aui_image.cpp:158** — `aui_BmpImageFormat::Load` copies `sizeof(BITMAPFILEHEADER)` bytes from `filebits` without first verifying that the mapped file is at least that large. A truncated BMP causes an out-of-bounds read.
  *Fix: Verify the mapped file size (obtainable from the memory map) is at least `sizeof(BITMAPFILEHEADER)` before the first `memcpy`.*

- **aui_image.cpp:167** — The code copies `sizeof(BITMAPINFOHEADER)` from `filebits + foffset` without confirming the file is large enough to contain the info header.
  *Fix: Verify the remaining file size is at least `sizeof(BITMAPINFOHEADER)` before copying.*

- **aui_image.cpp:186** — When `bih.biBitCount == 8`, the code copies `256 * sizeof(RGBQUAD)` bytes from the mapped file without checking that the file actually contains a full palette. A truncated file leads to an out-of-bounds read.
  *Fix: Ensure the mapped file has at least `bfh.bfOffBits` bytes (or `foffset + 256 * sizeof(RGBQUAD)`) before copying the palette.*

- **c3cmdline.cpp:2506** — `SlicCommand::Execute` uses `strcat(buf, argv[i])` in a loop against a 2048-byte buffer with no length checking. User-provided arguments can overflow the stack buffer.
  *Fix: Use `strncat` with remaining size tracking, or switch to `snprintf` into `buf`.*

- **c3cmdline.cpp:3078** — `ChatCommand::Execute` builds a chat string with a loop that checks `pos + strlen(argv[i]) >= k_MAX_CHAT_LEN` but does not account for the trailing space it appends. The final `str[pos] = 0` can write one byte past the `k_MAX_CHAT_LEN` buffer.
  *Fix: Change the check to `pos + strlen(argv[i]) + 1 >= k_MAX_CHAT_LEN` to reserve space for both the space separator and the final null terminator.*

- **c3errors.cpp:169** — `szFmtTmp` is computed as `szTitle + (lstrlen(szTitle)+2)*sizeof(TCHAR)`. Pointer arithmetic on `TCHAR*` already scales by `sizeof(TCHAR)`, so this double-multiplies the offset and places `szFmtTmp` far outside the allocated buffer on Unicode builds, causing `vsprintf` to write out of bounds.
  *Fix: Remove the redundant `sizeof(TCHAR)` factor: `szTitle + lstrlen(szTitle) + 2`.*

- **c3errors.cpp:51** — `vsprintf(str, fmt, list)` writes into `char str[_MAX_PATH]`. A format string with large expansions (e.g., `%s` pointing to a long string) overflows the fixed-size buffer.
  *Fix: Replace with `vsnprintf(str, sizeof(str), fmt, list);` and ensure null-termination.*

- **civ3_main.cpp:326** — After `readlink()`, the check is `size > sizeof(szTemp)`. If `readlink` returns exactly `sizeof(szTemp)`, the buffer is full and not null-terminated, but the code continues and writes `szTemp[size] = '\0'` one byte past the buffer.
  *Fix: Change the condition to `size >= sizeof(szTemp)`.*

- **civ3_main.cpp:398** — Same off-by-one `readlink` bounds check as line 326. If `readlink` returns `sizeof(szTemp)`, `szTemp[size] = 0` writes past the buffer.
  *Fix: Change `>` to `>=` in the bounds check.*

- **civapp.cpp:1888** — `g_player[g_scenarioUsePlayerNumber]` is accessed with only a lower-bound check (`> 0`). If `g_scenarioUsePlayerNumber` is >= `k_MAX_PLAYERS`, this reads/writes out of bounds.
  *Fix: Add an upper bound check: `if (g_scenarioUsePlayerNumber > 0 && g_scenarioUsePlayerNumber < k_MAX_PLAYERS && g_player[g_scenarioUsePlayerNumber])`.*

- **civapp.cpp:3489** — `strcpy(leaderName, g_theProfileDB->GetLeaderName());` copies into a `k_MAX_NAME_LEN` buffer without checking the source length. A leader name longer than the buffer causes a stack buffer overflow.
  *Fix: Use `strncpy(leaderName, g_theProfileDB->GetLeaderName(), k_MAX_NAME_LEN - 1); leaderName[k_MAX_NAME_LEN - 1] = '\0';`.*

- **civapp.cpp:3494** — `sprintf(filename, "%s-%s", autosaveName, leaderName);` writes into a `_MAX_PATH` buffer without length validation. Long autosave names or leader names can overflow the stack buffer.
  *Fix: Use `snprintf(filename, sizeof(filename), "%s-%s", autosaveName, leaderName);`.*

- **civapp.cpp:3502** — `sprintf(fullpath, "%s%s%s", path, FILE_SEP, leaderName);` constructs a path without bounds checking. Long paths or leader names can overflow the `_MAX_PATH` buffer.
  *Fix: Use `snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, FILE_SEP, leaderName);`.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:152** — `WriteFacedSpriteData` uses `normal_ssizes[k_MAX_FACINGS][128]` and indexes the second dimension with `num_frames` read from the sprite. If `num_frames > 128`, the loop writes past the array bounds.
  *Fix: Validate `num_frames <= 128` before entering the loops.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:376** — `ReadSpriteDataBasic` uses `uint32 ssizes[800]` and `msizes[800]` indexed by `s->GetNumFrames()`. Corrupt/malicious files with more than 800 declared frames overflow the stack buffers.
  *Fix: Reject or clamp `numFrames` to `<= 800` before reading size arrays.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:452** — `ReadSpriteDataFull` uses the same `ssizes[800]` / `msizes[800]` pattern indexed by file-controlled `GetNumFrames()`.
  *Fix: Validate `numFrames <= 800` after reading it and before populating the arrays.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:634** — `ReadFacedSpriteDataFull` declares `ssizes[k_MAX_FACINGS][800]` and `msizes[k_MAX_FACINGS][800]`, then uses `s->GetNumFrames()` as the inner loop bound without an upper bound check.
  *Fix: Assert or enforce `numFrames <= 800` before the loops.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:91** — `WriteSpriteData` declares fixed-size stack arrays `normal_ssizes[800]` and `normal_msizes[800]`, then indexes them with `s->GetNumFrames()` without validating the frame count. A malformed sprite file with `numFrames > 800` causes a stack buffer overflow.
  *Fix: Add a bounds check before the loop: `if (s->GetNumFrames() > 800) { return; /* or clamp */ }`.*

- **ctp2_code/gfx/tilesys/BaseTile.cpp:80** — `m_tileDataLen` is read from file and used as `new Pixel16[m_tileDataLen/2]`. If `m_tileDataLen` is odd, integer truncation causes the allocation to be one element smaller than the byte count that `c3files_fread` subsequently writes. This causes a heap buffer overflow.
  *Fix: Validate that `m_tileDataLen` is even and within a reasonable maximum before allocating, e.g., `Assert((m_tileDataLen % 2) == 0); if (m_tileDataLen % 2) return FALSE;`.*

- **ctp2_code/gs/database/profileDB.cpp:551** — After reading a line with `fgets`, whitespace is trimmed from the end in a `while(isspace(line[len - 1]))` loop. If the line consists only of whitespace (e.g., just `"\n"`), `len` decrements to 0 and the loop accesses `line[-1]`, causing a buffer underflow.
  *Fix: Change the loop to `while(len > 0 && isspace(line[len - 1]))`.*

- **diplomat.cpp:1372** — `if (foreignerId < 0 || static_cast<size_t>(foreignerId) > m_foreigners.size())` uses `>` instead of `>=`, allowing an off-by-one access at `m_foreigners.size()`.
  *Fix: Change `>` to `>=`.*

- **diplomat.cpp:2472-2551** — `s_responseNames[response.type]`, `s_proposalNames[response.counter.first_type]`, and `s_proposalNames[response.counter.second_type]` are indexed in DPRINTF/printf calls without validating the enum values are within array bounds.
  *Fix: Validate enum values before using them as array indices.*

- **diplomat.cpp:3090-3129** — `m_diplomacy[foreignerId].GetProposalElement(s_proposalTypeToElemIndex[PROPOSAL_TREATY_PEACE])->GetProposal()` and similar calls — if `GetProposalElement` returns NULL, it is dereferenced.
  *Fix: NULL-check the result of `GetProposalElement` before calling `GetProposal()`.*

- **diplomat.cpp:3416-3456** — `s_proposalTypeToElemIndex[proposalType]` is used as an index into `m_diplomacy[foreignerId].GetProposalElement(...)` without validating `proposalType` is within `s_proposalTypeToElemIndex` bounds.
  *Fix: Validate `proposalType < PROPOSAL_MAX` before indexing.*

- **diplomat.cpp:4820** — `SetHotwarAttack` directly indexes `m_foreigners[foreignerId]` without validating that `foreignerId` is within `[0, m_foreigners.size())`. An out-of-bounds value from a caller causes an immediate crash or heap corruption.
  *Fix: Add `Assert(foreignerId >= 0 && static_cast<size_t>(foreignerId) < m_foreigners.size());` before indexing, or use `.at()`.*

- **diplomat.cpp:4825** — `GetLastHotwarAttack` directly indexes `m_foreigners[foreignerId]` without bounds validation.
  *Fix: Same as above.*

- **diplomat.cpp:4830** — `SetColdwarAttack` directly indexes `m_foreigners[foreignerId]` without bounds validation.
  *Fix: Same as above.*

- **diplomat.cpp:4835** — `GetLastColdwarAttack` directly indexes `m_foreigners[foreignerId]` without bounds validation.
  *Fix: Same as above.*

- **diplomat.cpp:814** — `const Foreigner &foreigner = m_foreigners[tmp_foreignerId];` where `tmp_foreignerId` iterates up to `k_MAX_PLAYERS` but `m_foreigners` may be smaller than `k_MAX_PLAYERS`.
  *Fix: Check `tmp_foreignerId < m_foreigners.size()` before indexing.*

- **director.cpp:1382** — `g_player[g_selected_item->GetCurPlayer()]->m_current_round` accesses the global player array without bounds-checking the player index. An invalid index causes an out-of-bounds read.
  *Fix: `sint32 player = g_selected_item->GetCurPlayer(); if (player < 0 || player >= k_MAX_PLAYERS) return;`*

- **director.cpp:1414-1415** — `g_player[lastPlayer]` uses `lastPlayer` (set from `GetCurPlayer()`) without validation. If `lastPlayer` is invalid, this is an out-of-bounds access.
  *Fix: Validate `lastPlayer` before indexing `g_player`.*

- **director.cpp:1936** — `g_player[g_selected_item->GetVisiblePlayer()]` is dereferenced without validating that the player index is within `[0, k_MAX_PLAYERS)`. A bad index leads to an out-of-bounds array access.
  *Fix: Validate `visiblePlayer >= 0 && visiblePlayer < k_MAX_PLAYERS` before indexing.*

- **director.cpp:646-647** — `g_player[item->GetOwner()]` is dereferenced without validating that `item->GetOwner()` is within the valid player array bounds. A corrupted save or invalid owner value causes an out-of-bounds array access.
  *Fix: Add a bounds check: `if (item->GetOwner() < 0 || item->GetOwner() >= k_MAX_PLAYERS) continue;`*

- **directorevent.cpp:222** — `g_player[g_selected_item->GetVisiblePlayer()]->m_vision->IsVisible(attackPos)` indexes the player array without validating the visible player index.
  *Fix: Add bounds check before array access.*

- **effectspritegroup.cpp:186** — `for(i=0; i<effectSprite->GetNumFrames(); i++)` writes into `shadowNames[i]` and `imageNames[i]`, which are fixed arrays of size `k_MAX_NAMES`. A sprite file claiming more frames causes a stack buffer overflow.
  *Fix: Clamp `GetNumFrames()` to `k_MAX_NAMES` before the loop.*

- **effectspritegroup.cpp:226** — The same unbounded loop into `imageNames`/`shadowNames` for the flash sprite can overflow the fixed arrays.
  *Fix: Enforce `flashSprite->GetNumFrames() <= k_MAX_NAMES`.*

- **gameinit.cpp:1310** — `ai_players` is a stack array of size `k_MAX_PLAYERS`. The loop iterates over all `k_MAX_PLAYERS` slots and appends robot players to `ai_players[next++]`. If every player slot contains a robot, `next` reaches `k_MAX_PLAYERS` and the assignment `ai_players[next++]` writes one element past the end of the array.
  *Fix: Add a bounds check `Assert(next < k_MAX_PLAYERS)` before writing, or ensure the array is large enough.*

- **gameinit.cpp:2408** — `g_scenarioUsePlayerNumber` is checked for `> 0` but never validated against `k_MAX_PLAYERS` before being used as an index into `g_player[]`. A malformed value causes an out-of-bounds array access.
  *Fix: Add `Assert(g_scenarioUsePlayerNumber < k_MAX_PLAYERS)` before the array access.*

- **gameinit.cpp:525** — `gameinit_SpewUnits(sint32 player, MapPoint &pos)` uses `g_player[player]` without validating that `player` is in the range `[0, k_MAX_PLAYERS)`. A caller passing an out-of-range player index causes an out-of-bounds array access.
  *Fix: Add `Assert(player >= 0 && player < k_MAX_PLAYERS); if (player < 0 || player >= k_MAX_PLAYERS) return;` at the function entry.*

- **goodspritegroup.cpp:194** — `for(i=0; i<idleSprite->GetNumFrames(); i++)` writes into `shadowNames[i]` / `imageNames[i]` without ensuring `i < k_MAX_NAMES`.
  *Fix: Verify `idleSprite->GetNumFrames() <= k_MAX_NAMES` before looping.*

- **iparser.cpp:2039** — In `Next_Symbol`, the quoted-string copy loop writes characters into `new_symbol` without checking against `new_symbol_end`. A long quoted string can overflow the fixed-size `new_symbol` buffer.
  *Fix: Add `new_symbol < new_symbol_end` as a loop condition.*

- **iparser.cpp:2067** — In `Next_Symbol`, the `while(IParser_Find_Char_Type(the_string[*place_in_the_string]) isnt BOGUS)` loop reads from `the_string` without checking `*place_in_the_string < the_string_len`. If the string does not end with a BOGUS character, it reads past the buffer.
  *Fix: Add `*place_in_the_string < the_string_len` to the loop condition.*

- **iparser.cpp:675** — In `Read_Until_String`, `ch = parse_buffer[*parse_pos];` reads from the buffer **before** the bounds check. When `*parse_pos == buffer_length`, it reads one byte past the end of `parse_buffer`.
  *Fix: Check `*parse_pos < buffer_length` before reading `parse_buffer[*parse_pos]`.*

- **iparser.cpp:688** — In `Read_Until_String`, `(*declaration_buffer)[length-1] = ch;` writes into `declaration_buffer` without checking `length` against the allocated size (`buffer_length`). If the termination string is missing, `length` grows past the buffer, causing an out-of-bounds write. Line 734 (`(*declaration_buffer)[length] = 0;`) writes another byte past the end.
  *Fix: Bound the loop so it stops when `length >= buffer_length`, and treat missing terminator as an error.*

- **mapanalysis.cpp:823** — `return m_empireBoundingRect[player];` — `player` index is not validated against vector size.
  *Fix: Add bounds check before indexing.*

- **mapanalysis.cpp:828** — `return m_empireCenter[player];` — same unvalidated index access.
  *Fix: Add bounds check before indexing.*

- **net_action.cpp:339** — In `NET_ACTION_MOVE_UNIT` handling, `index` is derived from the network sender ID (`sint32 index = g_network.IdToIndex(id);`) and immediately used to access `g_player[index]->GetAllArmiesList()` without verifying `index` is valid (`>= 0` and `< k_MAX_PLAYERS`).
  *Fix: Add `Assert(index >= 0 && index < k_MAX_PLAYERS); if (index < 0 || index >= k_MAX_PLAYERS || !g_player[index]) return;` before the player dereference.*

- **net_army.cpp:192** — In `NetArmy::Unpacketize`, `c` is read as a single byte (`PULLBYTE(c)`) and used as the loop bound to read unit IDs. There is no validation against the actual packet size, so a malformed packet can cause out-of-bounds reads.
  *Fix: Validate `c` against `(size - pos) / sizeof(uint32)` to ensure the loop does not read past the packet buffer.*

- **net_army.cpp:255** — In `NetGroupRequest::Unpacketize`, `n` is read from the packet (`PULLBYTE(n)`) and used to loop over unit IDs. No check ensures `n * sizeof(uint32)` fits within the remaining packet bytes.
  *Fix: Validate `n` against the remaining buffer size before the loop.*

- **net_cheat.cpp:41** — `if(m_args[m_cheat] > 0)` indexes `m_args` using `m_cheat` without validating that `m_cheat` is within `[0, NET_CHEAT_MAX)`. A malicious or corrupted value causes an out-of-bounds read.
  *Fix: Add `Assert(m_cheat >= 0 && m_cheat < NET_CHEAT_MAX);` or an explicit bounds check.*

- **net_cheat.cpp:81** — `if(m_args[m_cheat] > 0)` in `Unpacketize` has the same unvalidated index into `m_args`.
  *Fix: Add a bounds check on `m_cheat` before indexing `m_args`.*

- **net_city.cpp:286** — `strcpy(home_city.GetData()->GetCityData()->m_name, name);` copies `name` into a fixed-size buffer without bounds checking. A long name from the network packet can overflow the buffer.
  *Fix: Use `strncpy` with the destination buffer size, ensuring null termination.*

- **net_city.cpp:544** — `len` is read from the network buffer (`PULLLONG(len)`) and used directly as the loop count for reading build queue nodes. A malicious value causes out-of-bounds reads and can allocate excessive `BuildNode` objects.
  *Fix: Cap `len` to a maximum reasonable build queue length and verify it does not exceed the remaining packet size.*

- **net_civ.cpp:111** — `numCityNames` is read from the network packet and then used to loop over `m_data->m_cityname_count[i]`. If `numCityNames` exceeds the actual array size, this causes an out-of-bounds write.
  *Fix: Clamp `numCityNames` to the known array size before looping.*

- **net_diff.cpp:22** — `Packetize` writes `sizeof(*m_difficulty)` bytes to `buf` starting at offset 4 without first checking that the caller-provided buffer is large enough. If the buffer is smaller than `4 + sizeof(*m_difficulty)`, this is a stack/heap buffer overflow.
  *Fix: Add a bounds check before writing, or pass the buffer capacity into `Packetize` and assert/check against it.*

- **net_info.cpp:297-313** — `Packetize` writes up to 24 bytes to `buf` based on `m_args[m_type]`, but never checks the caller-provided buffer capacity. The `m_type` value is also not validated against `NET_INFO_CODE_NULL` before use.
  *Fix: Validate `m_type < NET_INFO_CODE_NULL` and ensure the buffer is at least `4 + 4*m_args[m_type]` bytes.*

- **net_info.cpp:320-338** — `Unpacketize` reads from `buf` based on `m_args[m_type]` without runtime bounds checking (only an `Assert` which is removed in release builds). A malicious or truncated packet can cause out-of-bounds reads.
  *Fix: Add a runtime check: `if (size < 4 + 4 * m_args[m_type]) return;` before reading.*

- **net_info.cpp:514** — `m_data` is used as a player index to access `g_player[m_data]` without validating it is less than `k_MAX_PLAYERS`. If `m_data >= k_MAX_PLAYERS`, this is an out-of-bounds array access. Same pattern exists at lines 640, 644, 653, etc.
  *Fix: Validate `m_data < k_MAX_PLAYERS` before indexing `g_player[m_data]`.*

- **net_research.cpp:46** — In `Unpacketize`, the loop reads `buf[pos]` without ever checking that `pos < size`. A truncated or malformed packet causes an out-of-bounds read.
  *Fix: Add `Assert(pos < size);` or an explicit check before each access inside the loop.*

- **net_strengths.cpp:108** — `NetFullStrengths::Unpacketize` reads `m_startRound` and `m_endRound` from the network buffer and then loops reading data with `PULLLONG`. Neither the loop bounds nor the buffer position `pos` are validated against `size`.
  *Fix: Validate `pos <= size` before each `PULLLONG` and ensure `m_endRound >= m_startRound`.*

- **net_strengths.cpp:72-79** — `NetFullStrengths::Packetize` indexes `str->m_strengthRecords[i][r]` where `r` ranges from `m_startRound` to `m_endRound`. These values come from network data and are not validated against the actual array bounds.
  *Fix: Validate `r < str->m_strengthRecords[i].Num()` before indexing.*

- **net_terrain.cpp:131** — `Unpacketize` resets `pos = 6` and then performs multiple `PULLBYTE`/`PULLSHORT`/`PULLLONG` operations without verifying that `size >= 6` first. A short packet causes an out-of-bounds read.
  *Fix: Add `if (size < 6) return;` before resetting `pos`.*

- **net_thread.cpp:479** — `m_response->AddPlayer(getshort(&packet->m_buf[packet->m_len - 2]), ...)` accesses `packet->m_buf[packet->m_len - 2]`. If `packet->m_len < 2`, the index underflows (signed arithmetic) causing an out-of-bounds read.
  *Fix: Validate `packet->m_len >= 2` before computing the offset.*

- **net_thread.cpp:500** — `getlong(&packet->m_buf[1])` reads from offset 1 without checking `packet->m_len >= 5`.
  *Fix: Check `packet->m_len >= 5` before reading.*

- **net_thread.cpp:61** — `getlong(&buf[2])` is called without checking `len >= 6`. A short packet causes an out-of-bounds read.
  *Fix: Validate `len >= 6` before calling `getlong`.*

- **net_unit.cpp:139** — `Unpacketize` calls `getlong(&buf[2])` without first checking `size >= 6`. A short packet causes an out-of-bounds read.
  *Fix: Validate `size >= 4` before the first `getlong`.*

- **net_unit.cpp:219** — When creating a new unit from a network packet, hardcoded offsets like `getshort(&buf[10])`, `getlong(&buf[12])`, `getshort(&buf[16])`, `getshort(&buf[18])`, and `getlong(&buf[20])` are used without first checking that `size` is large enough.
  *Fix: Validate `size >= 24` (or the minimum required size) before accessing these offsets.*

- **net_unit.cpp:298-357** — `PacketizeUnit` writes many fields to `buf` without bounds checking. It can overflow the caller's buffer.
  *Fix: Track remaining capacity and abort if exceeded.*

- **net_unit.cpp:359-417** — `UnpacketizeUnit` reads many fields from `buf` without checking against `size`. It can read past the end of a truncated packet.
  *Fix: Track remaining bytes and abort if insufficient.*

- **network.cpp:2218-2221** — In `FindEmptySlot`, if the loop at line 2193 finds no player slots, `newslot` remains `-1`. The subsequent access `g_player[newslot]` reads `g_player[-1]`, an out-of-bounds array access.
  *Fix: Check `if (newslot < 0) return -1;` before accessing `g_player[newslot]`.*

- **network.cpp:3953** — `DechunkList` calls `getshort(&buf[pos])` and then `pos += 2` without verifying `pos + 2 <= len`. A malformed chunk list can read past the buffer.
  *Fix: Add `if (pos + 2 > len) break;` before `getshort`.*

- **proposalanalysis.cpp:73-74** — `s_proposalNames[proposal.detail.first_type]` is accessed without validating `first_type` is within the array bounds. A corrupted or invalid proposal type leads to out-of-bounds access.
  *Fix: Assert or check `first_type < s_proposalNames.size()` before indexing.*

- **proposalanalysis.cpp:96-97** — `s_responseNames[response.type]` accessed without bounds check.
  *Fix: Validate `response.type` before indexing.*

- **proposalanalysis.cpp:99** — `s_threatNames[response.threat.type]` accessed without bounds check.
  *Fix: Validate `response.threat.type` before indexing.*

- **scheduler.cpp:1321** — `GOAL_TYPE goal_type = GetMaxEvalExec(goal_element_ptr, max_eval, max_exec);` is followed by `m_goals_of_type[goal_type]` access. The function only `Assert`s the bounds (debug-only). In release builds, a malformed strategy database can cause an out-of-bounds array access.
  *Fix: Add explicit runtime bounds check: `if (goal_type < 0 || goal_type >= m_goals_of_type.size()) continue;`.*

- **settlemap.cpp:174** — `sint32 radius = g_theCitySizeDB->Get(citydata->GetSizeIndex())->GetIntRadius();` — `GetSizeIndex()` can return an out-of-bounds index, causing NULL or garbage from `Get()`.
  *Fix: Validate `GetSizeIndex()` is within `g_theCitySizeDB->NumRecords()` before indexing.*

- **settlemap.cpp:620-625** — `g_theWorld->GetContinent(iter->m_pos, cont, is_land);` returns `cont` which is then used to index `land_continent_count[cont]` and `water_continent_count[cont]` without bounds validation. A corrupted or edge-case continent ID can write out of bounds.
  *Fix: Validate `cont >= 0 && cont < max_land_cont` (or water) before indexing.*

- **slicif.cpp:1820** — `slicif_get_local_name` does `sprintf(localName, "%s#%s", s_current_segment_name, name)` into a 1024-byte buffer without length checks. Long segment names + variable names can overflow.
  *Fix: Use `snprintf(localName, 1024, "%s#%s", s_current_segment_name, name);`.*

- **slicif.cpp:649** — `strcpy(internalName, "_"); strcat(internalName, name);` writes into `char internalName[k_MAX_FUNCTION_NAME]` (256 bytes). A long SLIC function name from a mod script overflows the buffer.
  *Fix: Use `strncpy` / `strncat` or `snprintf(internalName, sizeof(internalName), "_%s", name);`.*

- **spritefile.cpp:152** — `uint32 normal_ssizes[k_MAX_FACINGS][128];` is written using `num_frames = s->GetNumFrames()`. A value larger than 128 overflows the second dimension.
  *Fix: Clamp `num_frames` to `128`.*

- **spritefile.cpp:1880** — `int offsets[ACTION_MAX];` is passed to `ReadData` with `sizeof(int) * (ACTION_MAX+1)`, writing one integer past the end of the array.
  *Fix: Declare the array as `int offsets[ACTION_MAX + 1];`.*

- **spritefile.cpp:2470** — In `DeCompressData_LZW1`, `uint8 *p = p_dst - offset;` is not checked against `p_dst_first`. A large or crafted `offset` causes an out-of-bounds read.
  *Fix: Verify `offset <= static_cast<size_t>(p_dst - p_dst_first)` before computing `p`.*

- **spritefile.cpp:2475** — `while (len--) *p_dst++ = *p++;` in the LZW1 decoder writes without verifying the destination limit (`p_dst < p_dst_first + dst_len`). Corrupted data can write past the allocated buffer.
  *Fix: Check that `p_dst + len <= p_dst_first + dst_len` before the copy loop.*

- **spritefile.cpp:376** — `uint32 ssizes[800];` in `ReadSpriteDataBasic` is populated from file-derived `s->GetNumFrames()` without an upper bound.
  *Fix: Cap the frame count to `800` before reading into the stack array.*

- **spritefile.cpp:452** — `uint32 ssizes[800];` in `ReadSpriteDataFull` has the same unbounded frame-count issue.
  *Fix: Validate `GetNumFrames() <= 800`.*

- **spritefile.cpp:507** — `uint32 ssizes[800];` in `SkipSpriteData` is filled from `numFrames` read directly from the file. A malicious file can specify more than 800 frames and overflow the stack.
  *Fix: Reject `numFrames > 800`.*

- **spritefile.cpp:545** — `size_t ssizes[k_MAX_FACINGS][512];` in `ReadFacedSpriteDataBasic` reads `s->GetNumFrames()` entries per facing without a `<= 512` check.
  *Fix: Clamp the frame count to `512`.*

- **spritefile.cpp:634** — `uint32 ssizes[k_MAX_FACINGS][800];` in `ReadFacedSpriteDataFull` uses `s->GetNumFrames()` without an upper bound.
  *Fix: Enforce `GetNumFrames() <= 800`.*

- **spritefile.cpp:695** — `uint32 ssizes[k_MAX_FACINGS][800];` in `SkipFacedSpriteData` is filled from file `numFrames` without validation.
  *Fix: Reject `numFrames > 800`.*

- **spritefile.cpp:742** — `uint32 ssizes[k_MAX_FACINGS][800];` in `ReadFacedSpriteWshadowData` is filled from `s->GetNumFrames()` without a cap.
  *Fix: Validate frame count against `800`.*

- **spritefile.cpp:91** — `uint32 normal_ssizes[800];` is filled based on `s->GetNumFrames()`. If the sprite reports more than 800 frames, the stack buffer overflows.
  *Fix: Reject or clamp `GetNumFrames()` to `800` before writing to the fixed arrays.*

- **targautils.cpp:190-192** — `Load_TGA_File_Simple` allocates `tmpbuf` with a fixed `MAX_DATASIZE` (262144 bytes), then reads `datasize` bytes from the file into it. `datasize` is derived from the actual file size and can exceed `MAX_DATASIZE`, causing a heap buffer overflow.
  *Fix: Clamp the read size: `if (datasize > MAX_DATASIZE) { fclose(fp); delete[] tmpbuf; return false; }` before calling `fread`.*

- **targautils.cpp:318** — `tmpbuf = new unsigned char[MAX_DATASIZE];` is followed by `fread(tmpbuf, datasize, 1, fp)` without checking whether `datasize <= MAX_DATASIZE`.
  *Fix: Reject or clamp `datasize` to `MAX_DATASIZE` before reading.*

- **terrainutil.cpp:878** — Inside `terrainutil_CanPlayerBuildAt`, the loop iterates `i < rec->GetNumPrerequisiteTileImp()` and then calls `cell->GetDBImprovement(i)`. The cell may have fewer DB improvements than the record has prerequisites, causing an out-of-bounds read.
  *Fix: Check `i < cell->GetNumDBImprovements()` before calling `cell->GetDBImprovement(i)`.*

- **thronedb.cpp:73** — `GetThroneInfo(sint32 type, sint32 level)` computes `index = (type * m_nThroneLevels) + level` without validating that `type` and `level` are non-negative or that `index` is within the allocated array bounds. Malformed database files or savegames can cause out-of-bounds access.
  *Fix: Add validation: `if (type < 0 || level < 0 || type >= m_nThroneTypes || level >= m_nThroneLevels) return NULL; sint32 index = (type * m_nThroneLevels) + level;`.*

- **tiledraw.cpp:3502** — `buildItemName = g_theUnitDB->Get(bn->m_type)->GetNameText();` accesses the unit database with `bn->m_type` without bounds checking. If the build queue contains a corrupted or out-of-range type index, this causes an out-of-bounds read or null dereference.
  *Fix: Validate `bn->m_type` against `g_theUnitDB->NumRecords()` before calling `Get()`.*

- **tiledraw.cpp:3505** — `buildItemName = g_theBuildingDB->Get(bn->m_type)->GetNameText();` — same issue as line 3502 but for the building database.
  *Fix: Validate `bn->m_type` against `g_theBuildingDB->NumRecords()` before calling `Get()`.*

- **tiledraw.cpp:3508** — `buildItemName = g_theWonderDB->Get(bn->m_type)->GetNameText();` — same issue as line 3502 but for the wonder database.
  *Fix: Validate `bn->m_type` against `g_theWonderDB->NumRecords()` before calling `Get()`.*

- **tileset.cpp:285** — `LoadBaseTiles()` stores `m_baseTiles[baseTile->GetTileNum()] = baseTile;` without checking that `GetTileNum()` is less than `k_MAX_BASE_TILES`. A malformed tileset file could cause an out-of-bounds write.
  *Fix: Add `if (baseTile->GetTileNum() >= k_MAX_BASE_TILES) { delete baseTile; continue; }` before storing.*

- **tileset.cpp:660** — `QuickLoadBaseTiles()` stores `m_baseTiles[baseTile->GetTileNum()] = baseTile;` without validating `GetTileNum()` against `k_MAX_BASE_TILES`.
  *Fix: Same bounds check as `LoadBaseTiles()`.*

- **tileutils.cpp:1377-1402** — `tileutils_ExtractStencils` indexes `g_transitions[fromType][toType][...]` using `sint16` values read from a script without validating they are within `[0, TERRAIN_MAX)`.
  *Fix: Add `Assert(fromType >= 0 && fromType < TERRAIN_MAX && toType >= 0 && toType < TERRAIN_MAX);` and early-return if out of range.*

- **tileutils.cpp:1569-1576** — `tileutils_BorkifyTile` indexes `tileImage[(y+yoffset)*k_TILE_PIXEL_WIDTH+x]` without verifying that `width >= k_TILE_PIXEL_WIDTH` or `height >= y+yoffset`. A smaller input TIF causes out-of-bounds read.
  *Fix: Assert or check that `width >= k_TILE_PIXEL_WIDTH` and `height >= k_TILE_PIXEL_HEIGHT + yoffset` before entering the loop.*

- **tracklen.cpp:369** — `tracklen_LoadEncryptedKey` reads a file and checks `(dwSize < 4) || (dwSize % 4)`, but never validates that `dwSize/4` fits in the caller's buffer (`trackLenBuf[DWVERSIONINFOLEN + tracklen_MAXTRACKS]`). A large file causes `fread`/`ReadFile` to overflow the buffer.
  *Fix: Reject files where `dwSize > sizeof(trackLenBuf)` (or pass the buffer size and check against it).*

- **tracklen.cpp:456** — `tracklen_LoadEncryptedKey` reads an entire file into `trackLenBuf` (a caller-provided `DWORD*`). The caller at line 507 allocates only `DWVERSIONINFOLEN + tracklen_MAXTRACKS` DWORDs on the stack. If the file on disk is larger, the read overflows the stack buffer.
  *Fix: Pass the buffer size into `tracklen_LoadEncryptedKey` and reject files that exceed it.*


### MISSING_BOUNDS_CHECK / NULL_DEREFERENCE

- **directorevent.cpp:222** — `g_player[g_selected_item->GetVisiblePlayer()]` is used without bounds checking the index, and the pointer is dereferenced without a null check.
  *Fix: Validate the index and check that `g_player[index]` is non-null before use.*


### NULL_DEREFERENCE

- **AgreementData.cpp:524** — `g_player[m_owner]->GetCivilisation()` is dereferenced without first checking whether `g_player[m_owner]` is non-NULL. If the owner player has been destroyed or is uninitialized, this crashes.
  *Fix: Add a null check before dereferencing: `if (g_player[m_owner]) civ = g_player[m_owner]->GetCivilisation();`*

- **AgreementData.cpp:540** — In `ExtractPlayer`, `g_player[m_owner]->GetCivilisation()` is dereferenced without checking whether `g_player[m_owner]` is non-NULL.
  *Fix: Add NULL check: `if (!g_player[m_owner]) return;`*

- **AgreementData.cpp:544** — `g_player[m_recipient]->GetCivilisation()` dereferenced without NULL check.
  *Fix: Add NULL check before dereferencing.*

- **AgreementData.cpp:544** — `g_player[m_recipient]->GetCivilisation()` is dereferenced without verifying `g_player[m_recipient]` is valid.
  *Fix: Guard with `if (g_player[m_recipient])` before the call.*

- **AgreementData.cpp:548** — `g_player[m_thirdParty]->GetCivilisation()` dereferenced without NULL check.
  *Fix: Add NULL check before dereferencing.*

- **AgreementData.cpp:548** — `g_player[m_thirdParty]->GetCivilisation()` is dereferenced without verifying `g_player[m_thirdParty]` is valid.
  *Fix: Guard with `if (g_player[m_thirdParty])` before the call.*

- **AgreementData.cpp:576** — `g_player[civ->GetOwner()]->GetGold()` is dereferenced without checking if `g_player[civ->GetOwner()]` is non-NULL.
  *Fix: Validate the player pointer before accessing member functions.*

- **ArmyData.cpp:1027** — `m_array[0].IsAsleep()` is called without checking whether `m_nElements > 0`. If the army is empty, `m_array[0]` is an out-of-bounds access.
  *Fix: Add guard: `if (m_nElements <= 0) return false;`*

- **ArmyData.cpp:1062** — `m_array[0].IsEntrenched()` accessed without checking `m_nElements > 0`.
  *Fix: Add guard before accessing `m_array[0]`.*

- **ArmyData.cpp:1068** — `m_array[0].IsEntrenching()` accessed without checking `m_nElements > 0`.
  *Fix: Add guard before accessing `m_array[0]`.*

- **ArmyData.cpp:2668** — `g_player[m_owner]->m_advances->HasAdvance(...)` dereferences `g_player[m_owner]` without a NULL check.
  *Fix: Add NULL check before dereferencing.*

- **ArmyData.cpp:2668** — `g_player[m_owner]->m_advances->HasAdvance(...)` is dereferenced without checking `g_player[m_owner]`.
  *Fix: Add null check on `g_player[m_owner]` before accessing `m_advances`.*

- **ArmyEvent.cpp:1098** — `g_player[attack_owner]->CreateLeader()` dereferences without NULL check. `attack_owner` comes from event arguments and could be invalid.
  *Fix: Validate index and check NULL: `if (attack_owner >= 0 && attack_owner < k_MAX_PLAYERS && g_player[attack_owner])`*

- **ArmyEvent.cpp:1145** — `g_player[defense_owner]->CreateLeader()` dereferences without NULL check.
  *Fix: Validate index and check NULL before dereferencing.*

- **ArmyEvent.cpp:766** — `g_player[defender->GetOwner()]->m_builtWonders` is accessed without validating the player index or pointer. If the defender owner is invalid or the player slot is empty, this crashes.
  *Fix: Check `defender->GetOwner()` is in bounds and `g_player[defender->GetOwner()]` is non-NULL.*

- **ArmyEvent.cpp:767** — `g_player[defender->GetOwner()]->m_builtWonders` dereferences `g_player[defender->GetOwner()]` without a NULL check.
  *Fix: Add NULL check: `if (defender->GetOwner() >= 0 && defender->GetOwner() < k_MAX_PLAYERS && g_player[defender->GetOwner()])`*

- **C3Robot.cpp:136** — `MapPoint *size = g_theWorld->GetSize();` is dereferenced immediately (`size->z`, `size->x`, `size->y`) without checking if `GetSize()` returned NULL.
  *Fix: Add a null check before dereferencing `size`.*

- **Cell.cpp:342** — `g_theResourceDB->Get(good)->GetFood()` is called without checking whether `Get(good)` returns NULL. If the good index is invalid, this dereferences a null pointer.
  *Fix: Check the return value: `const ResourceRecord *rec = g_theResourceDB->Get(good); if (rec) food += rec->GetFood();`.*

- **Cell.cpp:984** — `g_theTerrainDB->Get(m_terrain_type)->GetMovementType()` dereferences the database lookup result without a NULL check. If `m_terrain_type` is corrupted or invalid, this crashes.
  *Fix: Store the pointer and validate it before dereferencing: `const TerrainRecord *rec = g_theTerrainDB->Get(m_terrain_type); Assert(rec); if (rec) ...`.*

- **CityData.cpp:1198** — `g_player[m_owner]->GetGold()` is called without checking if `g_player[m_owner]` is non-NULL.
  *Fix: Guard the player pointer access with a null check.*

- **CityData.cpp:841** — `g_theUnitDB->Get(settlerType, g_player[m_owner]->GetGovernmentType())` dereferences `g_player[m_owner]` without a NULL check.
  *Fix: Add NULL check on `g_player[m_owner]` before use.*

- **CityData.cpp:841** — `settlerRec->GetSettleSize()` is called when `settlerRec` can be NULL (when `g_theUnitDB->Get(settlerType, ...)` fails).
  *Fix: Add `if (settlerRec)` before dereferencing.*

- **CityData.cpp:946** — `g_player[m_owner]->GetCivilisation()` dereferenced without NULL check.
  *Fix: Add NULL check on `g_player[m_owner]`.*

- **CityInfluenceIterator.cpp:234** — `g_theWorld->GetCell(center)->GetCityOwner()` dereferences the cell pointer without checking if `GetCell` returned NULL.
  *Fix: Store the cell pointer and verify it is non-NULL before dereferencing.*

- **FeatTracker.cpp:381** — `g_player[player]->m_score->AddFeat();` dereferences `g_player[player]` without a NULL check. If `player` is invalid or the player array slot is empty, this crashes.
  *Fix: Check `g_player[player] != NULL` before dereferencing.*

- **FeatTracker.cpp:527** — `g_player[city.GetOwner()]->m_all_cities->Num()` is accessed without verifying `g_player[city.GetOwner()]` is non-NULL.
  *Fix: Cache `city.GetOwner()`, validate the index, and check `g_player[owner] != NULL` before use.*

- **FeatTracker.cpp:570** — `g_player[defeated]->GetMaxCityCount()` is called without checking `g_player[defeated]` for NULL.
  *Fix: Add a NULL check before dereferencing.*

- **GWArchive.cpp:63** — `CreateDirectory(char *path)` passes `path` directly to `_mkdir(path)` and `TruncatePath(path)` without null checking. If called with NULL, it crashes.
  *Fix: Add `if (!path) return;` at the start of the function.*

- **GWFile.cpp:74** — `Deliver()` calls `strlen(stamp)` without checking if `stamp` is NULL. If a caller passes a NULL stamp, the game crashes immediately.
  *Fix: Add a NULL check before the strlen call: `if (!stamp) return false;`*

- **Goal.cpp:2185** — `g_player[target_owner]->GetNumCities()` is called without checking whether `g_player[target_owner]` is non-null. A dead or uninitialized player slot causes a crash.
  *Fix: Guard with `if (target_owner >= 0 && g_player[target_owner] && g_player[target_owner]->GetNumCities() < ...)`.*

- **Goal.cpp:2487** — `g_player[target_owner]->m_builtWonders` dereferences a player pointer that may be null if the target owner is a dead player.
  *Fix: Add `g_player[target_owner] != NULL` before accessing `m_builtWonders`.*

- **Goal.cpp:2513** — `m_target_army->RetPos()` is called inside `Get_Totally_Complete()` without first validating that `m_target_army` is a valid army instance. An invalid target army causes a null dereference.
  *Fix: Add `if (!m_target_army.IsValid()) return true;` at the start of the relevant branch.*

- **Goal.cpp:2795** — `g_player[target_owner]->m_advances` is dereferenced without checking if `g_player[target_owner]` is null. This occurs in the `CanStealTechnology` pretest.
  *Fix: Verify `g_player[target_owner] != NULL` before accessing its `m_advances` member.*

- **Goal.cpp (multiple lines):0** — `m_target_city.GetCityData()` is dereferenced without null checks at lines 2745, 2751, 2757, 2763, 2769, 2805, and 2812. If the city data is null (invalid city), the game crashes.
  *Fix: Introduce a local `CityData* cityData = m_target_city.GetCityData();` and return early if it is null.*

- **Happy.cpp:602** — `m_happiness += p->CityHappinessIncrease();` dereferences `p` without checking if it is NULL. If `g_player[cd.m_owner]` is NULL, this crashes.
  *Fix: Add null check: `if (!p) return;`*

- **Happy.cpp:697** — `CalcCrime(*cd, g_player[cd->m_owner]);` passes `g_player[cd->m_owner]` directly without null check. If the player pointer is NULL, `CalcCrime` dereferences it.
  *Fix: Validate player pointer before passing.*

- **Happy.cpp:717** — `Player *p = g_player[cd.m_owner];` followed by uses of `p` without null check in `GetGreedyPopHappiness`.
  *Fix: Add null check.*

- **MaterialPool.cpp:93** — `g_player[m_owner]->AddPoints(pointCost);` does not check `g_player[m_owner]` for NULL before dereferencing.
  *Fix: Add `if (g_player[m_owner])` before the call.*

- **MessageData.cpp:1413** — `SetTitle(MBCHAR *title)` calls `strlen(title)` without null check. If `title` is NULL, crash.
  *Fix: Add null check.*

- **MessageData.cpp:1416** — `SetTitle(MBCHAR *title)` calls `strlen(title)` without a NULL check. A NULL title pointer causes a crash.
  *Fix: Check `title != NULL` before `strlen`.*

- **MessageData.cpp:197** — `m_text = new char[strlen(s) + 1];` calls `strlen(s)` where `s` is a parameter that could be NULL. This crashes if `s` is NULL.
  *Fix: Add null check: `if (!s) { m_text = NULL; return; }`*

- **MessageData.cpp:197** — Constructor `MessageData(const ID id, const PLAYER_INDEX owner, const PLAYER_INDEX sender, const MESSAGE_TYPE type, MBCHAR *s)` calls `strlen(s)` without verifying `s` is non-NULL. A NULL message text causes an immediate crash.
  *Fix: Add `Assert(s != NULL)` or handle NULL gracefully before calling `strlen`.*

- **MessageData.cpp:328** — Destructor calls `g_player[m_owner]->StartResearching(m_advance)` when `m_advanceSet` is true, without checking `g_player[m_owner]` for NULL.
  *Fix: Add `if (g_player[m_owner])` before the call.*

- **MessageData.cpp:346** — `SetMsgText(MBCHAR const * s)` calls `strlen(s)` without checking for NULL. Passing NULL crashes.
  *Fix: Guard with `if (!s) return;` or `Assert(s)`.*

- **MessageData.cpp:348** — `SetMsgText(MBCHAR const * s)` calls `strlen(s)` and `strcpy(m_text, s)` without null check.
  *Fix: Add null check at function entry.*

- **MessagePool.cpp:106** — `MessagePool::Create` calls `g_player[owner]->AddMessage(newRequest)` without verifying `g_player[owner]` is non-NULL.
  *Fix: Add a NULL check before dereferencing.*

- **Order.cpp:119** — `m_round = g_turn->GetRound();` dereferences `g_turn` without null check. If `g_turn` is not yet initialized, crash.
  *Fix: Add null check: `if (g_turn) m_round = g_turn->GetRound(); else m_round = 0;`*

- **Order.cpp:120** — Constructor calls `g_turn->GetRound()` without checking that `g_turn` is non-NULL. If orders are created before the turn counter is initialized, this crashes.
  *Fix: Add `Assert(g_turn)` or a runtime NULL check before use.*

- **Order.cpp:198** — `operator delete(void *ptr)` does not check for NULL before casting and dereferencing: `(Order *)ptr` then `order->m_index`. In C++, `delete nullptr` calls this operator with NULL, causing a crash.
  *Fix: Add null guard: `if (!ptr) return;`*

- **Player.cpp:4047** — `Player::GiveAdvance()` dereferences `g_player[recipient]` without checking if it is NULL. The function only checks if the current player `HasAdvance(adv)` but does not validate that `recipient` is a valid, alive player before accessing their `m_advances`.
  *Fix: Add `if (!g_player[recipient]) return;` before the dereference.*

- **Player.cpp:4085-4099** — `Player::FormAlliance()` and `Player::SetAlliance()` dereference `g_player[ally]` without NULL check. `ally` is validated against `m_owner` but not against the player array bounds or NULL entries.
  *Fix: Add `if (!g_player[ally]) return;` at the start of both functions.*

- **Player.cpp:4118** — `Player::BreakAlliance()` dereferences `g_player[ally]->mask_alliance` without checking if `g_player[ally]` is NULL.
  *Fix: Add `if (!g_player[ally]) return;` before accessing `g_player[ally]->mask_alliance`.*

- **Player.cpp:4156** — `Player::GiveMap()` dereferences `g_player[recipient]->m_vision` without NULL check.
  *Fix: Add `if (!g_player[recipient]) return;` before the call.*

- **PlayerEvent.cpp:110** — `g_player[p1]->ContactMade(p2);` dereferences `g_player[p1]` without a NULL check. If the player slot is empty (e.g., in a game with fewer players), this crashes.
  *Fix: Add `if (!g_player[p1] || !g_player[p2]) return GEV_HD_Continue;` before the call.*

- **PlayerEvent.cpp:162** — `g_player[player]->BeginTurnPollution();` is called after validating the player index from the event args, but `g_player[player]` may still be NULL.
  *Fix: Add `if (!g_player[player]) return GEV_HD_Continue;` before the call.*

- **PlayerEvent.cpp:507** — `CreateImprovementEvent` handler calls `g_player[player]->CreateImprovement()` after checking `args->GetPlayer()` but does not verify `g_player[player] != NULL`.
  *Fix: Add `if (!g_player[player]) return GEV_HD_Continue;` before the call.*

- **PlayerEvent.cpp:537** — `GrantAdvanceEvent` handler calls `g_player[player]->m_advances->GiveAdvance()` without NULL check on `g_player[player]`.
  *Fix: Add NULL check before dereferencing.*

- **PlayerEvent.cpp:610** — `EstablishEmbassyEvent` handler calls `g_player[owner]->EstablishEmbassy()` without NULL check.
  *Fix: Add `if (!g_player[owner]) return GEV_HD_Continue;` before the call.*

- **PlayerEvent.cpp:717** — `GiveMapEvent` handler calls `g_player[from_player]->GiveMap()` after an `Assert` but no runtime NULL check.
  *Fix: Add runtime NULL check before the call.*

- **PlayerEvent.cpp:732** — `GiveCityEvent` handler calls `g_player[giftCity->GetOwner()]->GiveCity()` without checking if `g_player[giftCity->GetOwner()]` is NULL.
  *Fix: Add NULL check before dereferencing.*

- **PlayerEvent.cpp:746** — `EnterAgeEvent` handler calls `g_player[player]->EnterNewAge()` after an `Assert` but no runtime NULL check.
  *Fix: Add runtime NULL check before the call.*

- **Regard.cpp:79** — `Regard::GetUpdatedRegard()` dereferences `g_player[me]` with only an `Assert` guard. If `g_player[me]` is NULL (e.g., dead player), this crashes.
  *Fix: Add `if (!g_player[me]) return REGARD_TYPE_NEUTRAL;` before the dereference.*

- **SelItem.cpp:1126** — `army = g_theWorld->GetCell(pos)->UnitArmy();` dereferences the result of `GetCell` without checking for NULL. If the position is invalid, `GetCell` may return NULL.
  *Fix: `Cell *cell = g_theWorld->GetCell(pos); if (!cell) return; army = cell->UnitArmy();`.*

- **SelItem.cpp:1142** — `for(i = 0; i < army->Num(); i++)` dereferences `army` which may be NULL (from line 1126 where `GetCell` result is unchecked).
  *Fix: Ensure `army` is not NULL before dereferencing, or add a NULL check before the loop.*

- **SelItem.cpp:1312** — `if(cell->GetCity().m_id != 0)` dereferences `cell` without checking if `g_theWorld->GetCell(pos)` returned NULL.
  *Fix: Add `if (!cell) return false;` before using `cell`.*

- **SelItem.cpp:1340** — `for(sint32 i=0; i< cell->GetNumUnits(); i++)` dereferences `cell` without checking if `g_theWorld->GetCell(pos)` returned NULL.
  *Fix: Add a NULL check before accessing `cell`.*

- **SelItem.cpp:1433** — `Cell *cell = g_theWorld->GetCell(curPos);` is immediately used at line 1434 (`cell->GetCity()`) without checking for NULL.
  *Fix: Add `if (!cell) break;` after obtaining the cell pointer.*

- **SelItem.cpp:2643** — `else if(g_theWorld->GetCell(pos)->GetCity().m_id != (0))` dereferences the result of `GetCell` without checking for NULL.
  *Fix: Store the cell pointer in a local variable and check for NULL before accessing `GetCity()`.*

- **SlicFunc.cpp:2919** — In `Slic_IsHumanPlayer::Call`, `g_player[player]->IsNetwork()` is dereferenced inside an `else if(g_network.IsActive())` branch without first checking `g_player[player] != NULL`. `args->GetPlayer()` only validates the index range.
  *Fix: Add `if (!g_player[player]) { m_result.m_int = 0; return SFN_ERROR_OK; }` before the network branch.*

- **SlicFunc.cpp:3312** — In `Slic_CreateUnit::Call`, after validating `owner` is in range via `args->GetPlayer()`, the code calls `g_theUnitDB->Get(type, g_player[owner]->GetGovernmentType())` without checking if `g_player[owner]` is NULL. A dead/empty player slot causes an immediate segfault.
  *Fix: Add `if (!g_player[owner]) return SFN_ERROR_DEAD_PLAYER;` before the database lookup.*

- **SlicFunc.cpp:6436** — In `Slic_StringCompare::Call`, `cstring1` or `cstring2` may be NULL if `g_theStringDB->GetNameStr()` returns NULL for an invalid string ID. The subsequent `stricmp(string1, string2)` call dereferences NULL.
  *Fix: Add NULL checks after `GetNameStr()` calls and return `SFN_ERROR_OUT_OF_RANGE` if either string is NULL.*

- **SlicFunc.cpp:7397** — In `Slic_Pillage::Call`, `g_theGovernmentDB->Get(g_player[pl]->m_government_type)` is called without verifying `g_player[pl]` is non-NULL after range validation. Also `GetBuildingRushModifier()` result is used as a divisor without checking for zero.
  *Fix: Add NULL check for `g_player[pl]` before dereferencing; check divisor is non-zero before division.*

- **SlicSymbol.cpp:462** — `SlicSymbolData::GetPos` dereferences `m_val.m_struct->GetDataSymbol()` without first verifying `m_val.m_struct` is non-NULL. If `SetType(SLIC_SYM_STRUCT)` was called but no struct instance was allocated, this crashes.
  *Fix: Add `if (!m_val.m_struct) return FALSE;` before the dereference.*

- **SlicSymbol.cpp:534** — `SlicSymbolData::SetUnit` calls `GetStruct()->GetDataSymbol()`; `GetStruct()` may return NULL when the type is wrong or struct is unset, causing a crash.
  *Fix: Cache `GetStruct()` result and check for NULL before dereferencing.*

- **SlicSymbol.cpp:621** — `SlicSymbolData::GetText` does `dataSym = m_val.m_struct->GetDataSymbol();` then `dataSym->GetText(...)` without NULL checks on either pointer.
  *Fix: Check `m_val.m_struct` and `dataSym` for NULL before use.*

- **TaxRate.cpp:58** — `TaxRate::SetTaxRates()` accesses `g_player[owner]->m_government_type` without NULL check.
  *Fix: Add `if (!g_player[owner]) return;` before accessing.*

- **TradeOffer.cpp:24** — `TradeOffer::RemoveAllReferences()` calls `g_player[GetOwner()]->RemoveTradeOffer()` without NULL check.
  *Fix: Add `if (!g_player[GetOwner()]) return;` before the call.*

- **TradeOfferData.cpp:117** — `TradeOfferData::Accept()` calls `g_player[player]->GiveGold()` without NULL check on `g_player[player]`.
  *Fix: Add NULL check before the call.*

- **TradeOfferData.cpp:130** — `TradeOfferData::Accept()` calls `g_player[m_fromCity.GetOwner()]->CreateTradeRoute()` without NULL check.
  *Fix: Add NULL check before the call.*

- **TradeOfferData.cpp:136** — `TradeOfferData::Accept()` calls `g_player[player]->CreateTradeRoute()` without NULL check.
  *Fix: Add NULL check before the call.*

- **TurnCntEvent.cpp:65** — `Assert(g_player[player] != NULL)` is a debug-only macro. In release builds, if `player` is invalid or the slot is empty, line 68 (`g_player[player]->m_current_round = round;`) dereferences a null pointer without any runtime guard.
  *Fix: Add an explicit runtime check `if (!g_player[player]) return GEV_HD_Continue;` before using the pointer.*

- **Unit.cpp:120,125** — `Unit::KillUnit()` calls `g_player[GetOwner()]->AdjustEventPollution()` twice without checking if `g_player[GetOwner()]` is NULL.
  *Fix: Add NULL check or use a helper that safely handles NULL player pointers.*

- **Unit.cpp:232** — `Unit::RemoveAllReferences()` calls `g_player[owner]->RemoveUnitReference()` without NULL check.
  *Fix: Add `if (!g_player[owner]) return;` before the call.*

- **UnitActor.cpp:1933** — `g_theCivilisationDB->Get(civ)->GetNationUnitFlagIndex(civicon)` is called before checking `civ > -1`. When `civ` remains -1 (no matching civilization found), this dereferences an invalid / out-of-bounds record.
  *Fix: Swap the conditions: `if (civ > -1 && g_theCivilisationDB->Get(civ)->GetNationUnitFlagIndex(civicon))`.*

- **UnitData.cpp:404** — `Assert(g_player[m_owner]); g_player[m_owner]->RegisterYourArmyWasMoved(...)` only checks for NULL in debug builds. In release, if `m_owner` is invalid or the player slot is empty, this dereferences a NULL pointer.
  *Fix: Add a runtime null check before dereferencing: `if (g_player[m_owner]) g_player[m_owner]->RegisterYourArmyWasMoved(...);`*

- **agent.cpp:310** — `found_path = Path(m_army->GetOrder(0)->m_path);` — `GetOrder(0)` can return NULL if the army has no orders, causing a crash.
  *Fix: Check `m_army->GetOrder(0) != NULL` before dereferencing.*

- **agent.cpp:345** — `found_path = Path(army->GetOrder(0)->m_path);` — same unchecked `GetOrder(0)` dereference in `FindPath(Army, ...)`.
  *Fix: NULL-check `GetOrder(0)` before dereferencing.*

- **agent.cpp:687-703** — `const char * myText = rec->GetNameText();` — if `rec` is NULL (e.g., invalid goal type), `strlen(myText)` crashes. Also `new MBCHAR[strlen(myText) + 80]` and subsequent memory operations are unsafe.
  *Fix: Check `rec != NULL` before using it.*

- **agent.cpp:926-955** — Same pattern in `ClearOrders`: `const GoalRecord* rec = g_theGoalDB->Get(m_goal->Get_Goal_Type());` could return NULL if the goal type is invalid.
  *Fix: NULL-check `rec` before dereferencing.*

- **aui_control.cpp:1746** — `SetStatusTextCopy` calls `strlen(text)` without validating that `text` is non-NULL. If a caller passes `NULL`, `strlen` dereferences a null pointer.
  *Fix: Add an early return: `if (!text) return;`.*

- **aui_image.cpp:323** — In the `__AUI_USE_SDL__` branch, `SDL_LoadBMP(filename)` is called but the result is never checked for `NULL`. The very next lines access `bmp->format->Gmask` and `bmp->format->Gshift`, which will crash if the file failed to load.
  *Fix: Add `if (!bmp) return AUI_ERRCODE_LOADFAILED;` immediately after `SDL_LoadBMP`.*

- **aui_tipwindow.cpp:87** — `SetTipText` dereferences `m_staticTip` without checking for NULL. If `InitCommonLdl` did not find the tip block in LDL data, `m_staticTip` remains NULL and calling `SetText` or `GetTextFont` on it will crash.
  *Fix: Add a NULL check for `m_staticTip` at the start of `SetTipText`.*

- **background.cpp:372** — `tf->SetText(lemurpoo)` dereferences `tf` without checking if `aui_Ldl::GetObject("ScenarioEditor.WorldControls.PosField")` returned NULL.
  *Fix: Add `if (tf) tf->SetText(lemurpoo);`.*

- **c3cmdline.cpp:3168** — `Cell *c = g_theWorld->GetCell(pos.x, pos.y); c->Kill();` dereferences `c` without checking if `GetCell` returned NULL.
  *Fix: Add `if (!c) return;` before `c->Kill()`.*

- **c3fancywindow.cpp:236** — `m_border[TOPL]->Resize(...)` dereferences `m_border[TOPL]` without a NULL check. While some borders are checked in `InitCommon`, others like `TOPL`, `TOPR`, etc. are dereferenced unconditionally during resize.
  *Fix: Add NULL checks before each `m_border[i]->Resize(...)` call, similar to the null checks in `~C3FancyWindow`.*

- **civ3_main.cpp:1092** — `fprintf(txt, "%s\n", c3debug_ExceptionStackTraceFromFile(fopen("crash.txt", "r")));` passes the result of an inner `fopen` directly without checking for NULL. If "crash.txt" does not exist, `fopen` returns NULL and the exception handler dereferences it.
  *Fix: Store the inner `fopen` result in a variable and check for NULL before passing it.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1546** — `DrawHealthBar` calls `myCell->UnitArmy()->GetAverageHealthPercentage()` without verifying that `UnitArmy()` returns a non-NULL pointer.
  *Fix: `CellUnitList* army = myCell->UnitArmy(); if (army) ratio = std::max(0.0, army->GetAverageHealthPercentage());`.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:384** — `GetIDAndType` dereferences `g_player[owner]` without a NULL check: `g_theUnitDB->Get(unitType, g_player[owner]->GetGovernmentType())`. If `owner` is invalid or the player slot is empty, this crashes.
  *Fix: Add `if (!g_player[owner]) { *spriteID = CTPRecord::INDEX_INVALID; *groupType = GROUPTYPE_UNIT; return; }`.*

- **ctp2_code/gfx/tilesys/tiledraw.cpp:514** — `DrawPartiallyConstructedImprovement` calls `g_theTerrainImprovementDB->Get(type)` and immediately dereferences `rec->GetNumConstructionTiles()` without checking whether `rec` is NULL. A corrupt or missing DB entry causes an immediate null-pointer dereference.
  *Fix: Add a null check: `if (!rec) return;` before using `rec`.*

- **ctpai.cpp:631** — `sint32 round = g_player[playerId]->GetCurRound();` dereferences `g_player[playerId]` without checking for NULL. If the player slot is empty, this crashes.
  *Fix: Add `if (!g_player[playerId]) return GEV_HD_Continue;` before the dereference.*

- **ctpai.cpp:698** — `Player *player_ptr = g_player[playerId];` is immediately followed by `sint32 round = player_ptr->GetCurRound();` on line 699 without a NULL check. If `g_player[playerId]` is NULL, this crashes.
  *Fix: Add `if (!player_ptr) return GEV_HD_Continue;` before using `player_ptr`.*

- **diplomat.cpp:1740** — `if(g_player[sender]->HasWarWith(receiver))` — `g_player[sender]` is used without NULL check in `Execute_Proposal`.
  *Fix: Add NULL check before accessing `g_player[sender]`.*

- **director.cpp:719** — `SequencePtr seq = item->getSequence().lock();` is immediately used as `seq->GetAddedToActiveList(...)` without checking if `seq` is null. If the weak pointer has expired, this crashes.
  *Fix: Add `if (!seq) return;` before using `seq`.*

- **director.cpp:729-733** — `action->move_actor.lock()->IsActive()` calls `lock()` a second time without checking the result. Between the `expired()` check and the second `lock()`, the actor may have been destroyed, causing a null dereference.
  *Fix: Store the locked pointer in a local variable and null-check it.*

- **mapanalysis.cpp:247-248** — `m_projectedScience[player] = player_ptr->m_advances->GetProjectedScience();` — `m_advances` may be NULL.
  *Fix: NULL-check `m_advances` before calling `GetProjectedScience()`.*

- **mapanalysis.cpp:612** — `return g_theWorld->GetCell(pos)->CanEnter(m_movementTypeUnion[playerId]);` — `GetCell(pos)` can return NULL for invalid positions.
  *Fix: Check if `GetCell(pos)` is non-NULL before dereferencing.*

- **motivationevent.cpp:138** — `LowReserves_MotivationEvent` dereferences `g_player[playerId]` without a NULL check immediately after retrieving `playerId` from the event arguments. If the player has been eliminated, `g_player[playerId]` is NULL and the game crashes.
  *Fix: Add `if (g_player[playerId] == NULL) return GEV_HD_Continue;` before line 138.*

- **motivationevent.cpp:139** — Same function dereferences `g_player[playerId]->m_gold->GetIncome()` without a NULL check.
  *Fix: Same guard as above.*

- **motivationevent.cpp:140** — Same function dereferences `g_player[playerId]->GetGold()` without a NULL check.
  *Fix: Same guard as above.*

- **net_action.cpp:339** — `g_player[index]->GetAllArmiesList()->Access(m_data[0]).Num()` is executed without first checking whether `g_player[index]` is non-NULL. If the player does not exist, this dereferences a null pointer.
  *Fix: Add `if (!g_player[index]) break;` before the dereference.*

- **net_info.cpp:742** — In `NET_INFO_CODE_REMOVE_HUT` handling, `g_theWorld->GetCell(m_data, m_data2)->DeleteGoodyHut()` is called without checking the return value of `GetCell`. `GetCell` can return `NULL` for out-of-bounds coordinates (especially when `m_data`/`m_data2` come from a network packet), causing an immediate null pointer dereference.
  *Fix: `Cell *cell = g_theWorld->GetCell(m_data, m_data2); if (cell) cell->DeleteGoodyHut();`*

- **nproposalevent.cpp:100** — `General_NewProposalEvent` calls `g_player[sender]->IsRobot()` without verifying `g_player[sender]` is non-NULL. If the sender player has been eliminated, this crashes.
  *Fix: Add `if (g_player[sender] == NULL) return GEV_HD_Continue;` before the first dereference.*

- **nproposalevent.cpp:105** — Same function calls `g_player[sender]->IsRobot()` a second time without a NULL check.
  *Fix: Same guard as above.*

- **nproposalevent.cpp:1284** — `max_cost = g_theAdvanceDB->Get(next_advance)->GetCost();` — `Get(next_advance)` can return NULL if the advance index is invalid.
  *Fix: NULL-check the returned pointer.*

- **ns_string.cpp:30** — `string` can be NULL if `block->GetString("text")` returns NULL (when `k_NS_STRING_LDL_NODATABASE` is true or text is missing) or if `g_theStringDB->GetNameStr()` returns NULL. `strlen(string)` and `strcpy(m_string, string)` then dereference NULL.
  *Fix: Add a NULL check after assigning `string`: `if (!string) { m_string = new char[1]; m_string[0] = '\0'; return; }` or use a safe default string.*

- **primitives.cpp:3054** — `FILE * file = fopen("gtfb000.bin", "rb");` is followed immediately by `fread(..., file);` without checking if `fopen` returned NULL.
  *Fix: Add `if (!file) return;` after the `fopen` call.*

- **proposalanalysis.cpp:419-420** — `g_theAdvanceDB->Get(receiver_ptr->m_advances->GetResearching())->GetCost()` — if `GetResearching()` returns an invalid index, `Get()` returns NULL and is immediately dereferenced.
  *Fix: Store the result of `Get()` in a pointer and check for NULL before calling `GetCost()`.*

- **proposalanalysis.cpp:440-441** — Same pattern for `sender_ptr->m_advances->GetResearching()`.
  *Fix: NULL-check the AdvanceRecord pointer before dereferencing.*

- **proposalanalysis.cpp:870** — `GetDesire` calls `g_player[playerId]->GetPollutionLevel()` without checking if `g_player[playerId]` is NULL. The function takes `playerId` as a parameter with no validation.
  *Fix: Add `if (g_player[playerId] == NULL || g_player[foreignerId] == NULL) return;` at the start of the pollution-pact block.*

- **proposalanalysis.cpp:871** — `GetDesire` calls `g_player[foreignerId]->GetPollutionLevel()` without checking if `g_player[foreignerId]` is NULL.
  *Fix: Same guard as above.*

- **proposalresponseevent.cpp:1261** — `foreigner_ptr->GetRelativeStrength(sender);` — `foreigner_ptr` is not checked for NULL before this call.
  *Fix: Check `foreigner_ptr != NULL` before calling `GetRelativeStrength`.*

- **regardevent.cpp:108** — `g_theWorld->GetCell(u.RetPos())->GetCity().m_id == 0x0` — `GetCell()` result is dereferenced without NULL check.
  *Fix: Store the cell pointer and check for NULL.*

- **regardevent.cpp:178-179** — `g_theWorld->GetCell(from)->GetOwner()` and `g_theWorld->GetCell(to)->GetOwner()` — two unchecked `GetCell()` dereferences.
  *Fix: NULL-check both cell pointers.*

- **regardevent.cpp:653** — `Cell *cell = g_theWorld->GetCell(pos); PLAYER_INDEX victim = cell->AccessUnit(0)->GetOwner();` — `cell` and `AccessUnit(0)` are dereferenced without NULL checks.
  *Fix: Check `cell != NULL` and `cell->AccessUnit(0) != NULL`.*

- **regardevent.cpp:686-690** — `Cell *cell = g_theWorld->GetCell(pos);` then `cell->UnitArmy()` and `cell->AccessUnit(0)->GetOwner()` — unchecked dereferences.
  *Fix: NULL-check cell and unit pointers.*

- **robotcom.cpp:22** — If `new IC3RobotCom()` fails and returns NULL, `obj->AddRef()` dereferences a NULL pointer.
  *Fix: Check `if (!obj) return E_OUTOFMEMORY;` before calling `AddRef()`.*

- **scenarioeditor.cpp:569** — `if(!g_attractWindow) { g_attractWindow->Initialize(); }` calls `Initialize()` on a NULL pointer when `g_attractWindow` does not exist. The condition is inverted.
  *Fix: Change to `if(g_attractWindow) { g_attractWindow->Initialize(); }` or allocate `g_attractWindow` before calling `Initialize()`.*

- **segmentlist.cpp:282** — If `hash->m_segments[i]` is NULL, `m_segment` becomes NULL, and `m_segment->GetName()` in `Update()` dereferences NULL.
  *Fix: Add a NULL check before creating the `SegmentListItem` or inside `Update()` before calling `GetName()`.*

- **spriteutils.cpp:493** — `malloc(width * height * 8)` result is stored in `outBuf` but never checked for NULL before `destPixel = outBuf` is used. A failed allocation leads to immediate NULL dereference on the first pixel write.
  *Fix: Add `if (!outBuf) return;` after the malloc call.*

- **terrainutil.cpp:419** — `g_player[cellOwner]->AddUnitVision()` is called when `cellOwner >= 0`, but `g_player[cellOwner]` could still be NULL.
  *Fix: Add `if (cellOwner >= 0 && g_player[cellOwner])` before the call.*

- **tracklen.cpp:380** — `*(char*)_mbsrchr((BYTE*)szTemp, '\\') = 0;` does not check if `_mbsrchr` returned NULL. If the path contains no backslash, this dereferences NULL.
  *Fix: `char *pos = (char*)_mbsrchr((BYTE*)szTemp, '\\'); if (pos) *pos = '\0';`.*


### UNALIGNED_ACCESS

- **BaseTile.cpp:105** — `QuickRead()` performs direct pointer casts like `*(uint16 *)(*dataPtr)`, `*(uint8 *)(*dataPtr)`, and `(Pixel16 *)(*dataPtr)` to read data from a byte stream. On ARM64, dereferencing unaligned pointers through typed casts is undefined behavior and will crash with SIGBUS on strict-alignment architectures.
  *Fix: Replace all direct pointer dereferences with `memcpy()` into properly aligned local variables, e.g. `memcpy(&m_tileNum, *dataPtr, sizeof(uint16)); (*dataPtr) += sizeof(uint16);`.*

- **MapFile.cpp:641** — Inside `SaveCivilizations`, a `uint8*` pointer is cast to `uint32*` and dereferenced: `uint32 * longPtr = (uint32 *)ptr; *longPtr = ...;`. The `civs` buffer is allocated with `new uint8[...]`, which does not guarantee 4-byte alignment. Unaligned 32-bit access crashes on ARM64.
  *Fix: Replace the cast+dereference with `memcpy`: `memcpy(ptr, &value, sizeof(uint32));` where `value` is a local `uint32`.*

- **aui_sound.cpp:106** — `TrimWavHeader` iterates through WAV data byte-by-byte and casts `data` to `long *` to compare with the fourcc `"data"`. Because `data` is incremented one byte at a time, it is frequently not 4-byte aligned. Dereferencing an unaligned `long *` is undefined behavior and can crash on strict-alignment architectures (e.g., ARM, RISC-V) or be mis-optimized.
  *Fix: Use `memcmp(data, "data", 4)` instead of `*(long *)data`, and read the size field with `memcpy(&raw_data_size, data, 4)`.*

- **aui_sound.cpp:109** — Immediately after finding the `"data"` marker, the code reads the chunk size with `raw_data_size = *(long *)data`. The pointer `data` has the same alignment as the marker search position, which may not be 4-byte aligned.
  *Fix: Replace the cast with `memcpy(&raw_data_size, data, sizeof(long))`.*

- **net_diff.cpp:35** — The code casts a `uint8*` buffer directly to a `Difficulty*` struct pointer and dereferences it: `*g_player[pidx]->m_difficulty = *(Difficulty*)&buf[4];`. This is undefined behavior on ARM64 (and other strict-alignment architectures) because `buf` is a byte array with no guarantee of matching the `Difficulty` type's alignment requirement.
  *Fix: Use `memcpy` to copy the bytes into a properly aligned local `Difficulty` variable, then assign: `Difficulty tmp; memcpy(&tmp, &buf[4], sizeof(tmp)); *g_player[pidx]->m_difficulty = tmp;`*

- **netfunc.cpp:1242** — `PlayerStat::SetId` does `*(dpid_t *)&key.buf = i;`. `key.buf` is a `char` array that follows a `short len` member in `KeyStruct`, placing `buf` at offset 2. If `dpid_t` is 4 bytes (likely `uint32_t`/`DWORD`), this access is at a 2-byte-aligned address, which causes undefined behavior / bus error on strict-alignment architectures like ARM64.
  *Fix: Use `memcpy(&key.buf, &i, sizeof(i))` instead of the direct cast.*

- **netfunc.cpp:1247** — `PlayerStat::GetId` does `return *(dpid_t *)&key.buf;`. Same alignment issue as `SetId` above — `key.buf` is at offset 2 in `KeyStruct`, and reading a 4-byte `dpid_t` from there is unaligned on ARM64.
  *Fix: Use `memcpy(&i, &key.buf, sizeof(i)); return i;` instead of the direct cast.*

- **tileutils.cpp:488** — `RIMHeader * rhead = (RIMHeader *)buf;` casts a buffer pointer returned from `g_ImageMapPF->getData()` directly to a struct pointer. The buffer may not be aligned to the struct's alignment requirements, causing undefined behavior / crashes on ARM64.
  *Fix: Use `memcpy()` to copy the header into a local `RIMHeader` variable instead of direct pointer cast.*


### UNINITIALIZED_VARIABLE

- **ArmyData.cpp:4002** — In `CanConvertCity(double& death_chance, double& convert_chance)`, if `m_nElements == 0`, the function returns `true` at line ~4024 without ever initializing `best_death_chance`. The caller may then use the uninitialized value.
  *Fix: Initialize `best_death_chance` (and `best_convert_chance`) at declaration, or handle the empty-army case explicitly before the loop.*

- **segmentlist.cpp:237** — `AUI_ERRCODE retval` is declared but not initialized before being passed to `new SegmentListItem(&retval, ...)`. The `SegmentListItem` constructor checks `*retval` at line 257 before `InitCommonLdl` sets it, so the value is undefined.
  *Fix: Initialize `retval` to `AUI_ERRCODE_OK` before passing it to the constructor.*

- **sourcelist.cpp:318** — `AUI_ERRCODE retval` is declared but not initialized before being passed to `new SourceListItem(&retval, ...)`. The constructor checks `*retval` at line 424 before it is set by `InitCommonLdl`.
  *Fix: Initialize `retval` to `AUI_ERRCODE_OK` before the loop.*

- **sourcelist.cpp:325** — `m_segment` is not initialized in the `SourceList` constructor. If `ShowBreak()` is called before `DisplayWindow()` sets `m_segment`, `m_segment->FindLineNumber(offset)` dereferences an uninitialized/garbage pointer.
  *Fix: Initialize `m_segment = NULL` in the constructor and add a NULL check in `ShowBreak()`.*

- **tileutils.cpp:427-430** — In `tileutils_EncodeTile`, if every scanline is empty, `lastNonEmpty` remains -1 and the `else` branch only asserts (which is a no-op in release). `*endLinePtr` (pointing to uninitialized `new Pixel16[]` memory) is then used in the `tableSize` computation.
  *Fix: Initialize `*endLinePtr = 0` before the loop, or handle the all-empty case explicitly before computing `tableSize`.*

- **tracklen.cpp:123** — On the `USE_SDL` code path, `uint64 totalLen_ms;` is declared but never initialized. It is then used with `+=` in the track length accumulation loop and later compared against min/max disc lengths, leading to garbage values.
  *Fix: Initialize to zero: `uint64 totalLen_ms = 0;`.*

- **tracklen.cpp:~507** — In `tracklen_CheckTrackLengths`, after loading `dwSize` bytes into `trackLenBuf`, if `szVersionPtr` is non-NULL, the code computes `trackLenBuf[DWVERSIONINFOLEN] = trackLenBuf[0] - DWVERSIONINFOLEN`. If `trackLenBuf[0]` (which equals `dwSize/4`) is only slightly larger than `DWVERSIONINFOLEN`, `tracklen_CheckTrackLengths2(trackLenBuf + DWVERSIONINFOLEN)` is called with a small track count. The for-loop inside then reads `trackLenBuf[1]`, `trackLenBuf[2]`, etc., from uninitialized stack memory beyond what was loaded from the file.
  *Fix: Ensure the loaded file size is at least `sizeof(DWORD) * (DWVERSIONINFOLEN + 1 + tracklen_MAXTRACKS)` before calling `tracklen_CheckTrackLengths2`, or zero-initialize `trackLenBuf` before use.*

- **watchlist.cpp:268** — `AUI_ERRCODE retval` is declared but not initialized before being passed to `new WatchListItem(&retval, ...)`. The constructor checks `*retval` at line 292 before it is set by `InitCommonLdl`.
  *Fix: Initialize `retval` to `AUI_ERRCODE_OK` before passing it to the constructor.*


## MEDIUM Severity


### BUFFER_OVERFLOW

- **network.cpp:2431** — In chat `/rules` handling, `char buf[1024]` is used with `sprintf` and `sprintf(buf + strlen(buf), ...)` inside a loop over teammates. Leader names and civilization names (each up to 1024 bytes from `GetSingularCivName`) are concatenated without bounds checking, allowing `buf` to overflow.
  *Fix: Use `snprintf` with `sizeof(buf)` and track remaining space, or use a dynamically growing string buffer.*


### DANGEROUS_SHIFT

- **ArmyData.cpp:1487** — `(1 << m_array[0].GetOwner())` is undefined when the owner index is >= 32.
  *Fix: Cast to `uint32` before shifting and verify the index is < 32.*

- **ArmyData.cpp:2418** — `(1 << m_owner)` is undefined when `m_owner >= 32`.
  *Fix: Use `uint32(1) << m_owner` with a bounds check.*

- **ArmyEvent.cpp:1228** — `(0x1 << army_owner)` performs a left shift by `army_owner` bits. If `army_owner` is 32 or greater, this is undefined behavior for a 32-bit integer.
  *Fix: Use `(uint32(1) << army_owner)` and ensure `army_owner < 32`, or use a 64-bit mask.*

- **C3Player.cpp:1478** — `b = (0x00000001 << second);` left-shifts by `second` (a `PLAYER_INDEX`) without checking that it is less than 32. If `second >= 32`, the behavior is undefined.
  *Fix: Add a check: `if (second >= 0 && second < 32) b = (0x00000001 << second); else return false;`*

- **C3Player.cpp:1727** — `return (cd->m_built_improvements & ((uint64)1 << type)) != 0;` shifts by `type`. The asserts only check `type >= 0` and `type < NumRecords()`, but if the database has 64 or more records, `type` can be >= 64, causing undefined behavior.
  *Fix: Ensure `type < 64` before shifting, or use a wider bitset.*

- **C3Wonder.cpp:281** — `uint64 bit = (uint64(0x1) << idx_wonder);` shifts by `idx_wonder` without ensuring it is `< 64`. If the wonder database ever contains 64+ records, the shift is undefined behavior.
  *Fix: Add `Assert(idx_wonder < 64);` or cap the shift: `uint64 bit = (idx_wonder < 64) ? (uint64(0x1) << idx_wonder) : 0;`.*

- **CityData.cpp:1007** — `(uint64)1 << (uint64)i` in the single-player starting age loop has the same issue when `i >= 64`.
  *Fix: Assert `i < 64` before shifting.*

- **CityData.cpp:1716** — `buildingCheck = (uint64)1 << (uint64)i;` is undefined when `i >= 64`.
  *Fix: Guard with `Assert(i < 64)`.*

- **CityData.cpp:2162** — `(uint64)1 << b` in the building-enables-good loop is undefined if `b >= 64`.
  *Fix: Assert or check `b < 64`.*

- **CityData.cpp:2177** — `(uint64)1 << w` in the wonder-enables-good loop is undefined if `w >= 64`.
  *Fix: Assert or check `w < 64`.*

- **CityData.cpp:989** — `(uint64)1 << (uint64)i` is undefined behavior when `i >= 64`. The loop iterates over `g_theBuildingDB->NumRecords()` which could exceed 64.
  *Fix: Assert `i < 64` before shifting, or use a bitset for more than 64 buildings.*

- **MapPoint.cpp:631** — `x = (xy_pos.x - xy_pos.y) >> 1;` performs a right shift on a signed integer. If `xy_pos.x < xy_pos.y`, the subtraction yields a negative value, and right-shifting a negative signed integer is implementation-defined.
  *Fix: Cast to unsigned before shifting: `x = static_cast<sint16>(static_cast<uint16>(xy_pos.x - xy_pos.y) >> 1);` or use division by 2 instead.*

- **Player.cpp:380** — `mask_alliance = 0x01 << o;` performs a left shift using `o` (a `PLAYER_INDEX` / `sint32`) without a runtime bounds check. An `Assert(o < 32)` exists but is elided in release builds. If `o >= 32`, the shift is undefined behavior.
  *Fix: Add an explicit runtime guard before the shift: `if (o >= 32) { mask_alliance = 0; } else { mask_alliance = 0x01 << o; }`*

- **Player.cpp:4095** — `mask_alliance |= (0x01<<ally);` shifts by `ally` without checking `ally < 32`. If `ally` is out of range, undefined behavior occurs.
  *Fix: Add a runtime check `if (ally >= 0 && ally < 32)` before the shift.*

- **Player.cpp:4108** — `mask_alliance &= ~(0x01<<ally);` has the same issue as line 4095 — no runtime validation that `ally < 32`.
  *Fix: Add the same runtime bounds check before the shift.*

- **Player.cpp:5026** — `m_diplomatic_mute |= (1<<player);` shifts by `player` without runtime bounds validation. The `Assert` on the preceding line is not active in release.
  *Fix: Add `if (player >= 0 && player < 32)` before the shift.*

- **Player.cpp:5677** — `m_builtWonders |= ((uint64)1 << wonder);` shifts a 64-bit value by `wonder` without verifying `wonder < 64`. If a mod or data bug provides a wonder index >= 64, this is undefined behavior.
  *Fix: Guard the shift with `if (wonder >= 0 && wonder < 64)` or use a bounds-checked helper.*

- **Player.cpp:6713** — `m_embassies |= (1 << player);` shifts by `player` without ensuring `player < 32`. In release builds the preceding `Assert` is omitted.
  *Fix: Add `if (player >= 0 && player < 32)` before the bit operation.*

- **PlayerEvent.cpp:315** — `p->m_contactedPlayers & (1 << i)` where `i` loops up to `k_MAX_PLAYERS`. If `k_MAX_PLAYERS > 32`, the shift amount exceeds the bit width of `1` (a 32-bit int).
  *Fix: Use `1ULL << i` or add a static_assert that `k_MAX_PLAYERS <= 32`.*

- **SlicSegment.cpp:488** — `m_specialVariables |= (1 << which);` shifts by `which` without bounds checking. If `which >= 32`, this is undefined behavior.
  *Fix: Add `Assert(which < 32);` or use a wider type: `m_specialVariables |= (static_cast<uint32>(1) << which);` with a bounds check.*

- **Unit.cpp:1276** — `return GetDBRec()->GetVisionClass() & (1 << bit);` where `bit` is a `uint32` parameter. If `bit >= 32`, this is undefined behavior.
  *Fix: Add `Assert(bit < 32);` at the start of the function.*

- **UnitActor.cpp:2211** — `GetUnitVisibility() & (1 << visiblePlayer)` — `visiblePlayer` is `g_selected_item->GetVisiblePlayer()`. If this returns a value >= 32 (or negative), shifting by that amount is undefined behavior in C++.
  *Fix: Use a safe shift: `uint32 mask = (visiblePlayer >= 0 && visiblePlayer < 32) ? (1u << visiblePlayer) : 0;` before applying the bitwise AND.*

- **UnitActor.cpp:2246** — Same dangerous shift pattern as line 2211: `(1 << visiblePlayer)` where `visiblePlayer` is not range-checked.
  *Fix: Same safe-shift guard as line 2211.*

- **WonderTracker.cpp:132** — `GetBuiltWonders() & ((uint64)1 << which)` shifts by `which` without ensuring it is less than 64. An out-of-range building index triggers undefined behavior.
  *Fix: Validate `which` is in range `[0, 63]` before shifting.*

- **WonderTracker.cpp:90** — `(uint64)1 << (uint64)which` performs a 64-bit left shift without validating `which`. If `which >= 64`, the shift is undefined behavior.
  *Fix: Add a bounds check: `if (which < 0 || which >= 64) return false;` before the shift.*

- **WrlEnv.cpp:384** — `m_map[x][y]->m_env = ... | (t << k_SHIFT_ENV_MOVEMENT_TYPE);` left-shifts `t`, a `sint32` parameter. If `t` is negative, left-shifting a negative signed integer is undefined behavior.
  *Fix: Validate `t >= 0` before shifting, or cast `t` to `uint32`.*

- **aui_win.cpp:333** — `screen.y << 16` performs a left shift of a signed `sint32` by 16 bits. If `screen.y` is negative (possible from mouse coordinate calculations), left-shifting a negative signed integer is undefined behavior in C++.
  *Fix: Cast to `uint32` before shifting: `((uint32)(screen.y & 0xFFFF)) << 16`.*

- **aui_win.cpp:338** — `local.y << 16` shifts a signed `sint32` left by 16. If `local.y` is negative, this is undefined behavior.
  *Fix: Cast to `uint32` before shifting: `((uint32)(local.y & 0xFFFF)) << 16`.*

- **aui_win.cpp:387** — `local.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:479** — `screen.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:498** — `local.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:504** — `screen.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:523** — `local.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:579** — `screen.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:666** — `screen.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:685** — `local.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:723** — `screen.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **aui_win.cpp:742** — `local.y << 16` shifts a signed `sint32` left by 16. Negative values cause undefined behavior.
  *Fix: Cast to `uint32` before shifting.*

- **c3cmdline.cpp:2974** — `((uint64)1 << atoi(argv[1]))` shifts by an unchecked user-provided value. If `atoi(argv[1]) >= 64`, the shift is undefined behavior.
  *Fix: Validate the shift amount: `sint32 bit = atoi(argv[1]); if (bit < 0 || bit >= 64) return;`.*

- **c3cmdline.cpp:3002** — `if(g_debug_mask & (1 << bit))` shifts by `bit = atoi(argv[1])` without bounds checking. If `bit >= 32`, this is undefined behavior on 32-bit integers.
  *Fix: Check `bit >= 0 && bit < 32` before shifting, or use `(1ull << bit)` with a 64-bit mask.*

- **c3cmdline.cpp:3067** — `mask |= (1 << atoi(argv[i]));` shifts by an unvalidated integer parsed from user input. Values >= 32 trigger undefined behavior.
  *Fix: Validate `atoi(argv[i])` is in range `[0, 31]` before shifting.*

- **ctp2_code/gfx/gfx_utils/gfx_options.cpp:59** — `PackCellAVLKey` does `pos.x << 16`. If `pos.x` is negative (e.g., `MapPoint` uses signed coordinates), left-shifting a negative value is undefined behavior.
  *Fix: Cast to unsigned first: `return (static_cast<uint32>(pos.x) << 16) | static_cast<uint32>(pos.y & 0xFFFF);`.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:2211** — `(GetUnitVisibility() & (1 << visiblePlayer))` shifts by `visiblePlayer` without capping the shift amount. If `visiblePlayer >= 31` (or 63 on 64-bit), behavior is undefined.
  *Fix: Use `1ULL << visiblePlayer` and ensure `visiblePlayer < 64`.*

- **ctp2_code/gfx/spritesys/director.cpp:1254** — `(mover.GetVisibility() & (1 << g_selected_item->GetVisiblePlayer()))` shifts `1` (an `int`) by `GetVisiblePlayer()`. If the visible player index is 31 or higher, this is undefined behavior on platforms with 32-bit `int`.
  *Fix: Use `1u << static_cast<uint32>(g_selected_item->GetVisiblePlayer())` and cap the shift amount, or use a `uint64` mask.*

- **ctpai.cpp:823** — `((uint64)0x1 << (uint64)type)` uses `type` from packet data. If `type >= 64`, this is undefined behavior.
  *Fix: Mask the shift: `((uint64)1 << (type & 63))`.*

- **ctpai.cpp:823** — `GaiaController::sm_endgameImprovements & ((uint64)0x1 << (uint64)type)` shifts by `type`, a `sint32` read from event arguments. If `type >= 64`, this is undefined behavior.
  *Fix: Validate `type >= 0 && type < 64` before the shift.*

- **director.cpp:1151** — `(1 << g_selected_item->GetVisiblePlayer())` performs a left shift by an untrusted signed amount. If the visible player index is negative or >= 32, the shift is undefined behavior.
  *Fix: Validate the shift amount: `sint32 vis = g_selected_item->GetVisiblePlayer(); if (vis < 0 || vis >= 31) continue; ... (1u << vis)`.*

- **gfx_options.cpp:59** — `PackCellAVLKey` does `pos.x << 16` where `pos.x` is a signed `sint32`. If `pos.x` is negative, left-shifting a negative signed integer is undefined behavior in C++.
  *Fix: Cast to unsigned first: `return (static_cast<uint32>(pos.x) << 16 | static_cast<uint32>(pos.y));`*

- **gfx_options.cpp:59** — `return (pos.x << 16 | pos.y);` left-shifts a signed `pos.x`. If `pos.x` is negative, shifting a negative signed value is undefined behavior.
  *Fix: Cast to unsigned: `return (static_cast<uint32>(pos.x) << 16) | static_cast<uint32>(pos.y);`.*

- **net_info.cpp:1260** — `((uint64)1 << (uint64)m_data)` performs a 64-bit left shift using `m_data` which originates from untrusted network data. If `m_data >= 64`, this is undefined behavior in C++.
  *Fix: Mask the shift amount: `((uint64)1 << (m_data & 63))`.*

- **primitives.cpp:987** — `uint16 * pDest = (uint16 *)(pSurfBase + y1 * surfPitch + (x1 << 1));` left-shifts `x1` (a signed `sint32`) by 1. If `x1` is negative, this is undefined behavior.
  *Fix: Ensure `x1` is non-negative before the shift, or cast to `uint32`: `((uint32)x1 << 1)`.*

- **scenarioeditor.cpp:1567** — `city.CD()->GetBuiltWonders() & ((uint64)1 << (uint64)dbindex)` performs a 64-bit left shift by `dbindex` bits. If the wonder database contains 64 or more records, `dbindex >= 64` causes undefined behavior.
  *Fix: Add a bounds check: `if (dbindex < 64)` before the shift, or use a larger bitmask type / bitset.*

- **scenarioeditor.cpp:2020** — `city.CD()->GetBuiltWonders() | ((uint64)1 << (uint64)dbindex)` has the same dangerous shift issue when `dbindex >= 64`.
  *Fix: Add a bounds check before the shift.*

- **scenarioeditor.cpp:2126** — `city.CD()->GetBuiltWonders() & ((uint64)1 << (uint64)dbindex)` has the same dangerous shift issue when `dbindex >= 64`.
  *Fix: Add a bounds check before the shift.*

- **scheduler.cpp:1726** — `m_contactCache |= (1<<i);` performs a left shift where `i` iterates up to `k_MAX_PLAYERS`. If `k_MAX_PLAYERS` is 32 or greater, shifting a 32-bit `1` by 31+ bits is undefined behavior.
  *Fix: Use `(1u << i)` and ensure `i < 32`, or use a 64-bit mask with bounds checking.*

- **scheduler.cpp:1767** — `m_neutralRegardCache |= (1<<i);` same issue — shift amount can be >= 32 if `AgreementMatrix::s_agreements.GetMaxPlayers()` returns 32 or more.
  *Fix: Bounds-check `i` before shifting, or use a wider integer type.*

- **scheduler.cpp:1805** — `m_allyRegardCache |= (1<<i);` same dangerous shift issue when `i >= 32`.
  *Fix: Ensure `i < 32` or use `uint64_t` with `(1ull << i)`.*

- **terrainutil.cpp:428** — `~(1 << cellOwner)` where `cellOwner` is a `sint32` read from a cell. If `cellOwner >= 32`, this is undefined behavior.
  *Fix: Add `Assert(cellOwner >= 0 && cellOwner < 32);` or use `1ULL << cellOwner`.*

- **terrainutil.cpp:441** — `1 << cellOwner` with same issue as above.
  *Fix: Same as above.*

- **tileutils.cpp:1596** — `accum >>= 32-(endX-startX)-1;` simplifies to `accum >>= 31-(endX-startX)`. When `endX-startX` exceeds 31 (e.g., 64 for a full tile row), the shift amount becomes negative, which is undefined behavior. When `endX-startX` is negative, the shift amount exceeds 31, also undefined behavior.
  *Fix: Compute the shift amount safely: `sint32 shift = 31 - (endX - startX); if (shift >= 0 && shift < 32) accum >>= shift;`.*


### DIVISION_BY_ZERO

- **C3Wonder.cpp:213** — `p = the_city->GetStoredCityProduction() / node->m_cost;` does not check that `node->m_cost` is non-zero. A modded building with zero cost causes a division-by-zero crash.
  *Fix: Add `if (node->m_cost == 0) continue;` or `if (node->m_cost == 0) p = 0;`.*

- **FeatTracker.cpp:548** — `(numCities * 100) / g_player[...]->m_all_cities->Num()` divides by the city's count. If a player has zero cities, `Num()` returns 0, causing a division-by-zero crash.
  *Fix: Check that `Num() > 0` before dividing.*

- **GaiaController.cpp:239** — `m_percentCoverage = ((float) covered_cells / (g_theWorld->GetXWidth() * g_theWorld->GetYHeight()));` divides by world area. If world dimensions are zero (corrupted state), division by zero occurs.
  *Fix: Check denominator: `sint32 area = g_theWorld->GetXWidth() * g_theWorld->GetYHeight(); if (area > 0) m_percentCoverage = ...`*

- **GaiaController.cpp:239** — `m_percentCoverage` is computed by dividing by `(g_theWorld->GetXWidth() * g_theWorld->GetYHeight())`. If the world dimensions are zero, this causes a division-by-zero crash.
  *Fix: Check that the product is non-zero before dividing.*

- **GaiaController.cpp:801** — `return (float) (covered_cells / (g_theWorld->GetXWidth() * g_theWorld->GetYHeight()));` divides by world area without zero check.
  *Fix: Same as above.*

- **GaiaController.cpp:801** — `NewCoverageFrom` divides by `(g_theWorld->GetXWidth() * g_theWorld->GetYHeight())` without checking for zero area.
  *Fix: Add a zero-check guard before the division.*

- **MapPoint.cpp:520** — In `OldSquaredDistance` (debug build), the code computes `sint32 adjX1 = (((uPos.x + (uPos.y / 2)) % w) * 2) + (uPos.y & 1);` where `w = sint16(g_theWorld->GetXWidth())`. If the map width is 0, the modulo operator `% w` divides by zero.
  *Fix: Add `Assert(w > 0); if (w <= 0) return 0;` before the modulo.*

- **Player.cpp:3943** — `average /= g_theConstDB->Get(0)->GetAveragePollutionTurns();` divides by the database value without checking for zero. A zero value in const.txt causes a crash.
  *Fix: Add `sint32 turns = g_theConstDB->Get(0)->GetAveragePollutionTurns(); if (turns > 0) average /= turns;`.*

- **Player.cpp:7450** — `Player::GetPercentProductionToMilitary()` returns `m_readiness->GetCost() / m_total_production` without checking if `m_total_production` is zero. The caller does check for zero, but this is a public method that could be called from other contexts.
  *Fix: Add `if (m_total_production == 0) return 0.0;` at the start of the function.*

- **RejectResponseEvent.cpp:149-150** — `MapAnalysis::GetMapAnalysis().TotalThreat(sender) / MapAnalysis::GetMapAnalysis().TotalThreat(receiver)` — `TotalThreat(receiver)` can return 0.
  *Fix: Check for zero before dividing.*

- **agent.cpp:318** — `GetRoundsPrecise` computes `100.0 / move_points` where `move_points` comes from `m_army->MinMovementPoints(move_points)`. An immobilized or exhausted army can have zero movement points, causing division by zero.
  *Fix: Guard with `if (move_points > 0.0)` before the division; return a large value or handle appropriately when `move_points` is zero.*

- **agent.cpp:456** — `GetRoundsPrecise` computes `move_point_cost / min_move` where `min_move` comes from `m_army->MinMovementPoints(min_move)`. If the army has zero minimum movement points, this crashes.
  *Fix: Guard with `if (min_move > 0.0)` before the division.*

- **counterresponseevent.cpp:602** — `((double) receiver_piracy / sender_trade_total) < 0.5` — `sender_trade_total` can be 0.
  *Fix: Check `sender_trade_total > 0` before dividing.*

- **counterresponseevent.cpp:618** — `receiver_trade_total > (sender_trade_total * 2)` uses `sender_trade_total` as a multiplier but nearby line 622 uses it as a divisor: `sender_piracy > (sender_trade_total * 0.25)`. The actual division at line 602 is the concerning one.
  *Fix: Ensure `sender_trade_total != 0` before any ratio calculations.*

- **ctp2_code/gfx/spritesys/SpriteLow565.cpp:876** — `DrawScaledLow565` computes `vpos2 = (sint32)((double)(m_height - destHeight) / (double)destHeight);` without verifying `destHeight != 0`.
  *Fix: Return early if `destWidth <= 0 || destHeight <= 0` at the top of the function.*

- **ctp2_code/gfx/tilesys/tiledraw.cpp:1992** — `DrawDitheredOverlayScaled` computes `vpos2` using division by `(double)destHeight`. If `destHeight` is 0 (which can be passed from callers that don't validate), this triggers a floating-point division-by-zero.
  *Fix: Add an early return if `destWidth <= 0 || destHeight <= 0`.*

- **ctpai.cpp:1944** — `cityData->SlaveCount() / g_theConstDB->Get(0)->GetSlavesPerMilitaryUnit()` divides by `GetSlavesPerMilitaryUnit()` which could return 0 (e.g., from modded data).
  *Fix: Check `if (slavesPerUnit > 0)` before dividing.*

- **diplomat.cpp:4209** — `return static_cast<sint32>(floor( (current_savings / goldSpent) * 100.0 ));` — `goldSpent` can be 0 if all gold-spent categories sum to zero.
  *Fix: Check `goldSpent != 0` before dividing.*

- **diplomat.cpp:4209** — `GetGoldSurplusPercent` computes `current_savings / goldSpent` without checking if `goldSpent` is zero. `goldSpent` is the sum of several gold expenditure categories (cleric, crime, maintenance, wages, science), all of which can be zero simultaneously.
  *Fix: Add a guard: `if (goldSpent == 0) return 0;` before the division.*

- **diplomat.cpp:4228** — `return static_cast<sint32>(floor((foreign_advances/ my_advances) * 100.0));` — `my_advances` can be 0.
  *Fix: Check `my_advances != 0` before dividing.*

- **diplomat.cpp:4228** — `GetAdvanceLevelPercent` divides `foreign_advances / my_advances` without checking if `my_advances` is zero. At the very beginning of the game a player may have zero advances.
  *Fix: Add a guard: `if (my_advances == 0) return 0;` before the division.*

- **diplomat.cpp:4918** — `if (((double) tmp_nuke_count / our_vulnerable_city_count) > risk)` — `our_vulnerable_city_count` can be 0.
  *Fix: Check `our_vulnerable_city_count > 0` before dividing.*

- **diplomat.cpp:4918** — `ComputeNuclearLaunchTarget` divides `tmp_nuke_count / our_vulnerable_city_count`. `our_vulnerable_city_count` is computed by iterating the player's cities; if every city is safe from nukes (or the player has no cities), the count is zero.
  *Fix: Guard the division with `if (our_vulnerable_city_count > 0)`.*

- **diplomat.cpp:4932** — `if (((double) our_nuke_count / foreign_vulnerable_city_count) < superiority)` — `foreign_vulnerable_city_count` can be 0.
  *Fix: Check `foreign_vulnerable_city_count > 0` before dividing.*

- **diplomat.cpp:4932** — `ComputeNuclearLaunchTarget` divides `our_nuke_count / foreign_vulnerable_city_count`. `foreign_vulnerable_city_count` can be zero if the foreign player has no cities or all cities are safe from nukes.
  *Fix: Guard the division with `if (foreign_vulnerable_city_count > 0)`.*

- **governor.cpp:1503** — `double percentPerPlayer = 1.0 / double(numPlayers);` where `numPlayers` is computed from the player list. If no players exist, `numPlayers` is 0.
  *Fix: Check `if (numPlayers > 0)` before dividing.*

- **governor.cpp:1725** — Similar to line 1730, `World::GetAvgShieldsFromTerrain()` is used as a divisor without checking for zero.
  *Fix: Validate the divisor is non-zero before division.*

- **governor.cpp:1728** — Similar to line 1730, `World::GetAvgGoldFromTerrain()` is used as a divisor without checking for zero.
  *Fix: Validate the divisor is non-zero before division.*

- **governor.cpp:1730** — `double terr_food_rank = (g_theWorld->GetCell(pos)->GetFoodFromTerrain()) / (double) World::GetAvgFoodFromTerrain();` divides by the average food from terrain. If this average is 0 (e.g., an empty or uninitialized map), this crashes.
  *Fix: Check the return value before dividing: `double avgFood = World::GetAvgFoodFromTerrain(); if (avgFood == 0.0) avgFood = 1.0;`*

- **governor.cpp:2499** — `tmpCount = static_cast<sint32>(ceil(static_cast<double>(foodMissing)/farmersEff));` divides by `farmersEff`. If `city->GetFarmersEffect(i)` returns 0, this is a division by zero.
  *Fix: Check `if (farmersEff > 0)` before dividing.*

- **governor.cpp:2874** — `double const peopleFraction = totalTilesForWorking / numPeople;` where `numPeople = city->GetRingSize(i)`. If `GetRingSize(i)` returns 0, this is division by zero.
  *Fix: Check `if (numPeople > 0)` before dividing.*

- **governor.cpp:2977-2984** — `foodFraction = foodRequired / food`, `prodFraction = prodRequired / prod`, etc. If `food`, `prod`, `gold`, or `scie` is zero, division by zero occurs.
  *Fix: Check each denominator for zero before dividing.*

- **governor.cpp:2981** — `double foodFraction = foodRequired / food;` where `food` is `city->GetMaxProcessFood()`. If this returns 0, division by zero occurs. Same for `prod`, `gold`, and `scie` on lines 2982-2984.
  *Fix: Add checks like `if (food == 0.0) food = 1.0;` before dividing.*

- **governor.cpp:3071** — `double wages_percent = static_cast<double>(total_gold_cost) / static_cast<double>(gross_gold);` divides by `gross_gold`. If a city produces 0 gross gold, this crashes.
  *Fix: `if (gross_gold == 0) { gold_test = false; } else { ... }`*

- **mapanalysis.cpp:1068** — `GetPopulationPercent` divides by `m_worldPopulation` which is only protected by an `Assert`. In release builds (where asserts are disabled), `m_worldPopulation` can be zero at game start or in an empty world, causing a crash.
  *Fix: Replace the assert with a runtime guard: `if (m_worldPopulation == 0) return 0.0;`.*

- **mapanalysis.cpp:1074** — `return (double)m_totalPopulation[playerId] / m_worldPopulation;` — `m_worldPopulation` can be 0 at game start.
  *Fix: Check `m_worldPopulation != 0` before dividing.*

- **mapanalysis.cpp:1075** — `return (double)m_landArea[playerId] / (g_theWorld->GetWidth() * g_theWorld->GetHeight());` — world dimensions could theoretically be 0.
  *Fix: Check denominator is non-zero.*

- **mapanalysis.cpp:388-389** — `sint32 per_cell_value = (sint32)(((double)route_value / path->Num()) * 1000.0);` — `path->Num()` can return 0 for empty paths.
  *Fix: Check `path->Num() > 0` before dividing.*

- **motivationevent.cpp:220** — `if ( piracy_loss > income * 0.2)` — `income * 0.2` is a float comparison, but if `income` were used as a divisor elsewhere it would be problematic. The real issue is nearby in the same file where income is multiplied by 2 at line 138.
  *Fix: The main fix is for line 138 (already reported above).*

- **nproposalevent.cpp:1001** — `reduce_percent = (double)(receiver_pollution - sender_pollution) / receiver_pollution;` divides by `receiver_pollution` which may be zero.
  *Fix: Add zero-check before division.*

- **nproposalevent.cpp:1001** — Same function computes `(receiver_pollution - sender_pollution) / receiver_pollution` without guarding against `receiver_pollution == 0`.
  *Fix: Guard with `if (receiver_pollution > 0)` before the division.*

- **nproposalevent.cpp:1605** — `ReduceWeapons_NewProposalEvent` computes `receiver_nano_count / sender_city_count`. `sender_city_count` is obtained from `g_player[sender]->GetNumCities()` and can be zero if the sender has been eliminated.
  *Fix: Guard with `if (sender_city_count > 0)` before each division.*

- **nproposalevent.cpp:1605-1618** — `sender_losses` and `receiver_losses` are computed by dividing weapon counts by `sender_city_count` and `receiver_city_count`. If a player has zero cities, this causes division by zero.
  *Fix: Check `sender_city_count > 0` and `receiver_city_count > 0` before dividing.*

- **nproposalevent.cpp:1606** — Same function computes `sender_nano_count / receiver_city_count`. `receiver_city_count` can be zero if the receiver has been eliminated.
  *Fix: Guard with `if (receiver_city_count > 0)` before each division.*

- **nproposalevent.cpp:1611** — Same function computes `receiver_bio_count / sender_city_count` without guarding against `sender_city_count == 0`.
  *Fix: Guard with `if (sender_city_count > 0)` before the division.*

- **nproposalevent.cpp:1612** — Same function computes `sender_bio_count / receiver_city_count` without guarding against `receiver_city_count == 0`.
  *Fix: Guard with `if (receiver_city_count > 0)` before the division.*

- **nproposalevent.cpp:1617** — Same function computes `receiver_nukes_count / sender_city_count` without guarding against `sender_city_count == 0`.
  *Fix: Guard with `if (sender_city_count > 0)` before the division.*

- **nproposalevent.cpp:1618** — Same function computes `sender_nukes_count / receiver_city_count` without guarding against `receiver_city_count == 0`.
  *Fix: Guard with `if (receiver_city_count > 0)` before the division.*

- **nproposalevent.cpp:1846-1848** — `MapAnalysis::GetMapAnalysis().TotalThreat(sender)` is used as divisor without zero check.
  *Fix: Check for zero before dividing.*

- **nproposalevent.cpp:570** — `double pollution_ratio = (double) sender_pollution / receiver_pollution;` divides by `receiver_pollution` without checking if it is zero. A player with zero pollution causes undefined behavior.
  *Fix: Add a zero-check before dividing: `if (receiver_pollution == 0) return GEV_HD_Continue;`*

- **nproposalevent.cpp:570** — `ReducePollution_NewProposalEvent` computes `sender_pollution / receiver_pollution` without guarding against `receiver_pollution == 0`.
  *Fix: Guard with `if (receiver_pollution > 0)` before the division.*

- **nproposalevent.cpp:581** — Same function computes `(receiver_pollution - sender_pollution) / receiver_pollution` without guarding against `receiver_pollution == 0`.
  *Fix: Guard with `if (receiver_pollution > 0)` before the division.*

- **nproposalevent.cpp:658** — `double pollution_ratio = (double) receiver_pollution / promised_pollution;` divides by `promised_pollution` which can be zero if the agreement argument was initialized to zero.
  *Fix: Guard the division with `if (promised_pollution == 0) return GEV_HD_Continue;`*

- **nproposalevent.cpp:658** — `HonorPollutionPact_NewProposalEvent` computes `receiver_pollution / promised_pollution` without checking if `promised_pollution` is zero. `promised_pollution` is read from a prior agreement and could theoretically be zero.
  *Fix: Guard with `if (promised_pollution > 0)` before the division.*

- **nproposalevent.cpp:758** — `StopPiracy_NewProposalEvent` computes `sender_trade_from / sender_trade_total` without checking if `sender_trade_total` is zero.
  *Fix: Guard with `if (sender_trade_total > 0)` before the division.*

- **nproposalevent.cpp:990** — `double pollution_ratio = (double) sender_pollution / receiver_pollution;` in `PollutionPact_NewProposalEvent` divides by `receiver_pollution` without zero check.
  *Fix: Add `if (receiver_pollution == 0) return GEV_HD_Continue;` before the division.*

- **nproposalevent.cpp:990** — `OfferReducePollution_NewProposalEvent` computes `sender_pollution / receiver_pollution` without guarding against `receiver_pollution == 0`.
  *Fix: Guard with `if (receiver_pollution > 0)` before the division.*

- **proposalanalysis.cpp:284-285** — `map.GetAlliedValue(sender, city.RetPos()) / map.GetMaxAlliedValue(sender)` — `GetMaxAlliedValue` can return 0 if no allies exist.
  *Fix: Check for zero before dividing.*

- **proposalanalysis.cpp:305-307** — Similar division by `map.GetMaxAlliedValue(receiver)` without zero check.
  *Fix: Check for zero before dividing.*

- **proposalanalysis.cpp:453** — `scale_regard = (double) proposal_arg.gold / receiver_ptr->m_gold->GetIncome();` divides by income without zero check.
  *Fix: Check `GetIncome() != 0` before dividing.*

- **proposalanalysis.cpp:461-462** — Same pattern for sender income.
  *Fix: Check `GetIncome() != 0` before dividing.*

- **proposalanalysis.cpp:746** — `regard_ratio = regard_delta / diplomat.GetReceiverRegardResult(foreignerId, treaty_type);` — `GetReceiverRegardResult` can return 0.
  *Fix: Check for zero before dividing.*

- **proposalanalysis.cpp:754** — `GetDesire` computes `trade_from / trade_total` without verifying `trade_total` is non-zero. `trade_total` is fetched from `MapAnalysis::GetTotalTrade(playerId)` and can return 0 when a player has no trade routes.
  *Fix: Add a guard: `if (trade_total == 0) { not_trading_partner = true; } else { ... }`.*

- **proposalanalysis.cpp:755** — `double trade_percent = (double) trade_from / trade_total;` — `trade_total` can be 0.
  *Fix: Check `trade_total > 0` before dividing.*

- **proposalanalysis.cpp:872** — `double pollution_ratio = (double) player_pollution / foreigner_pollution;` — `foreigner_pollution` can be zero.
  *Fix: Check for zero before dividing.*

- **proposalanalysis.cpp:872** — `GetDesire` computes `player_pollution / foreigner_pollution` without checking if `foreigner_pollution` is zero. A player with no pollution sources will have a pollution level of 0.
  *Fix: Guard with `if (foreigner_pollution > 0)` before dividing.*

- **proposalanalysis.cpp:874** — `double requested_reduction = 1.0 - ((double)treaty_arg.pollution / player_pollution);` — `player_pollution` can be zero.
  *Fix: Check for zero before dividing.*

- **proposalanalysis.cpp:874** — `GetDesire` computes `treaty_arg.pollution / player_pollution` without checking if `player_pollution` is zero. This causes a division by zero when the proposing player has zero pollution.
  *Fix: Guard with `if (player_pollution > 0)` before dividing.*

- **proposalresponseevent.cpp:1057-1074** — In `ReduceWeapons_ProposalResponseEvent`, `sender_losses` and `receiver_losses` divide by `sender_city_count` and `receiver_city_count` which can be zero.
  *Fix: Check city counts are > 0 before dividing.*

- **proposalresponseevent.cpp:106-108** — `MapAnalysis::GetMapAnalysis().TotalThreat(sender)` is used as a divisor without checking for zero.
  *Fix: Add zero-check before division.*

- **proposalresponseevent.cpp:1060** — `ReduceWeapons_ProposalResponseEvent` computes `receiver_nano_count / sender_city_count`. `sender_city_count` can be zero.
  *Fix: Guard with `if (sender_city_count > 0)` before each division.*

- **proposalresponseevent.cpp:1061** — Same function computes `sender_nano_count / receiver_city_count`. `receiver_city_count` can be zero.
  *Fix: Guard with `if (receiver_city_count > 0)` before each division.*

- **proposalresponseevent.cpp:1066** — Same function computes `receiver_bio_count / sender_city_count` without guarding against `sender_city_count == 0`.
  *Fix: Guard with `if (sender_city_count > 0)` before the division.*

- **proposalresponseevent.cpp:1067** — Same function computes `sender_bio_count / receiver_city_count` without guarding against `receiver_city_count == 0`.
  *Fix: Guard with `if (receiver_city_count > 0)` before the division.*

- **proposalresponseevent.cpp:1072** — Same function computes `receiver_nukes_count / sender_city_count` without guarding against `sender_city_count == 0`.
  *Fix: Guard with `if (sender_city_count > 0)` before the division.*

- **proposalresponseevent.cpp:1073** — Same function computes `sender_nukes_count / receiver_city_count` without guarding against `receiver_city_count == 0`.
  *Fix: Guard with `if (receiver_city_count > 0)` before the division.*

- **proposalresponseevent.cpp:1151** — `HonorPollutionPact_ProposalResponseEvent` computes `sender_pollution / receiver_pollution` without guarding against `receiver_pollution == 0`.
  *Fix: Guard with `if (receiver_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:1166** — `reduce_percent = (double)(sender_pollution - receiver_pollution) / sender_pollution;` — `sender_pollution` can be 0.
  *Fix: Check for zero before dividing.*

- **proposalresponseevent.cpp:1166** — Same function computes `(sender_pollution - receiver_pollution) / sender_pollution` without guarding against `sender_pollution == 0`.
  *Fix: Guard with `if (sender_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:1352** — `RequestHonorPollutionPact_ProposalResponseEvent` computes `sender_pollution / sender_promised_pollution` without guarding against `sender_promised_pollution == 0`.
  *Fix: Guard with `if (sender_promised_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:1359** — Same function computes `((double)sender_pollution - sender_promised_pollution) / sender_pollution` without guarding against `sender_pollution == 0`.
  *Fix: Guard with `if (sender_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:1406** — `OfferHonorPollutionPact_ProposalResponseEvent` computes `receiver_pollution / sender_pollution` without guarding against `sender_pollution == 0`.
  *Fix: Guard with `if (sender_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:1414** — Same function computes `(sender_pollution - receiver_pollution) / sender_pollution` without guarding against `sender_pollution == 0`.
  *Fix: Guard with `if (sender_pollution > 0)` before the division.*

- **proposalresponseevent.cpp:246-248** — `MapAnalysis::GetMapAnalysis().TotalThreat(receiver)` is used as a divisor without checking for zero.
  *Fix: Add zero-check before division.*

- **scenariowindow.cpp:469** — `rand() % (g_theCivilisationDB->NumRecords() - 1)` will divide by zero if the civilisation database has exactly 1 record, because the modulus operand becomes 0.
  *Fix: Guard with `if (g_theCivilisationDB->NumRecords() > 1)` before computing the random index.*

- **threatresponseevent.cpp:103** — `CityThreat_ThreatResponseEvent` computes `value_requested / max_value` without checking if `max_value` is zero. `max_value` comes from `MapAnalysis::GetMaxAlliedValue(receiver)` which can return 0 if the receiver has no cities or no value at risk.
  *Fix: Guard with `if (max_value > 0)` before the division.*

- **threatresponseevent.cpp:113** — `double risk = MapAnalysis::GetMapAnalysis().CityAtRiskRatio(city, sender);` followed by `bool reasonable_demand = ((risk * value_at_risk) > 5 * value_requested);` — the actual division by zero risk is inside `CityAtRiskRatio` where `player_threat` is checked, but `GetMaxAlliedValue` can return 0.
  *Fix: Already covered by the `GetMaxAlliedValue` division-by-zero bugs.*


### FLOAT_CAST_OVERFLOW

- **GaiaController.cpp:775** — `add_radius = (sint16) (max_radius - min_radius);` casts a `sint32` difference to `sint16` without bounds checking. If the difference exceeds `INT16_MAX` (32767), the result is implementation-defined.
  *Fix: Clamp before cast: `add_radius = (sint16) std::min((sint32)INT16_MAX, max_radius - min_radius);`*

- **GaiaController.cpp:777** — `add_radius = (sint16) ceil(ratio * (double) (max_radius - min_radius));` casts a potentially large double to `sint16` without bounds checking.
  *Fix: Clamp the value before casting to `sint16`.*

- **GaiaController.cpp:780** — `return ((sint16) min_radius + add_radius);` casts `min_radius` (sint32) to `sint16`. If `min_radius` > 32767, implementation-defined behavior.
  *Fix: Clamp before cast.*

- **GameFile.cpp:223** — `uLong tsize = (uLong)(((double)insize * 1.01) + 12.5);` multiplies `insize` as a double and casts back to `uLong`. For very large `insize` values, the double may lose precision or overflow, and the cast back to unsigned long may produce an incorrect buffer size.
  *Fix: Check `insize` against `UINT32_MAX / 1.01` before the multiplication, or use saturating arithmetic.*

- **Happy.cpp:688** — `sint32 intHap = (sint32)m_happiness;` casts a double to sint32. If `m_happiness` exceeds `INT_MAX` or is below `INT_MIN`, the result is implementation-defined.
  *Fix: Clamp before cast: `sint32 intHap = (sint32)std::max(std::min(m_happiness, (double)INT_MAX), (double)INT_MIN);`*

- **MaterialPool.cpp:72** — `sint32 pointCost = sint32(double(amt) * g_theConstDB->Get(0)->GetPowerPointsToMaterials());` casts a double to sint32 without bounds checking. If the result exceeds `INT_MAX`, the cast is implementation-defined.
  *Fix: Clamp the double value before casting, or check range.*

- **MaterialPool.cpp:92** — Same unchecked double-to-sint32 cast as line 72 in `CheatSubtractMaterials`.
  *Fix: Clamp before cast.*

- **foreigner.cpp:526** — `m_center_of_mass->x = int(fz_com_city_decay * (double(sum.x) / count) + (1.0 - fz_com_city_decay) * m_center_of_mass->x);` casts a `double` result to `int` (and similarly for y/z). If the computed value exceeds `INT_MAX` or is below `INT_MIN`, the cast overflows.
  *Fix: Clamp the value to the `int` range before casting, or use `std::lround` with saturation.*

- **primitives.cpp:203** — `sint32 dstWidth = static_cast<sint32>(ceil(dstRect.right) - ceil(dstRect.left));` casts a potentially large `double` to `sint32`. If the float value exceeds `INT_MAX`, the cast overflows (undefined behavior).
  *Fix: Clamp the float values to the `sint32` range before casting, or use `llround` and check bounds.*

- **primitives.cpp:204** — `sint32 dstHeight = static_cast<sint32>(ceil(dstRect.bottom) - ceil(dstRect.top));` casts a potentially large `double` to `sint32` without overflow checking.
  *Fix: Clamp the float values to the `sint32` range before casting.*

- **proposalresponseevent.cpp:869** — `static_cast<sint32>(g_player[receiver]->GetGold() * 0.85)` — `GetGold()` returns a large sint32; multiplying by 0.85 as a float can lose precision or overflow the float before the cast back to sint32.
  *Fix: Use integer arithmetic: `(g_player[receiver]->GetGold() * 85) / 100`.*

- **tileutils.cpp:149** — `*width = (uint16)w;` where `w` is an `int`. If the TGA image width exceeds 65535, the value silently truncates. The caller then allocates a buffer based on the truncated size while the image loader may write the full width, causing a heap overflow.
  *Fix: Add overflow check: `if (w > UINT16_MAX || h > UINT16_MAX) { delete[] buffer; *width = *height = 0; return NULL; }` before truncation.*

- **tileutils.cpp:153** — `*height = (uint16)h;` — same truncation risk as the width case above.
  *Fix: Same overflow check before truncation.*


### FORMAT_STRING_BUG

- **AgreementData.cpp:436–464** — Multiple `sprintf(s, "%s ...", s, ...)` calls use the same buffer `s` as both destination and source argument. The C standard leaves this undefined behavior (source and destination may not overlap for `sprintf`).
  *Fix: Use a temporary buffer for the new string, then `strcpy(s, tempBuf)`; or use `snprintf` into a separate buffer first.*

- **ArmyData.cpp:10914** — `sprintf(buf, "%s%d", g_theStringDB->GetNameStr("ARMY_NAME_PREFIX"), m_id & (0x0fffffff))` writes into `buf` without knowing its size. If the prefix string is long, the buffer may overflow.
  *Fix: Use `snprintf(buf, sizeof(buf), ...)` or pass buffer size as a parameter.*

- **ArmyData.cpp:1145** — `DPRINTF(("... %lx ...", *this))`: `%lx` expects an integer but receives an `Army` object. This causes undefined behavior or garbage output.
  *Fix: Use the correct format specifier for the `Army` object's ID field, e.g., `%u` or cast the ID to the appropriate integer type.*

- **ArmyData.cpp:1173** — `DPRINTF(("... %lx ...", *this))`: `%lx` format specifier is used with an `Army` object. This is a type mismatch.
  *Fix: Use the correct member/field of `Army` for the format specifier.*

- **ArmyData.cpp:1239** — `DPRINTF(("... %lx ...", *this))`: `%lx` expects an integer, not an `Army` object.
  *Fix: Pass the appropriate integer ID instead of the whole object.*

- **ArmyData.cpp:1279** — `DPRINTF(("... %lx ...", *this))`: Format specifier `%lx` mismatched with `Army` object argument.
  *Fix: Use the correct ID field and format specifier.*

- **C3Robot.cpp:785** — `sprintf(timing_file_name, "Timing_%s.txt", time_stamp);` writes into a 300-byte buffer. If `time_stamp` is near 300 bytes, the prefixed string overflows the fixed-size buffer.
  *Fix: Use `snprintf(timing_file_name, sizeof(timing_file_name), "Timing_%s.txt", time_stamp);`*

- **C3Robot.cpp:835** — `sprintf(memory_file_name, "memorylog_%s.txt", time_stamp);` has the same overflow risk as the timing filename into a 300-byte buffer.
  *Fix: Use `snprintf(memory_file_name, sizeof(memory_file_name), "memorylog_%s.txt", time_stamp);`*

- **EditQueue.cpp:1346** — `sprintf(buf, g_theStringDB->GetNameStr("str_code_QuerySwitchProduction"), p)` uses a format string loaded from external translation data. If the translated string contains format specifiers like `%s`, `sprintf` will read `p` (a `sint32`) as a pointer, causing undefined behavior or a crash.
  *Fix: Use a constant format string: `sprintf(buf, "%s%d", g_theStringDB->GetNameStr("str_code_QuerySwitchProduction"), p)` or use `snprintf` with a verified constant format.*

- **EditQueue.cpp:1533** — Same pattern as above: `sprintf(buf, g_theStringDB->GetNameStr("str_code_QuerySwitchProduction"), p)` passes a user-controlled format string.
  *Fix: Use a constant format string with `sprintf` or `snprintf`.*

- **EditQueue.cpp:1602** — Same pattern: `sprintf(buf, g_theStringDB->GetNameStr("str_code_QuerySwitchProduction"), p)`.
  *Fix: Use a constant format string.*

- **EditQueue.cpp:1679** — Same pattern: `sprintf(buf, g_theStringDB->GetNameStr("str_code_QuerySwitchProduction"), p)`.
  *Fix: Use a constant format string.*

- **EditQueue.cpp:2262** — `sprintf(buf, fmt, loadName, s_editQueue->m_cityData->GetName())` where `fmt` is loaded from `g_theStringDB->GetNameStr("str_ldl_EditQueueReallyLoad")`. A malformed translation could introduce wrong format specifiers.
  *Fix: Use a constant format string (e.g., `"%s%s"`) and pass the translated text as an argument.*

- **EditQueue.cpp:2269** — `sprintf(buf, fmt, loadName)` where `fmt` comes from `g_theStringDB->GetNameStr("str_ldl_EditQueueReallyLoadMulti")`.
  *Fix: Use a constant format string.*

- **EditQueue.cpp:2507** — `sprintf(buf, fmt, queueName)` where `fmt` comes from `g_theStringDB->GetNameStr("str_ldl_EditQueueReallyDelete")`.
  *Fix: Use a constant format string.*

- **EditQueue.cpp:2665** — `sprintf(buf, fmt, m_text)` where `fmt` comes from `g_theStringDB->GetNameStr("str_ldl_EditQueueReallyOverwrite")`.
  *Fix: Use a constant format string.*

- **GWCivRecord.cpp:98** — `sprintf(fileName, "%s.txt", baseName)` writes into a 1024-byte stack buffer with no length limit on `baseName`. A very long base name causes a stack buffer overflow.
  *Fix: Use `snprintf(fileName, sizeof(fileName), "%s.txt", baseName)`.*

- **GameEventManager.cpp:638** — `sprintf(gameWatchFilename, "%s.gw", filepath)` concatenates into a `_MAX_PATH` buffer without length checks. If `filepath` is already near `_MAX_PATH` length, the suffix overflows the buffer.
  *Fix: Use `snprintf(gameWatchFilename, sizeof(gameWatchFilename), "%s.gw", filepath)`.*

- **GameFile.cpp:1817** — `sprintf(filepath, "%s%s%s", path, FILE_SEP, info->fileName)` concatenates three strings into a `_MAX_PATH` buffer without length validation. If the combined length exceeds `_MAX_PATH`, the buffer overflows.
  *Fix: Use `snprintf(filepath, sizeof(filepath), "%s%s%s", path, FILE_SEP, info->fileName)`.*

- **GameFile.cpp:1902** — `sprintf(path, "%s%s*.*", dirPath, FILE_SEP)` constructs a search pattern in a `_MAX_PATH` buffer without bounds checking.
  *Fix: Use `snprintf(path, sizeof(path), "%s%s*.*", dirPath, FILE_SEP)`.*

- **GameFile.cpp:1939** — `sprintf(gameInfo->path, "%s%s%s", dirPath, FILE_SEP, name)` concatenates paths without bounds checking.
  *Fix: Use `snprintf(gameInfo->path, sizeof(gameInfo->path), ...)`.*

- **GameFile.cpp:1978** — `sprintf(saveInfo->pathName, "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName)` concatenates paths without bounds checking.
  *Fix: Use `snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), ...)`.*

- **Player.cpp:1192** — `sprintf(buf, "%s%d", g_theStringDB->GetNameStr("ARMY_NAME_PREFIX"), m_totalArmiesCreated);` — if `GetNameStr` returns `NULL`, `%s` dereferences a null pointer. Also `buf[40]` may overflow if the prefix string is long.
  *Fix: Check the return value of `GetNameStr` before calling `sprintf`, and use `snprintf(buf, sizeof(buf), ...)`.*

- **Player.cpp:5332** — `Player::DumpAllies()` uses `sprintf(s, "%s P%d, ", s, i)` in a loop, appending to the same buffer `s` (size `_MAX_PATH`) without checking for overflow. With many alliances, this could overflow.
  *Fix: Track remaining buffer space with `snprintf` or `strncat`, or use `std::string`.*

- **SlicFrame.cpp:1498** — `ReportSFError` builds a message in a 1024-byte stack buffer using repeated `strcat` and `sprintf(buf + strlen(buf), ...)`. While currently within limits, adding new error cases could overflow the buffer. More importantly, `sprintf(buf + strlen(buf), ...)` with `%s` is safe here, but the pattern is fragile.
  *Fix: Use `snprintf(buf + len, sizeof(buf) - len, ...)` instead of `sprintf` and `strcat` to ensure the buffer is never exceeded.*

- **UnitSpriteGroup.cpp:344** — `vsprintf(name, format, v_args);` writes into `name` (a `MBCHAR *` parameter) with no size limit. A malformed format string or large arguments can overflow the caller's buffer.
  *Fix: Change to `vsnprintf(name, nameBufferSize, format, v_args);` and require the caller to pass the buffer size.*

- **UnitSpriteGroup.cpp:344** — `vsprintf(name, format, v_args)` writes into a fixed-size `name` buffer using a caller-supplied format string. A malicious or malformed format string can cause buffer overflow or format-string attacks.
  *Fix: Replace with `vsnprintf(name, k_MAX_NAME_LENGTH, format, v_args)` and validate the format string.*

- **UnitSpriteGroup.cpp:442-446** — Multiple `GetImageFileName` calls pass user-controlled `id`, `j`, `k` values into a format string that writes into fixed-size `MBCHAR` arrays. A large `id` or frame count can overflow the 512-byte `fname` buffer.
  *Fix: Use `snprintf` with explicit buffer size limits inside `GetImageFileName`.*

- **c3cmdline.cpp:~6348** — In `CommandLine::DisplayOutput`, `strcat(buf, rbuf)` is called inside nested loops over players, cities, and resources. With many resources, `buf[1024]` overflows.
  *Fix: Track remaining buffer size with `strncat` or `snprintf` into a growing offset.*

- **c3cmdline.cpp:~6366** — In the `m_displayOffers` branch of `DisplayOutput`, `strcat(buf, buf2)` is called repeatedly in a loop. With many trade offers, `buf[1024]` overflows.
  *Fix: Use `strncat` with remaining-size tracking or build strings with `snprintf` and an offset.*

- **c3debug.cpp:311** — `sprintf(stamp, "...%s on %s...", userName, computerName, ...)` into `stamp[1024]` can overflow. Both `userName` and `computerName` are each up to 256 characters, plus the format string, making overflow possible.
  *Fix: Use `snprintf(stamp, sizeof(stamp), ...)`.*

- **c3mem.cpp:19** — `sprintf(s, "... %ld\n", size);` uses `%ld` for a `size_t` argument. On 64-bit Windows where `long` is 32-bit and `size_t` is 64-bit, this is a format/width mismatch.
  *Fix: Use `%zu` (C99) or cast the argument to `(unsigned long)size`.*

- **c3mem.cpp:19** — `sprintf(s, "...%ld\n", size);` uses `%ld` to print a `size_t` value. This is a format specifier mismatch — `size_t` should use `%zu`. On platforms where `size_t` and `long` differ in size, this causes undefined behavior.
  *Fix: Change `%ld` to `%zu`.*

- **c3mem.cpp:23** — `DPRINTF(...("...%ld\n", size));` has the same format mismatch: `%ld` is used for a `size_t` argument.
  *Fix: Change `%ld` to `%zu`.*

- **civ3_main.cpp:1570** — `strcpy(launchcommand, exepath)` followed by `strcat(launchcommand, " -l\"%1\"")` can overflow `launchcommand[_MAX_PATH]` if `exepath` is already close to `_MAX_PATH` characters long.
  *Fix: Use `snprintf(launchcommand, sizeof(launchcommand), "%s -l\"%%1\"", exepath)` instead.*

- **civapp.cpp:3489** — `strcpy(leaderName, g_theProfileDB->GetLeaderName())` into a buffer of size `SAVE_LEADER_NAME_SIZE + 1` can overflow if the leader name is longer than `SAVE_LEADER_NAME_SIZE`.
  *Fix: Use `strncpy(leaderName, g_theProfileDB->GetLeaderName(), SAVE_LEADER_NAME_SIZE)` and null-terminate explicitly.*

- **ctp2_code/gfx/spritesys/UnitSpriteGroup.cpp:344** — `vsprintf(name, format, v_args)` writes into the `name` buffer without any size limit. The caller passes `name` as a pointer but no length is provided, and if `format` contains user-controlled input or is large, this causes a stack buffer overflow.
  *Fix: Use `vsnprintf(name, nameBufferSize, format, v_args)` and require the caller to pass the buffer size.*

- **diplomat.cpp:208** — `#define RELDBG(x) { FILE *f = fopen("reldbg.txt", "a"); fprintf x; fclose(f); }` — the macro argument `x` is pasted directly into `fprintf`. If any caller passes user-controlled data as the format string, it becomes a format-string vulnerability.
  *Fix: Change the macro to always use a literal format string: `#define RELDBG(fmt, ...) { FILE *f = fopen("reldbg.txt", "a"); fprintf(f, fmt, __VA_ARGS__); fclose(f); }`*

- **gameinit.cpp:1480** — `fscanf(fin, "%d", &seed)` is called where `seed` is declared as `uint32`. The `%d` format specifier expects a signed `int *`, not an unsigned 32-bit pointer. On platforms where `int` and `uint32` differ in size or representation, this is undefined behavior and can corrupt adjacent memory.
  *Fix: Use `fscanf(fin, "%u", &seed)` (or the platform-appropriate format for `uint32`).*

- **gameinit.cpp:1486** — `fprintf(fin, "%d\n", seed)` uses `%d` for a `uint32` value. This is a format specifier mismatch that can produce incorrect output or undefined behavior on some platforms.
  *Fix: Use `fprintf(fin, "%u\n", seed)` to match the unsigned type.*

- **netfunc.cpp:2687** — `strncpy(player.player.name, playername, dp_PNAMELEN)` copies up to `dp_PNAMELEN` bytes without guaranteeing null termination. Although the struct was zero-initialized in the constructor, an exact-length name overwrites the trailing zero, leaving the field unterminated.
  *Fix: Add `player.player.name[dp_PNAMELEN - 1] = '\0';` after the `strncpy`.*

- **netfunc.cpp:2688** — `strncpy(session.session.sessionName, sessionname, dp_SNAMELEN)` has the same missing-null-terminator issue as above when the source string is exactly `dp_SNAMELEN` characters.
  *Fix: Add `session.session.sessionName[dp_SNAMELEN - 1] = '\0';` after the `strncpy`.*

- **netfunc.cpp:3284** — `strncpy(sessionname, s->sessionName, dp_SNAMELEN)` copies into `char sessionname[dp_SNAMELEN]`. If the session name is exactly `dp_SNAMELEN` bytes, the destination array is not null-terminated.
  *Fix: Add `sessionname[dp_SNAMELEN - 1] = '\0';` after the `strncpy`.*

- **netfunc.cpp:3317** — `strncpy(playername, n, dp_PNAMELEN)` copies into `char playername[dp_PNAMELEN]`. If `n` is exactly `dp_PNAMELEN` non-null bytes, `playername` is left without a null terminator.
  *Fix: Add `playername[dp_PNAMELEN - 1] = '\0';` after the `strncpy`.*

- **profileDB.cpp:620** — `strcpy(var->m_stringValue, value);` copies `value` into `var->m_stringValue` without length checking. A maliciously crafted profile file with an oversized value can overflow the fixed-size string buffer.
  *Fix: Use `strncpy(var->m_stringValue, value, maxStringBufferSize - 1); var->m_stringValue[maxStringBufferSize - 1] = '\0';`.*

- **tracklen.cpp:415** — `strcpy(szTemp, szFile);` into `szTemp[MAX_PATH]` can overflow if `szFile` is longer than `MAX_PATH`.
  *Fix: Use `strncpy(szTemp, szFile, sizeof(szTemp) - 1); szTemp[sizeof(szTemp) - 1] = '\0';`.*


### INTEGER_OVERFLOW

- **Barbarians.cpp:557** — `sint16(g_rand->Next(g_theWorld->GetXWidth()))` casts a potentially large unsigned value to a signed 16-bit integer. If the map width exceeds 32767, the result overflows.
  *Fix: Use a wider type such as `sint32` for the coordinate, or clamp the value before casting.*

- **Barbarians.cpp:558** — `sint16(g_rand->Next(g_theWorld->GetYHeight()))` can overflow if the map height exceeds 32767.
  *Fix: Use `sint32` instead of `sint16` for the coordinate.*

- **Cont.cpp:37** — `m_num_land_cont = ai->m_world->GetMaxLandContinent() - m_min_land_cont;` can underflow if `GetMaxLandContinent() < GetMinLandContinent()`, producing a negative value used for array allocation.
  *Fix: Use signed underflow check or ensure ordering: `m_num_land_cont = std::max(0, max - min);`*

- **Cont.cpp:50** — `m_num_water_cont = ai->m_world->GetMaxWaterContinent() - m_min_water_cont;` can underflow similarly.
  *Fix: Same as above.*

- **FeatTracker.cpp:456** — `result += sub;` where both are `sint32`. If many feats accumulate, `result` can overflow.
  *Fix: Use saturation arithmetic or a wider integer type.*

- **FeatTracker.cpp:487** — `if(rec->GetDuration() + feat->GetRound() <= NewTurnCount::GetCurrentRound())` adds two sint32 values that could overflow.
  *Fix: Check for overflow before adding, or use a wider type.*

- **GaiaController.cpp:990** — `sint32 max_distance = (g_theWorld->GetHeight() * g_theWorld->GetWidth()) * (g_theWorld->GetHeight() * g_theWorld->GetWidth());` can overflow a 32-bit signed integer for large maps.
  *Fix: Use `sint64` for the intermediate calculation, or cap the dimensions before multiplication.*

- **Gold.cpp:196** — `if ((m_level - amount) < 0)` can cause signed integer underflow when `amount` is very large, which is undefined behavior. The subsequent `m_level -= amount;` may also underflow.
  *Fix: Check `if (amount > m_level)` before subtraction, or use unsigned arithmetic.*

- **Happy.cpp:248** — `res = -s * (num_cities - t);` multiplies two doubles, but `num_cities` and `t` are computed from game state. If the product is extremely large, converting back to other types could have issues. More importantly, this is a double operation so no integer overflow. Scratch this one.

- **MapFile.cpp:258** — `m_chunk.m_size = ((g_theWorld->GetXWidth() * g_theWorld->GetYHeight()) * sizeof(uint32)) + sizeof(sint16) * 2;` can overflow if map dimensions are very large, causing an incorrect size to be written to the file header.
  *Fix: Use `size_t` for the calculation and check for overflow before assigning to `m_chunk.m_size`.*

- **MapFile.cpp:296** — `fwrite(env, 1, x * y * sizeof(uint32), outfile) != (uint32)((x * y) * sizeof(uint32))` multiplies `x * y` as `sint16` values before promoting to `uint32`. If `x` and `y` are large (e.g., both 20000), the intermediate `sint16` product overflows.
  *Fix: Cast before multiplication: `static_cast<uint32>(x) * static_cast<uint32>(y) * sizeof(uint32)`.*

- **MapFile.cpp:477** — `m_chunk.m_size = (g_theWorld->GetXWidth() * g_theWorld->GetYHeight() * sizeof(uint16)) + sizeof(sint16) * 2 + sizeof(sint8);` multiplies map dimensions. On large maps this can overflow `sint32`.
  *Fix: Compute with `size_t` and verify no overflow occurred.*

- **MapPoint.cpp:1045** — `RadiusIterator` computes `m_squaredRadius (static_cast<double>(size * size))`. The multiplication `size * size` is evaluated on `sint32` before the cast to `double`. If `size` is near `INT_MAX` or `INT_MIN`, the multiplication overflows.
  *Fix: Cast before multiplying: `static_cast<double>(size) * static_cast<double>(size)`.*

- **MapPoint.cpp:1079** — `CircleIterator` computes `static_cast<double>(innerSize * innerSize)` and similarly for `outerSize`. The 32-bit signed multiplication can overflow before the `double` cast.
  *Fix: Cast to `double` before multiplication.*

- **MapPoint.cpp:651** — `sint16 const w = map_size.x * 2;` multiplies a map width by 2 and stores it in `sint16`. If `map_size.x` exceeds 16383, the product overflows a 16-bit signed integer.
  *Fix: Use `sint32` for the intermediate result, or add an overflow check/assert.*

- **SlicFrame.cpp:524** — `SOP_ADD` computes `sval3.m_int = Eval(type2, sval2) + Eval(type1, sval1);` without overflow checking. Two large positive or negative SLIC integers can wrap around, leading to incorrect script behavior.
  *Fix: Use safe-add logic or widen to `int64_t` for the intermediate sum before clamping to `sint32`.*

- **SlicFrame.cpp:534** — `SOP_SUB` computes `sval3.m_int = Eval(type2, sval2) - Eval(type1, sval1);` without checking for underflow/overflow on `sint32` boundaries.
  *Fix: Use `int64_t` intermediate or check for overflow before assigning the result.*

- **SlicFrame.cpp:543** — `SOP_MULT` computes `sval3.m_int = Eval(type2, sval2) * Eval(type1, sval1);` with no overflow guard. Large operands can silently wrap, corrupting script calculations.
  *Fix: Perform multiplication in `int64_t` and clamp or report an error if the result does not fit in `sint32`.*

- **SpriteLow565.cpp:1646** — `uint32 sum = (uint32)(pixel1 + pixel2 + pixel3 + pixel4) & 0xE79CE79C;` adds four `Pixel16` (uint16) values. The addition is performed in 16-bit arithmetic, which can wrap around before the cast to `uint32`. This corrupts the averaging result.
  *Fix: Cast each operand to `uint32` before adding: `(uint32)pixel1 + (uint32)pixel2 + ...`*

- **SpriteLow565.cpp:1685** — `uint32 sum = (uint32)(pixel1 + pixel2) & 0xFFFFE79C;` adds two `Pixel16` values in 16-bit arithmetic, causing wrap-around before the uint32 cast.
  *Fix: Cast before adding: `(uint32)pixel1 + (uint32)pixel2`.*

- **SpriteStateDB.cpp:176-182** — `SetSize(n + 1)` where `n` is a parsed `sint32`. If `n` is `INT_MAX`, `n + 1` overflows to a negative value, causing `new SpriteNameNode[negative]` to throw or behave unexpectedly.
  *Fix: Check `if (n >= INT_MAX - 1) return FALSE;` before `SetSize(n + 1)`.*

- **SpriteStateDB.cpp:182** — `SetSize(n + 1);` can overflow when `n == INT_MAX`, causing a tiny or negative allocation and subsequent out-of-bounds writes.
  *Fix: Check `n >= 0 && n < INT_MAX - 1` before calling `SetSize`.*

- **TradePool.cpp:168** — `return ((n * (n+1)) / 2) * g_theResourceDB->Get(resource)->GetGold();` — the intermediate `n * (n+1)` can overflow a 32-bit signed integer when `n` is large (e.g., > 46,340).
  *Fix: Cast to a wider type before multiplication: `return (sint64(n) * (n + 1) / 2) * gold;` and clamp the result.*

- **TradePool.cpp:181** — `(((nth_good * (nth_good + 1) / 2) - ((nth_good-1) * (nth_good) / 2 )) * ...)` — the sub-expressions `nth_good * (nth_good + 1)` can overflow 32-bit `sint32` for large values.
  *Fix: Perform the arithmetic in `sint64` and cast back after bounds checking.*

- **agent.cpp:692** — `for (uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)` — if `strlen(myText) < 5`, the subtraction underflows (wraps around for uint8), causing an infinite loop and potential buffer overflow.
  *Fix: Change loop variable to `size_t` and rewrite as `myComp + 5 < strlen(myText)`.*

- **agent.cpp:933** — `for(uint8 myComp = 0; myComp < strlen(myText) - 5; myComp++)` in `ClearOrders` — same underflow issue as `Group_With`.
  *Fix: Use `size_t` and rewrite the loop condition to avoid underflow.*

- **agreementmatrix.cpp:147** — `return (NewTurnCount::GetCurrentRound() - agreement.start);` subtracts `agreement.start` from the current round. If `agreement.start` is greater than the current round (e.g., data corruption), the subtraction underflows.
  *Fix: Use a signed check first: `if (agreement.start > currentRound) return -1;` or cast to a wider type.*

- **agreementmatrix.cpp:383** — `return (NewTurnCount::GetCurrentRound() - last_war.end);` can underflow if `last_war.end` is greater than the current round.
  *Fix: Validate values before subtraction or return a sentinel on underflow.*

- **agreementmatrix.cpp:408** — `return (NewTurnCount::GetCurrentRound() - last_war.start);` can underflow if `last_war.start` is greater than the current round.
  *Fix: Validate values before subtraction or return a sentinel on underflow.*

- **agreementmatrix.cpp:60** — `m_maxPlayers = std::min<sint16>(static_cast<sint16>(newMaxPlayers), k_MAX_PLAYERS);` casts `newMaxPlayers` (a `sint32`) to `sint16`. If `newMaxPlayers > 32767`, the cast overflows, producing a negative or truncated value.
  *Fix: Perform the min operation before casting: `m_maxPlayers = static_cast<sint16>(std::min(newMaxPlayers, static_cast<sint32>(k_MAX_PLAYERS)));` and clamp to valid range.*

- **agreementmatrix.cpp:72** — `m_agreements.resize(m_maxPlayers * m_maxPlayers * PROPOSAL_MAX, s_badAgreement);` can overflow if `m_maxPlayers` is large, causing a much smaller allocation than expected and subsequent out-of-bounds accesses.
  *Fix: Use `size_t` and check for overflow before resizing.*

- **aui_image.cpp:193** — `sint32 temp = width * bih.biBitCount / 8;` computes `width * bih.biBitCount` in unsigned arithmetic (`uint32 * uint16`). A malformed BMP with a very large `width` can cause this product to overflow, producing a truncated or wrapped value. The result is then assigned to a signed `sint32`, which can become negative and produce an invalid `bmpPitch`.
  *Fix: Use a wider type (e.g., `uint64`) for the intermediate product, or validate `width` against a safe maximum before multiplying.*

- **aui_ranger.cpp:566** — `sint32 w = sint32(double(m_width * page) / ...);` multiplies `m_width * page` as `sint32` before promoting to `double`. If `m_width` or `page` is large, the `sint32` product can overflow.
  *Fix: Cast one operand to `double` before the multiplication: `double(m_width) * double(page)`.*

- **aui_ranger.cpp:572** — Same issue as above for height: `m_height * page` is computed as `sint32` before the `double` cast and can overflow.
  *Fix: Use `double(m_height) * double(page)`.*

- **aui_uniqueid.cpp:10** — `static uint32 id` starts at `0x00000000` and is decremented with `id--`. When it wraps from 0 to 0xFFFFFFFF, this is unsigned integer wrap-around. The subsequent assert `(sint32)id < 0` relies on this overflow behavior, which is well-defined for unsigned but still a logical overflow pattern.
  *Fix: Use a signed counter or explicitly handle wrap-around without relying on overflow semantics.*

- **c3blitter.cpp:168** — `const sint32 scanWidth = 2 * ( srcRect->right - srcRect->left );` can overflow if the source rectangle width is very large (greater than `INT_MAX / 2`).
  *Fix: Use `sint64` for the intermediate calculation or add an overflow check.*

- **c3blitter.cpp:339** — `const sint32 scanWidth = 2 * ( srcRect->right - srcRect->left );` in `Blt16To16FastMMX` can overflow for large rectangles.
  *Fix: Use `sint64` for the intermediate calculation or add an overflow check.*

- **c3blitter.cpp:518** — `const sint32 scanWidth = 2 * ( srcRect->right - srcRect->left );` in `Blt16To16FastFPU` can overflow for large rectangles.
  *Fix: Use `sint64` for the intermediate calculation or add an overflow check.*

- **civarchive.cpp:129** — In `DoubleExpand`, `ulSize *= 2` is a `uint32` multiplication. Starting from `k_ALLOC_SIZE` (8096) and looping up to 31 times, the value can exceed `UINT_MAX` and wrap around. A wrapped value could then pass the allocation size check but represent a much smaller buffer than needed.
  *Fix: Check for overflow before doubling: `if (ulSize > UINT32_MAX / 2) { exit(0); } ulSize *= 2;`.*

- **ctp2_code/gfx/gfx_utils/targautils.cpp:176** — `fread(dataPtr, width * bpp, 1, fp)` computes `width * bpp` as `int * int`. For a very large TGA width, this can overflow.
  *Fix: Use `size_t rowBytes = static_cast<size_t>(width) * bpp; fread(dataPtr, rowBytes, 1, fp)`.*

- **ctp2_code/gfx/gfx_utils/targautils.cpp:297** — `fread(dataPtr, width * 2, 1, fp)` multiplies `width * 2` in `int` arithmetic. A large width can overflow.
  *Fix: Cast to `size_t` before multiplying.*

- **ctp2_code/gfx/spritesys/spriteutils.cpp:397** — `new Pixel16[(1+height+width*height)*8]` computes `width*height` in `sint32` arithmetic before the cast to size_t for `new[]`. Large sprites can overflow.
  *Fix: `size_t allocCount = (static_cast<size_t>(1) + height + static_cast<size_t>(width) * height) * 8;`.*

- **ctp2_code/gfx/spritesys/spriteutils.cpp:450** — Same overflow pattern as above in the second `spriteutils_RGB32ToEncoded` overload.
  *Fix: Use safe `size_t` casting for the allocation size expression.*

- **ctp2_code/gfx/spritesys/spriteutils.cpp:592** — `malloc(destWidth * destHeight * sizeof(Pixel32))` multiplies `sint32` values in 32-bit arithmetic before the `malloc` size_t conversion.
  *Fix: `size_t allocSize = static_cast<size_t>(destWidth) * destHeight * sizeof(Pixel32);`.*

- **ctp2_code/gs/database/GoalRecord.cpp:933** — `m_SquadClass |= (1 << bitindex);` performs a left shift on a signed 32-bit literal. If `bitindex` is ever 31 (or negative), this is undefined behavior due to signed integer overflow. Similar patterns exist for TargetType, TargetOwner, ThreatenType, and ForceMatch.
  *Fix: Use unsigned literals: `m_SquadClass |= (1u << bitindex);` and validate `bitindex < 31`.*

- **ctpai.cpp:2000** — `sint32 max_squared_dist = (g_theWorld->GetWidth() * g_theWorld->GetHeight()); max_squared_dist *= max_squared_dist;` can overflow a signed 32-bit integer when map dimensions are large (e.g., 256x256 gives 4,294,967,296 which exceeds INT_MAX).
  *Fix: Use `sint64` for the intermediate calculation: `sint64 max_squared_dist = (sint64)g_theWorld->GetWidth() * g_theWorld->GetHeight(); max_squared_dist *= max_squared_dist;`.*

- **diplomat.cpp:4071** — `for (foreignerId = 1; foreignerId < s_theDiplomats.size(); foreignerId)` — missing increment expression (`foreignerId++`). The loop body does not modify `foreignerId`, so this is an infinite loop. While not strictly an overflow, it causes the index to be used repeatedly and could eventually wrap if `foreignerId` were modified.
  *Fix: Add the increment: `++foreignerId`.*

- **iparser.cpp:848** — `new_string = (char *)MALLOC((3*source_length)+1);` can overflow if `source_length` is very large (e.g., > 1.4 billion on 32-bit). The wrap-around produces a small allocation, but the caller then writes up to `3*source_length` bytes into it.
  *Fix: Check for overflow before multiplication, or cap `source_length`.*

- **mapanalysis.cpp:150** — `sint32 index = (victimId * m_piracyLossGrid.size()) + playerId;` — `victimId * size` can overflow if both values are large.
  *Fix: Use a 64-bit type for the multiplication or bounds-check the inputs.*

- **mapanalysis.cpp:150** — `AddPiracyIncome` computes `sint32 index = (victimId * m_piracyLossGrid.size()) + playerId;`. `victimId` is a signed `PLAYER_INDEX` and `m_piracyLossGrid.size()` is `size_t`. If `victimId` is negative, the implicit conversion to `size_t` produces a huge value, causing overflow/wraparound. The resulting `index` may then be negative, bypassing the subsequent `Assert(index >= 0)` in release builds and leading to an out-of-bounds write.
  *Fix: Validate `victimId >= 0` before the multiplication, or cast after validation: `sint32 index = (static_cast<sint32>(victimId) * static_cast<sint32>(m_piracyLossGrid.size())) + playerId;` with an overflow check.*

- **mapanalysis.cpp:833-834** — `sint32 min_squared_distance = (g_theWorld->GetWidth() * g_theWorld->GetHeight()); min_squared_distance *= min_squared_distance;` — the product can overflow sint32 on large maps.
  *Fix: Use sint64 for the intermediate calculation.*

- **mapanalysis.cpp:980** — `sint32 index = (victimId * m_piracyLossGrid.size()) + playerId;` — same overflow pattern as line 150.
  *Fix: Use 64-bit arithmetic or validate inputs.*

- **mapanalysis.cpp:980** — `GetPiracyIncomeByPlayer` computes the same index formula: `sint32 index = (victimId * m_piracyLossGrid.size()) + playerId;`. A negative `victimId` causes the same overflow and potential out-of-bounds read.
  *Fix: Same as above.*

- **motivationevent.cpp:138-140** — `sint32 needed_reserves = g_player[playerId]->m_gold->GetIncome() * 2;` — `GetIncome()` * 2 can overflow a signed 32-bit integer.
  *Fix: Cast to `sint64` before multiplying.*

- **nproposalevent.cpp:1722-1723** — `sint32 at_risk_value = static_cast<sint32>((static_cast<double>(at_risk_value_percent) / 100.0) * MapAnalysis::GetMapAnalysis().TotalValue(receiver));` — intermediate double may exceed sint32 range, causing implementation-defined behavior on cast.
  *Fix: Clamp the double value to `INT_MIN`/`INT_MAX` before casting, or use saturating integer arithmetic.*

- **nproposalevent.cpp:1836-1839** — Same `static_cast<sint32>(... * TotalValue(receiver))` pattern with potential overflow.
  *Fix: Clamp before casting.*

- **nproposalevent.cpp:495** — `sint32 max_can_pay_gold = std::min(g_player[receiver]->m_gold->GetIncome() * 2, g_player[sender]->GetGold());` — `GetIncome() * 2` can overflow a sint32 for large income values.
  *Fix: Use `std::min(g_player[receiver]->m_gold->GetIncome() * 2LL, (sint64)g_player[sender]->GetGold())` or check for overflow.*

- **proposalresponseevent.cpp:869** — `std::min(3 * g_player[receiver]->m_gold->GetIncome(), ...)` — `3 * income` can overflow sint32.
  *Fix: Cast to sint64 before multiplication: `3LL * g_player[receiver]->m_gold->GetIncome()`.*

- **scheduler.cpp:1647** — `max_eval = (sint16) floor(tmp_eval);` truncates a `double` to a signed 16-bit integer without range checking. If `tmp_eval` exceeds 32767, the result is implementation-defined (usually wraps).
  *Fix: Clamp the value before casting: `max_eval = (sint16)std::min<double>(floor(tmp_eval), 32767.0);`.*

- **scheduler.cpp:2004** — `current_garrison += agent_iter->second->Get_Army()->Num();` accumulates into `sint8 current_garrison`. If the number of garrison units exceeds 127, this signed 8-bit variable overflows.
  *Fix: Use `sint32` for `current_garrison` to match the range of `Num()`.*

- **settlemap.cpp:178** — `radius += radius + 1;` — if `radius` is near `INT_MAX / 2`, this overflows.
  *Fix: Use a saturated addition or check for overflow.*

- **spriteutils.cpp:596** — `srcBuf[(i * 2) * srcWidth + (j * 2)]` computes an index with unchecked signed multiplication. Large `srcWidth` or `i` values can overflow `sint32`, producing a negative or wrapped index.
  *Fix: Use `size_t` for the index calculation: `static_cast<size_t>(i * 2) * srcWidth + (j * 2)`.*

- **targautils.cpp:419** — `fwrite(data, width * height * BYTES_PER_PIXEL, 1, fp)` multiplies three values without overflow checks. A crafted `width`/`height` can wrap the product.
  *Fix: Compute the product in `size_t` with overflow checks before writing.*

- **tracklen.cpp:184** — `totalLen_ms += cdrom->track[i - 1].length * 1000;` performs 32-bit unsigned multiplication before promoting to `uint64`. If `length` is large, the intermediate 32-bit result wraps around, corrupting the total.
  *Fix: Cast before multiplying: `totalLen_ms += (uint64)cdrom->track[i - 1].length * 1000;`.*


### INTEGER_OVERFLOW / DANGEROUS_SHIFT

- **ColorSet.cpp:84** — `((c & 0x001F) << 19)` promotes `c & 0x001F` to signed `int`. Left-shifting a signed int so the result exceeds `INT_MAX` (`0x1F << 19 == 0xF8000000`) is undefined behavior in C++.
  *Fix: Cast to unsigned before shifting: `static_cast<uint32>(c & 0x001F) << 19`.*

- **ColorSet.cpp:89** — Same signed left-shift issue as line 84 in the RGB555 branch.
  *Fix: Cast the masked value to `uint32` before shifting.*


### MISSING_BOUNDS_CHECK

> **Resolution status — first P3 HIGH MISSING_BOUNDS_CHECK mini-batch verified in code 2026-07-02.**
> Findings are static-analysis leads, not verdicts; line numbers had drifted since May 2026.
>
> **Fixed this pass (commit pending):** `Army.cpp:71` now uses `safe_player(GetOwner())` before removing the army from the owner, and `player_view::ArmyRemoved` rejects invalid player indices at the shared UI-observer boundary, so invalid owner indices cannot index player/UI arrays out of bounds.
> `Civilisation.cpp:152` now validates `pi` before assigning `player_arr_Get()[pi]`, so malformed caller input cannot write past the global player array.
> `civilisationpool.cpp:100` now fatals before `rand_ptr()->Next(numCivs)` or DB lookups when the civilisation database is empty.
> `GameEventManager.cpp:261` now validates `m_processingEvent` before indexing `event_description(...)` while processing the event head.
> `Regard.cpp:28/36` now uses `< k_MAX_PLAYERS` plus runtime guards before reading/writing `m_regard[player]`.
> `Player.cpp:6494` now rejects `type == g_theGovernmentDB->NumRecords()` before `g_theGovernmentDB->Get(type)`.
> `aui_control.cpp:742` now requires `state < m_stringTable->GetNumStrings()` before `GetString(state)`.
> `SpriteGroupList.cpp:91` now validates `index < k_MAX_SPRITES` before `m_spriteList[index]`.
> `effectspritegroup.cpp:46/72` now guards `EFFECTACTION` bounds before indexing `m_sprites[action]`.
> `goodspritegroup.cpp:53/74/84` now guards `GOODACTION` bounds before indexing `m_sprites[action]`.
> `c3cmdline.cpp:1177/5207/5287` now validates parsed player and city indices before `Diplomat::GetDiplomat`, `player_Get(...)`, and city-list access in the matching diplomacy/trade commands.
> `SlicSegment.cpp:605` was already converted from `sprintf` to `snprintf`, but used `sizeof(str)` on a pointer; it now respects the caller-provided `maxsize`.
> `profileDB.cpp:620` now uses `strlcpy(..., k_MAX_NAME_LEN)` after the existing max-length check instead of unbounded `strcpy`.
>
> **Already fixed before this pass (verified present):** `Agreement.cpp:42/45` already uses `safe_player(GetRecipient())` / `safe_player(GetOwner())`; `ArmyData.cpp:1027/1060/1066` already returns false for empty armies before reading `m_array[0]`; `ArmyData.cpp:1413` is only reached from `CheckActiveDefenders`, which returns false when `m_nElements <= 0` before calling `GetActiveDefenders`; `Order.cpp:254` already guards `order < 0 || order >= UNIT_ORDER_MAX` before indexing `s_orderToEventMap`.
> `agreementmatrix.cpp:124` already returns `s_badAgreement` when the computed index is outside `m_agreements`; `c3cmdline.cpp:5539` was already guarded; `c3debug.cpp:233` already uses bounded `vsnprintf` with the remaining global-buffer capacity; `SlicBuiltin.cpp:1000/1059` already uses `strlcpy(..., maxLen)` for the reported leader/pronoun text copies; `SlicEngine.cpp:2599` already uses `strlcpy(m_researchText, text, sizeof(m_researchText));`; `appstrings.cpp:41` already checks `len > 0` before writing `inStr[len - 1]`.
>
> **Not bugs / stale leads (skipped):** `Barbarians.cpp` `g_rand->Next(x - 1)` leads are covered by `RandomGenerator::Next(sint32)`, which returns 0 for `r <= 0`; `Cont.cpp`, `Installation.cpp`, and the GameWatch files (`GWArchive.cpp`, `GWCivRecord.cpp`, `GWFile.cpp`) are absent from the current tree.

- **Agreement.cpp:42** — `g_player[GetRecipient()]` indexes the global player array using `GetRecipient()` without first verifying the index is within `[0, k_MAX_PLAYERS)`. Only a NULL check is performed on the resulting pointer.
  *Fix: Add index validation: `if (GetRecipient() < 0 || GetRecipient() >= k_MAX_PLAYERS) return;`*

- **Agreement.cpp:45** — Same as above for `g_player[GetOwner()]`.
  *Fix: Add index validation before array access.*

- **Army.cpp:71** — `g_player[GetOwner()]->RemoveArmy(...)` indexes `g_player` with `GetOwner()` without validating the index is in range `[0, k_MAX_PLAYERS)`.
  *Fix: Add bounds check: `if (GetOwner() < 0 || GetOwner() >= k_MAX_PLAYERS) return;`*

- **ArmyData.cpp:1027** — `m_array[0].IsAsleep()` is accessed without first verifying `m_nElements > 0`. On an empty army this is an out-of-bounds read.
  *Fix: Add `if (m_nElements <= 0) return false;` before accessing `m_array[0]`.*

- **ArmyData.cpp:1060** — `m_array[0].IsEntrenched()` is accessed without checking `m_nElements > 0`.
  *Fix: Guard with an `m_nElements` check.*

- **ArmyData.cpp:1066** — `m_array[0].IsEntrenching()` is accessed without checking `m_nElements > 0`.
  *Fix: Guard with an `m_nElements` check.*

- **ArmyData.cpp:1413** — `m_array[0].GetOwner()` is accessed without verifying `m_nElements > 0`.
  *Fix: Add an `m_nElements > 0` guard.*

- **Barbarians.cpp:193** — `g_rand->Next(g_theRiskDB->Get(...)->GetHutMaxBarbarians() - 1)` can be called with `-1` if `GetHutMaxBarbarians()` returns 0, causing undefined behavior in the RNG.
  *Fix: Ensure the argument to `Next` is non-negative, e.g. `max(0, hutMax - 1)`.*

- **Barbarians.cpp:197** — `g_rand->Next(g_theRiskDB->Get(...)->GetMaxSpontaniousBarbarians() - 1)` can pass `-1` to `Next` if the DB value is 0.
  *Fix: Clamp the argument to `Next` to be >= 0.*

- **Barbarians.cpp:346** — Same unchecked `g_rand->Next(... - 1)` pattern in `AddPirates` for hut barbarians.
  *Fix: Clamp the value before passing to `Next`.*

- **Barbarians.cpp:350** — Same unchecked `g_rand->Next(... - 1)` pattern in `AddPirates` for spontaneous barbarians.
  *Fix: Clamp the value before passing to `Next`.*

- **Civilisation.cpp:152** — `g_player[pi] = new Player(...)` assigns to the global array using `pi`, which is validated as `!= PLAYER_INDEX_INVALID` but not checked against `k_MAX_PLAYERS`. An invalid `pi` could write past the array bounds.
  *Fix: Add explicit bounds check: `Assert(pi >= 0 && pi < k_MAX_PLAYERS); if (pi < 0 || pi >= k_MAX_PLAYERS) return;`*

- **CivilisationPool.cpp:165** — `g_rand->Next(numCivs)` is called without checking that `numCivs > 0`. If the civilisation database is empty, this is undefined behavior.
  *Fix: Assert or check `numCivs > 0` before calling `Next`.*

- **Cont.cpp:512** — `m_land_cities[idx]++` in `IncLandCities` does not validate `idx` against `m_num_land_cont`.
  *Fix: Add assertion/runtime check: `Assert(idx >= 0 && idx < m_num_land_cont); if (idx < 0 || idx >= m_num_land_cont) return;`*

- **Cont.cpp:522** — `m_land_num_next_to_edge_cities[idx]++` does not validate `idx`.
  *Fix: Add bounds check.*

- **Cont.cpp:532** — `m_land_armies[idx]++` does not validate `idx`.
  *Fix: Add bounds check.*

- **Cont.cpp:554** — `m_water_armies[idx]++` does not validate `idx` against `m_num_water_cont`.
  *Fix: Add bounds check.*

- **Cont.cpp:564** — `m_water_cities[idx]++` does not validate `idx`.
  *Fix: Add bounds check.*

- **Cont.cpp:573** — `m_water_num_next_to_edge_cities[idx]++` does not validate `idx`.
  *Fix: Add bounds check.*

- **Cont.cpp:654** — `m_mst[idx_cont] = mst;` does not validate `idx_cont` against `m_num_land_cont`.
  *Fix: Add bounds check.*

- **Cont.cpp:680** — `m_land_space_launchers[idx_cont]++` does not validate `idx_cont`.
  *Fix: Add bounds check.*

- **Cont.cpp:688** — `m_water_space_launchers[idx_cont]++` does not validate `idx_cont`.
  *Fix: Add bounds check.*

- **GWArchive.cpp:33** — `sprintf(exportName, "%s\\export%d", basePath, index)` writes into a 1024-byte stack buffer without verifying that `basePath` plus suffix fits. A very long `basePath` causes a stack buffer overflow.
  *Fix: Use `snprintf(exportName, sizeof(exportName), "%s\\export%d", basePath, index)` or validate `strlen(basePath)` first.*

- **GWArchive.cpp:55** — `sprintf(mergeName, "%s\\summary", basePath)` is another unbounded write into a 1024-byte stack buffer.
  *Fix: Replace with `snprintf(mergeName, sizeof(mergeName), ...)`.*

- **GWArchive.cpp:55** — `sprintf(mergeName, "%s\\summary", basePath);` can overflow the 1024-byte `mergeName` buffer if `basePath` is too long.
  *Fix: Use `snprintf(mergeName, sizeof(mergeName), "%s\\summary", basePath);`.*

- **GWCivRecord.cpp:299** — `strcpy(unitRecord->name, unitName)` copies into a fixed 128-byte field without checking `unitName` length.
  *Fix: Use `strncpy(unitRecord->name, unitName, sizeof(unitRecord->name) - 1)` and null-terminate.*

- **GWCivRecord.cpp:330** — `strcpy(unitRecord->name, unitName)` in `UnitKill` has the same unbounded copy into a 128-byte buffer.
  *Fix: Use `strncpy` with size limit.*

- **GWCivRecord.cpp:335** — `strcpy(unitRecord->killerAIP, killerAIP)` copies into an 80-byte field without a length check.
  *Fix: Use `strncpy` with `sizeof(unitRecord->killerAIP) - 1`.*

- **GWCivRecord.cpp:340** — `strcpy(unitRecord->killedAIP, killedAIP)` copies into an 80-byte field without a length check.
  *Fix: Use `strncpy` with `sizeof(unitRecord->killedAIP) - 1`.*

- **GWCivRecord.cpp:94** — `sprintf(fileName, "%s.txt", baseName);` can overflow the 1024-byte `fileName` buffer if `baseName` is too long.
  *Fix: Use `snprintf(fileName, sizeof(fileName), "%s.txt", baseName);`.*

- **GWCivRecord.cpp:98** — `sprintf(fileName, "%s.txt", baseName)` writes into a 1024-byte local buffer without length checks on `baseName`.
  *Fix: Use `snprintf(fileName, sizeof(fileName), ...)`.*

- **GWFile.cpp:128** — Same unbounded `sprintf` into a 32-byte `shortName` in the `Receive` function.
  *Fix: Use `snprintf` with `sizeof(shortName)`.*

- **GWFile.cpp:156** — `stampSize` is read directly from a file (`fread(&stampSize, ...)`) and then used to allocate `new char[stampSize]`. No validation that the size is sane, allowing excessive allocation or a zero-length allocation.
  *Fix: Add a sanity cap (e.g., `stampSize <= MAX_STAMP_SIZE`) before allocating.*

- **GWFile.cpp:52** — `sprintf(shortName, "record%d.dat", fileNumber)` writes into a 32-byte buffer. A large `fileNumber` can overflow the buffer.
  *Fix: Use `snprintf(shortName, sizeof(shortName), "record%d.dat", fileNumber)`.*

- **GameEventManager.cpp:261** — `g_eventDescriptions[m_processingEvent].name` is accessed without re-validating the index. `m_processingEvent` is set from `event->GetType()` (line 258). Events are validated on insertion, but a corrupted save game or deserialization bug could load an event with an out-of-range type, leading to an out-of-bounds array read.
  *Fix: Add bounds check before access: `Assert(m_processingEvent >= 0 && m_processingEvent < GEV_MAX); if (m_processingEvent < 0 || m_processingEvent >= GEV_MAX) return GEV_ERR_BadEvent;`*

- **Installation.cpp:33** — `if(GetOwner() >= 0 && g_player[GetOwner()])` only checks the lower bound. `GetOwner()` could be `>= k_MAX_PLAYERS`, causing an out-of-bounds read.
  *Fix: Also check `GetOwner() < k_MAX_PLAYERS`.*

- **MessageData.cpp:1385** — `strcpy(m_caption, caption)` copies into a fixed-size buffer `m_caption[k_MAX_MSG_LEN]` without checking the length of `caption`. An overlong caption overflows the buffer.
  *Fix: Use `strncpy(m_caption, caption, k_MAX_MSG_LEN - 1)` and null-terminate.*

- **Order.cpp:254** — `return s_orderToEventMap[order];` indexes array with `order` (UNIT_ORDER_TYPE) without validating it is < `UNIT_ORDER_MAX`. Invalid enum values cause out-of-bounds read.
  *Fix: Add bounds check: `if (order < 0 || order >= UNIT_ORDER_MAX) return GEV_MAX;`*

- **Player.cpp:6494** — `if(type < 0 || type > g_theGovernmentDB->NumRecords())` uses `>` instead of `>=`. When `type == NumRecords()`, the subsequent `g_theGovernmentDB->Get(type)` accesses an out-of-bounds record.
  *Fix: Change `>` to `>=`.*

- **Pollution.cpp:531** — `Pollution::GetPollutionAtRound()` returns `g_player[player]->GetPollutionHistory()[current_round - round]`. While there is a check against `k_MAX_POLLUTION_HISTORY`, the index `current_round - round` is an `sint32` and there is no explicit check that `round >= 0` or that the index is within the actual array bounds of the player's pollution history.
  *Fix: Add explicit bounds check on the computed index before array access.*

- **Regard.cpp:28** — `Assert((player>=0) && (player<=k_MAX_PLAYERS))` allows `player == k_MAX_PLAYERS`, but `m_regard` is declared with size `k_MAX_PLAYERS`. This leads to an out-of-bounds write in release builds.
  *Fix: Change the assertion and guard to `player < k_MAX_PLAYERS`.*

- **Regard.cpp:36** — `GetForPlayer` repeats the same off-by-one assertion (`player<=k_MAX_PLAYERS`), allowing an out-of-bounds read of `m_regard`.
  *Fix: Use `player < k_MAX_PLAYERS` in the assertion and add a runtime guard.*

- **SlicBuiltin.cpp:1000** — `PlayerSymbol_LeaderName::GetText` uses `strcpy(text, g_player[pl]->GetLeaderName())` instead of `strncpy(text, ..., maxLen)`. The caller passes `maxLen` but it is ignored, allowing a buffer overflow if the leader name exceeds the destination buffer.
  *Fix: Replace `strcpy(text, ...)` with `strncpy(text, g_player[pl]->GetLeaderName(), maxLen); text[maxLen - 1] = 0;`*

- **SlicBuiltin.cpp:1000** — `strcpy(text, g_player[pl]->GetLeaderName());` copies a leader name into `text` without respecting `maxLen`. If the leader name is longer than the caller-allocated buffer, this overflows.
  *Fix: Use `strncpy(text, g_player[pl]->GetLeaderName(), maxLen)` and ensure null-termination.*

- **SlicBuiltin.cpp:1059** — `PlayerSymbol_He::GetText` uses `strcpy(text, g_theStringDB->GetNameStr(...))` ignoring the `maxLen` parameter, allowing overflow if the string database entry is longer than the destination buffer.
  *Fix: Use `strncpy(text, g_theStringDB->GetNameStr(...), maxLen); text[maxLen - 1] = 0;`*

- **SlicEngine.cpp:2599** — `SlicEngine::AddResearchOnUnblank` copies `text` into `m_researchText` using `strcpy()` without length validation. `m_researchText` is a fixed 256-byte array; a caller-supplied string longer than 255 bytes overflows the buffer.
  *Fix: Use `strncpy(m_researchText, text, 255); m_researchText[255] = 0;`*

- **SlicSegment.cpp:605** — `GetDescription` calls `sprintf(str, "'%s': %s@%d", m_id, m_filename, m_firstLineNumber)` without respecting `maxsize`. A long filename or segment ID overflows the caller's buffer.
  *Fix: Use `snprintf(str, maxsize, ...)`.*

- **SlicSegment.cpp:647** — `FindNextLine` iterates bytecode with `while(codePtr < m_code + m_codeSize)` but inside the loop cases like `SOP_PUSHI` do `codePtr += sizeof(int)` without checking there are enough bytes left. Malformed bytecode causes out-of-bounds read.
  *Fix: Before each advance, verify `codePtr + advanceAmount <= m_code + m_codeSize`.*

- **SlicSegment.cpp:759** — `GetSourceLines` calls `slicif_read_sint32(codePtr, &line)` and advances `codePtr` without verifying the pointer is still within `m_code + m_codeSize` after `FindNextLine` returns.
  *Fix: Add bounds checks after `FindNextLine` and before each fixed-size read.*

- **SpriteStateDB.cpp:185-189** — `ParseASpriteState(spriteToken, count)` is called in a loop with `count` incrementing. `SetName(count, str)` inside only asserts the bounds. In release, if the file contains more entries than declared, it writes out of bounds.
  *Fix: Add `if (count >= m_size) { ...; return FALSE; }` before calling `SetName`/`SetVal`.*

- **TurnCnt.cpp:1166** — `strcpy(fullPath, g_civPaths->GetDesktopPath())` copies an externally provided path into a `MBCHAR fullPath[_MAX_PATH]` buffer without length validation. If the desktop path is longer than `_MAX_PATH`, this overflows the stack buffer.
  *Fix: Use `strncpy(fullPath, g_civPaths->GetDesktopPath(), _MAX_PATH - 1)` and ensure null-termination.*

- **WrlEnv.cpp:100** — `IsShallowWater(const sint32 x, const sint32 y)` directly accesses `m_map[x][y]->m_env` without validating that `x` and `y` are within the map dimensions. The same pattern exists in `IsLand(x,y)`, `IsSpace(x,y)`, `IsRiver(x,y)`, `SetCurrent(x,y)`, `SetRiver`, `UnsetRiver`, `SetGood`, `SetRandomGood`, `SetMovementType`, and others.
  *Fix: Add bounds checks (`0 <= x < m_size.x`, `0 <= y < m_size.y`) before array access, or centralize validation in `GetCell(x,y)`.*

- **agreementmatrix.cpp:124** — `GetAgreement` computes an index and returns `m_agreements[index]`. In release builds, there is no validation that `index < m_agreements.size()` or that `sender_player`/`receiver_player` are within `[0, m_maxPlayers)`.
  *Fix: Add bounds checks before indexing: `if (sender_player < 0 || sender_player >= m_maxPlayers || receiver_player < 0 || receiver_player >= m_maxPlayers) return s_badAgreement;`.*

- **appstrings.cpp:41** — `inStr[strlen(inStr)-1] = '\0';` assumes `strlen(inStr) > 0`. If `fgets` reads an empty line, `strlen(inStr)` is 0 and the index underflows (size_t wrap-around), causing an out-of-bounds write.
  *Fix: Check `if (strlen(inStr) > 0) inStr[strlen(inStr)-1] = '\0';`.*

- **aui_blitter.cpp:1134** — In `BevelBlt8`, `destBuf` is offset using `bevelRect->top * destPitch + bevelRect->left`. `BevelBlt` only checks that `bevelRect` intersects `clippedDestRect`; it does **not** clip `bevelRect` to the actual surface dimensions. Consequently, a malformed `bevelRect` can write outside the surface buffer.
  *Fix: Clip `bevelRect` to the surface bounds in `BevelBlt` before passing it down, or add explicit bounds checks in `BevelBlt8`.*

- **aui_blitter.cpp:1232** — `BevelBlt16` has the same issue: it offsets `destBuf` with `bevelRect->top * destPitch + bevelRect->left` without ensuring `bevelRect` lies entirely inside the surface.
  *Fix: Apply the same clipping/bounds checks as suggested for `BevelBlt8`.*

- **aui_blitter.cpp:2278** — `StretchBlt8To8` advances the source pointer via `srcPtr += stepHorizontal` (stored in a `double`) and casts back to a pointer. There is no verification that the resulting `srcBuf` stays within the allocated source buffer. A large step or destination rectangle can cause out-of-bounds reads.
  *Fix: Clamp `srcBuf` to `[origSrcBuf, origSrcBuf + srcPitch * srcHeight)` before each dereference, or pre-compute bounded offsets.*

- **aui_blitter.cpp:2417** — `StretchBlt16To16` uses the same double-based pointer arithmetic (`srcPtr += stepHorizontal`) without bounds checking. Additionally, the alignment fix at line 2423 (`srcBuf = (uint16 *)(size_t(srcBuf) - 1)`) can move the pointer before `origSrcBuf`, causing an out-of-bounds read.
  *Fix: Bounds-check the computed source address against the source buffer limits; avoid the alignment hack by using byte-level reads or ensuring the step calculation produces aligned addresses.*

- **aui_control.cpp:742** — `DrawThisStateImage` accesses `m_stringTable->GetString(state)` after clamping `state` to `GetNumStrings() - 1`. If `GetNumStrings()` returns `0`, `state` becomes `-1`, but the subsequent `if (state >= 0)` guard only protects when `state` was negative to begin with. After the clamp `state = 0 - 1 = -1`, the `state >= 0` check is false, so it's actually safe... Wait, re-reading: if `GetNumStrings() == 0` and original `state == 0`, line 734 sets `state = 0`, line 736 `0 > 0` is false, so state stays 0, line 739 `0 >= 0` is true, and `GetString(0)` is called on an empty table. Out-of-bounds access.
  *Fix: Change the condition to `if (state >= 0 && state < m_stringTable->GetNumStrings())`.*

- **c3cmdline.cpp:1177** — `sint32 player1 = atoi(argv[1]);` is passed to `Diplomat::GetDiplomat(player1)` without validating it is within `[0, k_MAX_PLAYERS)`. A negative or out-of-range value causes out-of-bounds access.
  *Fix: Add `if (player1 < 0 || player1 >= k_MAX_PLAYERS) return;`.*

- **c3cmdline.cpp:5207** — `g_player[atoi(argv[1])]` is dereferenced without bounds checking the parsed integer.
  *Fix: Store the result in a variable and validate `0 <= player && player < k_MAX_PLAYERS` before indexing.*

- **c3cmdline.cpp:5287** — `d_plr = atoi(argv[4]);` is used to index `g_player[d_plr]` without bounds validation.
  *Fix: Validate `d_plr` is in `[0, k_MAX_PLAYERS)` before indexing.*

- **c3cmdline.cpp:5539** — `player = atoi(argv[3]);` is only guarded by `Assert`, which is stripped in release builds. A malformed command can pass an out-of-range player index to `g_player[player]`.
  *Fix: Add a runtime check: `if (player < 0 || player >= k_MAX_PLAYERS) return;`.*

- **c3debug.cpp:233** — `vsprintf(g_last_debug_text + strlen(g_last_debug_text), format, list);` appends to a 4096-byte global buffer without checking remaining capacity. Long or repeated debug output overflows the buffer.
  *Fix: Use `vsnprintf` with a computed remaining-size limit.*

- **civapp.cpp:1174** — `sprintf(lastdot, "%d.txt", 0);` writes into `g_citysize_filename` at the position of the last dot. If the remaining space after `lastdot` is smaller than 5 bytes (for "0.txt"), the buffer overflows.
  *Fix: Use `snprintf(lastdot, remaining_space, "%d.txt", 0)` where `remaining_space` is computed from the buffer end.*

- **civapp.cpp:502** — `strcpy(ruleSets, g_theProfileDB->GetRuleSets());` copies into a `MAX_PATH` stack buffer without verifying the source string length. An overly long ruleset string overflows the buffer.
  *Fix: Use `strncpy(ruleSets, g_theProfileDB->GetRuleSets(), sizeof(ruleSets) - 1); ruleSets[sizeof(ruleSets) - 1] = '\0';`.*

- **ctp2_code/gfx/spritesys/SpriteGroupList.cpp:91** — `LoadSprite` accesses `m_spriteList[index]` where `index` is a `uint32` parameter. There is no bounds check before this array access; the check happens later in `GetSprite` but not in `LoadSprite` itself.
  *Fix: Add `Assert(index < k_MAX_SPRITES); if (index >= k_MAX_SPRITES) return SPRITELISTERR_NOTFOUND;` at the top of `LoadSprite`.*

- **ctp2_code/gfx/spritesys/UnitSpriteGroup.cpp:743** — `GetHotPoint` checks `if (facing >= k_NUM_FACINGS) facing = k_MAX_FACINGS - facing;`, but if `facing` is negative or larger than `2*k_MAX_FACINGS`, the adjusted value can still be negative or out of bounds, leading to an invalid array index cast to `uint16`.
  *Fix: Clamp `facing` to the valid range with `facing = std::max(0, std::min(facing, k_NUM_FACINGS - 1));`.*

- **ctp2_code/gfx/spritesys/effectspritegroup.cpp:46** — `Draw` checks `action` with `Assert` only, then immediately uses `m_sprites[action]`. If assertions are disabled and `action` is out of range, this is an array out-of-bounds access.
  *Fix: Replace the assert with an explicit runtime check: `if (action <= EFFECTACTION_NONE || action >= EFFECTACTION_MAX) return;`.*

- **ctp2_code/gfx/spritesys/goodspritegroup.cpp:53** — `Draw` relies on `Assert(action > GOODACTION_NONE && action < GOODACTION_MAX)` before indexing `m_sprites[action]`. With assertions disabled, invalid actions cause out-of-bounds access.
  *Fix: Add an explicit guard returning early when `action` is invalid.*

- **ctp2_code/gfx/spritesys/spritefile.cpp:68** — `SpriteFile` constructor copies `name` into `m_filename` with `strcpy(m_filename, name)` without checking that `name` fits in the member buffer.
  *Fix: Use `strncpy(m_filename, name, sizeof(m_filename)-1)` and null-terminate.*

- **ctp2_code/gs/events/GameEventArgList.cpp:64** — The constructor iterates through a `va_list` and advances `argString` by two characters per argument (`argString++; argString++;`). There is no check that `argString` hasn't run past the end of the `g_eventDescriptions[eventType].args` format string, leading to out-of-bounds reads.
  *Fix: Track the length of `argString` and break if the remaining length is less than 2 before each advance.*

- **effectspritegroup.cpp:46** — `m_sprites[action]` is indexed after an `Assert` but with no runtime bounds check. In release builds a malformed `action` value can read/write outside the array.
  *Fix: Add an explicit `if (action <= EFFECTACTION_NONE || action >= EFFECTACTION_MAX) return;` before indexing.*

- **effectspritegroup.cpp:46-51** — `Draw()` asserts `action < EFFECTACTION_MAX` but then indexes `m_sprites[action]` and `m_anims[action]`. In release builds where asserts are disabled, a malformed/corrupt action value causes an out-of-bounds array access.
  *Fix: Replace the assert with a runtime bounds check that returns early.*

- **gameinit.cpp:430** — In `gameinit_PlaceInitalUnits`, `nPlayers` is passed in and used to loop `for (i=1; i<nPlayers; i++)`. Inside the loop, `g_player[i]` and `g_player_start_list[which]` are accessed. There is no check that `nPlayers <= k_MAX_PLAYERS` before the loop, allowing out-of-bounds access if a caller passes a large value.
  *Fix: Add `Assert(nPlayers <= k_MAX_PLAYERS); if (nPlayers > k_MAX_PLAYERS) nPlayers = k_MAX_PLAYERS;` before the loop.*

- **goodspritegroup.cpp:53-80** — `Draw()` and `DrawDirect()` assert action bounds but index `m_sprites[action]` without a runtime check. In release builds this is an out-of-bounds access if action is corrupt.
  *Fix: Add explicit bounds check before array indexing.*

- **goodspritegroup.cpp:56** — `m_sprites[action]` is accessed after only an `Assert` with no runtime check on `action`.
  *Fix: Add a runtime bounds check before indexing `m_sprites`.*

- **goodspritegroup.cpp:84-89** — `GetHotPoint()` does not assert or check `action` bounds before indexing `m_sprites[action]`.
  *Fix: Add bounds check before array access.*

- **iparser.cpp:2879** — `sprintf(full_file_name, "%s%s", directory_name, file_name);` writes into a 500-byte buffer without length checking. A long directory path combined with a file name can overflow `full_file_name`.
  *Fix: Use `snprintf(full_file_name, sizeof(full_file_name), "%s%s", directory_name, file_name);`.*

- **net_city.cpp:300** — `strcpy(home_city.GetData()->GetCityData()->m_name, name);` copies `name` into the city's name field without checking the destination buffer size. A long name from the network can overflow the buffer.
  *Fix: Use `strncpy` with the destination buffer size and ensure null-termination.*

- **net_ready.cpp:14-27** — `Packetize` writes multiple `PUSHBYTE`/`PUSHDOUBLE` operations to `buf` without bounds checking. `Unpacketize` similarly reads with `PULLBYTE`/`PULLDOUBLE` without verifying `size`.
  *Fix: Add size validation in both directions.*

- **net_ready.cpp:39** — `Unpacketize` reads fields using `PULLBYTE`/`PULLDOUBLE` macros without checking that the buffer `size` is large enough to contain all the data. A truncated packet causes an out-of-bounds read.
  *Fix: Track `pos` and validate `pos <= size` after each pull operation.*

- **net_research.cpp:22-29** — `Packetize` writes to `buf[size]` incrementally without checking buffer capacity. The loop can write up to `(m_adv->m_size / 8) + 1` bytes past the header.
  *Fix: Accept a buffer capacity parameter and check `size < capacity` before each write.*

- **net_research.cpp:46-51** — `Unpacketize` reads from `buf[pos]` and increments `pos` without checking against `size`. Corrupt packets cause out-of-bounds reads.
  *Fix: Check `pos < size` before accessing `buf[pos]`.*

- **net_strengths.cpp:21-29** — `NetStrengths::Packetize` writes to `buf` without bounds checking. `PUSHLONG` and `PUSHID` macros expand to unchecked writes.
  *Fix: Pass and validate buffer capacity.*

- **net_strengths.cpp:39-59** — `NetStrengths::Unpacketize` reads with `PULLLONG` without validating `size`. The loop at line 50 reads `STRENGTH_CAT_MAX - 1` longs without bounds checking.
  *Fix: Verify `size >= expected_size` before entering the loop.*

- **net_terrain.cpp:131-138** — `Unpacketize` uses `PULLLONG`, `PULLBYTE`, `PULLSHORT` etc. without verifying that `pos + sizeof(type) <= size`. Corrupt packets cause out-of-bounds reads.
  *Fix: Check remaining bytes before each pull operation.*

- **net_terrain.cpp:78-91** — `Packetize` writes approximately 18 bytes to `buf` without checking the buffer capacity.
  *Fix: Add a capacity parameter and validate.*

- **net_thread.cpp:486** — `m_response->RemovePlayer(getshort(packet->m_buf));` reads 2 bytes without verifying `packet->m_len >= 2`.
  *Fix: Validate `packet->m_len >= sizeof(uint16)` before calling `getshort`.*

- **net_thread.cpp:492** — `m_response->ChangeHost(getshort(packet->m_buf));` reads 2 bytes without verifying `packet->m_len >= 2`.
  *Fix: Validate `packet->m_len >= sizeof(uint16)` before calling `getshort`.*

- **net_thread.cpp:500** — `uLongf uSize = getlong(&packet->m_buf[1]);` reads 4 bytes starting at offset 1 without checking `packet->m_len >= 5`. A short packet causes an out-of-bounds read.
  *Fix: Add `if (packet->m_len < 5) { delete packet; continue; }`.*

- **net_thread.cpp:82-83** — `TPacketData` constructor allocates `new uint8[len + headerLen]` but does not check for integer overflow in `len + headerLen`. If both are large, the sum can wrap, allocating a tiny buffer that is then overfilled by the two `memcpy` calls.
  *Fix: Check for overflow: `if (len > SIZE_MAX - headerLen) abort;`.*

- **net_wonder.cpp:21** — `Unpacketize` reads a `uint64` with `PULLLONG64` and then a `uint32` with `PULLLONG` without validating that `pos` does not exceed `size`.
  *Fix: Track `pos` and add `Assert(pos <= size);` after each read.*

- **net_wonder.cpp:21-27** — `Unpacketize` reads from `buf` without validating `size`. `PULLLONG64` reads 8 bytes without checking.
  *Fix: Validate remaining bytes before each pull.*

- **net_wonder.cpp:6-12** — `Packetize` writes to `buf` without bounds checking.
  *Fix: Add capacity validation.*

- **network.cpp:2450-2461** — In `SendChatText`, parsing `/msg` copies characters into `destination[256]` without bounds checking. A long name overflows the stack buffer.
  *Fix: Replace with `strncpy(destination, c, sizeof(destination) - 1)` or similar.*

- **network.cpp:2473-2475** — Copying the leader name into `name[256]` has no bounds check. A long leader name overflows the stack buffer.
  *Fix: Use a bounded copy like `strncpy`.*

- **network.cpp:680** — `uint8 buf[512]` is passed to `guid->Packetize(buf, size)` and `report->Packetize(buf, size)`. If either packetizer produces more than 512 bytes, this overflows the stack buffer.
  *Fix: Pass `sizeof(buf)` to `Packetize` and enforce the limit.*

- **network.cpp:791** — `uint8 buf[8192]` is passed to `packet->Packetize(buf, psize)`. If a packet exceeds 8192 bytes, this overflows the stack buffer.
  *Fix: Pass `sizeof(buf)` and abort if the packet is too large.*

- **profileDB.cpp:467** — `g_theCivilisationDB->Get(m_civIndex)->GetLeaderNameMale()` — `m_civIndex` is initialized to `PLAYER_COUNT_MAX_DEFAULT` (16) in the constructor, but there is no guarantee the civilization database has that many records. An out-of-bounds access can occur on startup with a minimal database.
  *Fix: Check `if (m_civIndex >= 0 && m_civIndex < g_theCivilisationDB->NumRecords())` before calling `Get()`.*

- **profileDB.cpp:549-554** — After `sint32 len = strlen(line);`, a `while(isspace(line[len - 1]))` loop decrements `len`. If `line` is an empty string, `len` is 0 and `line[len - 1]` becomes `line[-1]`, an out-of-bounds read.
  *Fix: Add `if (len == 0) continue;` before the while-loop.*

- **robotcom.cpp:436** — `sprintf(fullname, "personality%s.fli", filename)` writes into `fullname[1024]` without checking if `filename` is short enough. A long `filename` causes a stack buffer overflow.
  *Fix: Use `snprintf(fullname, sizeof(fullname), "personality%s.fli", filename)`.*

- **segmentlist.cpp:100** — `strcpy(windowBlock, ldlBlock)` copies the `ldlBlock` parameter without verifying its length fits into `windowBlock[k_AUI_LDL_MAXBLOCK + 1]`.
  *Fix: Use `strncpy(windowBlock, ldlBlock, k_AUI_LDL_MAXBLOCK)` and null-terminate, or assert `strlen(ldlBlock) <= k_AUI_LDL_MAXBLOCK`.*

- **segmentlist.cpp:181** — `sprintf(controlBlock, "%s.%s", windowBlock, "SegmentList")` can write past the end of `controlBlock[k_AUI_LDL_MAXBLOCK + 1]` if `windowBlock` is already near the buffer limit.
  *Fix: Use `snprintf(controlBlock, k_AUI_LDL_MAXBLOCK + 1, "%s.%s", windowBlock, "SegmentList")`.*

- **sourcelist.cpp:135** — `strcpy(windowBlock, ldlBlock)` copies the `ldlBlock` parameter without verifying its length fits into `windowBlock[k_AUI_LDL_MAXBLOCK + 1]`.
  *Fix: Use `strncpy(windowBlock, ldlBlock, k_AUI_LDL_MAXBLOCK)` and null-terminate.*

- **sourcelist.cpp:227** — `sprintf(controlBlock, "%s.%s", windowBlock, "SourceList")` can overflow `controlBlock[k_AUI_LDL_MAXBLOCK + 1]`.
  *Fix: Use `snprintf(controlBlock, k_AUI_LDL_MAXBLOCK + 1, ...)` throughout `Initialize()`.*

- **sourcelist.cpp:362** — `sprintf(statusBuf, "Break at %s:%d", m_segment->GetName(), lineNumber)` writes into `statusBuf[1024]` without checking if `GetName()` is long enough to overflow the buffer.
  *Fix: Use `snprintf(statusBuf, sizeof(statusBuf), ...)`.*

- **sourcelist.cpp:370** — `strcat(statusBuf, " when ")` and `strcat(statusBuf, cond->GetExpression())` append to `statusBuf[1024]` without verifying there is enough remaining space.
  *Fix: Use `strncat` with calculated remaining space, or use `snprintf` to build the entire string safely.*

- **thronedb.cpp:222** — `strcpy(throneInfo->m_zoomedImageFilename, str)` copies into a fixed-size member without length validation. An overly long input string overflows the buffer.
  *Fix: Use `strncpy(throneInfo->m_zoomedImageFilename, str, sizeof(throneInfo->m_zoomedImageFilename) - 1)` and null-terminate.*

- **tracklen.cpp:59** — `tracklen_cryptAscii` copies into a static `buf[256]` but the input `tracklen_buf` is 512 bytes. If the input string is longer than 255 characters, the function overflows its internal buffer.
  *Fix: Use a bounds-limited loop or pass the output buffer size and enforce it.*

- **victorymoviewin.cpp:153** — `g_theVictoryMovieDB->FindTypeIndex(whichMovie)` returns -1 when the movie type is not found, but the result is passed directly to `GetMovieFilename(index)` without validation. Passing -1 as an array index causes out-of-bounds access.
  *Fix: Check `if (index >= 0)` before calling `GetMovieFilename(index)`.*

- **watchlist.cpp:116** — `strcpy(windowBlock, ldlBlock ? ldlBlock : "WatchListPopup")` copies without verifying length fits in `windowBlock[k_AUI_LDL_MAXBLOCK + 1]`.
  *Fix: Use `strncpy(windowBlock, ldlBlock ? ldlBlock : "WatchListPopup", k_AUI_LDL_MAXBLOCK)` and null-terminate.*

- **watchlist.cpp:205** — `sprintf(controlBlock, "%s.%s", windowBlock, "WatchList")` can overflow `controlBlock[k_AUI_LDL_MAXBLOCK + 1]`.
  *Fix: Use `snprintf(controlBlock, k_AUI_LDL_MAXBLOCK + 1, ...)` throughout `Initialize()`.*

- **watchlist.cpp:289** — `strncpy(m_line, line, k_MAX_WATCH_LINE)` does not guarantee null-termination if `line` is `>= k_MAX_WATCH_LINE` characters. `SetFieldText(m_line)` in `InitCommonLdl` may then read past the buffer.
  *Fix: After `strncpy`, explicitly null-terminate: `m_line[k_MAX_WATCH_LINE - 1] = '\0';`.*


### MISSING_BOUNDS_CHECK/NULL_DEREFERENCE

- **c3cmdline.cpp:~5718** — `SetWorkdayCommand::Execute`, `SetWagesCommand::Execute`, and `SetRationsCommand::Execute` all access `argv[1]` without first verifying `argc > 1`.
  *Fix: Add `if (argc < 2) return;` at the start of each function.*


### NULL_DEREFERENCE

> **Resolution status — obvious P3 HIGH NULL_DEREFERENCE mini-batch verified in code 2026-07-02.**
> `aui_tab.cpp:91` now rejects a null `ldlBlock` before formatting it with `%s`.
> `c3cmdline.cpp:5580` / `TaxCommand::Execute` now requires the three tax arguments before reading `argv[1..3]` and validates the optional player id before `player_Get(player)`.
> `c3errors.cpp:64/104` now uses `abort()` instead of deliberate null-pointer writes for release-Win32 crash paths.
> `director.cpp:1250` now returns when `mover.GetActor()` is null before asserting or queueing a move action.
> `SetWorkdayCommand` / `SetWagesCommand` / `SetRationsCommand` now require exactly one value argument and validate the visible player before dereferencing `player_Get(...)`.
> `Player.cpp:5151` was verified against current code and both city-transfer paths now return when `GetCityData()` is null before `TeleportUnits(...)`.
> `UnitActor.cpp` draw helpers now guard the reported null tile/image/player/shield/civilisation-record lookups before dereferencing them.
> `directorevent.cpp:222/224` now validates the visible player, vision pointer, and general-success special-effect record before dereferencing.
> `CityData.cpp:1466` is stale in the current tree; the `SPECATTACK_REVOLUTION` record is checked before `GetSoundIDIndex()`.
> `gfx_options.cpp:164` is already fixed; `AddTextToCell` checks `text` before `strlen(text)`.
> `c3files.cpp:617` is stale/obsolete in the current tree; CD validation is now the stub `c3files_HasLegalCD() { return true; }`.
> `c3files_getfilelist` now uses `strlcpy` for fixed-size filename buffers and allocates the POSIX `NAME_MAX + 1` byte terminator.

- **CityData.cpp:1466** — `specRec->GetSoundIDIndex()` is called when `unitutil_GetSpecialAttack(SPECATTACK_REVOLUTION)` could return NULL.
  *Fix: Ensure `specRec != NULL` before calling `GetSoundIDIndex()`.*

- **Player.cpp:5151** — `cityData->TeleportUnits(newPos, revealed_foreign_units, recipient);` uses `cityData` obtained from `city.GetData()->GetCityData()` without verifying it is non-NULL. If `city` is not a valid city, this crashes.
  *Fix: Add `if (!cityData) return;` before calling `TeleportUnits`.*

- **UnitActor.cpp:998** — `g_tiledMap->GetTileSet()->GetImprovementData(34)` dereferences the result of `GetTileSet()` without checking for NULL. If the tileset is missing, this crashes.
  *Fix: Add null checks for each step: `TileSet* ts = g_tiledMap ? g_tiledMap->GetTileSet() : NULL; if (!ts) return;`.*

- **WorkWin.cpp:82** — `g_selected_item->GetSelectedCity(city)` is called without checking whether `g_selected_item` is non-null. The same function checks `g_workWindow` and `g_c3ui` before dereferencing them, but not `g_selected_item`.
  *Fix: Add a null check: `if (!g_selected_item) return;` before the call.*

- **aui_tab.cpp:91** — `InitCommon(MBCHAR *ldlBlock)` passes `ldlBlock` directly to `sprintf` as a `%s` argument. If `ldlBlock` is `NULL`, `sprintf` dereferences a null pointer (behavior is undefined and typically crashes).
  *Fix: Add `if (!ldlBlock) return AUI_ERRCODE_INVALIDPARAM;` before the `sprintf`.*

- **c3cmdline.cpp:5580** — `TaxCommand::Execute` calls `sscanf(argv[1], "%lf", &s)` without first checking `argc > 1`. If `argc` is 1, this reads from a NULL or invalid pointer.
  *Fix: Add `if (argc < 2) return;` at the start.*

- **c3errors.cpp:104** — Same as line 64 — intentional null pointer dereference (`sint32 *s = 0; *s = 0;`) used to crash the process.
  *Fix: Use `abort()` or `raise(SIGABRT)` for a controlled crash.*

- **c3errors.cpp:64** — `sint32 *s = 0; *s = 0;` deliberately dereferences a null pointer to force a crash. While intentional, it is undefined behavior and may not produce the desired result on all platforms.
  *Fix: Use `abort()` or `raise(SIGABRT)` instead of a null dereference.*

- **c3files.cpp:617** — `const char *cd_dev = SDL_CDName(id); FILE *cd = fopen(cd_dev, "rb");` does not check if `cd_dev` is NULL before passing it to `fopen`. If SDL returns NULL, this can crash.
  *Fix: Add `if (!cd_dev) return NULL;` before `fopen`.*

- **civ3_main.cpp:1092** — `c3debug_ExceptionStackTraceFromFile(fopen("crash.txt", "r"))` passes the result of `fopen` directly without a null check. If the file does not exist, the inner function dereferences a null FILE*.
  *Fix: Store the fopen result in a variable, check for null, and only then call the stack-trace function.*

- **civ3_main.cpp:1093** — `FILE *txt = fopen("crashmap.txt", "w");` is not checked for null before `fprintf(txt, ...)`. A failed open causes an immediate crash.
  *Fix: `if (!txt) return;` before the fprintf call.*

- **ctp2_code/gfx/gfx_utils/gfx_options.cpp:164** — `AddTextToCell` calls `strlen(text)` to allocate `newText`, but `text` is not checked for NULL. A NULL `text` pointer will crash.
  *Fix: Add `if (!text) return false;` at the start of the function, or before the strlen call.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1097** — `DrawCityWalls` calls `g_theCityStyleDB->Get(unit.CD()->GetCityStyle())` and only checks `styleRec`, but then calls `styleRec->GetAgeStyle(g_player[unit->GetOwner()]->m_age)` where `g_player[unit->GetOwner()]` is unchecked. Later, `ageStyleRec` results are dereferenced without checking.
  *Fix: Check `g_player[unit->GetOwner()]` for NULL before dereferencing, and guard `ageStyleRec` and `matchingSprite` similarly.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1239** — `DrawForceField` calls `g_tiledMap->GetTileSet()->GetImprovementData((uint16)which)` and uses the pointer without a NULL check.
  *Fix: `if (!cityImage) return;` before the draw calls.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1567** — `DrawHealthBar` dereferences `shieldPoint` from `GetShieldPoints(...)` without checking for NULL at lines 1574 and 1582.
  *Fix: Add `if (!shieldPoint) return;` or use a fallback after each `GetShieldPoints` call.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:1933** — `DrawSpecialIndicators` evaluates `g_theCivilisationDB->Get(civ)->GetNationUnitFlagIndex(civicon)` before confirming the pointer is valid. If `Get(civ)` returns NULL, it crashes.
  *Fix: Store the record pointer first and check it: `const CivilisationRecord* rec = g_theCivilisationDB->Get(civ); if (rec && rec->GetNationUnitFlagIndex(civicon) && civ > -1) { ... }`.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:998** — `DrawFortified` calls `g_tiledMap->GetTileSet()->GetImprovementData(34)` and uses the returned pointer without checking for NULL.
  *Fix: Check `if (!fortifiedImage) return;` before drawing.*

- **ctp2_code/gfx/spritesys/director.cpp:1250** — `AddMove` calls `Assert(actor->GetUnitID() == mover.m_id)` immediately after `UnitActorPtr actor = mover.GetActor();` without verifying `actor` is non-NULL.
  *Fix: Add `if (!actor) return;` before the assertion.*

- **ctp2_code/gfx/spritesys/directorevent.cpp:222** — `DirectorActionSuccessful` dereferences `g_player[g_selected_item->GetVisiblePlayer()]` without a NULL check before accessing `->m_vision->IsVisible(attackPos)`.
  *Fix: `Player* vp = g_player[g_selected_item->GetVisiblePlayer()]; if (!vp || !vp->m_vision) return GEV_HD_Continue;`.*

- **ctp2_code/gfx/spritesys/directorevent.cpp:224** — `g_theSpecialEffectDB->Get(g_theSpecialEffectDB->FindTypeIndex(...))` may return NULL if the lookup fails, but it is immediately dereferenced with `->GetValue()`.
  *Fix: Store the pointer and check before dereferencing.*

- **ctpai.cpp:1646** — `Player *player_ptr = g_player[player];` is dereferenced at line 1649 (`player_ptr->GetGovernmentType()`) without a null check.
  *Fix: Add `if (!player_ptr) return;` before using the pointer.*

- **directorevent.cpp:224** — `g_theSpecialEffectDB->Get(...)->GetValue()` does not check whether `Get()` returns `NULL` before dereferencing the result.
  *Fix: Store the returned pointer, check for `NULL`, and handle the error gracefully.*

- **gameinit.cpp:1352** — In `spriteEditor_Initialize`, after allocating `g_player = new Player*[k_MAX_PLAYERS]` and zero-filling it, the code unconditionally does `g_player[1]->m_gold->SetLevel(1000000);`. If `k_MAX_PLAYERS <= 1` this is an out-of-bounds access; more importantly, there is no null check confirming `g_player[1]` was successfully created (the allocation at line 1255 could theoretically fail or be skipped in a modified build).
  *Fix: Add `Assert(g_player[1]); if (!g_player[1]) return;` before the dereference.*

- **gameinit.cpp:2464** — `g_player[i]->m_email = new MBCHAR[strlen(g_hsPlayerSetup[i].email) + 1];` is followed immediately by `strcpy(g_player[i]->m_email, g_hsPlayerSetup[i].email);`. If `new` fails and returns NULL (as can happen on memory-constrained systems or with certain compiler settings), `strcpy` dereferences a null pointer.
  *Fix: Check the return value of `new` before using it, or use a safe string assignment helper.*

- **network.cpp:1259** — In `Network::SetReady`, `MapPoint* size = g_theWorld->GetSize();` is dereferenced throughout the function without checking if `g_theWorld` or `GetSize()` returned NULL. In early initialization or error conditions this can crash.
  *Fix: Add `Assert(g_theWorld && size); if (!g_theWorld || !size) return;` after the call.*

- **slicif.cpp:246** — `obj->m_code = (unsigned char *)malloc(s_code_ptr - s_code); memcpy(obj->m_code, s_code, s_code_ptr - s_code);` does not check if `malloc` returned NULL. On out-of-memory, `memcpy` dereferences NULL.
  *Fix: Add `if (!obj->m_code) { /* handle error */ return; }` after the malloc.*


### UNALIGNED_ACCESS

- **BaseTile.cpp:105-138** — `QuickRead` casts `uint8*` to `uint16*` and reads through it (`*(uint16 *)(*dataPtr)`). If the source byte buffer is not 2-byte aligned, this causes undefined behavior on strict-alignment architectures.
  *Fix: Use `memcpy` into a local `uint16` variable instead of direct pointer cast and dereference.*

- **GWCivRecord.cpp:32** — The constructor casts a raw `void *data` buffer directly to `GWUnitRecord*` and indexes it. If the buffer is not aligned to the struct's alignment requirements (e.g., 4-byte for `int` members), this is undefined behavior on ARM64.
  *Fix: Use `memcpy` to copy each field from the raw buffer instead of casting and indexing.*

- **SlicFrame.cpp:96** — `memcpy(&dbIndex, instructionPointer, sizeof(int));` reads an `int` from an `unsigned char*` that has been incremented byte-by-byte through bytecode. The pointer is not guaranteed to be aligned to `sizeof(int)`, causing unaligned memory access.
  *Fix: Use `memcpy` into a properly aligned local variable, or use byte-by-byte extraction with `slicif_read_sint32`.*

- **SlicSegment.cpp:798** — Direct dereference of a `uint8*` as `sint32*` without alignment guarantee. `codePtr` points into bytecode which is not guaranteed to be 4-byte aligned. On architectures with strict alignment (e.g., ARM, SPARC, RISC-V) this causes a bus error or SIGBUS.
  *Fix: Replace `curLine = *(sint32 *)codePtr;` with `memcpy(&curLine, codePtr, sizeof(sint32));`*

- **SlicSegment.cpp:798** — `curLine = *(sint32 *)codePtr;` in `FindLineNumber` reads a 32-bit integer from a byte-aligned code buffer, causing unaligned memory access on strict-alignment architectures.
  *Fix: Use `memcpy(&curLine, codePtr, sizeof(curLine))` instead of a direct cast.*

- **SlicSegment.cpp:798** — `curLine = *(sint32 *)codePtr;` reads a 32-bit integer after `codePtr++`. Since `codePtr` is a `uint8*` traversing bytecode, incrementing by 1 leaves it at an odd offset, resulting in an unaligned read.
  *Fix: Use `memcpy` or a dedicated unaligned-safe read function instead of casting the pointer.*

- **SlicSegment.cpp:828** — Same pattern as line 798: `curLine = *(sint32 *)codePtr;` reads a 32-bit integer from potentially unaligned bytecode memory.
  *Fix: Use `memcpy(&curLine, codePtr, sizeof(sint32));` instead of pointer cast dereference.*

- **SlicSegment.cpp:828** — `curLine = *(sint32 *)codePtr;` in `GetCodePointer` has the same unaligned read issue.
  *Fix: Replace with `memcpy(&curLine, codePtr, sizeof(curLine))`.*

- **SlicSegment.cpp:828** — Same unaligned read pattern as above: `curLine = *(sint32 *)codePtr;` after `codePtr++` in `GetCodePointer`.
  *Fix: Use `memcpy` or a byte-safe read helper to avoid unaligned access.*

- **SlicSegment.cpp:853** — `*((SlicConditional**)codePtr)` dereferences a `uint8*` as a pointer-sized type. If `codePtr` is not aligned to the platform pointer size, this is undefined behavior and can crash on strict-alignment architectures.
  *Fix: Use `memcpy` to read the pointer value into a local variable.*

- **SlicSegment.cpp:853** — `LineHasBreak` reads `*((SlicConditional**)codePtr)` from a byte-aligned position, causing unaligned pointer access.
  *Fix: Use `memcpy` to read the pointer value.*

- **SlicSegment.cpp:884** — `*((SlicConditional **)codePtr) = NULL;` writes a pointer through a potentially unaligned `uint8*` address.
  *Fix: Use `memcpy(codePtr, &nullPtr, sizeof(SlicConditional*));` or ensure alignment before writing.*

- **SlicSegment.cpp:884** — `RemoveConditional` writes `*((SlicConditional **)codePtr) = NULL;` to a potentially unaligned address.
  *Fix: Use `memcpy(codePtr, &nullPtr, sizeof(nullPtr))` or a properly aligned intermediate.*

- **SlicSegment.cpp:908** — `*((SlicConditional **)codePtr) = new SlicConditional(expression);` writes a pointer value to bytecode memory that may not be pointer-aligned.
  *Fix: Use `memcpy(codePtr, &newCond, sizeof(SlicConditional*));` instead of casted assignment.*

- **SlicSegment.cpp:908** — `NewConditional` writes a pointer with `*((SlicConditional **)codePtr) = new SlicConditional(expression);` to a potentially unaligned address.
  *Fix: Use `memcpy(codePtr, &ptr, sizeof(ptr))` after allocating the object.*

- **ctp2_code/gfx/tilesys/BaseTile.cpp:105** — `QuickRead` performs unaligned reads via `*(uint16 *)(*dataPtr)` to parse `m_tileNum`, `m_tileDataLen`, and `size` directly from a byte stream. On architectures with strict alignment requirements, this causes undefined behavior or crashes.
  *Fix: Use `memcpy(&m_tileNum, *dataPtr, sizeof(uint16))` instead of casting the pointer.*

- **net_diff.cpp:35** — `Unpacketize` casts `&buf[4]` (a `uint8*`) directly to `Difficulty*` and dereferences it. If `buf` is not aligned to the alignment requirement of `Difficulty`, this causes undefined behavior on architectures that require aligned access.
  *Fix: Use `memcpy` to copy the data into a properly aligned local `Difficulty` variable, then assign.*

- **net_diff.cpp:35** — The expression `*(Difficulty*)&buf[4]` casts an arbitrary byte offset in a `uint8` buffer to a `Difficulty` struct pointer. If `buf[4]` is not aligned to the alignment requirement of `Difficulty` (e.g., 4-byte or 8-byte alignment), this causes an unaligned memory access, which is undefined behavior and can crash on strict-alignment architectures.
  *Fix: Use `memcpy` to copy the bytes into a properly aligned local `Difficulty` variable instead of casting the buffer pointer.*

- **net_thread.cpp:486** — `getshort(packet->m_buf)` reads a 16-bit value from a `uint8*` without alignment guarantee.
  *Fix: Use `memcpy` into a local `uint16`.*

- **net_thread.cpp:492** — `getshort(packet->m_buf)` in `ChangeHost` RPC handler has the same unaligned read issue.
  *Fix: Use `memcpy` into a local `uint16`.*

- **net_thread.cpp:61** — `getlong(&buf[2])` reads a 32-bit value from an unaligned `uint8*` address.
  *Fix: Use `memcpy` into a local `uint32`.*

- **net_unit.cpp:139** — `getlong(&buf[2])` reads a 32-bit value from an unaligned `uint8*` address. On strict-alignment architectures this causes a bus fault.
  *Fix: Use `memcpy` into a local `uint32` or an alignment-safe deserialization routine.*

- **targautils.cpp:306** — `reinterpret_cast<Pixel16 *>(dataPtr)` casts a byte-offset pointer to `Pixel16*`. If the computed `dataPtr` is not 2-byte aligned (e.g., because `data` or the offset is odd), this is undefined behavior on strict-alignment architectures like ARM64.
  *Fix: Ensure the destination buffer is properly aligned, or process pixels with `memcpy`.*

- **tileset.cpp:489-491** — In `LoadMapIcons`, a `uint8*` buffer from `getData` is cast directly to `RIMHeader*` and then `Pixel16*`. If the buffer offset is not aligned to the struct/pixel alignment requirements, this causes unaligned access.
  *Fix: Use `memcpy` to copy header fields from the buffer into a local `RIMHeader` variable.*

- **tileset.cpp:611-646** — `QuickLoadTransitions` and similar functions cast `uint8*` directly to `sint16*` and `Pixel16*` (`m_transitions[from][to][k] = (Pixel16 *)(*dataPtr);`). These pointers may not be aligned to the type's requirements.
  *Fix: Use `memcpy` for each transition instead of direct pointer assignment, or ensure the source buffer is properly aligned.*


### UNINITIALIZED_VARIABLE

- **CityData.cpp:1045** — The `CityData(CivArchive &archive)` constructor only initializes `m_happy` and `m_sentInefficientMessageAlready` before calling `Serialize`. Many other member variables (e.g., `m_secthappy`, `m_bonusProd`, ring arrays) are not initialized; if `Serialize` skips fields due to version mismatches, they remain uninitialized.
  *Fix: Initialize all members in the constructor initializer list or call `Init()` before `Serialize`.*

- **aui_control.cpp:93** — The first `aui_Control` constructor (the one taking `MBCHAR *ldlBlock`) initializes `m_stringTable`, `m_allocatedTip`, etc., but omits `m_renderFlags` from its initializer list. The second constructor correctly initializes it to `k_AUI_CONTROL_LAYER_FLAG_ALWAYS`. Using the first constructor leaves `m_renderFlags` uninitialized, leading to unpredictable drawing behavior or crashes when `DrawThisStateImage` reads it.
  *Fix: Add `m_renderFlags(k_AUI_CONTROL_LAYER_FLAG_ALWAYS)` to the first constructor's initializer list.*

- **ctp2_code/gfx/gfx_utils/tiffutils.cpp:224** — `imageLength`, `imageWidth`, `RowsPerStrip`, and `PhotometricInterpretation` are read via `TIFFGetField` without checking return values, then used in arithmetic and loops.
  *Fix: Initialize all four to 0 and verify each `TIFFGetField` call succeeded.*

- **ctp2_code/gfx/gfx_utils/tiffutils.cpp:42** — `uint32 w, h;` are passed to `TIFFGetField`, but the return value is not checked. If the tags are missing, `w` and `h` remain uninitialized and are used immediately in `w * h`.
  *Fix: Initialize to zero and check `if (!TIFFGetField(...)) { TIFFClose(tif); return NULL; }`.*

- **diplomat.cpp:2557** — `Response response;` is declared in `ExecuteResponse` but never fully initialized before being used in some code paths (e.g., `response.id = GetNextId();` sets only one field).
  *Fix: Zero-initialize the struct: `Response response = {};`*

- **iparser.cpp:76** — `Report_Error` declares `va_list list` and calls `va_start(list, message)`, but the function is **not** variadic (no `...` parameter). This leaves `list` effectively uninitialized/garbage. The subsequent `vsprintf(buf, message, list)` reads undefined stack/register contents.
  *Fix: Either make the function variadic (`char *raw_message, int report_type, ...`) or remove the `va_list`/`vsprintf` logic entirely.*

- **proposalresponseevent.cpp:331-335** — `DiplomacyResult proposal_sender_result;` and `proposal_receiver_result;` are declared but not initialized before being passed to `ProposalAnalysis::ComputeResult`. While `ComputeResult` usually fills them, if the function returns early (e.g., NULL player), the results remain uninitialized and are later used in comparisons.
  *Fix: Zero-initialize both structs: `DiplomacyResult proposal_sender_result = {};`*

- **spritefile.cpp:2335** — `uint8 *hash[4096];` is declared on the stack but never initialized. The first time an index is hit, `p = hash[index]` reads garbage, leading to wild-pointer arithmetic.
  *Fix: Zero-initialize the table: `uint8 *hash[4096] = {};`.*

- **sstateevent.cpp:739-768** — `AiState state;` is declared and only `priority` and `dbIndex` are set in conditional branches. Other fields (spyStrId, adviceStrId, newsStrId) remain uninitialized if the branch is not taken, then the struct is passed to `ConsiderStrategicState`.
  *Fix: Initialize the entire `state` struct at declaration with `AiState state = {};` or set all fields in every branch.*

- **sstateevent.cpp:785-864** — Same `AiState state;` pattern in the `else` branch — fields other than `priority` and `dbIndex` are left uninitialized.
  *Fix: Zero-initialize the struct at declaration.*


## LOW Severity


### FLOAT_CAST_OVERFLOW

- **ArmyData.cpp:2147** — `sint32(chance * 100.0)` in `Franchise()` can overflow if `chance` is larger than expected.
  *Fix: Clamp the product to `sint32` limits before casting.*

- **ArmyData.cpp:2601** — `sint32(chance * 100.0)` in `CauseUnhappiness()` can overflow.
  *Fix: Clamp before casting.*

- **ArmyData.cpp:2774** — `sint32(escape_chance)` in `PlantNuke()` can overflow if `escape_chance` exceeds `sint32` max.
  *Fix: Clamp before casting.*

- **ArmyData.cpp:3036** — `sint32(success * 100.0)` in `SlaveRaid()` can overflow.
  *Fix: Clamp before casting.*

- **ArmyData.cpp:3077** — `sint32(death * 100.0)` in `SlaveRaid()` can overflow.
  *Fix: Clamp before casting.*

- **ArmyData.cpp:3394** — `sint32(success * 100.0)` in `UndergroundRailway()` can overflow.
  *Fix: Clamp before casting.*

- **ArmyData.cpp:3406** — `sint32(death * 100.0)` in `UndergroundRailway()` can overflow.
  *Fix: Clamp before casting.*

- **ArmyEvent.cpp:1095** — `sint32(g_theConstDB->Get(0)->GetCombatLeaderChance() * 100.0)` can overflow if the database value is unexpectedly large, producing undefined behavior when casting from double to sint32.
  *Fix: Clamp the double result to the `sint32` range before casting.*

- **ArmyEvent.cpp:1102** — `sint32(g_theConstDB->Get(0)->GetCombatEliteChance() * 100.0)` can overflow if the DB value is very large.
  *Fix: Clamp the value to the valid `sint32` range before casting.*

- **ArmyEvent.cpp:1109** — `sint32(g_theConstDB->Get(0)->GetCombatVeteranChance() * 100.0)` can overflow for large DB values.
  *Fix: Clamp before casting to `sint32`.*

- **ctp2_code/gfx/spritesys/UnitActor.cpp:986** — `(sint32)((k_ACTOR_CENTER_OFFSET_X - 48) * g_tiledMap->GetScale())` casts a `double` to `sint32` without checking range. If `GetScale()` is extremely large, the result is implementation-defined/undefined.
  *Fix: Clamp the double value to `INT_MIN..INT_MAX` before casting, or assert on reasonable scale values.*

- **nproposalevent.cpp:1012** — `sint32 target_pollution = static_cast<sint16>(reduce_percent * receiver_pollution);` — if `reduce_percent * receiver_pollution` exceeds sint16 range, the cast overflows.
  *Fix: Clamp the result to sint16 range before casting.*


### FORMAT_STRING_BUG

- **GWArchive.cpp:33** — `sprintf(exportName, "%s\\export%d", basePath, index)` into `exportName[1024]` can overflow if `basePath` is extremely long.
  *Fix: Use `snprintf(exportName, sizeof(exportName), "%s\\export%d", basePath, index)`.*


### INTEGER_OVERFLOW

- **GameEventManager.cpp:199** — `m_serial++` increments a `sint32` counter. After 2^31 events the counter overflows, which is undefined behavior for signed integers.
  *Fix: Use `uint32` for the serial number, or add an explicit wrap-around check.*


### MEMCPY_OVERLAP

- **Happy.cpp:928** — `Copy()` uses `memcpy(&m_happiness, &copy->m_happiness, ...)` to clone member data. If `this == copy` (self-copy), the source and destination overlap, violating `memcpy`'s non-overlap precondition.
  *Fix: Use `memmove` instead of `memcpy`, or add an early return when `this == copy`.*

- **ctp2_code/gs/fileio/GameFile.cpp:2072** — `SaveInfo::SaveInfo(SaveInfo *copyMe)` uses `memcpy(this, copyMe, sizeof(SaveInfo))` before re-allocating pointer members. While typically safe, if `copyMe` aliases `this` (which a copy constructor should prevent), it would be an overlap. More importantly, `memcpy` on non-trivially-copyable types with C++ objects is undefined behavior.
  *Fix: Replace `memcpy` with explicit member-by-member assignment or use `std::copy`/`operator=` for each field.*


### MISSING_BOUNDS_CHECK

- **aui_control.cpp:206** — `sprintf(stblock, "%s.%s", ldlBlock, k_AUI_CONTROL_LDL_STRINGTABLE)` writes into a fixed-size buffer `stblock[k_AUI_LDL_MAXBLOCK + 1]` without length checking. An overly long `ldlBlock` can overflow the stack buffer.
  *Fix: Use `snprintf(stblock, sizeof(stblock), "%s.%s", ldlBlock, ...)`.*

- **aui_listbox.cpp:230** — Multiple `sprintf` calls into `block[k_AUI_LDL_MAXBLOCK + 1]` (e.g., line 230, 257, 286) concatenate `ldlBlock` with suffixes without length checks, risking buffer overflow.
  *Fix: Replace all instances with `snprintf(block, sizeof(block), ...)`.*


### UNINITIALIZED_VARIABLE

- **SlicFunc.cpp:6800** — In `Slic_CityHasWonder::Call`, `wonder` is read via `args->GetInt(1, wonder)` but there is no explicit initialization of `wonder` before the call. If `GetInt` fails to set the variable and returns false (which is checked), this is safe, but defensive initialization would prevent issues if the control flow is later modified.
  *Fix: Initialize `sint32 wonder = -1;` at declaration.*
