#pragma once

#include "Isaac.h"

/// <summary>
/// Global utility classes and functions that don't really fit anywhere else, but are
/// too small to justify being in their own file.
/// </summary>
namespace EmberNs
{
#ifndef _WIN32
	#define THREAD_PRIORITY_LOWEST       1
	#define THREAD_PRIORITY_BELOW_NORMAL 25
	#define THREAD_PRIORITY_NORMAL       50
	#define THREAD_PRIORITY_ABOVE_NORMAL 75
	#define THREAD_PRIORITY_HIGHEST      99
#endif

/// <summary>
/// Enum to encapsulate and add type safety to the thread priority defines.
/// </summary>
enum eThreadPriority
{
	LOWEST       = THREAD_PRIORITY_LOWEST,//-2
	BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL,//-1
	NORMAL       = THREAD_PRIORITY_NORMAL,//0
	ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL,//1
	HIGHEST      = THREAD_PRIORITY_HIGHEST//2
};

/// <summary>
/// Thin wrapper around std::find_if() to relieve the caller of having to
/// pass the implicitly obvious .begin() and .end(), and then compare the results to .end().
/// </summary>
/// <param name="container">The container to call find_if() on</param>
/// <param name="pred">The lambda to call on each element</param>
/// <returns>True if pred returned true once, else false.</returns>
template<class c, class pr>
static inline bool FindIf(c& container, pr pred)
{
	return std::find_if(container.begin(), container.end(), pred) != container.end();
}

/// <summary>
/// Thin wrapper around std::find_if() determine if a value exists at least once.
/// </summary>
/// <param name="container">The container to call find_if() on</param>
/// <param name="val">The value to search for</param>
/// <returns>True if the value was contained at least once, else false.</returns>
template<class c, class T>
static inline bool Contains(c& container, const T& val)
{
	return std::find_if(container.begin(), container.end(), [&](const T & t) -> bool { return t == val; }) != container.end();
}

/// <summary>
/// Thin wrapper around computing the total size of a vector.
/// </summary>
/// <param name="vec">The vector to compute the size of</param>
/// <returns>The size of one element times the length.</returns>
template<typename T>
static inline size_t SizeOf(vector<T>& vec)
{
	return sizeof(vec[0]) * vec.size();
}

/// <summary>
/// After a run completes, information about what was run can be saved as strings to the comments
/// section of a jpg or png file. This class is just a container for those values.
/// </summary>
class EMBER_API EmberImageComments
{
public:
	/// <summary>
	/// Empty destructor.
	/// Needed to eliminate warnings about inlining.
	/// </summary>
	~EmberImageComments()
	{
	}

	/// <summary>
	/// Set all values to the empty string.
	/// </summary>
	void Clear()
	{
		m_Genome = "";
		m_Badvals = "";
		m_NumIters = "";
		m_Runtime = "";
	}

	string m_Genome;
	string m_Badvals;
	string m_NumIters;
	string m_Runtime;
};

/// <summary>
/// Since running is an incredibly complex process with multiple points of possible failure,
/// it's important that as much information as possible is captured if something goes wrong.
/// Classes wishing to capture this failure information will derive from this class and populate
/// the vector of strings with any useful error information. Note that a small complication can occur
/// when a class derives from this class, yet also has one or more members which do too. In that case, they should
/// override the methods to aggregate the error information from themselves, as well as their members.
/// </summary>
class EMBER_API EmberReport
{
public:
	/// <summary>
	/// Virtual destructor needed for virtual classes.
	/// </summary>
	virtual ~EmberReport() { }

	/// <summary>
	/// Write the entire error report as a single string to the console.
	/// Derived classes with members that also derive from EmberReport should override this to capture
	/// their error information as well as that of their members.
	/// </summary>
	virtual void DumpErrorReport() { cout << ErrorReportString(); }

	/// <summary>
	/// Clear the error report string vector.
	/// Derived classes with members that also derive from EmberReport should override this to clear
	/// their error information as well as that of their members.
	/// </summary>
	virtual void ClearErrorReport() { m_ErrorReport.clear(); }

