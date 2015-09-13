#pragma once

#include "EmberCLPch.h"
#include "OpenCLWrapper.h"
#include "IterOpenCLKernelCreator.h"

/// <summary>
/// RendererClDevice class.
/// </summary>

namespace EmberCLns
{
/// <summary>
/// Class to manage a device that does the iteration portion of
/// the rendering process. Having a separate class for this purpose
/// enables multi-GPU support.
/// </summary>
class EMBERCL_API RendererClDevice : public EmberReport
{
public:
	RendererClDevice(bool doublePrec, size_t platform, size_t device, bool shared);
	bool Init();
	bool Ok() const;
	bool Shared() const;
	bool Nvidia() const;
	size_t WarpSize() const;
	size_t PlatformIndex() const;
	size_t DeviceIndex() const;

	size_t m_Calls;
	OpenCLWrapper m_Wrapper;

private:
	bool m_Init;
	bool m_Shared;
	bool m_NVidia;
	size_t m_WarpSize;
	size_t m_PlatformIndex;
	size_t m_DeviceIndex;
	OpenCLInfo& m_Info;
};
}
