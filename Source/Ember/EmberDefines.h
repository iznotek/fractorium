#pragma once

#include "EmberPch.h"

/// <summary>
/// Basic #defines used throughout the library.
/// </summary>

#ifdef _WIN32
	#if defined(BUILDING_EMBER)
		#define EMBER_API __declspec(dllexport)
	#else
		#define EMBER_API __declspec(dllimport)
	#endif
#else
	#define EMBER_API
	#define fopen_s(pFile,filename,mode) ((*(pFile)=fopen((filename),(mode)))==nullptr)
	#define _stat stat
	#define _fstat fstat
	#define _stricmp strcmp
	#define sscanf_s sscanf
	#define sprintf_s snprintf
	#define snprintf_s snprintf
	typedef int errno_t;
#endif

#define RESTRICT __restrict//This might make things faster, unsure if it really does though.
//#define RESTRICT

//Wrap the sincos function for Macs and PC.
#if defined(__APPLE__) || defined(_MSC_VER)
#define sincos(x, s, c) *(s)=std::sin(x); *(c)=std::cos(x);
#else
static void sincos(float x, float* s, float* c)
{
	*s = std::sin(x);
	*c = std::cos(x);
}
#endif

namespace EmberNs
{
#define EMBER_VERSION "0.9.9.2"
#define EPS6 T(1e-6)
#define EPS std::numeric_limits<T>::epsilon()//Apoplugin.h uses -20, but it's more mathematically correct to do it this way.
#define ISAAC_SIZE 4
#define MEMALIGN 32
#define DE_THRESH 100
#define MAX_VARS_PER_XFORM 8
#define DEG_2_RAD (M_PI / 180)
#define RAD_2_DEG (180 / M_PI)
#define DEG_2_RAD_T (T(M_PI) / T(180))
#define RAD_2_DEG_T (T(180) / T(M_PI))
#define M_2PI (T(M_PI * 2))
#define M_3PI (T(M_PI * 3))
#define SQRT5 T(2.2360679774997896964091736687313)
#define M_PHI T(1.61803398874989484820458683436563)
#define COLORMAP_LENGTH 256//These will need to change if 2D palette support is ever added, or variable sized palettes.
#define COLORMAP_LENGTH_MINUS_1 255
#define WHITE 255
#define DEFAULT_SBS (1024 * 10)
//#define XC(c) ((const xmlChar*)(c))
#define XC(c) (reinterpret_cast<const xmlChar*>(c))
#define CX(c) (reinterpret_cast<char*>(c))
#define CCX(c) (reinterpret_cast<const char*>(c))
#define BadVal(x) (((x) != (x)) || ((x) > 1e10) || ((x) < -1e10))
#define Rint(A) floor((A) + (((A) < 0) ? T(-0.5) : T(0.5)))
#define Vlen(x) (sizeof(x) / sizeof(*x))
#define SQR(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define TLOW std::numeric_limits<T>::lowest()
#define TMAX std::numeric_limits<T>::max()
#define FLOAT_MAX_TAN 8388607.0f
#define FLOAT_MIN_TAN -FLOAT_MAX_TAN
#define EMPTYFIELD -9999
typedef std::chrono::high_resolution_clock Clock;

/// <summary>
/// Thin wrapper around getting the current time in milliseconds.
/// </summary>
static inline size_t NowMs()
{
	return duration_cast<milliseconds>(Clock::now().time_since_epoch()).count();
}

#ifndef byte
	typedef unsigned char byte;
#endif

#define DO_DOUBLE 1//Comment this out for shorter build times during development. Always uncomment for release.
//#define ISAAC_FLAM3_DEBUG 1//This is almost never needed, but is very useful when troubleshooting difficult bugs. Enable it to do a side by side comparison with flam3.

//These two must always match.
#ifdef WIN32
	#define ALIGN __declspec(align(16))
#else
	#define ALIGN __attribute__ ((aligned (16)))
#endif

#define ALIGN_CL "((aligned (16)))"//The extra parens are necessary.

#if GLM_VERSION >= 96
	#define v2T  glm::tvec2<T, glm::defaultp>
	#define v3T  glm::tvec3<T, glm::defaultp>
	#define v4T  glm::tvec4<T, glm::defaultp>
	#define v4bT glm::tvec4<bucketT, glm::defaultp>
	#define m2T  glm::tmat2x2<T, glm::defaultp>
	#define m3T  glm::tmat3x3<T, glm::defaultp>
	#define m4T  glm::tmat4x4<T, glm::defaultp>
	#define m23T glm::tmat2x3<T, glm::defaultp>
#else
	#define v2T  glm::detail::tvec2<T, glm::defaultp>
	#define v3T  glm::detail::tvec3<T, glm::defaultp>
	#define v4T  glm::detail::tvec4<T, glm::defaultp>
	#define v4bT glm::detail::tvec4<bucketT, glm::defaultp>
	#define m2T  glm::detail::tmat2x2<T, glm::defaultp>
	#define m3T  glm::detail::tmat3x3<T, glm::defaultp>
	#define m4T  glm::detail::tmat4x4<T, glm::defaultp>
	#define m23T glm::detail::tmat2x3<T, glm::defaultp>
#endif

enum class eInterp : uint { EMBER_INTERP_LINEAR = 0, EMBER_INTERP_SMOOTH = 1 };
enum class eAffineInterp : uint { AFFINE_INTERP_LINEAR = 0, AFFINE_INTERP_LOG = 1, AFFINE_INTERP_COMPAT = 2, AFFINE_INTERP_OLDER = 3 };
enum class ePaletteMode : uint { PALETTE_STEP = 0, PALETTE_LINEAR = 1 };
enum class ePaletteInterp : uint { INTERP_HSV = 0, INTERP_SWEEP = 1 };
enum class eMotion : uint { MOTION_SIN = 1, MOTION_TRIANGLE = 2, MOTION_HILL = 3, MOTION_SAW = 4 };
enum class eProcessAction : uint { NOTHING = 0, ACCUM_ONLY = 1, FILTER_AND_ACCUM = 2, KEEP_ITERATING = 3, FULL_RENDER = 4 };
enum class eProcessState : uint { NONE = 0, ITER_STARTED = 1, ITER_DONE = 2, FILTER_DONE = 3, ACCUM_DONE = 4 };
enum class eInteractiveFilter : uint { FILTER_LOG = 0, FILTER_DE = 1 };
enum class eScaleType : uint { SCALE_NONE = 0, SCALE_WIDTH = 1, SCALE_HEIGHT = 2 };
enum class eRenderStatus : uint { RENDER_OK = 0, RENDER_ERROR = 1, RENDER_ABORT = 2 };
enum class eEmberMotionParam : uint//These must remain in this order forever.
{
	FLAME_MOTION_NONE,
	FLAME_MOTION_ZOOM,
	FLAME_MOTION_ZPOS,
	FLAME_MOTION_PERSPECTIVE,
	FLAME_MOTION_YAW,
	FLAME_MOTION_PITCH,
	FLAME_MOTION_DEPTH_BLUR,
	FLAME_MOTION_CENTER_X,
	FLAME_MOTION_CENTER_Y,
	FLAME_MOTION_ROTATE,
	FLAME_MOTION_BRIGHTNESS,
	FLAME_MOTION_GAMMA,
	FLAME_MOTION_GAMMA_THRESH,
	FLAME_MOTION_HIGHLIGHT_POWER,
	FLAME_MOTION_BACKGROUND_R,
	FLAME_MOTION_BACKGROUND_G,
	FLAME_MOTION_BACKGROUND_B,
	FLAME_MOTION_VIBRANCY
};

/// <summary>
/// Thin wrapper to allow << operator on interp type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const eInterp& t)
{
	switch (t)
	{
		case EmberNs::eInterp::EMBER_INTERP_LINEAR:
			stream << "linear";
			break;

		case EmberNs::eInterp::EMBER_INTERP_SMOOTH:
			stream << "smooth";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}

/// <summary>
/// Thin wrapper to allow << operator on affine interp type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const eAffineInterp& t)
{
	switch (t)
	{
		case EmberNs::eAffineInterp::AFFINE_INTERP_LINEAR:
			stream << "linear";
			break;

		case EmberNs::eAffineInterp::AFFINE_INTERP_LOG:
			stream << "log";
			break;

		case EmberNs::eAffineInterp::AFFINE_INTERP_COMPAT:
			stream << "compat";
			break;

		case EmberNs::eAffineInterp::AFFINE_INTERP_OLDER:
			stream << "older";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}

/// <summary>
/// Thin wrapper to allow << operator on palette mode type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const ePaletteMode& t)
{
	switch (t)
	{
		case EmberNs::ePaletteMode::PALETTE_STEP:
			stream << "step";
			break;

		case EmberNs::ePaletteMode::PALETTE_LINEAR:
			stream << "linear";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}

/// <summary>
/// Thin wrapper to allow << operator on palette interp type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const ePaletteInterp& t)
{
	switch (t)
	{
		case EmberNs::ePaletteInterp::INTERP_HSV:
			stream << "hsv";
			break;

		case EmberNs::ePaletteInterp::INTERP_SWEEP:
			stream << "sweep";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}

/// <summary>
/// Thin wrapper to allow << operator on scale type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const eScaleType& t)
{
	switch (t)
	{
		case EmberNs::eScaleType::SCALE_NONE:
			stream << "none";
			break;

		case EmberNs::eScaleType::SCALE_WIDTH:
			stream << "width";
			break;

		case EmberNs::eScaleType::SCALE_HEIGHT:
			stream << "height";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}

/// <summary>
/// Thin wrapper to allow << operator on motion type.
/// </summary>
/// <param name="stream">The stream to insert into</param>
/// <param name="t">The type whose string representation will be inserted into the stream</param>
/// <returns></returns>
static std::ostream& operator<<(std::ostream& stream, const eMotion& t)
{
	switch (t)
	{
		case EmberNs::eMotion::MOTION_SIN:
			stream << "sin";
			break;

		case EmberNs::eMotion::MOTION_TRIANGLE:
			stream << "triangle";
			break;

		case EmberNs::eMotion::MOTION_HILL:
			stream << "hill";
			break;

		case EmberNs::eMotion::MOTION_SAW:
			stream << "saw";
			break;

		default:
			stream << "error";
			break;
	}

	return stream;
}
}