	/// <summary>
	/// Return the entire error report as a single string.
	/// Derived classes with members that also derive from EmberReport should override this to capture
	/// their error information as well as that of their members.
	/// </summary>
	/// <returns>The entire error report as a single string. Empty if no errors.</returns>
	virtual string ErrorReportString() { return StaticErrorReportString(m_ErrorReport); }

	/// <summary>
	/// Return the entire error report as a vector of strings.
	/// Derived classes with members that also derive from EmberReport should override this to capture
	/// their error information as well as that of their members.
	/// </summary>
	/// <returns>The entire error report as a vector of strings. Empty if no errors.</returns>
	virtual vector<string> ErrorReport() { return m_ErrorReport; }

	/// <summary>
	/// Add string to report.
	/// </summary>
	/// <param name="s">The string to add</param>
	virtual void AddToReport(const string& s) { if (!Contains(m_ErrorReport, s)) m_ErrorReport.push_back(s); }

	/// <summary>
	/// Add a vector of strings to report.
	/// </summary>
	/// <param name="vec">The vector of strings to add</param>
	virtual void AddToReport(const vector<string>& vec) { for (auto& v : vec) AddToReport(v); }

	/// <summary>
	/// Static function to dump a vector of strings passed in.
	/// </summary>
	/// <param name="errorReport">The vector of strings to dump</param>
	static void StaticDumpErrorReport(const vector<string>& errorReport) { cout << StaticErrorReportString(errorReport); }

