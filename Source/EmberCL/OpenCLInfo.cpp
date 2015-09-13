#include "EmberCLPch.h"
#include "OpenCLInfo.h"

namespace EmberCLns
{
/// <summary>
/// Initialize and return a reference to the one and only OpenCLInfo object.
/// </summary>
/// <returns>A reference to the only OpenCLInfo object.</returns>
OpenCLInfo& OpenCLInfo::Instance()
{
	static OpenCLInfo instance;

	return instance;
}

/// <summary>
/// Initialize the all platforms and devices and keep information about them in lists.
/// </summary>
OpenCLInfo::OpenCLInfo()
{
	cl_int err;
	vector<cl::Platform> platforms;
	vector<vector<cl::Device>> devices;
	intmax_t workingPlatformIndex = -1;

	m_Init = false;
	cl::Platform::get(&platforms);
	devices.resize(platforms.size());
	m_Platforms.reserve(platforms.size());
	m_Devices.reserve(platforms.size());
	m_DeviceNames.reserve(platforms.size());
	m_AllDeviceNames.reserve(platforms.size());
	m_DeviceIndices.reserve(platforms.size());

	for (size_t i = 0; i < platforms.size(); i++)
		platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices[i]);

	for (size_t platform = 0; platform < platforms.size(); platform++)
	{
		bool platformOk = false;
		bool deviceOk = false;
		cl::Context context;

		if (CreateContext(platforms[platform], context, false))//Platform is ok, now do context. Unshared by default.
		{
			size_t workingDeviceIndex = 0;

			for (size_t device = 0; device < devices[platform].size(); device++)//Context is ok, now do devices.
			{
				auto q = cl::CommandQueue(context, devices[platform][device], 0, &err);//At least one GPU device is present, so create a command queue.

				if (CheckCL(err, "cl::CommandQueue()"))
				{
					if (!platformOk)
					{
						m_Platforms.push_back(platforms[platform]);
						m_PlatformNames.push_back(platforms[platform].getInfo<CL_PLATFORM_VENDOR>(nullptr) + " " + platforms[platform].getInfo<CL_PLATFORM_NAME>(nullptr) + " " + platforms[platform].getInfo<CL_PLATFORM_VERSION>(nullptr));
						workingPlatformIndex++;
						platformOk = true;
					}

					if (!deviceOk)
					{
						m_Devices.push_back(vector<cl::Device>());
						m_DeviceNames.push_back(vector<string>());
						m_Devices.back().reserve(devices[platform].size());
						m_DeviceNames.back().reserve(devices[platform].size());
						deviceOk = true;
					}

					m_Devices.back().push_back(devices[platform][device]);
					m_DeviceNames.back().push_back(devices[platform][device].getInfo<CL_DEVICE_VENDOR>(nullptr) + " " + devices[platform][device].getInfo<CL_DEVICE_NAME>(nullptr));// + " " + devices[platform][device].getInfo<CL_DEVICE_VERSION>());
					m_AllDeviceNames.push_back(m_DeviceNames.back().back());
					m_DeviceIndices.push_back(pair<size_t, size_t>(workingPlatformIndex, workingDeviceIndex++));
					m_Init = true;//If at least one platform and device succeeded, OpenCL is ok. It's now ok to begin building and running programs.
				}
			}
		}
	}
}

/// <summary>
/// Get a const reference to the vector of available platforms.
/// </summary>
/// <returns>A const reference to the vector of available platforms</returns>
const vector<cl::Platform>& OpenCLInfo::Platforms() const
{
	return m_Platforms;
}

/// <summary>
/// Get a const reference to the platform name at the specified index.
/// </summary>
/// <param name="i">The platform index to get the name of</param>
/// <returns>The platform name if found, else empty string</returns>
const string& OpenCLInfo::PlatformName(size_t platform) const
{
	static string s;
	return platform < m_PlatformNames.size() ? m_PlatformNames[platform] : s;
}

/// <summary>
/// Get a const reference to a vector of all available platform names on the system as a vector of strings.
/// </summary>
/// <returns>All available platform names on the system as a vector of strings</returns>
const vector<string>& OpenCLInfo::PlatformNames() const
{
	return m_PlatformNames;
}

