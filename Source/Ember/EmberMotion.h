#pragma once

#include "EmberDefines.h"

namespace EmberNs
{
/// <summary>
/// Thin derivation of pair<eEmberMotionParam, T> for the element type
/// of EmberMotion<T>::m_MotionParams to allow for copying vectors
/// of different types of T.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API MotionParam : public pair <eEmberMotionParam, T>
{
public:
	/// <summary>
	/// Default constructor, which calls the base, which sets first and second to their defaults.
	/// </summary>
	MotionParam()
	{
	}

	/// <summary>
	/// Member-wise constructor.
	/// </summary>
	/// <param name="e">The eEmberMotionParam value to assign to first</param>
	/// <param name="t">The T value to assign to second</param>
	MotionParam(eEmberMotionParam e, T t)
		: pair<eEmberMotionParam, T>(e, t)
	{
	}

	/// <summary>
	/// Default copy constructor.
	/// </summary>
	/// <param name="other">The MotionParam object to copy</param>
	MotionParam(const MotionParam<T>& other)
		: pair <eEmberMotionParam, T>()
	{
		operator=<T>(other);
	}

	/// <summary>
	/// Copy constructor to copy a MotionParam object of type U.
	/// </summary>
	/// <param name="other">The MotionParam object to copy</param>
	template <typename U>
	MotionParam(const MotionParam<U>& other)
	{
		operator=<U>(other);
	}

	/// <summary>
	/// Default assignment operator.
	/// </summary>
	/// <param name="other">The MotionParam object to copy</param>
	MotionParam<T>& operator = (const MotionParam<T>& other)
	{
		if (this != &other)
			MotionParam<T>::operator=<T>(other);

		return *this;
	}

	/// <summary>
	/// Assignment operator to assign a MotionParam object of type U.
	/// </summary>
	/// <param name="other">The MotionParam object to copy.</param>
	/// <returns>Reference to updated self</returns>
	template <typename U>
	MotionParam &operator = (const MotionParam<U>& other)
	{
		this->first = other.first;
		this->second = T(other.second);

		return *this;
	}
};

/// <summary>
/// EmberMotion elements allow for motion of the flame parameters such as zoom, yaw, pitch and friends
/// The values in these elements can be used to modify flame parameters during rotation in much the same
/// way as motion elements on xforms do.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API EmberMotion
{
public:
	/// <summary>
	/// Default constructor to initialize motion freq and offset to 0 and the motion func to SIN.
	/// </summary>
	EmberMotion()
	{
		m_MotionFreq = 0;
		m_MotionFunc = MOTION_SIN;
		m_MotionOffset = 0;
	}

	/// <summary>
	/// Default copy constructor.
	/// </summary>
	/// <param name="other">The EmberMotion object to copy</param>
	EmberMotion(const EmberMotion<T>& other)
	{
		operator=<T>(other);
	}

	/// <summary>
	/// Copy constructor to copy a EmberMotion object of type U.
	/// </summary>
	/// <param name="other">The EmberMotion object to copy</param>
	template <typename U>
	EmberMotion(const EmberMotion<U>& other)
	{
		operator=<U>(other);
	}

	/// <summary>
	/// Default assignment operator.
	/// </summary>
	/// <param name="other">The EmberMotion object to copy</param>
	EmberMotion<T>& operator = (const EmberMotion<T>& other)
	{
		if (this != &other)
			EmberMotion<T>::operator=<T>(other);

		return *this;
	}

	/// <summary>
	/// Assignment operator to assign a EmberMotion object of type U.
	/// </summary>
	/// <param name="other">The EmberMotion object to copy.</param>
	/// <returns>Reference to updated self</returns>
	template <typename U>
	EmberMotion &operator = (const EmberMotion<U>& other)
	{
		CopyVec(m_MotionParams, other.m_MotionParams);
		m_MotionFunc = other.m_MotionFunc;
		m_MotionFreq = T(other.m_MotionFreq);
		m_MotionOffset = T(other.m_MotionOffset);
		return *this;
	}

	T m_MotionFreq;
	T m_MotionOffset;
	eMotion m_MotionFunc;
	vector<MotionParam<T>> m_MotionParams;
};
}