	/// <summary>
	/// Static function to return the entire error report passed in as a single string.
	/// </summary>
	/// <param name="errorReport">The vector of strings to concatenate</param>
	/// <returns>A string containing all strings in the vector passed in separated by newlines</returns>
	static string StaticErrorReportString(const vector<string>& errorReport)
	{
		stringstream ss;

		for (auto& s : errorReport) ss << s << endl;

		return ss.str();
	}

private:
	vector<string> m_ErrorReport;
};

/// <summary>
/// A base class for handling singletons that ensures only one instance exists, but
/// also deletes the instance after there are no more references to it.
/// This fixes the problem of the normal singleton pattern that uses a static function
/// variable. That pattern does not delete the instance until after main() exits
/// which can cause serious problems with certain libraries.
/// This class will delete before main exits.
/// Note that it still uses a local static variable because static templated
/// member variables cannot be exported across module boundaries.
/// Derived classes should inherit from this using the CRTP, and declare a friend to it.
/// They also should make their constructors private and destructors public.
/// Attribution: This class is a combination of
/// http://btorpey.github.io/blog/2014/02/12/shared-singletons/
/// and
/// http://enki-tech.blogspot.com/2012/08/c11-generic-singleton.html
/// </summary>
template <class T>
class Singleton
{
public:
	/// <summary>
	/// Create and return an instance of T.
	/// </summary>
	/// <param name="...args">The args to forward to the constructor of T</param>
	/// <returns>A shared_ptr<T></returns>
	template <typename... Args>
	static shared_ptr<T> Instance(Args... args)
	{
		static weak_ptr<T> staticInstance;
		auto temp = staticInstance.lock();

		if (!temp)
		{
			temp.reset(new T(std::forward<Args>(args)...));
			staticInstance = temp;
		}

		return temp;
	}
};

//Use this if the body of the destructor will be implemented in a cpp file.
#define SINGLETON_DERIVED_DECL(x) \
	friend class Singleton<x>; \
	public: \
	~x(); \
	\
	private: \
	x(const x& other) = delete; \
    const x& operator=(const x& other) = delete//Semicolon deliberately omitted to force it on the caller.

//Use this if the body of the destructor is empty and is will be implemented inline in the header file.
#define SINGLETON_DERIVED_IMPL(x) \
	friend class Singleton<x>; \
	public: \
	~x(){} \
	\
	private: \
	x(const x& other) = delete; \
    const x& operator=(const x& other) = delete

/// <summary>
/// Open a file in binary mode and read its entire contents into a vector of bytes. Optionally null terminate.
/// </summary>
/// <param name="filename">The full path to the file to read</param>
/// <param name="buf">The vector which will be populated with the file's contents</param>
/// <param name="nullTerminate">Whether to append a NULL character as the last element of the vector. Needed when reading text files. Default: true.</param>
/// <returns>True if successfully read and populated, else false</returns>
static bool ReadFile(const char* filename, string& buf, bool nullTerminate = true)
{
	bool b = false;
	FILE* f = nullptr;

	try
	{
		fopen_s(&f, filename, "rb");//Open in binary mode.

		if (f)
		{
			struct _stat statBuf;
#if defined(_WIN32) || defined(__APPLE__)
			int statResult = _fstat(f->_file, &statBuf);//Get data associated with file.
#else
			int statResult = _fstat(f->_fileno, &statBuf);//Get data associated with file.
#endif

			if (statResult == 0)//Check if statistics are valid.
			{
				buf.resize(statBuf.st_size + (nullTerminate ? 1 : 0));//Allocate vector to be the size of the entire file, with an optional additional character for nullptr.

				if (buf.size() == static_cast<size_t>(statBuf.st_size + 1))//Ensure allocation succeeded.
				{
					size_t bytesRead = fread(&buf[0], 1, statBuf.st_size, f);//Read the entire file at once.

					if (bytesRead == (static_cast<size_t>(statBuf.st_size)))//Ensure the number of bytes read matched what was requested.
					{
						if (nullTerminate)//Optionally nullptr terminate if they want to treat it as a string.
							buf[buf.size() - 1] = 0;

						b = true;//Success.
					}
				}
			}

			fclose(f);
			f = nullptr;
		}
	}
	catch (const std::exception& e)
	{
		cout << "Error: Reading file " << filename << " failed: " << e.what() << endl;
		b = false;
	}
	catch (...)
	{
		cout << "Error: Reading file " << filename << " failed." << endl;
		b = false;
	}

	if (f)
		fclose(f);

	return b;
}

/// <summary>
/// Clear dest and copy all of the elements of vector source with elements of type U to the vector
/// dest with elements of type T.
/// </summary>
/// <param name="dest">The vector of type T to copy to</param>
/// <param name="source">The vector of type U to copy from</param>
template <typename T, typename U>
static void CopyVec(vector<T>& dest, const vector<U>& source)
{
	dest.clear();
	dest.resize(source.size());

	for (size_t i = 0; i < source.size(); i++)
		dest[i] = static_cast<T>(source[i]);//Valid assignment operator between T and U types must be defined somewhere.
}

/// <summary>
/// Clear dest and copy all of the elements of vector source with elements of type U to the vector
/// dest with elements of type T.
/// Call a function on each element after it's been copied.
/// </summary>
/// <param name="dest">The vector of type T to copy to</param>
/// <param name="source">The vector of type U to copy from</param>
/// <param name="perElementOperation">A function to call on each element after it's copied</param>
template <typename T, typename U>
static void CopyVec(vector<T>& dest, const vector<U>& source, std::function<void(T& t)> perElementOperation)
{
	dest.clear();
	dest.resize(source.size());

	for (size_t i = 0; i < source.size(); i++)
	{
		dest[i] = static_cast<T>(source[i]);//Valid assignment operator between T and U types must be defined somewhere.
		perElementOperation(dest[i]);
	}
}

/// <summary>
/// Clear a vector of pointers to any type by checking each element for nullptr and calling delete on it, then clearing the entire vector.
/// Optionally call array delete if the elements themselves are pointers to dynamically allocated arrays.
/// </summary>
/// <param name="vec">The vector to be cleared</param>
/// <param name="arrayDelete">Whether to call delete or delete []. Default: false.</param>
template <typename T>
static void ClearVec(vector<T*>& vec, bool arrayDelete = false)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i])
		{
			if (arrayDelete)
				delete [] vec[i];
			else
				delete vec[i];
		}

		vec[i] = nullptr;
	}

	vec.clear();
}