/// <summary>
/// Get a const reference to a vector of vectors of all available devices on the system.
/// Each outer vector is a different platform.
/// </summary>
/// <returns>All available devices on the system, grouped by platform.</returns>
const vector<vector<cl::Device>>& OpenCLInfo::Devices() const
{
	return m_Devices;
}

/// <summary>
/// Get a const reference to the device name at the specified index on the platform
/// at the specified index.
/// </summary>
/// <param name="platform">The platform index of the device</param>
/// <param name="device">The device index</param>
/// <returns>The name of the device if found, else empty string</returns>
const string& OpenCLInfo::DeviceName(size_t platform, size_t device) const
{
	static string s;

	if (platform < m_Platforms.size() && platform < m_Devices.size())
		if (device < m_Devices[platform].size())
			return m_DeviceNames[platform][device];

	return s;
}

/// <summary>
/// Get a const reference to a vector of pairs of uints which contain the platform,device
/// indices of all available devices on the system.
/// </summary>
/// <returns>All available devices on the system as platform,device index pairs</returns>
const vector<pair<size_t, size_t>>& OpenCLInfo::DeviceIndices() const
{
	return m_DeviceIndices;
}

/// <summary>
/// Get a const reference to a vector of all available device names on the system as a vector of strings.
/// </summary>
/// <returns>All available device names on the system as a vector of strings</returns>
const vector<string>& OpenCLInfo::AllDeviceNames() const
{
	return m_AllDeviceNames;
}

/// <summary>
/// Get a const reference to a vector of all available device names on the platform
/// at the specified index as a vector of strings.
/// </summary>
/// <param name="platform">The platform index whose devices names will be returned</param>
/// <returns>All available device names on the platform at the specified index as a vector of strings if within range, else empty vector.</returns>
const vector<string>& OpenCLInfo::DeviceNames(size_t platform) const
{
	static vector<string> v;

	if (platform < m_DeviceNames.size())
		return m_DeviceNames[platform];

	return v;
}

/// <summary>
/// Get the total device index at the specified platform and device index.
/// </summary>
/// <param name="platform">The platform index of the device</param>
/// <param name="device">The device index within the platform</param>
/// <returns>The total device index if found, else 0</returns>
size_t OpenCLInfo::TotalDeviceIndex(size_t platform, size_t device) const
{
	size_t index = 0;
	pair<size_t, size_t> p{ platform, device };

	for (size_t i = 0; i < m_DeviceIndices.size(); i++)
	{
		if (p == m_DeviceIndices[i])
		{
			index = i;
			break;
		}
	}

	return index;
}

/// <summary>
/// Create a context that is optionally shared with OpenGL and plact it in the
/// passed in context ref parameter.
/// </summary>
/// <param name="platform">The platform object to create the context on</param>
/// <param name="context">The context object to store the result in</param>
/// <param name="shared">True if shared with OpenGL, else not shared.</param>
/// <returns>True if success, else false.</returns>
bool OpenCLInfo::CreateContext(const cl::Platform& platform, cl::Context& context, bool shared)
{
	cl_int err;

	if (shared)
	{
		//Define OS-specific context properties and create the OpenCL context.
		#if defined (__APPLE__) || defined(MACOSX)
			CGLContextObj kCGLContext = CGLGetCurrentContext();
			CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
			cl_context_properties props[] =
			{
				CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
				0
			};

			context = cl::Context(CL_DEVICE_TYPE_GPU, props, nullptr, nullptr, &err);//May need to tinker with this on Mac.
		#else
			#if defined WIN32
				cl_context_properties props[] =
				{
					CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
					CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
					CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>((platform)()),
					0
				};

				context = cl::Context(CL_DEVICE_TYPE_GPU, props, nullptr, nullptr, &err);
			#else
				cl_context_properties props[] =
				{
					CL_GL_CONTEXT_KHR, cl_context_properties(glXGetCurrentContext()),
					CL_GLX_DISPLAY_KHR, cl_context_properties(glXGetCurrentDisplay()),
					CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>((platform)()),
					0
				};

				context = cl::Context(CL_DEVICE_TYPE_GPU, props, nullptr, nullptr, &err);
			#endif
		#endif
	}
	else
	{
		cl_context_properties props[3] =
		{
			CL_CONTEXT_PLATFORM,
			reinterpret_cast<cl_context_properties>((platform)()),
			0
		};

		context = cl::Context(CL_DEVICE_TYPE_ALL, props, nullptr, nullptr, &err);
	}

	return CheckCL(err, "cl::Context()");
}

