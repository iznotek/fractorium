#pragma once

#include "EmberCLPch.h"

/// <summary>
/// OpenCLInfo class.
/// </summary>

namespace EmberCLns
{
/// <summary>
/// Keeps information about all valid OpenCL devices on this system.
/// Devices which do not successfully create a test command queue are not
/// added to the list.
/// The pattern is singleton, so there is only one instance per program,
/// retreivable by reference via the Instance() function.
/// This class derives from EmberReport, so the caller is able
/// to retrieve a text dump of error information if any errors occur.
/// </summary>
class EMBERCL_API OpenCLInfo : public EmberReport, public Singleton<OpenCLInfo>
{
public:
	const vector<cl::Platform>& Platforms() const;
	const string& PlatformName(size_t platform) const;
	const vector<string>& PlatformNames() const;
	const vector<vector<cl::Device>>& Devices() const;
	const string& DeviceName(size_t platform, size_t device) const;
	const vector<pair<size_t, size_t>>& DeviceIndices() const;
	const vector<string>& AllDeviceNames() const;
	const vector<string>& DeviceNames(size_t platform) const;
	size_t TotalDeviceIndex(size_t platform, size_t device) const;
	string DumpInfo() const;
	bool Ok() const;
	bool CreateContext(const cl::Platform& platform, cl::Context& context, bool shared);
	bool CheckCL(cl_int err, const char* name);
	string ErrorToStringCL(cl_int err);

	/// <summary>
	/// Get device information for the specified field.
	/// Template argument expected to be cl_ulong, cl_uint or cl_int;
	/// </summary>
	/// <param name="platform">The index platform of the platform to use</param>
	/// <param name="device">The index device of the device to use</param>
	/// <param name="name">The device field/feature to query</param>
	/// <returns>The value of the field</returns>
	template<typename T>
	T GetInfo(size_t platform, size_t device, cl_device_info name) const
	{
		T val = T();

		if (platform < m_Devices.size() && device < m_Devices[platform].size())
			m_Devices[platform][device].getInfo(name, &val);

		return val;
	}

	SINGLETON_DERIVED_IMPL(OpenCLInfo);

private:
	OpenCLInfo();

	bool m_Init;
	vector<cl::Platform> m_Platforms;
	vector<vector<cl::Device>> m_Devices;
	vector<string> m_PlatformNames;
	vector<vector<string>> m_DeviceNames;
	vector<pair<size_t, size_t>> m_DeviceIndices;
	vector<string> m_AllDeviceNames;
};
}