/// <summary>
/// Determine whether all elements in two containers are equal.
/// </summary>
/// <param name="c1">The first collection to compare</param>
/// <param name="c2">The second collection to compare</param>
/// <returns>True if the sizes and all elements in both collections are equal, else false.</returns>
template <typename T>
static bool Equal(const T& c1, const T& c2)
{
	bool equal = c1.size() == c2.size();

	if (equal)
	{
		for (auto it1 = c1.begin(), it2 = c2.begin(); it1 != c1.end(); ++it1, ++it2)
		{
			if (*it1 != *it2)
			{
				equal = false;
				break;
			}
		}
	}

	return equal;
}

/// <summary>
/// Thin wrapper around passing a vector to memset() to relieve
/// the caller of having to pass the size.
/// </summary>
/// <param name="vec">The vector to memset</param>
/// <param name="val">The value to set each element to, default 0.</param>
template<typename T>
static inline void Memset(vector<T>& vec, int val = 0)
{
	memset(static_cast<void*>(vec.data()), val, SizeOf(vec));
}

/// <summary>
/// System floor() extremely slow because it accounts for various error conditions.
/// This is a much faster version that works on data that is not NaN.
/// </summary>
/// <param name="x">The value to return the floor of</param>
/// <returns>The floored value</returns>
template <typename T>
static inline intmax_t Floor(T val)
{
	if (val >= 0)
	{
		return static_cast<intmax_t>(val);
	}
	else
	{
		intmax_t i = static_cast<intmax_t>(val);//Truncate.
		return i - (i > val);//Convert trunc to floor.
	}
}

/// <summary>
/// Clamp and return a value to be greater than or equal to a specified minimum and less than
/// or equal to a specified maximum.
/// </summary>
/// <param name="val">The value to be clamped</param>
/// <param name="min">A value which the clamped value must be greater than or equal to</param>
/// <param name="max">A value which the clamped value must be less than or equal to</param>
/// <returns>The clamped value</returns>
template <typename T>
static inline T Clamp(T val, T min, T max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}

/// <summary>
/// Clamp and return a value to be greater than or equal to a specified minimum and less than
/// or equal to a specified maximum. If lesser, the value is fmod(val - min, max - min). If greater,
/// the value is max - fmod(max - val, max - min).
/// </summary>
/// <param name="val">The value to be clamped</param>
/// <param name="min">A value which the clamped value must be greater than or equal to</param>
/// <param name="max">A value which the clamped value must be less than or equal to</param>
/// <returns>The clamped and modded value</returns>
template <typename T>
static inline T ClampMod(T val, T min, T max)
{
	if (val < min)
		return min + fmod(val - min, max - min);
	else if (val > max)
		return max - fmod(max - val, max - min);
	else
		return val;
}

/// <summary>
/// Similar to Clamp(), but clamps a reference value in place rather than returning.
/// </summary>
/// <param name="val">The reference value to be clamped in place</param>
/// <param name="min">A value which the clamped value must be greater than or equal to</param>
/// <param name="max">A value which the clamped value must be less than or equal to</param>
template <typename T>
static inline void ClampRef(T& val, T min, T max)
{
	if (val < min)
		val = min;
	else if (val > max)
		val = max;
}

/// <summary>
/// Similar to Clamp(), but clamps a reference value in place rather than returning.
/// </summary>
/// <param name="val">The reference value to be clamped in place</param>
/// <param name="gte">A value which the clamped value must be less than or equal to</param>
template <typename T>
static inline void ClampLteRef(T& val, T lte)
{
	if (val > lte)
		val = lte;
}

/// <summary>
/// Clamp and return a value to be greater than or equal to a specified value.
/// Useful for ensuring something is not less than zero.
/// </summary>
/// <param name="val">The value to be clamped</param>
/// <param name="gte">A value which the clamped value must be greater than or equal to</param>
/// <returns>The clamped value</returns>
template <typename T>
static inline T ClampGte(T val, T gte)
{
	return (val < gte) ? gte : val;
}

/// <summary>
/// Similar to Clamp(), but clamps a reference value in place rather than returning.
/// </summary>
/// <param name="val">The reference value to be clamped in place</param>
/// <param name="gte">A value which the clamped value must be greater than or equal to</param>
template <typename T>
static inline void ClampGteRef(T& val, T gte)
{
	if (val < gte)
		val = gte;
}

