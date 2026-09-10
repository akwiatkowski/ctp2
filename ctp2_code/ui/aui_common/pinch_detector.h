//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Two-finger pinch detector for an indirect touch device
//                (P11 pinch zoom, v1)
//
//----------------------------------------------------------------------------
//
// Pure geometry, no SDL types: the input layer feeds normalized [0..1]
// finger positions from the trackpad's touch events, and the detector emits
// whole zoom steps (+1 = fingers spread past the ratio threshold = zoom in,
// -1 = pinched together = zoom out).
//
// The hard part is what it must NOT do: a two-finger scroll also has two
// fingers down, so a naive distance tracker would zoom while the user pans.
// A scroll translates both fingers (the centroid moves, the spread stays
// ~constant); a pinch changes the spread faster than it moves the centroid.
// Steps therefore require BOTH a spread ratio past the threshold AND spread
// change dominating centroid drift since the gesture (re)baseline.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef PINCH_DETECTOR_H_
#define PINCH_DETECTOR_H_

#include <cmath>

class PinchDetector
{
public:
	// Feed one finger event; returns the zoom steps it triggered
	// (+1 zoom in / -1 zoom out, 0 otherwise). Ids are opaque; only the
	// first two concurrent fingers are tracked, later ones are ignored.
	int Down(long long id, float x, float y)
	{
		if (m_count < 2 && FindFinger(id) < 0)
		{
			m_id[m_count] = id;
			m_x[m_count] = x;
			m_y[m_count] = y;
			++m_count;
			if (m_count == 2)
				Rebase();
		}
		return 0;
	}

	int Motion(long long id, float x, float y)
	{
		int const i = FindFinger(id);
		if (i < 0)
			return 0;
		m_x[i] = x;
		m_y[i] = y;
		if (m_count < 2 || m_baseDist < k_MIN_BASE_DIST)
			return 0;

		float const dist = Spread();
		float const spreadChange = std::fabs(dist - m_baseDist);
		float const drift = CentroidDrift();

		// Scroll rejection: the spread change must dominate how far the
		// finger pair travelled as a whole since the (re)baseline.
		if (spreadChange < k_DRIFT_DOMINANCE * drift)
			return 0;

		float const ratio = dist / m_baseDist;
		if (ratio >= k_STEP_RATIO)
		{
			Rebase();
			return +1;
		}
		if (ratio <= 1.0f / k_STEP_RATIO)
		{
			Rebase();
			return -1;
		}
		return 0;
	}

	void Up(long long id)
	{
		int const i = FindFinger(id);
		if (i < 0)
			return;
		// Keep the surviving finger in slot 0; the pair (and its baseline)
		// is gone — a future second finger starts a fresh gesture.
		if (i == 0 && m_count == 2)
		{
			m_id[0] = m_id[1];
			m_x[0] = m_x[1];
			m_y[0] = m_y[1];
		}
		if (m_count > 0)
			--m_count;
	}

private:
	// One zoom step per 30% spread growth (or shrink to 1/1.3). A relaxed
	// trackpad pinch spans roughly 2.5x, so a full gesture yields 2-3 steps —
	// matching a couple of presses of the zoom keys.
	static constexpr float k_STEP_RATIO = 1.30f;
	// Fingers starting nearly together make the ratio explode from noise;
	// ignore gestures with a baseline under ~5% of the pad.
	static constexpr float k_MIN_BASE_DIST = 0.05f;
	// Spread change must exceed this multiple of centroid drift to count as
	// a pinch (scrolls move the centroid with a ~constant spread).
	static constexpr float k_DRIFT_DOMINANCE = 1.2f;

	int FindFinger(long long id) const
	{
		for (int i = 0; i < m_count; ++i)
			if (m_id[i] == id)
				return i;
		return -1;
	}

	float Spread() const
	{
		float const dx = m_x[0] - m_x[1];
		float const dy = m_y[0] - m_y[1];
		return std::sqrt(dx * dx + dy * dy);
	}

	float CentroidDrift() const
	{
		float const cx = (m_x[0] + m_x[1]) * 0.5f - m_baseCx;
		float const cy = (m_y[0] + m_y[1]) * 0.5f - m_baseCy;
		return std::sqrt(cx * cx + cy * cy);
	}

	void Rebase()
	{
		m_baseDist = Spread();
		m_baseCx = (m_x[0] + m_x[1]) * 0.5f;
		m_baseCy = (m_y[0] + m_y[1]) * 0.5f;
	}

	long long	m_id[2] = { 0, 0 };
	float		m_x[2] = { 0.0f, 0.0f };
	float		m_y[2] = { 0.0f, 0.0f };
	int		m_count = 0;
	float		m_baseDist = 0.0f;
	float		m_baseCx = 0.0f;
	float		m_baseCy = 0.0f;
};

#endif // PINCH_DETECTOR_H_