/// <summary>
/// Return whether at least one device has been found and properly initialized.
/// </summary>
/// <returns>True if success, else false.</returns>
bool OpenCLInfo::Ok() const
{
	return m_Init;
}

/// <summary>
/// Get all information about all platforms and devices.
/// </summary>
/// <returns>A string with all information about all platforms and devices</returns>
string OpenCLInfo::DumpInfo() const
{
	ostringstream os;
	vector<size_t> sizes;

	os.imbue(locale(""));

	for (size_t platform = 0; platform < m_Platforms.size(); platform++)
	{
		os << "Platform " << platform << ": " << PlatformName(platform) << endl;

		for (size_t device = 0; device < m_Devices[platform].size(); device++)
		{
			os << "Device " << device << ": " << DeviceName(platform, device) << endl;
			os << "CL_DEVICE_OPENCL_C_VERSION: " << GetInfo<string>(platform, device, CL_DEVICE_OPENCL_C_VERSION) << endl;
			os << "CL_DEVICE_LOCAL_MEM_SIZE: " << GetInfo<cl_ulong>(platform, device, CL_DEVICE_LOCAL_MEM_SIZE) << endl;
			os << "CL_DEVICE_LOCAL_MEM_TYPE: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_LOCAL_MEM_TYPE) << endl;
			os << "CL_DEVICE_MAX_COMPUTE_UNITS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_MAX_COMPUTE_UNITS) << endl;
			os << "CL_DEVICE_MAX_READ_IMAGE_ARGS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_MAX_READ_IMAGE_ARGS) << endl;
			os << "CL_DEVICE_MAX_WRITE_IMAGE_ARGS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS) << endl;
			os << "CL_DEVICE_MAX_MEM_ALLOC_SIZE: " << GetInfo<cl_ulong>(platform, device, CL_DEVICE_MAX_MEM_ALLOC_SIZE) << endl;
			os << "CL_DEVICE_ADDRESS_BITS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_ADDRESS_BITS) << endl;

			os << "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE) << endl;
			os << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE) << endl;
			os << "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: " << GetInfo<cl_ulong>(platform, device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE) << endl;
			os << "CL_DEVICE_GLOBAL_MEM_SIZE: " << GetInfo<cl_ulong>(platform, device, CL_DEVICE_GLOBAL_MEM_SIZE) << endl;
			os << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: " << GetInfo<cl_ulong>(platform, device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE) << endl;

			os << "CL_DEVICE_MAX_CONSTANT_ARGS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_MAX_CONSTANT_ARGS) << endl;
			os << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: " << GetInfo<cl_uint>(platform, device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) << endl;
			os << "CL_DEVICE_MAX_WORK_GROUP_SIZE: " << GetInfo<size_t>(platform, device, CL_DEVICE_MAX_WORK_GROUP_SIZE) << endl;

			sizes = GetInfo<vector<size_t>>(platform, device, CL_DEVICE_MAX_WORK_ITEM_SIZES);
			os << "CL_DEVICE_MAX_WORK_ITEM_SIZES: " << sizes[0] << ", " << sizes[1] << ", " << sizes[2] << endl << endl;

			if (device != m_Devices[platform].size() - 1 && platform != m_Platforms.size() - 1)
				os << endl;
		}

		os << endl;
	}

	return os.str();
}

/// <summary>
/// Check an OpenCL return value for errors.
/// </summary>
/// <param name="err">The error code to inspect</param>
/// <param name="name">A description of where the value was gotten from</param>
/// <returns>True if success, else false.</returns>
bool OpenCLInfo::CheckCL(cl_int err, const char* name)
{
	if (err != CL_SUCCESS)
	{
		ostringstream ss;
		ss << "ERROR: " << ErrorToStringCL(err) << " in " << name << "." << endl;
		m_ErrorReport.push_back(ss.str());
	}

	return err == CL_SUCCESS;
}