/// <summary>
/// Thin wrapper around a call to ClampGte() with a gte value of zero.
/// </summary>
/// <param name="val">The value to be clamped</param>
/// <returns>The clamped value</returns>
template <typename T>
static inline T ClampGte0(T val)
{
	return ClampGte<T>(val, 0);
}

/// <summary>
/// Thin wrapper around a call to ClampGteRef() with a gte value of zero.
/// </summary>
/// <param name="val">The reference value to be clamped in place</param>
template <typename T>
static inline void ClampGte0Ref(T& val)
{
	ClampGteRef<T>(val, 0);
}

/// <summary>
/// Return a value rounded up or down. Works for positive and negative numbers.
/// </summary>
/// <param name="r">The value to round</param>
/// <returns>The rounded value</returns>
template <typename T>
static inline T Round(T r)
{
	return (r > 0) ? static_cast<T>(Floor<T>(r + T(0.5))) : ceil(r - T(0.5));
}

/// <summary>
/// Special rounding for certain variations, gotten from Apophysis.
/// </summary>
/// <param name="x">The value to round</param>
/// <returns>The rounded value</returns>
static inline float LRint(float x)
{
	int temp = (x >= 0 ? static_cast<int>(x + 0.5f) : static_cast<int>(x - 0.5f));
	return static_cast<float>(temp);
}

/// <summary>
/// Special rounding for certain variations, gotten from Apophysis.
/// </summary>
/// <param name="x">The value to round</param>
/// <returns>The rounded value</returns>
static inline double LRint(double x)
{
	glm::int64_t temp = (x >= 0 ? static_cast<int64_t>(x + 0.5) : static_cast<int64_t>(x - 0.5));
	return static_cast<double>(temp);
}

/// <summary>
/// Never really understood what this did.
/// </summary>
/// <param name="r">The value to round</param>
/// <returns>The rounded value</returns>
template <typename T>
static inline T Round6(T r)
{
	r *= 1e6;

	if (r < 0)
		r -= 1;

	return static_cast<T>(1e-6 * static_cast<int>(r + T(0.5)));
}

/// <summary>
/// Return -1 if the value is less than 0, 1 if it's greater and
/// 0 if it's equal to 0.
/// </summary>
/// <param name="v">The value to inspect</param>
/// <returns>-1, 0 or 1</returns>
template <typename T>
static inline T Sign(T v)
{
	return (v < 0) ? static_cast<T>(-1) : (v > 0) ? static_cast<T>(1) : static_cast<T>(0);
}

/// <summary>
/// Return -1 if the value is less than 0, 1 if it's greater.
/// This differs from Sign() in that it doesn't return 0.
/// </summary>
/// <param name="v">The value to inspect</param>
/// <returns>-1 or 1</returns>
template <typename T>
static inline T SignNz(T v)
{
	return (v < 0) ? static_cast<T>(-1) : static_cast<T>(1);
}

/// <summary>
/// Return the square of the passed in value.
/// This is useful when the value is a result of a computation
/// rather than a fixed number. Otherwise, use the SQR macro.
/// </summary>
/// <param name="v">The value to square</param>
/// <returns>The squared value</returns>
template <typename T>
static inline T Sqr(T t)
{
	return t * t;
}

/// <summary>
/// Taking the square root of numbers close to zero is dangerous.  If x is negative
/// due to floating point errors, it can return NaN results.
/// </summary>
template <typename T>
static inline T SafeSqrt(T x)
{
	if (x <= 0)
		return 0;

	return std::sqrt(x);
}

template <typename T>
static inline T SafeTan(T x)
{
	return x;
}

template <>
#ifdef _WIN32
	static
#endif
float SafeTan<float>(float x)
{
	return std::tan(Clamp<float>(x, FLOAT_MIN_TAN, FLOAT_MAX_TAN));
}

template <>
#ifdef _WIN32
	static
#endif
double SafeTan<double>(double x)
{
	return std::tan(x);
}

