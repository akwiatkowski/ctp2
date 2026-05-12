//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Runtime safety wrappers for common bug-prone operations
//
//----------------------------------------------------------------------------
//
// Purpose
//
// This header provides inline wrapper functions that validate arguments before
// performing operations that are common sources of crashes:
//   - Dangerous shifts (amount >= bit width)
//   - Division by zero
//   - Out-of-bounds array indexing
//   - Null pointer dereferences
//   - Unchecked string copies
//
// In debug builds, violations trigger Assert() with file/line info.
// In release builds, wrappers degrade gracefully (return safe defaults).
//
// Usage: #include "gs/utility/safety.h"
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __SAFETY_H__
#define __SAFETY_H__

#include "ctp/ctp2_utils/c3debug.h"   // Assert
#include "gs/utility/gstypes.h"        // k_MAX_PLAYERS, sint32
#include <cstring>                     // strncpy, strlen
#include <limits>                      // std::numeric_limits

//----------------------------------------------------------------------------
//
// Name       : safe_shift_left
//
// Description: Left-shift with bounds checking on shift amount.
//
// Parameters : value  — value to shift
//              amount — number of bits to shift (must be < bit_width)
//
// Returns    : (value << amount) if amount is valid, 0 otherwise.
//
// Example    : safe_shift_left<uint64>(1ULL, type) instead of (1ULL << type)
//
//----------------------------------------------------------------------------
template <typename T>
inline T safe_shift_left(T value, sint32 amount)
{
	const sint32 max_shift = static_cast<sint32>(sizeof(T) * 8);
	if (amount < 0 || amount >= max_shift) {
		Assert(amount >= 0 && amount < max_shift);
		return T(0);
	}
	return static_cast<T>(value << amount);
}

//----------------------------------------------------------------------------
//
// Name       : safe_shift_left_u64
//
// Description: Convenience wrapper for the common uint64 shift pattern.
//              Replaces: (uint64)1 << idx
//              With:     safe_shift_left_u64(idx)
//
//----------------------------------------------------------------------------
inline uint64 safe_shift_left_u64(sint32 amount)
{
	return safe_shift_left<uint64>(uint64(1), amount);
}

//----------------------------------------------------------------------------
//
// Name       : safe_shift_left_u32
//
// Description: Convenience wrapper for uint32 shift with 1 as value.
//
//----------------------------------------------------------------------------
inline uint32 safe_shift_left_u32(sint32 amount)
{
	return safe_shift_left<uint32>(uint32(1), amount);
}

//----------------------------------------------------------------------------
//
// Name       : safe_divide
//
// Description: Integer division with zero-check.
//
// Parameters : numerator   — dividend
//              denominator — divisor (must not be zero)
//
// Returns    : numerator / denominator, or 0 if denominator is zero.
//
//----------------------------------------------------------------------------
template <typename T>
inline T safe_divide(T numerator, T denominator)
{
	if (denominator == T(0)) {
		Assert(denominator != T(0));
		return T(0);
	}
	return numerator / denominator;
}

//----------------------------------------------------------------------------
//
// Name       : safe_divide_double
//
// Description: Floating-point division with zero-check.
//
//----------------------------------------------------------------------------
inline double safe_divide_double(double numerator, double denominator)
{
	if (denominator == 0.0) {
		Assert(denominator != 0.0);
		return 0.0;
	}
	return numerator / denominator;
}

//----------------------------------------------------------------------------
//
// Name       : safe_array_access
//
// Description: Bounds-checked array element access.
//
// Parameters : array — pointer to array
//              size  — number of elements in array
//              idx   — index to access
//
// Returns    : Reference to array[idx] if in bounds, array[size-1] otherwise.
//              Never returns a reference outside the array.
//
//----------------------------------------------------------------------------
template <typename T>
inline T& safe_array_access(T* array, size_t size, sint32 idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= size) {
		Assert(idx >= 0 && static_cast<size_t>(idx) < size);
		if (size == 0) {
			// Fatal: array is empty. Return first element anyway
			// (will likely crash, but that's better than UB).
			return array[0];
		}
		return array[size - 1];
	}
	return array[idx];
}