/// <summary>
/// Translate an OpenCL error code into a human readable string.
/// </summary>
/// <param name="err">The error code to translate</param>
/// <returns>A human readable description of the error passed in</returns>
string OpenCLInfo::ErrorToStringCL(cl_int err)
{
	switch (err)
	{
		case CL_SUCCESS:								   return "Success";
		case CL_DEVICE_NOT_FOUND:						   return "Device not found";
		case CL_DEVICE_NOT_AVAILABLE:					   return "Device not available";
		case CL_COMPILER_NOT_AVAILABLE:					   return "Compiler not available";
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:			   return "Memory object allocation failure";
		case CL_OUT_OF_RESOURCES:						   return "Out of resources";
		case CL_OUT_OF_HOST_MEMORY:						   return "Out of host memory";
		case CL_PROFILING_INFO_NOT_AVAILABLE:			   return "Profiling information not available";
		case CL_MEM_COPY_OVERLAP:						   return "Memory copy overlap";
		case CL_IMAGE_FORMAT_MISMATCH:					   return "Image format mismatch";
		case CL_IMAGE_FORMAT_NOT_SUPPORTED:				   return "Image format not supported";
		case CL_BUILD_PROGRAM_FAILURE:					   return "Program build failure";
		case CL_MAP_FAILURE:							   return "Map failure";
		case CL_MISALIGNED_SUB_BUFFER_OFFSET:			   return "Misaligned sub buffer offset";
		case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "Exec status error for events in wait list";
		case CL_INVALID_VALUE:							   return "Invalid value";
		case CL_INVALID_DEVICE_TYPE:					   return "Invalid device type";
		case CL_INVALID_PLATFORM:						   return "Invalid platform";
		case CL_INVALID_DEVICE:							   return "Invalid device";
		case CL_INVALID_CONTEXT:						   return "Invalid context";
		case CL_INVALID_QUEUE_PROPERTIES:				   return "Invalid queue properties";
		case CL_INVALID_COMMAND_QUEUE:					   return "Invalid command queue";
		case CL_INVALID_HOST_PTR:						   return "Invalid host pointer";
		case CL_INVALID_MEM_OBJECT:						   return "Invalid memory object";
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:		   return "Invalid image format descriptor";
		case CL_INVALID_IMAGE_SIZE:						   return "Invalid image size";
		case CL_INVALID_SAMPLER:						   return "Invalid sampler";
		case CL_INVALID_BINARY:							   return "Invalid binary";
		case CL_INVALID_BUILD_OPTIONS:					   return "Invalid build options";
		case CL_INVALID_PROGRAM:						   return "Invalid program";
		case CL_INVALID_PROGRAM_EXECUTABLE:				   return "Invalid program executable";
		case CL_INVALID_KERNEL_NAME:					   return "Invalid kernel name";
		case CL_INVALID_KERNEL_DEFINITION:				   return "Invalid kernel definition";
		case CL_INVALID_KERNEL:							   return "Invalid kernel";
		case CL_INVALID_ARG_INDEX:						   return "Invalid argument index";
		case CL_INVALID_ARG_VALUE:						   return "Invalid argument value";
		case CL_INVALID_ARG_SIZE:						   return "Invalid argument size";
		case CL_INVALID_KERNEL_ARGS:					   return "Invalid kernel arguments";
		case CL_INVALID_WORK_DIMENSION:					   return "Invalid work dimension";
		case CL_INVALID_WORK_GROUP_SIZE:				   return "Invalid work group size";
		case CL_INVALID_WORK_ITEM_SIZE:					   return "Invalid work item size";
		case CL_INVALID_GLOBAL_OFFSET:					   return "Invalid global offset";
		case CL_INVALID_EVENT_WAIT_LIST:				   return "Invalid event wait list";
		case CL_INVALID_EVENT:							   return "Invalid event";
		case CL_INVALID_OPERATION:						   return "Invalid operation";
		case CL_INVALID_GL_OBJECT:						   return "Invalid OpenGL object";
		case CL_INVALID_BUFFER_SIZE:					   return "Invalid buffer size";
		case CL_INVALID_MIP_LEVEL:						   return "Invalid mip-map level";
		case CL_INVALID_GLOBAL_WORK_SIZE:				   return "Invalid global work size";
		case CL_INVALID_PROPERTY:						   return "Invalid property";
		default:
		{
			ostringstream ss;
			ss << "<Unknown error code> " << err;
			return ss.str();
		}
	}
}
}
