#pragma once

#include "EmberDefines.h"

namespace EmberNs
{

/// <summary>
/// FlameMotion elements allow for motion of the flame parameters such as zoom, yaw, pitch and friends
/// The values in these elements can be used to modify flame parameters during rotation in much the same
/// way as motion elements on xforms do.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API FlameMotion
{
public:

	FlameMotion()
	{
		m_MotionFreq = 0;
		m_MotionFunc = MOTION_SIN;
		m_MotionOffset = 0;
	}

	FlameMotion(const FlameMotion<T> &other)
	{
		operator=<T>(other);
	}

	template <typename U>
	FlameMotion(const FlameMotion<U> &other)
	{
		operator=<U>(other);
	}

	FlameMotion<T>& operator = (const FlameMotion<T>& other)
	{
		if (this != &other)
			FlameMotion<T>::operator=<T>(other);

		return *this;
	}

	template <typename U>
	FlameMotion &operator = (const FlameMotion<U> &other)
	{
		m_MotionParams.clear();

		for (int i = 0; i < other.m_MotionParams.size(); ++i)
			m_MotionParams.push_back(pair<eFlameMotionParam, T>(other.m_MotionParams[i].first, T(other.m_MotionParams[i].second)));

		m_MotionFunc = other.m_MotionFunc;
		m_MotionFreq = T(other.m_MotionFreq);
		m_MotionOffset = T(other.m_MotionOffset);
		return *this;
	}

	T m_MotionFreq;
	T m_MotionOffset;
	eMotion m_MotionFunc;
	vector<pair<eFlameMotionParam, T>> m_MotionParams;

};

}
