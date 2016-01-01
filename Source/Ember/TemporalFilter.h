#pragma once

#include "EmberDefines.h"

/// <summary>
/// TemporalFilter base, derived and factory classes.
/// </summary>

namespace EmberNs
{
/// <summary>
/// The types of temporal filters available.
/// </summary>
enum class eTemporalFilterType : size_t
{
	BOX_TEMPORAL_FILTER,
	GAUSSIAN_TEMPORAL_FILTER,
	EXP_TEMPORAL_FILTER
};

/// <summary>
/// g++ needs a forward declaration here.
/// </summary>
template <typename T> class TemporalFilterCreator;

#define TEMPORALFILTERUSINGS \
	using TemporalFilter<T>::m_Filter; \
	using TemporalFilter<T>::m_FilterExp; \
	using TemporalFilter<T>::Size; \
	using TemporalFilter<T>::FinishFilter;

/// <summary>
/// Temporal filter is for doing motion blur while rendering a series of frames for animation.
/// The filter created is used as a vector of scalar values to multiply the time value by in between embers.
/// There are three possible types: Gaussian, Box and Exp.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API TemporalFilter
{
public:
	/// <summary>
	/// Constructor to set up basic filtering parameters, allocate buffers and calculate deltas.
	/// Derived class constructors will complete the final part of filter setup.
	/// </summary>
	/// <param name="filterType">Type of the filter.</param>
	/// <param name="temporalSamples">The number of temporal samples in the ember being rendered</param>
	/// <param name="filterWidth">The width of the filter.</param>
	TemporalFilter(eTemporalFilterType filterType, size_t temporalSamples, T filterWidth)
	{
		size_t i, steps = temporalSamples;
		m_TemporalSamples = temporalSamples;
		m_FilterWidth = filterWidth;
		m_Deltas.resize(steps);
		m_Filter.resize(steps);
		m_FilterType = filterType;
		m_FilterExp = 1;

		if (steps == 1)
		{
			m_SumFilt = 1;
			m_Deltas[0] = 0;
			m_Filter[0] = 1;
		}
		else
		{
			//Define the temporal deltas.
			for (i = 0; i < steps; i++)
				m_Deltas[i] = (T(i) / T(steps - 1) - T(0.5)) * filterWidth;
		}
	}

	/// <summary>
	/// Copy constructor.
	/// </summary>
	/// <param name="filter">The TemporalFilter object to copy</param>
	TemporalFilter(const TemporalFilter<T>& filter)
	{
		*this = filter;
	}

	/// <summary>
	/// Virtual destructor so derived class destructors get called.
	/// </summary>
	virtual ~TemporalFilter()
	{
	}

	/// <summary>
	/// Assignment operator.
	/// </summary>
	/// <param name="filter">The TemporalFilter object to copy.</param>
	/// <returns>Reference to updated self</returns>
	TemporalFilter<T>& operator = (const TemporalFilter<T>& filter)
	{
		if (this != &filter)
		{
			m_TemporalSamples = filter.m_TemporalSamples;
			m_FilterWidth = filter.m_FilterWidth;
			m_FilterExp = filter.m_FilterExp;
			m_SumFilt = filter.m_SumFilt;
			m_Deltas = filter.m_Deltas;
			m_Filter = filter.m_Filter;
			m_FilterType = filter.m_FilterType;
		}

		return *this;
	}

	/// <summary>
	/// Return a string representation of this filter.
	/// </summary>
	/// <returns>The string representation of this filter</returns>
	string ToString() const
	{
		size_t i;
		stringstream ss;
		ss  << "Temporal Filter:" << endl
			<< "	       Size: " << Size() << endl
			<< "           Type: " << TemporalFilterCreator<T>::ToString(m_FilterType) << endl
			<< "       Sum Filt: " << SumFilt() << endl;
		ss << "Deltas: " << endl;

		for (i = 0; i < m_Deltas.size(); i++)
		{
			ss << "Deltas[" << i << "]: " << m_Deltas[i] << endl;
		}

		ss << "Filter: " << endl;

		for (i = 0; i < m_Filter.size(); i++)
		{
			ss << "Filter[" << i << "]: " << m_Filter[i] << endl;
		}

		return ss.str();
	}

	/// <summary>
	/// Accessors.
	/// </summary>
	size_t Size() const { return m_Filter.size(); }
	size_t TemporalSamples() const { return m_TemporalSamples; }
	T FilterWidth() const { return m_FilterWidth; }
	T FilterExp() const { return m_FilterExp; }
	T SumFilt() const { return m_SumFilt; }
	T* Deltas() { return &m_Deltas[0]; }
	T* Filter() { return &m_Filter[0]; }
	eTemporalFilterType FilterType() const { return m_FilterType; }

protected:
	/// <summary>
	/// Normalize the filter and the sum filt.
	/// </summary>
	/// <param name="maxFilt">The maximum filter value contained in the filter vector after it was created</param>
	void FinishFilter(T maxFilt)
	{
		m_SumFilt = 0;

		for (size_t i = 0; i < Size(); i++)
		{
			m_Filter[i] /= maxFilt;
			m_SumFilt += m_Filter[i];
		}

		m_SumFilt /= Size();
	}

	T m_SumFilt;//The sum of all filter values.
	T m_FilterWidth;
	T m_FilterExp;
	size_t m_TemporalSamples;
	vector<T> m_Deltas;//Delta vector.
	vector<T> m_Filter;//Filter vector.
	eTemporalFilterType m_FilterType;//The type of filter this is.
};

/// <summary>
/// Derivation which implements the Exp filter.
/// </summary>
template <typename T>
class EMBER_API ExpTemporalFilter : public TemporalFilter<T>
{
	TEMPORALFILTERUSINGS
public:
	/// <summary>
	/// Constructor to create an Exp filter.
	/// </summary>
	/// <param name="temporalSamples">The number of temporal samples in the ember being rendered</param>
	/// <param name="filterWidth">The width of the filter.</param>
	/// <param name="filterExp">The filter exp.</param>
	ExpTemporalFilter(size_t temporalSamples, T filterWidth, T filterExp)
		: TemporalFilter<T>(eTemporalFilterType::BOX_TEMPORAL_FILTER, temporalSamples, filterWidth)
	{
		if (Size() > 1)
		{
			T slpx, maxFilt = 0;

			for (size_t i = 0; i < Size(); i++)
			{
				if (filterExp >= 0)
					slpx = (T(i) + 1) / Size();
				else
					slpx = T(Size() - i) / Size();

				//Scale the color based on these values.
				m_Filter[i] = std::pow(slpx, fabs(filterExp));

				//Keep the max.
				if (m_Filter[i] > maxFilt)
					maxFilt = m_Filter[i];
			}

			m_FilterExp = filterExp;
			FinishFilter(maxFilt);
		}
	}
};

/// <summary>
/// Derivation which implements the Gaussian filter.
/// </summary>
template <typename T>
class EMBER_API GaussianTemporalFilter : public TemporalFilter<T>
{
	TEMPORALFILTERUSINGS
public:
	/// <summary>
	/// Constructor to create a Gaussian filter.
	/// </summary>
	/// <param name="temporalSamples">The number of temporal samples in the ember being rendered</param>
	/// <param name="filterWidth">The width of the filter.</param>
	GaussianTemporalFilter(size_t temporalSamples, T filterWidth)
		: TemporalFilter<T>(eTemporalFilterType::GAUSSIAN_TEMPORAL_FILTER, temporalSamples, filterWidth)
	{
		if (Size() > 1)
		{
			T maxFilt = 0, halfSteps = T(Size()) / T(2);
			GaussianFilter<T> gaussian(1, 1);//Just pass dummy values, they are unused in this case.

			for (size_t i = 0; i < Size(); i++)
			{
				m_Filter[i] = gaussian.Filter(gaussian.Support() * fabs(i - halfSteps) / halfSteps);

				//Keep the max.
				if (m_Filter[i] > maxFilt)
					maxFilt = m_Filter[i];
			}

			FinishFilter(maxFilt);
		}
	}
};

/// <summary>
/// Derivation which implements the Box filter.
/// </summary>
template <typename T>
class EMBER_API BoxTemporalFilter : public TemporalFilter<T>
{
	TEMPORALFILTERUSINGS
public:
	/// <summary>
	/// Constructor to create a Box filter.
	/// </summary>
	/// <param name="temporalSamples">The number of temporal samples in the ember being rendered</param>
	/// <param name="filterWidth">The width of the filter.</param>
	BoxTemporalFilter(size_t temporalSamples, T filterWidth)
		: TemporalFilter<T>(eTemporalFilterType::BOX_TEMPORAL_FILTER, temporalSamples, filterWidth)
	{
		if (Size() > 1)
		{
			for (size_t i = 0; i < Size(); i++)
				m_Filter[i] = 1;

			FinishFilter(1);
		}
	}
};

/// <summary>
/// Convenience class to assist in converting between filter names and the filter objects themselves.
/// </summary>
template <typename T>
class EMBER_API TemporalFilterCreator
{
public:
	/// <summary>
	/// Creates the specified filter type based on the filterType enum parameter.
	/// </summary>
	/// <param name="filterType">Type of the filter</param>
	/// <param name="temporalSamples">The number of temporal samples in the ember being rendered</param>
	/// <param name="filterWidth">The width of the filter</param>
	/// <param name="filterExp">The filter exp, only used with Exp filter, otherwise ignored.</param>
	/// <returns>A pointer to the newly created filter object</returns>
	static TemporalFilter<T>* Create(eTemporalFilterType filterType, size_t temporalSamples, T filterWidth, T filterExp = 1)
	{
		TemporalFilter<T>* filter = nullptr;

		switch (filterType)
		{
			case EmberNs::eTemporalFilterType::BOX_TEMPORAL_FILTER:
				filter = new BoxTemporalFilter<T>(temporalSamples, filterWidth);
				break;

			case EmberNs::eTemporalFilterType::GAUSSIAN_TEMPORAL_FILTER:
				filter = new GaussianTemporalFilter<T>(temporalSamples, filterWidth);
				break;

			case EmberNs::eTemporalFilterType::EXP_TEMPORAL_FILTER:
				filter = new ExpTemporalFilter<T>(temporalSamples, filterWidth, filterExp);
				break;

			default:
				filter = new BoxTemporalFilter<T>(temporalSamples, filterWidth);//Default to box if bad enum passed in.
				break;
		}

		return filter;
	}

