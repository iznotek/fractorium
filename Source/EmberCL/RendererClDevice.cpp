#include "EmberCLPch.h"
#include "RendererClDevice.h"

namespace EmberCLns
{
/// <summary>
/// Constructor that assigns members.
/// The object is not fully initialized at this point, the caller
/// must manually call Init().
/// </summary>
/// <param name="platform">The index of the platform to use</param>
/// <param name="device">The index device of the device to use</param>
/// <param name="shared">True if shared with OpenGL, else false.</param>
/// <returns>True if success, else false.</returns>
RendererClDevice::RendererClDevice(bool doublePrec, size_t platform, size_t device, bool shared)
{
	m_Init = false;
	m_Shared = shared;
	m_NVidia = false;
	m_WarpSize = 0;
	m_Calls = 0;
	m_PlatformIndex = platform;
	m_DeviceIndex = device;
	m_Info = OpenCLInfo::Instance();
}

/// <summary>
/// Initialization of the OpenCLWrapper member.
/// </summary>
/// <returns>True if success, else false.</returns>
bool RendererClDevice::Init()
{
	bool b = true;

	if (!m_Wrapper.Ok())
	{
		m_Init = false;
		b = m_Wrapper.Init(m_PlatformIndex, m_DeviceIndex, m_Shared);
	}

	if (b && m_Wrapper.Ok() && !m_Init)
	{
		m_NVidia = ToLower(m_Info->PlatformName(m_PlatformIndex)).find_first_of("nvidia") != string::npos && m_Wrapper.LocalMemSize() > (32 * 1024);
		m_WarpSize = m_NVidia ? 32 : 64;
		m_Init = true;
	}

	return b;
}

/// <summary>
/// OpenCL property accessors, getters only.
/// </summary>
bool RendererClDevice::Ok() const { return m_Init; }
bool RendererClDevice::Shared() const { return m_Shared; }
bool RendererClDevice::Nvidia() const { return m_NVidia; }
size_t RendererClDevice::WarpSize() const { return m_WarpSize; }
size_t RendererClDevice::PlatformIndex() const { return m_PlatformIndex; }
size_t RendererClDevice::DeviceIndex() const { return m_DeviceIndex; }
}