/// <summary>
/// Return the cube of the passed in value.
/// This is useful when the value is a result of a computation
/// rather than a fixed number. Otherwise, use the CUBE macro.
/// </summary>
/// <param name="v">The value to cube</param>
/// <returns>The cubed value</returns>
template <typename T>
static inline T Cube(T t)
{
	return t * t * t;
}

/// <summary>
/// Return the hypotenuse of the passed in values.
/// </summary>
/// <param name="x">The x distance</param>
/// <param name="y">The y distance</param>
/// <returns>The hypotenuse</returns>
template <typename T>
static inline T Hypot(T x, T y)
{
	return std::sqrt(SQR(x) + SQR(y));
}

/// <summary>
/// Spread the values.
/// </summary>
/// <param name="x">The x distance</param>
/// <param name="y">The y distance</param>
/// <returns>The spread</returns>
template <typename T>
static inline T Spread(T x, T y)
{
	return Hypot<T>(x, y) * ((x) > 0 ? 1 : -1);
}

/// <summary>
/// Unsure.
/// </summary>
/// <param name="x">The x distance</param>
/// <param name="y">The y distance</param>
/// <returns>The powq4</returns>
template <typename T>
static inline T Powq4(T x, T y)
{
	return std::pow(std::fabs(x), y) * SignNz(x);
}

/// <summary>
/// Unsure.
/// </summary>
/// <param name="x">The x distance</param>
/// <param name="y">The y distance</param>
/// <returns>The powq4c</returns>
template <typename T>
static inline T Powq4c(T x, T y)
{
	return y == 1 ? x : Powq4(x, y);
}

/// <summary>
/// Return EPS if the passed in value was zero, else return the value.
/// </summary>
/// <param name="x">The value</param>
/// <param name="y">The y distance</param>
/// <returns>EPS or the value if it was non-zero</returns>
template <typename T>
static inline T Zeps(T x)
{
	return x == 0 ? EPS : x;
}

/// <summary>
/// Interpolate a given percentage between two values.
/// </summary>
/// <param name="a">The first value to interpolate between.</param>
/// <param name="b">The secod value to interpolate between.</param>
/// <param name="p">The percentage between the two values to calculate.</param>
/// <returns>The interpolated value.</returns>
template <typename T>
static inline T Lerp(T a, T b, T p)
{
	return a + (b - a) * p;
}

/// <summary>
/// Thin wrapper around a call to modf that discards the integer portion
/// and returns the signed fractional portion.
/// </summary>
/// <param name="v">The value to retrieve the signed fractional portion of.</param>
/// <returns>The signed fractional portion of v.</returns>
template <typename T>
static inline T Fabsmod(T v)
{
	T dummy;
	return modf(v, &dummy);
}

/// <summary>
/// Unsure.
/// </summary>
/// <param name="p">Unsure.</param>
/// <param name="amp">Unsure.</param>
/// <param name="ph">Unsure.</param>
/// <returns>Unsure.</returns>
template <typename T>
static inline T Fosc(T p, T amp, T ph)
{
	return T(0.5) - std::cos(p * amp + ph) * T(0.5);
}

/// <summary>
/// Unsure.
/// </summary>
/// <param name="p">Unsure.</param>
/// <param name="ph">Unsure.</param>
/// <returns>Unsure.</returns>
template <typename T>
static inline T Foscn(T p, T ph)
{
	return T(0.5) - std::cos(p + ph) * T(0.5);
}

/// <summary>
/// Log scale from Apophysis.
/// </summary>
/// <param name="x">The value to log scale</param>
/// <returns>The log scaled value</returns>
template <typename T>
static inline T LogScale(T x)
{
	return x == 0 ? 0 : std::log((fabs(x) + 1) * T(M_E)) * SignNz(x) / T(M_E);
}

/// <summary>
/// Log map from Apophysis.
/// </summary>
/// <param name="x">The value to log map</param>
/// <returns>The log mapped value</returns>
template <typename T>
static inline T LogMap(T x)
{
	return x == 0 ? 0 : (T(M_E) + std::log(x * T(M_E))) * T(0.25) * SignNz(x);
}