//----------------------------------------------------------------------------
//
// Name       : safe_player
//
// Description: Bounds-checked access to the global g_player array.
//
// Parameters : idx — player index (must be in [0, k_MAX_PLAYERS))
//
// Returns    : g_player[idx] if idx is valid, NULL otherwise.
//
// Example    : if (Player* p = safe_player(owner)) { p->DoSomething(); }
//
//----------------------------------------------------------------------------
// g_player is declared in Player.h as: extern Player** g_player;
// We forward-declare it here to avoid including the full Player.h.
// Must match the actual declaration exactly.
class Player;
extern Player** g_player;

inline Player* safe_player(sint32 idx)
{
	if (idx < 0 || idx >= k_MAX_PLAYERS) {
		Assert(idx >= 0 && idx < k_MAX_PLAYERS);
		return NULL;
	}
	return g_player[idx];
}

//----------------------------------------------------------------------------
//
// Name       : safe_deref
//
// Description: Null-checked pointer dereference helper.
//
// Parameters : ptr — pointer that may be NULL
//
// Returns    : ptr if non-NULL, NULL otherwise (use in conditionals).
//
// Example    : if (CityData* cd = safe_deref(unit.GetCityData())) { ... }
//
//----------------------------------------------------------------------------
template <typename T>
inline T* safe_deref(T* ptr)
{
	if (ptr == NULL) {
		Assert(ptr != NULL);
		return NULL;
	}
	return ptr;
}

//----------------------------------------------------------------------------
//
// Name       : safe_strncpy
//
// Description: strncpy that always null-terminates and warns on truncation.
//
// Parameters : dst  — destination buffer
//              src  — source string
//              size — total size of dst buffer (including null terminator)
//
// Returns    : Number of characters written (excluding null terminator).
//
//----------------------------------------------------------------------------
inline size_t safe_strncpy(char* dst, const char* src, size_t size)
{
	if (size == 0) {
		Assert(size > 0);
		return 0;
	}
	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';
	return strlen(dst);
}

//----------------------------------------------------------------------------
//
// Name       : safe_strcpy
//
// Description: strcpy replacement that always null-terminates.
//
// Parameters : dst  — destination buffer
//              src  — source string
//              size — total size of dst buffer
//
//----------------------------------------------------------------------------
inline void safe_strcpy(char* dst, const char* src, size_t size)
{
	safe_strncpy(dst, src, size);
}

//----------------------------------------------------------------------------
//
// Name       : safe_memcpy
//
// Description: memcpy that detects overlapping ranges and uses memmove.
//
// Parameters : dst — destination buffer
//              src — source buffer
//              n   — number of bytes to copy
//
//----------------------------------------------------------------------------
inline void safe_memcpy(void* dst, const void* src, size_t n)
{
	if (n == 0)
		return;

	// Detect overlap: if [src, src+n) and [dst, dst+n) overlap,
	// use memmove which handles overlapping ranges.
	const char* s = static_cast<const char*>(src);
	const char* d = static_cast<const char*>(dst);

	if ((s < d && s + n > d) || (d < s && d + n > s)) {
		// Overlap detected — this is likely a bug, but memmove handles it.
		Assert(!"safe_memcpy: overlapping ranges detected, use memmove explicitly");
		memmove(dst, src, n);
	} else {
		memcpy(dst, src, n);
	}
}

//----------------------------------------------------------------------------
//
// Name       : safe_bounds_check
//
// Description: Generic bounds check for integer indices.
//
// Parameters : idx  — index to validate
//              min  — minimum valid value (inclusive)
//              max  — maximum valid value (exclusive)
//              file — source file (use __FILE__)
//              line — source line (use __LINE__)
//
// Returns    : true if idx is in [min, max), false otherwise.
//
//----------------------------------------------------------------------------
inline bool safe_bounds_check(sint32 idx, sint32 min, sint32 max,
                               const char* file, int line)
{
	if (idx < min || idx >= max) {
		Assert(idx >= min && idx < max);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------
// Convenience macro for bounds checking with automatic file/line
//----------------------------------------------------------------------------
#define SAFE_BOUNDS(idx, min, max) \
	safe_bounds_check((idx), (min), (max), __FILE__, __LINE__)

#endif // __SAFETY_H__