	/// <summary>
	/// Return a string vector of the available filter types.
	/// </summary>
	/// <returns>A vector of strings populated with the available filter types</returns>
	static vector<string> FilterTypes()
	{
		vector<string> v;
		v.reserve(3);
		v.push_back("Box");
		v.push_back("Gaussian");
		v.push_back("Exp");
		return v;
	}

	/// <summary>
	/// Convert between the filter name string and its type enum.
	/// </summary>
	/// <param name="filterType">The string name of the filter</param>
	/// <returns>The filter type enum</returns>
	static eTemporalFilterType FromString(const string& filterType)
	{
		if (!_stricmp(filterType.c_str(), "box"))
			return eTemporalFilterType::BOX_TEMPORAL_FILTER;
		else if (!_stricmp(filterType.c_str(), "gaussian"))
			return eTemporalFilterType::GAUSSIAN_TEMPORAL_FILTER;
		else if (!_stricmp(filterType.c_str(), "exp"))
			return eTemporalFilterType::EXP_TEMPORAL_FILTER;
		else
			return eTemporalFilterType::BOX_TEMPORAL_FILTER;
	}

	/// <summary>
	/// Convert between the filter type enum and its name string.
	/// </summary>
	/// <param name="eTemporalFilterType">The filter type enum</param>
	/// <returns>The string name of the filter</returns>
	static string ToString(eTemporalFilterType filterType)
	{
		string filter;

		switch (filterType)
		{
			case EmberNs::eTemporalFilterType::BOX_TEMPORAL_FILTER:
				filter = "Box";
				break;

			case EmberNs::eTemporalFilterType::GAUSSIAN_TEMPORAL_FILTER:
				filter = "Gaussian";
				break;

			case EmberNs::eTemporalFilterType::EXP_TEMPORAL_FILTER:
				filter = "Exp";
				break;

			default:
				filter = "Box";
				break;
		}

		return filter;
	}
};

/// <summary>
/// Thin wrapper around TemporalFilterCreator::ToString() to allow << operator on temporal filter type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const eTemporalFilterType& t)
{
	stream << TemporalFilterCreator<float>::ToString(t);
	return stream;
}
}