/// <summary>
/// Thin wrapper around calling xmlStrcmp() on an Xml tag to tell
/// if its name is a given value.
/// </summary>
/// <param name="name">The name of the tag of the to inspect</param>
/// <param name="val">The value compare against</param>
/// <returns>True if the comparison matched, else false</returns>
static inline bool Compare(const xmlChar* name, const char* val)
{
	return xmlStrcmp(name, XC(val)) != 0;
}

/// <summary>
/// Determine whether the specified value is very close to zero.
/// This is useful for determining equality of float/double types.
/// </summary>
/// <param name="val">The value to compare against</param>
/// <param name="tolerance">The tolerance. Default: 1e-6.</param>
/// <returns>True if the value was very close to zero, else false</returns>
template <typename T>
static inline bool IsNearZero(T val, T tolerance = 1e-6)
{
	return (val > -tolerance && val < tolerance);
}

/// <summary>
/// Determine whether a specified value is very close to another value.
/// This is useful for determining equality of float/double types.
/// </summary>
/// <param name="val1">The first value.</param>
/// <param name="val2">The second value.</param>
/// <param name="tolerance">The tolerance. Default: 1e-6.</param>
/// <returns>True if the values were very close to each other, else false</returns>
template <typename T>
static inline bool IsClose(T val1, T val2, T tolerance = 1e-6)
{
	return IsNearZero(val1 - val2, tolerance);
}

/// <summary>
/// Put an angular measurement in degrees into the range of -180 - 180.
/// </summary>
/// <param name="angle">The angle to normalize</param>
/// <returns>The normalized angle in a range of -180 - 180</returns>
template <typename T>
static inline T NormalizeDeg180(T angle)
{
	angle = fmod(angle, 360);

	if (angle > 180)
	{
		angle -= 360;
	}
	else if (angle < -180)
	{
		angle += 360;
	}

	return angle;
}

/// <summary>
/// Put an angular measurement in degrees into the range of 0 - 360.
/// </summary>
/// <param name="angle">The angle to normalize</param>
/// <returns>The normalized angle in a range of 0 - 360</returns>
template <typename T>
static inline T NormalizeDeg360(T angle)
{
	if (angle > 360 || angle < -360)
		angle = fmod(angle, 360);

	if (angle < 0)
		angle += 360;

	return angle;
}

/// <summary>
/// Return a lower case copy of a string.
/// </summary>
/// <param name="str">The string to copy and make lower case</param>
/// <returns>The lower case string</returns>
static string ToLower(const string& str)
{
	string lower;
	lower.resize(str.size());//Allocate the destination space.
	std::transform(str.begin(), str.end(), lower.begin(), ::tolower);//Convert the source string to lower case storing the result in the destination string.
	return lower;
}

/// <summary>
/// Return an upper case copy of a string.
/// </summary>
/// <param name="str">The string to copy and make upper case</param>
/// <returns>The upper case string</returns>
static string ToUpper(const string& str)
{
	string upper;
	upper.resize(str.size());//Allocate the destination space.
	std::transform(str.begin(), str.end(), upper.begin(), ::toupper);//Convert the source string to lower case storing the result in the destination string.
	return upper;
}

/// <summary>
/// Return a copy of a string with leading and trailing occurrences of a specified character removed.
/// The default character is a space.
/// </summary>
/// <param name="str">The string to trim</param>
/// <param name="ch">The character to trim. Default: space.</param>
/// <returns>The trimmed string</returns>
static string Trim(const string& str, char ch = ' ')
{
	string ret;

	if (str != "")
	{
		size_t firstChar = str.find_first_not_of(ch);
		size_t lastChar = str.find_last_not_of(ch);

		if (firstChar == string::npos)
			firstChar = 0;

		if (lastChar == string::npos)
			lastChar = str.size();

		ret = str.substr(firstChar, lastChar - firstChar + 1);
	}

	return ret;
}

/// <summary>
/// Return a copy of a file path string with the path portion removed.
/// </summary>
/// <param name="filename">The string to retrieve the path from</param>
/// <returns>The path portion of the string</returns>
static string GetPath(const string& filename)
{
	string s;
	const size_t lastSlash = filename.find_last_of("\\/");

	if (std::string::npos != lastSlash)
		s = filename.substr(0, lastSlash + 1);
	else
		s = "";

	return s;
}

