#ifdef WIN32
	#pragma once
#endif

/// <summary>
/// Precompiled header file. Place all system includes here with appropriate #defines for different operating systems and compilers.
/// </summary>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN//Exclude rarely-used stuff from Windows headers.
#define _USE_MATH_DEFINES

#include "Timing.h"
#include "Renderer.h"

#if defined(_WIN32)
	#include <windows.h>
	#include <SDKDDKVer.h>
	#include "GL/gl.h"
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
#else
	#include "GL/glx.h"
#endif

#include <utility>
#include <CL/cl.hpp>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <iterator>
#include <time.h>
#include <unordered_map>

#ifdef _WIN32
	#if defined(BUILDING_EMBERCL)
		#define EMBERCL_API __declspec(dllexport)
	#else
		#define EMBERCL_API __declspec(dllimport)
	#endif
#else
	#define EMBERCL_API
#endif

using namespace std;
using namespace EmberNs;
//#define TEST_CL 1
//#define TEST_CL_BUFFERS 1
