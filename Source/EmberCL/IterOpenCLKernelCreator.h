#pragma once

#include "EmberCLPch.h"
#include "EmberCLStructs.h"
#include "EmberCLFunctions.h"

/// <summary>
/// IterOpenCLKernelCreator class.
/// </summary>

namespace EmberCLns
{
/// <summary>
/// Class for creating the main iteration code in OpenCL.
/// It uses the Cuburn method of iterating where all conditionals
/// are stripped out and a specific kernel is compiled at run-time.
/// It uses a very sophisticated method for randomization that avoids
/// the problem of warp/wavefront divergence that would occur if every
/// thread selected a random xform to apply.
/// This only works with embers of type float, double is not supported.
/// </summary>
template <typename T>
class EMBERCL_API IterOpenCLKernelCreator
{
public:
	IterOpenCLKernelCreator(bool nVidia);
	string ZeroizeKernel();
	string ZeroizeEntryPoint();
	string IterEntryPoint();
	string CreateIterKernelString(Ember<T>& ember, string& parVarDefines, bool lockAccum = false, bool doAccum = true);
	static void ParVarIndexDefines(Ember<T>& ember, pair<string, vector<T>>& params, bool doVals = true, bool doString = true);
	static bool IsBuildRequired(Ember<T>& ember1, Ember<T>& ember2);

private:
	string CreateZeroizeKernelString();
	string CreateProjectionString(Ember<T>& ember);

	string m_IterEntryPoint;
	string m_ZeroizeKernel;
	string m_ZeroizeEntryPoint;
	bool m_NVidia;
};

#ifdef OPEN_CL_TEST_AREA
typedef void (*KernelFuncPointer) (uint gridWidth, uint gridHeight, uint blockWidth, uint blockHeight,
								   uint BLOCK_ID_X, uint BLOCK_ID_Y, uint THREAD_ID_X, uint THREAD_ID_Y);

static void OpenCLSim(uint gridWidth, uint gridHeight, uint blockWidth, uint blockHeight, KernelFuncPointer func)
{
	cout << "OpenCLSim(): " << endl;
	cout << "	Params: " << endl;
	cout << "		gridW: " << gridWidth << endl;
	cout << "		gridH: " << gridHeight << endl;
	cout << "		blockW: " << blockWidth << endl;
	cout << "		blockH: " << blockHeight << endl;

	for (uint i = 0; i < gridHeight; i += blockHeight)
	{
		for (uint j = 0; j < gridWidth; j += blockWidth)
		{
			for (uint k = 0; k < blockHeight; k++)
			{
				for (uint l = 0; l < blockWidth; l++)
				{
					func(gridWidth, gridHeight, blockWidth, blockHeight, j / blockWidth, i / blockHeight, l, k);
				}
			}
		}
	}
}
#endif
}