/// <summary>
/// Placeholder for a templated function to query the value of a specified system environment variable
/// of a specific type. This function does nothing as the functions for specific types implement the behavior
/// via template specialization.
/// </summary>
/// <param name="name">The name of the environment variable to query</param>
/// <param name="def">The default value to return if the environment variable was not present</param>
/// <returns>The value of the specified environment variable if found, else default</returns>
template <typename T>
static inline T Arg(char* name, T def)
{
	char* ch;
	T returnVal;
#ifdef WIN32
	size_t len;
	errno_t err = _dupenv_s(&ch, &len, name);
#else
	int err = 1;
	ch = getenv(name);
#endif

	if (err || !ch)
		returnVal = def;
	else
	{
		T tempVal;
		istringstream istr(ch);
		istr >> tempVal;

		if (!istr.bad() && !istr.fail())
			returnVal = tempVal;
		else
			returnVal = def;
	}

#ifdef WIN32
	free(ch);
#endif
	return returnVal;
}

/// <summary>
/// Template specialization for Arg<>() with a type of bool.
/// </summary>
/// <param name="name">The name of the environment variable to query</param>
/// <param name="def">The default value to return if the environment variable was not present</param>
/// <returns>The value of the specified environment variable if found, else default</returns>
template <>
#ifdef _WIN32
	static
#endif
bool Arg<bool>(char* name, bool def)
{
	return (Arg<int>(name, -999) != -999) ? true : def;
}

/// <summary>
/// Template specialization for Arg<>() with a type of string.
/// </summary>
/// <param name="name">The name of the environment variable to query</param>
/// <param name="def">The default value to return if the environment variable was not present</param>
/// <returns>The value of the specified environment variable if found, else default</returns>
template <>
#ifdef _WIN32
	static
#endif
string Arg<string>(char* name, string def)
{
	char* ch;
	string returnVal;
#ifdef WIN32
	size_t len;
	errno_t err = _dupenv_s(&ch, &len, name);
#else
	int err = 1;
	ch = getenv(name);
#endif

	if (err || !ch)
	{
		if (def != "")
			returnVal = def;
	}
	else
		returnVal = string(ch);

#ifdef WIN32
	free(ch);
#endif
	return returnVal;
}

/// <summary>
/// Replaces all instances of a value within a collection, with the specified value.
/// Taken from a StackOverflow.com post.
/// Modified to account for the scenario where the find and replace strings each start with
/// the same character.
/// Template argument should be any STL container.
/// </summary>
/// <param name="source">Collection to replace values in</param>
/// <param name="find">The value to replace</param>
/// <param name="replace">The value to replace with</param>
/// <returns>The number of instances replaced</returns>
template<typename T>
static uint FindAndReplace(T& source, const T& find, const T& replace)
{
	uint replaceCount = 0;
	typename T::size_type fLen = find.size();
	typename T::size_type rLen = replace.size();

	for (typename T::size_type pos = 0; (pos = source.find(find, pos)) != T::npos; pos += rLen)
	{
		typename T::size_type pos2 = source.find(replace, pos);

		if (pos != pos2)
		{
			replaceCount++;
			source.replace(pos, fLen, replace);
		}
	}

	return replaceCount;
}

/// <summary>
/// Split a string into tokens and place them in a vector.
/// </summary>
/// <param name="str">The string to split</param>
/// <param name="del">The delimiter to split the string on</param>
/// <returns>The split strings, each as an element in a vector.</returns>
static vector<string> Split(const string& str, char del)
{
	string tok;
	vector<string> vec;
	stringstream ss(str);

	while (getline(ss, tok, del))
		vec.push_back(tok);

	return vec;
}

/// <summary>
/// Return a character pointer to a version string composed of the EMBER_OS and EMBER_VERSION values.
/// </summary>
static inline const char* EmberVersion()
{
	return EMBER_OS "-" EMBER_VERSION;
}
}
