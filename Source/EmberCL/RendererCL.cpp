#include "EmberCLPch.h"
#include "RendererCL.h"

namespace EmberCLns
{
/// <summary>
/// Constructor that inintializes various buffer names, block dimensions, image formats
/// and finally initializes one or more OpenCL devices using the passed in parameters.
/// When running with multiple devices, the first device is considered the "primary", while
/// others are "secondary".
/// The differences are:
///		-Only the primary device will report progress, however the progress count will contain the combined progress of all devices.
///		-The primary device runs in this thread, while others run on their own threads.
///		-The primary device does density filtering and final accumulation, while the others only iterate.
///		-Upon completion of iteration, the histograms from the secondary devices are:
///			Copied to a temporary host side buffer.
///			Copied from the host side buffer to the primary device's density filtering buffer as a temporary device storage area.
///			Summed from the density filtering buffer, to the primary device's histogram.
///			When this process happens for the last device, the density filtering buffer is cleared since it will be used shortly.
/// Kernel creators are set to be non-nvidia by default. Will be properly set in Init().
/// </summary>
/// <param name="devices">A vector of the platform,device index pairs to use. The first device will be the primary and will run non-threaded.</param>
/// <param name="shared">True if shared with OpenGL, else false. Default: false.</param>
/// <param name="outputTexID">The texture ID of the shared OpenGL texture if shared. Default: 0.</param>
template <typename T, typename bucketT>
RendererCL<T, bucketT>::RendererCL(const vector<pair<size_t, size_t>>& devices, bool shared, GLuint outputTexID)
	:
	m_IterOpenCLKernelCreator(),
	m_DEOpenCLKernelCreator(typeid(T) == typeid(double), false),
	m_FinalAccumOpenCLKernelCreator(typeid(T) == typeid(double))
{
	Init();
	Init(devices, shared, outputTexID);
}

/// <summary>
/// Initialization of fields, no OpenCL initialization is done here.
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::Init()
{
	m_Init = false;
	m_DoublePrecision = typeid(T) == typeid(double);
	m_NumChannels = 4;
	//Buffer names.
	m_EmberBufferName = "Ember";
	m_XformsBufferName = "Xforms";
	m_ParVarsBufferName = "ParVars";
	m_GlobalSharedBufferName = "GlobalShared";
	m_SeedsBufferName = "Seeds";
	m_DistBufferName = "Dist";
	m_CarToRasBufferName = "CarToRas";
	m_DEFilterParamsBufferName = "DEFilterParams";
	m_SpatialFilterParamsBufferName = "SpatialFilterParams";
	m_DECoefsBufferName = "DECoefs";
	m_DEWidthsBufferName = "DEWidths";
	m_DECoefIndicesBufferName = "DECoefIndices";
	m_SpatialFilterCoefsBufferName = "SpatialFilterCoefs";
	m_CurvesCsaName = "CurvesCsa";
	m_HistBufferName = "Hist";
	m_AccumBufferName = "Accum";
	m_FinalImageName = "Final";
	m_PointsBufferName = "Points";
	//It's critical that these numbers never change. They are
	//based on the cuburn model of each kernel launch containing
	//256 threads. 32 wide by 8 high. Everything done in the OpenCL
	//iteraion kernel depends on these dimensions.
	m_IterCountPerKernel = 256;
	m_IterBlockWidth = 32;
	m_IterBlockHeight = 8;
	m_IterBlocksWide = 64;
	m_IterBlocksHigh = 2;
	m_PaletteFormat.image_channel_order = CL_RGBA;
	m_PaletteFormat.image_channel_data_type = CL_FLOAT;
	m_FinalFormat.image_channel_order = CL_RGBA;
	m_FinalFormat.image_channel_data_type = CL_UNORM_INT8;//Change if this ever supports 2BPC outputs for PNG.
}

/// <summary>
/// Virtual destructor.
/// </summary>
template <typename T, typename bucketT>
RendererCL<T, bucketT>::~RendererCL()
{
}

/// <summary>
/// Non-virtual member functions for OpenCL specific tasks.
/// </summary>

/// <summary>
/// Initialize OpenCL.
/// In addition to initializing, this function will create the zeroization program,
/// as well as the basic log scale filtering programs. This is done to ensure basic
/// compilation works. Further compilation will be done later for iteration, density filtering,
/// and final accumulation.
/// </summary>
/// <param name="devices">A vector of the platform,device index pairs to use. The first device will be the primary and will run non-threaded.</param>
/// <param name="shared">True if shared with OpenGL, else false.</param>
/// <param name="outputTexID">The texture ID of the shared OpenGL texture if shared</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::Init(const vector<pair<size_t, size_t>>& devices, bool shared, GLuint outputTexID)
{
	if (devices.empty())
		return false;

	bool b = false;
	const char* loc = __FUNCTION__;
	auto& zeroizeProgram = m_IterOpenCLKernelCreator.ZeroizeKernel();
	auto& sumHistProgram = m_IterOpenCLKernelCreator.SumHistKernel();
	ostringstream os;
	m_Init = false;
	m_Devices.clear();
	m_Devices.reserve(devices.size());
	m_OutputTexID = outputTexID;
	m_GlobalShared.second.resize(16);//Dummy data until a real alloc is needed.

	for (size_t i = 0; i < devices.size(); i++)
	{
		try
		{
			unique_ptr<RendererClDevice> cld(new RendererClDevice(typeid(T) == typeid(double), devices[i].first, devices[i].second, i == 0 ? shared : false));

			if ((b = cld->Init()))//Build a simple program to ensure OpenCL is working right.
			{
				if (b && !(b = cld->m_Wrapper.AddProgram(m_IterOpenCLKernelCreator.ZeroizeEntryPoint(), zeroizeProgram, m_IterOpenCLKernelCreator.ZeroizeEntryPoint(), m_DoublePrecision))) { AddToReport(loc); }

				if (b && !(b = cld->m_Wrapper.AddAndWriteImage("Palette", CL_MEM_READ_ONLY, m_PaletteFormat, 256, 1, 0, nullptr))) { AddToReport(loc); }

				if (b && !(b = cld->m_Wrapper.AddAndWriteBuffer(m_GlobalSharedBufferName, m_GlobalShared.second.data(), m_GlobalShared.second.size() * sizeof(m_GlobalShared.second[0])))) { AddToReport(loc); }//Empty at start, will be filled in later if needed.

				if (b)
				{
					m_Devices.push_back(std::move(cld));//Success, so move to the vector, else it will go out of scope and be deleted.
				}
				else
				{
					os << loc << ": failed to init platform " << devices[i].first << ", device " << devices[i].second;
					AddToReport(loc);
					break;
				}
			}
		}
		catch (const std::exception& e)
		{
			os << loc << ": failed to init platform " << devices[i].first << ", device " << devices[i].second << ": " << e.what();
			AddToReport(os.str());
		}
		catch (...)
		{
			os << loc << ": failed to init platform " << devices[i].first << ", device " << devices[i].second;
			AddToReport(os.str());
		}
	}

	if (b && m_Devices.size() == devices.size())
	{
		auto& firstWrapper = m_Devices[0]->m_Wrapper;
		m_DEOpenCLKernelCreator = DEOpenCLKernelCreator(m_DoublePrecision, m_Devices[0]->Nvidia());

		//Build a simple program to ensure OpenCL is working right.
		if (b && !(b = firstWrapper.AddProgram(m_DEOpenCLKernelCreator.LogScaleAssignDEEntryPoint(), m_DEOpenCLKernelCreator.LogScaleAssignDEKernel(), m_DEOpenCLKernelCreator.LogScaleAssignDEEntryPoint(), m_DoublePrecision))) { AddToReport(loc); }

		if (b && !(b = firstWrapper.AddProgram(m_IterOpenCLKernelCreator.SumHistEntryPoint(), sumHistProgram, m_IterOpenCLKernelCreator.SumHistEntryPoint(), m_DoublePrecision))) { AddToReport(loc); }

		if (b)
		{
			//This is the maximum box dimension for density filtering which consists of (blockSize  * blockSize) + (2 * filterWidth).
			//These blocks must be square, and ideally, 32x32.
			//Sadly, at the moment, Fermi runs out of resources at that block size because the DE filter function is so complex.
			//The next best block size seems to be 24x24.
			//AMD is further limited because of less local memory so these have to be 16 on AMD.
			m_MaxDEBlockSizeW = m_Devices[0]->Nvidia() ? 24 : 16;//These *must* both be divisible by 8 or else pixels will go missing.
			m_MaxDEBlockSizeH = m_Devices[0]->Nvidia() ? 24 : 16;
			FillSeeds();

			for (size_t device = 0; device < m_Devices.size(); device++)
				if (b && !(b = m_Devices[device]->m_Wrapper.AddAndWriteBuffer(m_SeedsBufferName, reinterpret_cast<void*>(m_Seeds[device].data()), SizeOf(m_Seeds[device])))) { AddToReport(loc); break; }
		}

		m_Init = b;
	}
	else
	{
		m_Devices.clear();
		os << loc << ": failed to init all devices and platforms.";
		AddToReport(os.str());
	}

	return m_Init;
}

/// <summary>
/// Set the shared output texture of the primary device where final accumulation will be written to.
/// </summary>
/// <param name="outputTexID">The texture ID of the shared OpenGL texture if shared</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::SetOutputTexture(GLuint outputTexID)
{
	bool success = true;
	const char* loc = __FUNCTION__;

	if (!m_Devices.empty())
	{
		OpenCLWrapper& firstWrapper = m_Devices[0]->m_Wrapper;
		m_OutputTexID = outputTexID;
		EnterResize();

		if (!firstWrapper.AddAndWriteImage(m_FinalImageName, CL_MEM_WRITE_ONLY, m_FinalFormat, FinalRasW(), FinalRasH(), 0, nullptr, firstWrapper.Shared(), m_OutputTexID))
		{
			AddToReport(loc);
			success = false;
		}

		LeaveResize();
	}
	else
		success = false;

	return success;
}

/// <summary>
/// OpenCL property accessors, getters only.
/// </summary>

//Iters per kernel/block/grid.
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterCountPerKernel() const { return m_IterCountPerKernel;						  }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterCountPerBlock()  const { return IterCountPerKernel() * IterBlockKernelCount(); }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterCountPerGrid()   const { return IterCountPerKernel() * IterGridKernelCount();  }

//Kernels per block.
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterBlockKernelWidth()  const { return m_IterBlockWidth;								    }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterBlockKernelHeight() const { return m_IterBlockHeight;								}
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterBlockKernelCount()  const { return IterBlockKernelWidth() * IterBlockKernelHeight(); }

//Kernels per grid.
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridKernelWidth()  const { return IterGridBlockWidth() * IterBlockKernelWidth();   }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridKernelHeight() const { return IterGridBlockHeight() * IterBlockKernelHeight(); }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridKernelCount()  const { return IterGridKernelWidth() * IterGridKernelHeight();  }

//Blocks per grid.
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridBlockWidth()  const { return m_IterBlocksWide;							    }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridBlockHeight() const { return m_IterBlocksHigh;							    }
template <typename T, typename bucketT> size_t RendererCL<T, bucketT>::IterGridBlockCount()  const { return IterGridBlockWidth() * IterGridBlockHeight();   }

/// <summary>
/// Read the histogram of the specified into the host side CPU buffer.
/// </summary>
/// <param name="device">The index device of the device whose histogram will be read</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ReadHist(size_t device)
{
	if (device < m_Devices.size())
		if (Renderer<T, bucketT>::Alloc(true))//Allocate the histogram memory to read into, other buffers not needed.
			return m_Devices[device]->m_Wrapper.ReadBuffer(m_HistBufferName, reinterpret_cast<void*>(HistBuckets()), SuperSize() * sizeof(v4bT));

	return false;
}

/// <summary>
/// Read the density filtering buffer into the host side CPU buffer.
/// Used for debugging.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ReadAccum()
{
	if (Renderer<T, bucketT>::Alloc() && !m_Devices.empty())//Allocate the memory to read into.
		return m_Devices[0]->m_Wrapper.ReadBuffer(m_AccumBufferName, reinterpret_cast<void*>(AccumulatorBuckets()), SuperSize() * sizeof(v4bT));

	return false;
}

/// <summary>
/// Read the temporary points buffer from a device into a host side CPU buffer.
/// Used for debugging.
/// </summary>
/// <param name="device">The index in the device buffer whose points will be read</param>
/// <param name="vec">The host side buffer to read into</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ReadPoints(size_t device, vector<PointCL<T>>& vec)
{
	vec.resize(IterGridKernelCount());//Allocate the memory to read into.

	if (vec.size() >= IterGridKernelCount() && device < m_Devices.size())
		return m_Devices[device]->m_Wrapper.ReadBuffer(m_PointsBufferName, reinterpret_cast<void*>(vec.data()), IterGridKernelCount() * sizeof(PointCL<T>));

	return false;
}

/// <summary>
/// Clear the histogram buffer for all devices with all zeroes.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ClearHist()
{
	bool b = !m_Devices.empty();
	const char* loc = __FUNCTION__;

	for (size_t i = 0; i < m_Devices.size(); i++)
		if (b && !(b = ClearBuffer(i, m_HistBufferName, uint(SuperRasW()), uint(SuperRasH()), sizeof(v4bT)))) { AddToReport(loc); break; }

	return b;
}

/// <summary>
/// Clear the histogram buffer for a single device with all zeroes.
/// </summary>
/// <param name="device">The index in the device buffer whose histogram will be cleared</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ClearHist(size_t device)
{
	bool b = device < m_Devices.size();
	const char* loc = __FUNCTION__;

	if (b && !(b = ClearBuffer(device, m_HistBufferName, uint(SuperRasW()), uint(SuperRasH()), sizeof(v4bT)))) { AddToReport(loc); }

	return b;
}

/// <summary>
/// Clear the density filtering buffer with all zeroes.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ClearAccum()
{
	return ClearBuffer(0, m_AccumBufferName, uint(SuperRasW()), uint(SuperRasH()), sizeof(v4bT));
}

/// <summary>
/// Write values from a host side CPU buffer into the temporary points buffer for the specified device.
/// Used for debugging.
/// </summary>
/// <param name="device">The index in the device buffer whose points will be written to</param>
/// <param name="vec">The host side buffer whose values to write</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::WritePoints(size_t device, vector<PointCL<T>>& vec)
{
	bool b = false;
	const char* loc = __FUNCTION__;

	if (device < m_Devices.size())
		if (!(b = m_Devices[device]->m_Wrapper.WriteBuffer(m_PointsBufferName, reinterpret_cast<void*>(vec.data()), SizeOf(vec)))) { AddToReport(loc); }

	return b;
}

#ifdef TEST_CL
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::WriteRandomPoints(size_t device)
{
	size_t size = IterGridKernelCount();
	vector<PointCL<T>> vec(size);

	for (int i = 0; i < size; i++)
	{
		vec[i].m_X = m_Rand[0].Frand11<T>();
		vec[i].m_Y = m_Rand[0].Frand11<T>();
		vec[i].m_Z = 0;
		vec[i].m_ColorX = m_Rand[0].Frand01<T>();
		vec[i].m_LastXfUsed = 0;
	}

	return WritePoints(device, vec);
}
#endif

/// <summary>
/// Get the kernel string for the last built iter program.
/// </summary>
/// <returns>The string representation of the kernel for the last built iter program.</returns>
template <typename T, typename bucketT>
const string& RendererCL<T, bucketT>::IterKernel() const { return m_IterKernel; }

/// <summary>
/// Get the kernel string for the last built density filtering program.
/// </summary>
/// <returns>The string representation of the kernel for the last built density filtering program.</returns>
template <typename T, typename bucketT>
const string& RendererCL<T, bucketT>::DEKernel() const { return m_DEOpenCLKernelCreator.GaussianDEKernel(Supersample(), m_DensityFilterCL.m_FilterWidth); }

/// <summary>
/// Get the kernel string for the last built final accumulation program.
/// </summary>
/// <returns>The string representation of the kernel for the last built final accumulation program.</returns>
template <typename T, typename bucketT>
const string& RendererCL<T, bucketT>::FinalAccumKernel() const { return m_FinalAccumOpenCLKernelCreator.FinalAccumKernel(EarlyClip(), Renderer<T, bucketT>::NumChannels(), Transparency()); }

/// <summary>
/// Virtual functions overridden from RendererCLBase.
/// </summary>

/// <summary>
/// Read the final image buffer buffer from the primary device into the host side CPU buffer.
/// This must be called before saving the final output image to file.
/// </summary>
/// <param name="pixels">The host side buffer to read into</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ReadFinal(byte* pixels)
{
	if (pixels && !m_Devices.empty())
		return m_Devices[0]->m_Wrapper.ReadImage(m_FinalImageName, FinalRasW(), FinalRasH(), 0, m_Devices[0]->m_Wrapper.Shared(), pixels);

	return false;
}

/// <summary>
/// Clear the final image output buffer of the primary device with all zeroes by copying a host side buffer.
/// Slow, but never used because the final output image is always completely overwritten.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ClearFinal()
{
	vector<byte> v;

	if (!m_Devices.empty())
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		uint index = wrapper.FindImageIndex(m_FinalImageName, wrapper.Shared());

		if (this->PrepFinalAccumVector(v))
		{
			bool b = wrapper.WriteImage2D(index, wrapper.Shared(), FinalRasW(), FinalRasH(), 0, v.data());

			if (!b)
				AddToReport(__FUNCTION__);

			return b;
		}
		else
			return false;
	}
	else
		return false;
}

/// <summary>
/// Public virtual functions overridden from Renderer or RendererBase.
/// </summary>

/// <summary>
/// The amount of video RAM available on the first GPU to render with.
/// </summary>
/// <returns>An unsigned 64-bit integer specifying how much video memory is available</returns>
template <typename T, typename bucketT>
size_t RendererCL<T, bucketT>::MemoryAvailable()
{
	return Ok() ? m_Devices[0]->m_Wrapper.GlobalMemSize() : 0ULL;
}

/// <summary>
/// Return whether OpenCL has been properly initialized.
/// </summary>
/// <returns>True if OpenCL has been properly initialized, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::Ok() const
{
	return !m_Devices.empty() && m_Init;
}

/// <summary>
/// Override to force num channels to be 4 because RGBA is always used for OpenCL
/// since the output is actually an image rather than just a buffer.
/// </summary>
/// <param name="numChannels">The number of channels, ignored.</param>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::NumChannels(size_t numChannels)
{
	m_NumChannels = 4;
}

/// <summary>
/// Clear the error report for this class as well as the OpenCLWrapper members of each device.
/// </summary>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::ClearErrorReport()
{
	EmberReport::ClearErrorReport();

	for (auto& device : m_Devices)
		device->m_Wrapper.ClearErrorReport();
}

/// <summary>
/// The sub batch size for OpenCL will always be how many
/// iterations are ran per kernel call. The caller can't
/// change this.
/// </summary>
/// <returns>The number of iterations ran in a single kernel call</returns>
template <typename T, typename bucketT>
size_t RendererCL<T, bucketT>::SubBatchSize() const
{
	return IterCountPerGrid();
}

/// <summary>
/// The thread count for OpenCL is always considered to be 1, however
/// the kernel internally runs many threads.
/// </summary>
/// <returns>1</returns>
template <typename T, typename bucketT>
size_t RendererCL<T, bucketT>::ThreadCount() const
{
	return 1;
}

/// <summary>
/// Create the density filter in the base class and copy the filter values
/// to the corresponding OpenCL buffers on the primary device.
/// </summary>
/// <param name="newAlloc">True if a new filter instance was created, else false.</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::CreateDEFilter(bool& newAlloc)
{
	bool b = true;

	if (!m_Devices.empty() && Renderer<T, bucketT>::CreateDEFilter(newAlloc))
	{
		//Copy coefs and widths here. Convert and copy the other filter params right before calling the filtering kernel.
		if (newAlloc)
		{
			const char* loc = __FUNCTION__;
			auto& wrapper = m_Devices[0]->m_Wrapper;

			if (b && !(b = wrapper.AddAndWriteBuffer(m_DECoefsBufferName,		reinterpret_cast<void*>(const_cast<bucketT*>(m_DensityFilter->Coefs())),	m_DensityFilter->CoefsSizeBytes())))		{ AddToReport(loc); }

			if (b && !(b = wrapper.AddAndWriteBuffer(m_DEWidthsBufferName,	    reinterpret_cast<void*>(const_cast<bucketT*>(m_DensityFilter->Widths())),	m_DensityFilter->WidthsSizeBytes())))		{ AddToReport(loc); }

			if (b && !(b = wrapper.AddAndWriteBuffer(m_DECoefIndicesBufferName, reinterpret_cast<void*>(const_cast<uint*>(m_DensityFilter->CoefIndices())), m_DensityFilter->CoefsIndicesSizeBytes()))) { AddToReport(loc); }
		}
	}
	else
		b = false;

	return b;
}

/// <summary>
/// Create the spatial filter in the base class and copy the filter values
/// to the corresponding OpenCL buffers on the primary device.
/// </summary>
/// <param name="newAlloc">True if a new filter instance was created, else false.</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::CreateSpatialFilter(bool& newAlloc)
{
	bool b = true;

	if (!m_Devices.empty() && Renderer<T, bucketT>::CreateSpatialFilter(newAlloc))
	{
		if (newAlloc)
			if (!(b = m_Devices[0]->m_Wrapper.AddAndWriteBuffer(m_SpatialFilterCoefsBufferName, reinterpret_cast<void*>(m_SpatialFilter->Filter()), m_SpatialFilter->BufferSizeBytes()))) { AddToReport(__FUNCTION__); }
	}
	else
		b = false;

	return b;
}

/// <summary>
/// Get the renderer type enum.
/// </summary>
/// <returns>OPENCL_RENDERER</returns>
template <typename T, typename bucketT>
eRendererType RendererCL<T, bucketT>::RendererType() const
{
	return OPENCL_RENDERER;
}

/// <summary>
/// Concatenate and return the error report for this class and the
/// OpenCLWrapper member of each device as a single string.
/// </summary>
/// <returns>The concatenated error report string</returns>
template <typename T, typename bucketT>
string RendererCL<T, bucketT>::ErrorReportString()
{
	auto s = EmberReport::ErrorReportString();

	for (auto& device : m_Devices)
		s += device->m_Wrapper.ErrorReportString();

	return s;
}

/// <summary>
/// Concatenate and return the error report for this class and the
/// OpenCLWrapper member of each device as a vector of strings.
/// </summary>
/// <returns>The concatenated error report vector of strings</returns>
template <typename T, typename bucketT>
vector<string> RendererCL<T, bucketT>::ErrorReport()
{
	auto ours = EmberReport::ErrorReport();

	for (auto& device : m_Devices)
	{
		auto s = device->m_Wrapper.ErrorReport();
		ours.insert(ours.end(), s.begin(), s.end());
	}

	return ours;
}

/// <summary>
/// Set the vector of random contexts on every device.
/// Call the base, and reset the seeds vector.
/// Used on the command line when the user wants a specific set of seeds to start with to
/// produce an exact result. Mostly for debugging.
/// </summary>
/// <param name="randVec">The vector of random contexts to assign</param>
/// <returns>True if the size of the vector matched the number of threads used for rendering and writing seeds to OpenCL succeeded, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::RandVec(vector<QTIsaac<ISAAC_SIZE, ISAAC_INT>>& randVec)
{
	bool b = Renderer<T, bucketT>::RandVec(randVec);
	const char* loc = __FUNCTION__;

	if (!m_Devices.empty())
	{
		FillSeeds();

		for (size_t device = 0; device < m_Devices.size(); device++)
			if (b && !(b = m_Devices[device]->m_Wrapper.AddAndWriteBuffer(m_SeedsBufferName, reinterpret_cast<void*>(m_Seeds[device].data()), SizeOf(m_Seeds[device])))) { AddToReport(loc); break; }
	}
	else
		b = false;

	return b;
}

/// <summary>
/// Protected virtual functions overridden from Renderer.
/// </summary>

/// <summary>
/// Allocate all buffers required for running as well as the final
/// 2D image.
/// Note that only iteration-related buffers are allocated on secondary devices.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::Alloc(bool histOnly)
{
	if (!Ok())
		return false;

	EnterResize();
	m_XformsCL.resize(m_Ember.TotalXformCount());
	bool b = true;
	size_t histLength = SuperSize() * sizeof(v4bT);
	size_t accumLength = SuperSize() * sizeof(v4bT);
	const char* loc = __FUNCTION__;
	auto& wrapper = m_Devices[0]->m_Wrapper;

	if (b && !(b = wrapper.AddBuffer(m_DEFilterParamsBufferName, sizeof(m_DensityFilterCL))))		 { AddToReport(loc); }

	if (b && !(b = wrapper.AddBuffer(m_SpatialFilterParamsBufferName, sizeof(m_SpatialFilterCL))))	 { AddToReport(loc); }

	if (b && !(b = wrapper.AddBuffer(m_CurvesCsaName, SizeOf(m_Csa.m_Entries))))					 { AddToReport(loc); }

	if (b && !(b = wrapper.AddBuffer(m_AccumBufferName, accumLength)))								 { AddToReport(loc); }//Accum buffer.

	for (auto& device : m_Devices)
	{
		if (b && !(b = device->m_Wrapper.AddBuffer(m_EmberBufferName, sizeof(m_EmberCL))))							 { AddToReport(loc); break; }

		if (b && !(b = device->m_Wrapper.AddBuffer(m_XformsBufferName, SizeOf(m_XformsCL))))						 { AddToReport(loc); break; }

		if (b && !(b = device->m_Wrapper.AddBuffer(m_ParVarsBufferName, 128 * sizeof(T))))							 { AddToReport(loc); break; }

		if (b && !(b = device->m_Wrapper.AddBuffer(m_DistBufferName, CHOOSE_XFORM_GRAIN)))							 { AddToReport(loc); break; }//Will be resized for xaos.

		if (b && !(b = device->m_Wrapper.AddBuffer(m_CarToRasBufferName, sizeof(m_CarToRasCL))))					 { AddToReport(loc); break; }

		if (b && !(b = device->m_Wrapper.AddBuffer(m_HistBufferName, histLength)))									 { AddToReport(loc); break; }//Histogram. Will memset to zero later.

		if (b && !(b = device->m_Wrapper.AddBuffer(m_PointsBufferName, IterGridKernelCount() * sizeof(PointCL<T>)))) { AddToReport(loc); break; }//Points between iter calls.

		//Global shared is allocated once and written when building the kernel.
	}

	LeaveResize();

	if (b && !(b = SetOutputTexture(m_OutputTexID))) { AddToReport(loc); }

	return b;
}

/// <summary>
/// Clear OpenCL histogram on all devices and/or density filtering buffer on the primary device to all zeroes.
/// </summary>
/// <param name="resetHist">Clear histogram if true, else don't.</param>
/// <param name="resetAccum">Clear density filtering buffer if true, else don't.</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ResetBuckets(bool resetHist, bool resetAccum)
{
	bool b = true;

	if (resetHist)
		b &= ClearHist();

	if (resetAccum)
		b &= ClearAccum();

	return b;
}

/// <summary>
/// Perform log scale density filtering on the primary device.
/// </summary>
/// <param name="forceOutput">Whether this output was forced due to an interactive render</param>
/// <returns>True if success and not aborted, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::LogScaleDensityFilter(bool forceOutput)
{
	return RunLogScaleFilter();
}

/// <summary>
/// Run gaussian density estimation filtering on the primary device.
/// </summary>
/// <returns>True if success and not aborted, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::GaussianDensityFilter()
{
	//This commented section is for debugging density filtering by making it run on the CPU
	//then copying the results back to the GPU.
	//if (ReadHist())
	//{
	//	uint accumLength = SuperSize() * sizeof(glm::detail::tvec4<T>);
	//	const char* loc = __FUNCTION__;
	//
	//	Renderer<T, bucketT>::ResetBuckets(false, true);
	//	Renderer<T, bucketT>::GaussianDensityFilter();
	//
	//	if (!m_Wrapper.WriteBuffer(m_AccumBufferName, AccumulatorBuckets(), accumLength)) { AddToReport(loc); return RENDER_ERROR; }
	//		return RENDER_OK;
	//}
	//else
	//	return RENDER_ERROR;
	//Timing t(4);
	eRenderStatus status = RunDensityFilter();
	//t.Toc(__FUNCTION__ " RunKernel()");
	return status;
}

/// <summary>
/// Run final accumulation on the primary device.
/// If pixels is nullptr, the output will remain in the OpenCL 2D image.
/// However, if pixels is not nullptr, the output will be copied. This is
/// useful when rendering in OpenCL, but saving the output to a file.
/// </summary>
/// <param name="pixels">The pixels to copy the final image to if not nullptr</param>
/// <param name="finalOffset">Offset in the buffer to store the pixels to</param>
/// <returns>True if success and not aborted, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::AccumulatorToFinalImage(byte* pixels, size_t finalOffset)
{
	auto status = RunFinalAccum();

	if (status == eRenderStatus::RENDER_OK && pixels && !m_Devices.empty() && !m_Devices[0]->m_Wrapper.Shared())
	{
		pixels += finalOffset;

		if (!ReadFinal(pixels))
			status = eRenderStatus::RENDER_ERROR;
	}

	return status;
}

/// <summary>
/// Run the iteration algorithm for the specified number of iterations, splitting the work
/// across devices.
/// This is only called after all other setup has been done.
/// This will recompile the OpenCL program on every device if this ember differs significantly
/// from the previous run.
/// Note that the bad value count is not recorded when running with OpenCL. If it's
/// needed, run on the CPU.
/// </summary>
/// <param name="iterCount">The number of iterations to run</param>
/// <param name="temporalSample">The temporal sample within the current pass this is running for</param>
/// <returns>Rendering statistics</returns>
template <typename T, typename bucketT>
EmberStats RendererCL<T, bucketT>::Iterate(size_t iterCount, size_t temporalSample)
{
	bool b = true;
	EmberStats stats;//Do not record bad vals with with GPU. If the user needs to investigate bad vals, use the CPU.
	const char* loc = __FUNCTION__;

	//Only need to do this once on the beginning of a new render. Last iter will always be 0 at the beginning of a full render or temporal sample.
	if (m_LastIter == 0)
	{
		ConvertEmber(m_Ember, m_EmberCL, m_XformsCL);
		ConvertCarToRas(CoordMap());

		//Rebuilding is expensive, so only do it if it's required.
		if (IterOpenCLKernelCreator<T>::IsBuildRequired(m_Ember, m_LastBuiltEmber))
			b = BuildIterProgramForEmber(true);

		if (b)
		{
			//Setup buffers on all devices.
			for (auto& device : m_Devices)
			{
				auto& wrapper = device->m_Wrapper;

				if (b && !(b = wrapper.WriteBuffer(m_EmberBufferName, reinterpret_cast<void*>(&m_EmberCL), sizeof(m_EmberCL))))
					break;

				if (b && !(b = wrapper.WriteBuffer(m_XformsBufferName, reinterpret_cast<void*>(m_XformsCL.data()), sizeof(m_XformsCL[0]) * m_XformsCL.size())))
					break;

				if (b && !(b = wrapper.AddAndWriteBuffer(m_DistBufferName, reinterpret_cast<void*>(const_cast<byte*>(XformDistributions())), XformDistributionsSize())))//Will be resized for xaos.
					break;

				if (b && !(b = wrapper.WriteBuffer(m_CarToRasBufferName, reinterpret_cast<void*>(&m_CarToRasCL), sizeof(m_CarToRasCL))))
					break;

				if (b && !(b = wrapper.AddAndWriteImage("Palette", CL_MEM_READ_ONLY, m_PaletteFormat, m_Dmap.m_Entries.size(), 1, 0, m_Dmap.m_Entries.data())))
					break;

				if (b)
				{
					IterOpenCLKernelCreator<T>::ParVarIndexDefines(m_Ember, m_Params, true, false);//Always do this to get the values (but no string), regardless of whether a rebuild is necessary.

					//Don't know the size of the parametric varations parameters buffer until the ember is examined.
					//So set it up right before the run.
					if (!m_Params.second.empty())
						if (!wrapper.AddAndWriteBuffer(m_ParVarsBufferName, m_Params.second.data(), m_Params.second.size() * sizeof(m_Params.second[0])))
							break;
				}
				else
					break;
			}
		}
	}

	if (b)
	{
		m_IterTimer.Tic();//Tic() here to avoid including build time in iter time measurement.

		if (m_LastIter == 0 && m_ProcessAction != eProcessAction::KEEP_ITERATING)//Only reset the call count on the beginning of a new render. Do not reset on KEEP_ITERATING.
			for (auto& dev : m_Devices)
				dev->m_Calls = 0;

		b = RunIter(iterCount, temporalSample, stats.m_Iters);

		if (!b || stats.m_Iters == 0)//If no iters were executed, something went catastrophically wrong.
			m_Abort = true;

		stats.m_IterMs = m_IterTimer.Toc();
	}
	else
	{
		m_Abort = true;
		AddToReport(loc);
	}

	return stats;
}

/// <summary>
/// Private functions for making and running OpenCL programs.
/// </summary>

/// <summary>
/// Build the iteration program on every device for the current ember.
/// This is parallelized by placing the build for each device on its own thread.
/// </summary>
/// <param name="doAccum">Whether to build in accumulation, only for debugging. Default: true.</param>
/// <returns>True if successful for all devices, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::BuildIterProgramForEmber(bool doAccum)
{
	//Timing t;
	bool b = !m_Devices.empty();
	const char* loc = __FUNCTION__;
	IterOpenCLKernelCreator<T>::ParVarIndexDefines(m_Ember, m_Params, false, true);//Do with string and no vals.
	IterOpenCLKernelCreator<T>::SharedDataIndexDefines(m_Ember, m_GlobalShared, true, true);//Do with string and vals only once on build since it won't change until another build occurs.

	if (b)
	{
		m_IterKernel = m_IterOpenCLKernelCreator.CreateIterKernelString(m_Ember, m_Params.first, m_GlobalShared.first, m_LockAccum, doAccum);
		//cout << "Building: " << endl << iterProgram << endl;
		vector<std::thread> threads;
		std::function<void(RendererClDevice*)> func = [&](RendererClDevice * dev)
		{
			if (!dev->m_Wrapper.AddProgram(m_IterOpenCLKernelCreator.IterEntryPoint(), m_IterKernel, m_IterOpenCLKernelCreator.IterEntryPoint(), m_DoublePrecision))
			{
				m_ResizeCs.Enter();//Just use the resize CS for lack of a better one.
				b = false;
				AddToReport(string(loc) + "()\n" + dev->m_Wrapper.DeviceName() + ":\nBuilding the following program failed: \n" + m_IterKernel + "\n");
				m_ResizeCs.Leave();
			}
			else if (!m_GlobalShared.second.empty())
			{
				if (!dev->m_Wrapper.AddAndWriteBuffer(m_GlobalSharedBufferName, m_GlobalShared.second.data(), m_GlobalShared.second.size() * sizeof(m_GlobalShared.second[0])))
				{
					m_ResizeCs.Enter();//Just use the resize CS for lack of a better one.
					b = false;
					AddToReport(string(loc) + "()\n" + dev->m_Wrapper.DeviceName() + ":\nAdding global shared buffer failed.\n");
					m_ResizeCs.Leave();
				}
			}
		};
		threads.reserve(m_Devices.size() - 1);

		for (size_t device = m_Devices.size() - 1; device >= 0 && device < m_Devices.size(); device--)//Check both extents because size_t will wrap.
		{
			if (!device)//Secondary devices on their own threads.
				threads.push_back(std::thread([&](RendererClDevice * dev) { func(dev); }, m_Devices[device].get()));
			else//Primary device on this thread.
				func(m_Devices[device].get());
		}

		for (auto& th : threads)
			if (th.joinable())
				th.join();

		if (b)
		{
			//t.Toc(__FUNCTION__ " program build");
			//cout << string(loc) << "():\nBuilding the following program succeeded: \n" << iterProgram << endl;
			m_LastBuiltEmber = m_Ember;
		}
	}

	return b;
}

/// <summary>
/// Run the iteration kernel on all devices.
/// Fusing on the CPU is done once per sub batch, usually 10,000 iters. Here,
/// the same fusing frequency is kept, but is done per kernel thread.
/// </summary>
/// <param name="iterCount">The number of iterations to run</param>
/// <param name="temporalSample">The temporal sample this is running for</param>
/// <param name="itersRan">The storage for the number of iterations ran</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::RunIter(size_t iterCount, size_t temporalSample, size_t& itersRan)
{
	//Timing t;//, t2(4);
	bool success = !m_Devices.empty();
	uint histSuperSize = uint(SuperSize());
	size_t launches = size_t(ceil(double(iterCount) / IterCountPerGrid()));
	const char* loc = __FUNCTION__;
	vector<std::thread> threadVec;
	std::atomic<size_t> atomLaunchesRan;
	std::atomic<intmax_t> atomItersRan, atomItersRemaining;
	size_t adjustedIterCountPerKernel = m_IterCountPerKernel;
	itersRan = 0;
	atomItersRan.store(0);
	atomItersRemaining.store(iterCount);
	atomLaunchesRan.store(0);
	threadVec.reserve(m_Devices.size());

	//If a very small number of iters is requested, and multiple devices
	//are present, then try to spread the launches over the devices.
	//Otherwise, only one device would get used.
	//Note that this can lead to doing a few more iterations than requested
	//due to rounding up to ~32k kernel threads per launch.
	if (m_Devices.size() >= launches)
	{
		launches = m_Devices.size();
		adjustedIterCountPerKernel = size_t(ceil(ceil(double(iterCount) / m_Devices.size()) / IterGridKernelCount()));
	}

	size_t fuseFreq = Renderer<T, bucketT>::SubBatchSize() / adjustedIterCountPerKernel;//Use the base sbs to determine when to fuse.
#ifdef TEST_CL
	m_Abort = false;
#endif
	std::function<void(size_t, int)> iterFunc = [&](size_t dev, int kernelIndex)
	{
		bool b = true;
		auto& wrapper = m_Devices[dev]->m_Wrapper;
		intmax_t itersRemaining;

		while (atomLaunchesRan.fetch_add(1), (b && (atomLaunchesRan.load() <= launches) && ((itersRemaining = atomItersRemaining.load()) > 0) && !m_Abort))
		{
			cl_uint argIndex = 0;
#ifdef TEST_CL
			uint fuse = 0;
#else
			uint fuse = uint((m_Devices[dev]->m_Calls % fuseFreq) == 0u ? FuseCount() : 0u);
#endif
			//Similar to what's done in the base class.
			//The number of iters per thread must be adjusted if they've requested less iters than is normally ran in a grid (256 * 256 * 64 * 2 = 32,768).
			uint iterCountPerKernel = std::min<uint>(uint(adjustedIterCountPerKernel), uint(ceil(double(itersRemaining) / IterGridKernelCount())));
			size_t iterCountThisLaunch = iterCountPerKernel * IterGridKernelWidth() * IterGridKernelHeight();
			//cout << "itersRemaining " << itersRemaining << ", iterCountPerKernel " << iterCountPerKernel << ", iterCountThisLaunch " << iterCountThisLaunch << endl;

			if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, iterCountPerKernel)))        { AddToReport(loc); }//Number of iters for each thread to run.

			if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, fuse)))                      { AddToReport(loc); }//Number of iters to fuse.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_SeedsBufferName)))         { AddToReport(loc); }//Seeds.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_EmberBufferName)))         { AddToReport(loc); }//Ember.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_XformsBufferName)))        { AddToReport(loc); }//Xforms.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_ParVarsBufferName)))       { AddToReport(loc); }//Parametric variation parameters.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_GlobalSharedBufferName)))  { AddToReport(loc); }//Global shared data.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_DistBufferName)))          { AddToReport(loc); }//Xform distributions.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_CarToRasBufferName)))      { AddToReport(loc); }//Coordinate converter.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_HistBufferName)))          { AddToReport(loc); }//Histogram.

			if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, histSuperSize)))		      { AddToReport(loc); }//Histogram size.

			if (b && !(b = wrapper.SetImageArg (kernelIndex, argIndex++, false, "Palette")))          { AddToReport(loc); }//Palette.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_PointsBufferName)))        { AddToReport(loc); }//Random start points.

			if (b && !(b = wrapper.RunKernel(kernelIndex,
											 IterGridKernelWidth(),//Total grid dims.
											 IterGridKernelHeight(),
											 1,
											 IterBlockKernelWidth(),//Individual block dims.
											 IterBlockKernelHeight(),
											 1)))
			{
				success = false;
				m_Abort = true;
				AddToReport(loc);
				atomLaunchesRan.fetch_sub(1);
				break;
			}

			atomItersRan.fetch_add(iterCountThisLaunch);
			atomItersRemaining.store(iterCount - atomItersRan.load());
			m_Devices[dev]->m_Calls++;

			if (m_Callback && !dev)//Will only do callback on the first device, however it will report the progress of all devices.
			{
				double percent = 100.0 *
								 double
								 (
									 double
									 (
										 double
										 (
											 double(m_LastIter + atomItersRan.load()) / double(ItersPerTemporalSample())
										 ) + temporalSample
									 ) / double(TemporalSamples())
								 );
				double percentDiff = percent - m_LastIterPercent;
				double toc = m_ProgressTimer.Toc();

				if (percentDiff >= 10 || (toc > 1000 && percentDiff >= 1))//Call callback function if either 10% has passed, or one second (and 1%).
				{
					double etaMs = ((100.0 - percent) / percent) * m_RenderTimer.Toc();

					if (!m_Callback->ProgressFunc(m_Ember, m_ProgressParameter, percent, 0, etaMs))
						Abort();

					m_LastIterPercent = percent;
					m_ProgressTimer.Tic();
				}
			}
		}
	};

	//Iterate backward to run all secondary devices on threads first, then finally the primary device on this thread.
	for (size_t device = m_Devices.size() - 1; device >= 0 && device < m_Devices.size(); device--)//Check both extents because size_t will wrap.
	{
		int index = m_Devices[device]->m_Wrapper.FindKernelIndex(m_IterOpenCLKernelCreator.IterEntryPoint());

		if (index == -1)
		{
			success = false;
			break;
		}

		//If animating, treat each temporal sample as a newly started render for fusing purposes.
		if (temporalSample > 0)
			m_Devices[device]->m_Calls = 0;

		if (device != 0)//Secondary devices on their own threads.
			threadVec.push_back(std::thread([&](size_t dev, int kernelIndex) { iterFunc(dev, kernelIndex); }, device, index));
		else//Primary device on this thread.
			iterFunc(device, index);
	}

	for (auto& th : threadVec)
		if (th.joinable())
			th.join();

	itersRan = atomItersRan.load();

	if (m_Devices.size() > 1)//Determine whether/when to sum histograms of secondary devices with the primary.
	{
		if (((TemporalSamples() == 1) || (temporalSample == TemporalSamples() - 1)) &&//If there are no temporal samples (not animating), or the current one is the last...
				((m_LastIter + itersRan) >= ItersPerTemporalSample()))//...and the required number of iters for that sample have completed...
			if (success && !(success = SumDeviceHist())) { AddToReport(loc); }//...read the histogram from the secondary devices and sum them to the primary.
	}

	//t2.Toc(__FUNCTION__);
	return success;
}

/// <summary>
/// Run the log scale filter on the primary device.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::RunLogScaleFilter()
{
	//Timing t(4);
	bool b = !m_Devices.empty();

	if (b)
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		int kernelIndex = wrapper.FindKernelIndex(m_DEOpenCLKernelCreator.LogScaleAssignDEEntryPoint());
		const char* loc = __FUNCTION__;

		if (kernelIndex != -1)
		{
			ConvertDensityFilter();
			cl_uint argIndex = 0;
			size_t blockW = m_Devices[0]->WarpSize();
			size_t blockH = 4;//A height of 4 seems to run the fastest.
			size_t gridW = m_DensityFilterCL.m_SuperRasW;
			size_t gridH = m_DensityFilterCL.m_SuperRasH;
			OpenCLWrapper::MakeEvenGridDims(blockW, blockH, gridW, gridH);

			if (b && !(b = wrapper.AddAndWriteBuffer(m_DEFilterParamsBufferName, reinterpret_cast<void*>(&m_DensityFilterCL), sizeof(m_DensityFilterCL)))) { AddToReport(loc); }

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_HistBufferName)))           { AddToReport(loc); }//Histogram.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_AccumBufferName)))          { AddToReport(loc); }//Accumulator.

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_DEFilterParamsBufferName))) { AddToReport(loc); }//DensityFilterCL.

			//t.Tic();
			if (b && !(b = wrapper.RunKernel(kernelIndex, gridW, gridH, 1, blockW, blockH, 1))) { AddToReport(loc); }

			//t.Toc(loc);
		}
		else
		{
			b = false;
			AddToReport(loc);
		}

		if (b && m_Callback && m_LastIterPercent >= 99.0)//Only update progress if we've really reached the end, not via forced output.
			m_Callback->ProgressFunc(m_Ember, m_ProgressParameter, 100.0, 1, 0.0);
	}

	return b ? eRenderStatus::RENDER_OK : eRenderStatus::RENDER_ERROR;
}

/// <summary>
/// Run the Gaussian density filter on the primary device.
/// Method 7: Each block processes a 16x16(AMD) or 24x24(Nvidia) block and exits. No column or row advancements happen.
/// </summary>
/// <returns>True if success and not aborted, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::RunDensityFilter()
{
	bool b = !m_Devices.empty();
	Timing t(4);// , t2(4);
	ConvertDensityFilter();
	int kernelIndex = MakeAndGetDensityFilterProgram(Supersample(), m_DensityFilterCL.m_FilterWidth);
	const char* loc = __FUNCTION__;

	if (kernelIndex != -1)
	{
		uint leftBound  = m_DensityFilterCL.m_Supersample - 1;
		uint rightBound = m_DensityFilterCL.m_SuperRasW - (m_DensityFilterCL.m_Supersample - 1);
		uint topBound   = leftBound;
		uint botBound   = m_DensityFilterCL.m_SuperRasH - (m_DensityFilterCL.m_Supersample - 1);
		size_t gridW      = rightBound - leftBound;
		size_t gridH      = botBound - topBound;
		size_t blockSizeW = m_MaxDEBlockSizeW;//These *must* both be divisible by 16 or else pixels will go missing.
		size_t blockSizeH = m_MaxDEBlockSizeH;
		auto& wrapper = m_Devices[0]->m_Wrapper;

		//OpenCL runs out of resources when using double or a supersample of 2.
		//Remedy this by reducing the height of the block by 2.
		if (m_DoublePrecision || m_DensityFilterCL.m_Supersample > 1)
			blockSizeH -= 2;

		//Can't just blindly pass dimension in vals. Must adjust them first to evenly divide the block count
		//into the total grid dimensions.
		OpenCLWrapper::MakeEvenGridDims(blockSizeW, blockSizeH, gridW, gridH);
		//t.Tic();
		//The classic problem with performing DE on adjacent pixels is that the filter will overlap.
		//This can be solved in 2 ways. One is to use atomics, which is unacceptably slow.
		//The other is to proces the entire image in multiple passes, and each pass processes blocks of pixels
		//that are far enough apart such that their filters do not overlap.
		//Do the latter.
		//Gap is in terms of blocks. How many blocks must separate two blocks running at the same time.
		uint gapW = uint(ceil((m_DensityFilterCL.m_FilterWidth * 2.0) / double(blockSizeW)));
		uint chunkSizeW = gapW + 1;
		uint gapH = uint(ceil((m_DensityFilterCL.m_FilterWidth * 2.0) / double(blockSizeH)));
		uint chunkSizeH = gapH + 1;
		double totalChunks = chunkSizeW * chunkSizeH;

		if (b && !(b = wrapper.AddAndWriteBuffer(m_DEFilterParamsBufferName, reinterpret_cast<void*>(&m_DensityFilterCL), sizeof(m_DensityFilterCL)))) { AddToReport(loc); }

#ifdef ROW_ONLY_DE
		blockSizeW = 64;//These *must* both be divisible by 16 or else pixels will go missing.
		blockSizeH = 1;
		gapW = (uint)ceil((m_DensityFilterCL.m_FilterWidth * 2.0) / (double)blockSizeW);
		chunkSizeW = gapW + 1;
		gapH = (uint)ceil((m_DensityFilterCL.m_FilterWidth * 2.0) / (double)32);//Block height is 1, but iterates over 32 rows.
		chunkSizeH = gapH + 1;
		totalChunks = chunkSizeW * chunkSizeH;
		OpenCLWrapper::MakeEvenGridDims(blockSizeW, blockSizeH, gridW, gridH);
		gridW /= chunkSizeW;
		gridH /= chunkSizeH;

		for (uint rowChunk = 0; b && !m_Abort && rowChunk < chunkSizeH; rowChunk++)
		{
			for (uint colChunk = 0; b && !m_Abort && colChunk < chunkSizeW; colChunk++)
			{
				//t2.Tic();
				if (b && !(b = RunDensityFilterPrivate(kernelIndex, gridW, gridH, blockSizeW, blockSizeH, chunkSizeW, chunkSizeH, colChunk, rowChunk))) { m_Abort = true; AddToReport(loc); }

				//t2.Toc(loc);

				if (b && m_Callback)
				{
					double percent = (double((rowChunk * chunkSizeW) + (colChunk + 1)) / totalChunks) * 100.0;
					double etaMs = ((100.0 - percent) / percent) * t.Toc();

					if (!m_Callback->ProgressFunc(m_Ember, m_ProgressParameter, percent, 1, etaMs))
						Abort();
				}
			}
		}

#else
		gridW /= chunkSizeW;
		gridH /= chunkSizeH;
		OpenCLWrapper::MakeEvenGridDims(blockSizeW, blockSizeH, gridW, gridH);

		for (uint rowChunk = 0; b && !m_Abort && rowChunk < chunkSizeH; rowChunk++)
		{
			for (uint colChunk = 0; b && !m_Abort && colChunk < chunkSizeW; colChunk++)
			{
				//t2.Tic();
				if (b && !(b = RunDensityFilterPrivate(kernelIndex, gridW, gridH, blockSizeW, blockSizeH, chunkSizeW, chunkSizeH, colChunk, rowChunk))) { m_Abort = true; AddToReport(loc); }

				//t2.Toc(loc);

				if (b && m_Callback)
				{
					double percent = (double((rowChunk * chunkSizeW) + (colChunk + 1)) / totalChunks) * 100.0;
					double etaMs = ((100.0 - percent) / percent) * t.Toc();

					if (!m_Callback->ProgressFunc(m_Ember, m_ProgressParameter, percent, 1, etaMs))
						Abort();
				}
			}
		}

#endif

		if (b && m_Callback)
			m_Callback->ProgressFunc(m_Ember, m_ProgressParameter, 100.0, 1, 0.0);

		//t2.Toc(__FUNCTION__ " all passes");
	}
	else
	{
		b = false;
		AddToReport(loc);
	}

	return m_Abort ? eRenderStatus::RENDER_ABORT : (b ? eRenderStatus::RENDER_OK : eRenderStatus::RENDER_ERROR);
}

/// <summary>
/// Run final accumulation to the 2D output image on the primary device.
/// </summary>
/// <returns>True if success and not aborted, else false.</returns>
template <typename T, typename bucketT>
eRenderStatus RendererCL<T, bucketT>::RunFinalAccum()
{
	//Timing t(4);
	bool b = true;
	double alphaBase;
	double alphaScale;
	int accumKernelIndex = MakeAndGetFinalAccumProgram(alphaBase, alphaScale);
	cl_uint argIndex;
	size_t gridW;
	size_t gridH;
	size_t blockW;
	size_t blockH;
	uint curvesSet = m_CurvesSet ? 1 : 0;
	const char* loc = __FUNCTION__;

	if (!m_Abort && accumKernelIndex != -1)
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		//This is needed with or without early clip.
		ConvertSpatialFilter();

		if (b && !(b = wrapper.AddAndWriteBuffer(m_SpatialFilterParamsBufferName, reinterpret_cast<void*>(&m_SpatialFilterCL), sizeof(m_SpatialFilterCL)))) { AddToReport(loc); }

		if (b && !(b = wrapper.AddAndWriteBuffer(m_CurvesCsaName,				  m_Csa.m_Entries.data(),					   SizeOf(m_Csa.m_Entries))))   { AddToReport(loc); }

		//Since early clip requires gamma correcting the entire accumulator first,
		//it can't be done inside of the normal final accumulation kernel, so
		//an additional kernel must be launched first.
		if (b && EarlyClip())
		{
			int gammaCorrectKernelIndex = MakeAndGetGammaCorrectionProgram();

			if (gammaCorrectKernelIndex != -1)
			{
				argIndex = 0;
				blockW = m_Devices[0]->WarpSize();
				blockH = 4;//A height of 4 seems to run the fastest.
				gridW = m_SpatialFilterCL.m_SuperRasW;//Using super dimensions because this processes the density filtering bufer.
				gridH = m_SpatialFilterCL.m_SuperRasH;
				OpenCLWrapper::MakeEvenGridDims(blockW, blockH, gridW, gridH);

				if (b && !(b = wrapper.SetBufferArg(gammaCorrectKernelIndex, argIndex++, m_AccumBufferName)))               { AddToReport(loc); }//Accumulator.

				if (b && !(b = wrapper.SetBufferArg(gammaCorrectKernelIndex, argIndex++, m_SpatialFilterParamsBufferName))) { AddToReport(loc); }//SpatialFilterCL.

				if (b && !(b = wrapper.RunKernel(gammaCorrectKernelIndex, gridW, gridH, 1, blockW, blockH, 1)))			  { AddToReport(loc); }
			}
			else
			{
				b = false;
				AddToReport(loc);
			}
		}

		argIndex = 0;
		blockW = m_Devices[0]->WarpSize();
		blockH = 4;//A height of 4 seems to run the fastest.
		gridW = m_SpatialFilterCL.m_FinalRasW;
		gridH = m_SpatialFilterCL.m_FinalRasH;
		OpenCLWrapper::MakeEvenGridDims(blockW, blockH, gridW, gridH);

		if (b && !(b = wrapper.SetBufferArg(accumKernelIndex, argIndex++, m_AccumBufferName)))                  { AddToReport(loc); }//Accumulator.

		if (b && !(b = wrapper.SetImageArg(accumKernelIndex,  argIndex++, wrapper.Shared(), m_FinalImageName)))	{ AddToReport(loc); }//Final image.

		if (b && !(b = wrapper.SetBufferArg(accumKernelIndex, argIndex++, m_SpatialFilterParamsBufferName)))    { AddToReport(loc); }//SpatialFilterCL.

		if (b && !(b = wrapper.SetBufferArg(accumKernelIndex, argIndex++, m_SpatialFilterCoefsBufferName)))     { AddToReport(loc); }//Filter coefs.

		if (b && !(b = wrapper.SetBufferArg(accumKernelIndex, argIndex++, m_CurvesCsaName)))					{ AddToReport(loc); }//Curve points.

		if (b && !(b = wrapper.SetArg	   (accumKernelIndex, argIndex++, curvesSet)))                          { AddToReport(loc); }//Do curves.

		if (b && !(b = wrapper.SetArg	   (accumKernelIndex, argIndex++, bucketT(alphaBase))))                 { AddToReport(loc); }//Alpha base.

		if (b && !(b = wrapper.SetArg	   (accumKernelIndex, argIndex++, bucketT(alphaScale))))                { AddToReport(loc); }//Alpha scale.

		if (b && wrapper.Shared())
			if (b && !(b = wrapper.EnqueueAcquireGLObjects(m_FinalImageName))) { AddToReport(loc); }

		if (b && !(b = wrapper.RunKernel(accumKernelIndex, gridW, gridH, 1, blockW, blockH, 1))) { AddToReport(loc); }

		if (b && wrapper.Shared())
			if (b && !(b = wrapper.EnqueueReleaseGLObjects(m_FinalImageName))) { AddToReport(loc); }

		//t.Toc((char*)loc);
	}
	else
	{
		b = false;
		AddToReport(loc);
	}

	return b ? eRenderStatus::RENDER_OK : eRenderStatus::RENDER_ERROR;
}

/// <summary>
/// Zeroize a buffer of the specified size on the specified device.
/// </summary>
/// <param name="device">The index in the device buffer to clear</param>
/// <param name="bufferName">Name of the buffer to clear</param>
/// <param name="width">Width in elements</param>
/// <param name="height">Height in elements</param>
/// <param name="elementSize">Size of each element</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::ClearBuffer(size_t device, const string& bufferName, uint width, uint height, uint elementSize)
{
	bool b = false;

	if (device < m_Devices.size())
	{
		auto& wrapper = m_Devices[device]->m_Wrapper;
		int kernelIndex = wrapper.FindKernelIndex(m_IterOpenCLKernelCreator.ZeroizeEntryPoint());
		cl_uint argIndex = 0;
		const char* loc = __FUNCTION__;

		if (kernelIndex != -1)
		{
			size_t blockW = m_Devices[device]->Nvidia() ? 32 : 16;//Max work group size is 256 on AMD, which means 16x16.
			size_t blockH = m_Devices[device]->Nvidia() ? 32 : 16;
			size_t gridW = width * elementSize;
			size_t gridH = height;
			b = true;
			OpenCLWrapper::MakeEvenGridDims(blockW, blockH, gridW, gridH);

			if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, bufferName)))          { AddToReport(loc); }//Buffer of byte.

			if (b && !(b = wrapper.SetArg(kernelIndex, argIndex++, width * elementSize)))		{ AddToReport(loc); }//Width.

			if (b && !(b = wrapper.SetArg(kernelIndex, argIndex++, height)))					{ AddToReport(loc); }//Height.

			if (b && !(b = wrapper.RunKernel(kernelIndex, gridW, gridH, 1, blockW, blockH, 1))) { AddToReport(loc); }
		}
		else
		{
			AddToReport(loc);
		}
	}

	return b;
}

/// <summary>
/// Private wrapper around calling Gaussian density filtering kernel.
/// The parameters are very specific to how the kernel is internally implemented.
/// </summary>
/// <param name="kernelIndex">Index of the kernel to call</param>
/// <param name="gridW">Grid width</param>
/// <param name="gridH">Grid height</param>
/// <param name="blockW">Block width</param>
/// <param name="blockH">Block height</param>
/// <param name="chunkSizeW">Chunk size width (gapW + 1)</param>
/// <param name="chunkSizeH">Chunk size height (gapH + 1)</param>
/// <param name="rowParity">Row parity</param>
/// <param name="colParity">Column parity</param>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::RunDensityFilterPrivate(size_t kernelIndex, size_t gridW, size_t gridH, size_t blockW, size_t blockH, uint chunkSizeW, uint chunkSizeH, uint chunkW, uint chunkH)
{
	//Timing t(4);
	bool b = true;
	cl_uint argIndex = 0;
	const char* loc = __FUNCTION__;

	if (!m_Devices.empty())
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_HistBufferName)))           { AddToReport(loc); } argIndex++;//Histogram.

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_AccumBufferName)))          { AddToReport(loc); } argIndex++;//Accumulator.

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_DEFilterParamsBufferName))) { AddToReport(loc); } argIndex++;//FlameDensityFilterCL.

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_DECoefsBufferName)))        { AddToReport(loc); } argIndex++;//Coefs.

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_DEWidthsBufferName)))       { AddToReport(loc); } argIndex++;//Widths.

		if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex, m_DECoefIndicesBufferName)))  { AddToReport(loc); } argIndex++;//Coef indices.

		if (b && !(b = wrapper.SetArg(kernelIndex, argIndex, chunkSizeW)))						 { AddToReport(loc); } argIndex++;//Chunk size width (gapW + 1).

		if (b && !(b = wrapper.SetArg(kernelIndex, argIndex, chunkSizeH)))						 { AddToReport(loc); } argIndex++;//Chunk size height (gapH + 1).

		if (b && !(b = wrapper.SetArg(kernelIndex, argIndex, chunkW)))							 { AddToReport(loc); } argIndex++;//Column chunk.

		if (b && !(b = wrapper.SetArg(kernelIndex, argIndex, chunkH)))							 { AddToReport(loc); } argIndex++;//Row chunk.

		//t.Toc(__FUNCTION__ " set args");

		//t.Tic();
		if (b && !(b = wrapper.RunKernel(kernelIndex, gridW, gridH, 1, blockW, blockH, 1))) { AddToReport(loc); }//Method 7, accumulating to temp box area.

		//t.Toc(__FUNCTION__ " RunKernel()");
		return b;
	}

	return false;
}

/// <summary>
/// Make the Gaussian density filter program on the primary device and return its index.
/// </summary>
/// <param name="ss">The supersample being used for the current ember</param>
/// <param name="filterWidth">Width of the gaussian filter</param>
/// <returns>The kernel index if successful, else -1.</returns>
template <typename T, typename bucketT>
int RendererCL<T, bucketT>::MakeAndGetDensityFilterProgram(size_t ss, uint filterWidth)
{
	int kernelIndex = -1;

	if (!m_Devices.empty())
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		auto& deEntryPoint = m_DEOpenCLKernelCreator.GaussianDEEntryPoint(ss, filterWidth);
		const char* loc = __FUNCTION__;

		if ((kernelIndex = wrapper.FindKernelIndex(deEntryPoint)) == -1)//Has not been built yet.
		{
			auto& kernel = m_DEOpenCLKernelCreator.GaussianDEKernel(ss, filterWidth);

			if (wrapper.AddProgram(deEntryPoint, kernel, deEntryPoint, m_DoublePrecision))
				kernelIndex = wrapper.FindKernelIndex(deEntryPoint);//Try to find it again, it will be present if successfully built.
			else
				AddToReport(string(loc) + "():\nBuilding the following program failed: \n" + kernel + "\n");
		}
	}

	return kernelIndex;
}

/// <summary>
/// Make the final accumulation on the primary device program and return its index.
/// There are many different kernels for final accum, depending on early clip, alpha channel, and transparency.
/// Loading all of these in the beginning is too much, so only load the one for the current case being worked with.
/// </summary>
/// <param name="alphaBase">Storage for the alpha base value used in the kernel. 0 if transparency is true, else 255.</param>
/// <param name="alphaScale">Storage for the alpha scale value used in the kernel. 255 if transparency is true, else 0.</param>
/// <returns>The kernel index if successful, else -1.</returns>
template <typename T, typename bucketT>
int RendererCL<T, bucketT>::MakeAndGetFinalAccumProgram(double& alphaBase, double& alphaScale)
{
	int kernelIndex = -1;

	if (!m_Devices.empty())
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		auto& finalAccumEntryPoint = m_FinalAccumOpenCLKernelCreator.FinalAccumEntryPoint(EarlyClip(), Renderer<T, bucketT>::NumChannels(), Transparency(), alphaBase, alphaScale);
		const char* loc = __FUNCTION__;

		if ((kernelIndex = wrapper.FindKernelIndex(finalAccumEntryPoint)) == -1)//Has not been built yet.
		{
			auto& kernel = m_FinalAccumOpenCLKernelCreator.FinalAccumKernel(EarlyClip(), Renderer<T, bucketT>::NumChannels(), Transparency());

			if (wrapper.AddProgram(finalAccumEntryPoint, kernel, finalAccumEntryPoint, m_DoublePrecision))
				kernelIndex = wrapper.FindKernelIndex(finalAccumEntryPoint);//Try to find it again, it will be present if successfully built.
			else
				AddToReport(loc);
		}
	}

	return kernelIndex;
}

/// <summary>
/// Make the gamma correction program on the primary device for early clipping and return its index.
/// </summary>
/// <returns>The kernel index if successful, else -1.</returns>
template <typename T, typename bucketT>
int RendererCL<T, bucketT>::MakeAndGetGammaCorrectionProgram()
{
	if (!m_Devices.empty())
	{
		auto& wrapper = m_Devices[0]->m_Wrapper;
		auto& gammaEntryPoint = m_FinalAccumOpenCLKernelCreator.GammaCorrectionEntryPoint(Renderer<T, bucketT>::NumChannels(), Transparency());
		int kernelIndex = wrapper.FindKernelIndex(gammaEntryPoint);
		const char* loc = __FUNCTION__;

		if (kernelIndex == -1)//Has not been built yet.
		{
			auto& kernel = m_FinalAccumOpenCLKernelCreator.GammaCorrectionKernel(Renderer<T, bucketT>::NumChannels(), Transparency());
			bool b = wrapper.AddProgram(gammaEntryPoint, kernel, gammaEntryPoint, m_DoublePrecision);

			if (b)
				kernelIndex = wrapper.FindKernelIndex(gammaEntryPoint);//Try to find it again, it will be present if successfully built.
			else
				AddToReport(loc);
		}

		return kernelIndex;
	}

	return -1;
}

/// <summary>
/// Sum all histograms from the secondary devices with the histogram on the primary device.
/// </summary>
/// <returns>True if success, else false.</returns>
template <typename T, typename bucketT>
bool RendererCL<T, bucketT>::SumDeviceHist()
{
	if (m_Devices.size() > 1)
	{
		//Timing t;
		bool b = true;
		auto& wrapper = m_Devices[0]->m_Wrapper;
		const char* loc = __FUNCTION__;
		size_t blockW = m_Devices[0]->Nvidia() ? 32 : 16;//Max work group size is 256 on AMD, which means 16x16.
		size_t blockH = m_Devices[0]->Nvidia() ? 32 : 16;
		size_t gridW = SuperRasW();
		size_t gridH = SuperRasH();
		OpenCLWrapper::MakeEvenGridDims(blockW, blockH, gridW, gridH);
		int kernelIndex = wrapper.FindKernelIndex(m_IterOpenCLKernelCreator.SumHistEntryPoint());

		if ((b = (kernelIndex != -1)))
		{
			for (size_t device = 1; device < m_Devices.size(); device++)
			{
				if ((b = (ReadHist(device) && ClearHist(device))))//Must clear hist on secondary devices after reading and summing because they'll be reused on a quality increase (KEEP_ITERATING).
				{
					if ((b = wrapper.WriteBuffer(m_AccumBufferName, reinterpret_cast<void*>(HistBuckets()), SuperSize() * sizeof(v4bT))))
					{
						cl_uint argIndex = 0;

						if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_AccumBufferName)))						 { break; }//Source buffer of v4bT.

						if (b && !(b = wrapper.SetBufferArg(kernelIndex, argIndex++, m_HistBufferName)))						 { break; }//Dest buffer of v4bT.

						if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, uint(SuperRasW()))))						 { break; }//Width in pixels.

						if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, uint(SuperRasH()))))						 { break; }//Height in pixels.

						if (b && !(b = wrapper.SetArg	   (kernelIndex, argIndex++, (device == m_Devices.size() - 1) ? 1 : 0))) { break; }//Clear the source buffer on the last device.

						if (b && !(b = wrapper.RunKernel   (kernelIndex, gridW, gridH, 1, blockW, blockH, 1)))					 { break; }
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
		}

		if (!b)
		{
			ostringstream os;
			os << loc << ": failed to sum histograms from the secondary device(s) to the primary device.";
			AddToReport(os.str());
		}

		//t.Toc(loc);
		return b;
	}
	else
	{
		return m_Devices.size() == 1;
	}
}

/// <summary>
/// Private functions passing data to OpenCL programs.
/// </summary>

/// <summary>
/// Convert the currently used host side DensityFilter object into the DensityFilterCL member
/// for passing to OpenCL.
/// Some of the values are note populated when the filter object is null. This will be the case
/// when only log scaling is needed.
/// </summary>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::ConvertDensityFilter()
{
	m_DensityFilterCL.m_Supersample = uint(Supersample());
	m_DensityFilterCL.m_SuperRasW = uint(SuperRasW());
	m_DensityFilterCL.m_SuperRasH = uint(SuperRasH());
	m_DensityFilterCL.m_K1 = K1();
	m_DensityFilterCL.m_K2 = K2();

	if (m_DensityFilter.get())
	{
		m_DensityFilterCL.m_Curve = m_DensityFilter->Curve();
		m_DensityFilterCL.m_KernelSize = uint(m_DensityFilter->KernelSize());
		m_DensityFilterCL.m_MaxFilterIndex = uint(m_DensityFilter->MaxFilterIndex());
		m_DensityFilterCL.m_MaxFilteredCounts = uint(m_DensityFilter->MaxFilteredCounts());
		m_DensityFilterCL.m_FilterWidth = uint(m_DensityFilter->FilterWidth());
	}
}

/// <summary>
/// Convert the currently used host side SpatialFilter object into the SpatialFilterCL member
/// for passing to OpenCL.
/// </summary>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::ConvertSpatialFilter()
{
	bucketT g, linRange, vibrancy;
	Color<bucketT> background;

	if (m_SpatialFilter.get())
	{
		this->PrepFinalAccumVals(background, g, linRange, vibrancy);
		m_SpatialFilterCL.m_SuperRasW = uint(SuperRasW());
		m_SpatialFilterCL.m_SuperRasH = uint(SuperRasH());
		m_SpatialFilterCL.m_FinalRasW = uint(FinalRasW());
		m_SpatialFilterCL.m_FinalRasH = uint(FinalRasH());
		m_SpatialFilterCL.m_Supersample = uint(Supersample());
		m_SpatialFilterCL.m_FilterWidth = uint(m_SpatialFilter->FinalFilterWidth());
		m_SpatialFilterCL.m_NumChannels = uint(Renderer<T, bucketT>::NumChannels());
		m_SpatialFilterCL.m_BytesPerChannel = uint(BytesPerChannel());
		m_SpatialFilterCL.m_DensityFilterOffset = uint(DensityFilterOffset());
		m_SpatialFilterCL.m_Transparency = Transparency();
		m_SpatialFilterCL.m_YAxisUp = uint(m_YAxisUp);
		m_SpatialFilterCL.m_Vibrancy = vibrancy;
		m_SpatialFilterCL.m_HighlightPower = HighlightPower();
		m_SpatialFilterCL.m_Gamma = g;
		m_SpatialFilterCL.m_LinRange = linRange;
		m_SpatialFilterCL.m_Background = background;
	}
}

/// <summary>
/// Convert the host side Ember object into an EmberCL object
/// and a vector of XformCL for passing to OpenCL.
/// </summary>
/// <param name="ember">The Ember object to convert</param>
/// <param name="emberCL">The converted EmberCL</param>
/// <param name="xformsCL">The converted vector of XformCL</param>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::ConvertEmber(Ember<T>& ember, EmberCL<T>& emberCL, vector<XformCL<T>>& xformsCL)
{
	memset(&emberCL, 0, sizeof(EmberCL<T>));//Might not really be needed.
	emberCL.m_RotA           = m_RotMat.A();
	emberCL.m_RotB           = m_RotMat.B();
	emberCL.m_RotD           = m_RotMat.D();
	emberCL.m_RotE           = m_RotMat.E();
	emberCL.m_CamMat		 = ember.m_CamMat;
	emberCL.m_CenterX        = CenterX();
	emberCL.m_CenterY		 = ember.m_RotCenterY;
	emberCL.m_CamZPos		 = ember.m_CamZPos;
	emberCL.m_CamPerspective = ember.m_CamPerspective;
	emberCL.m_CamYaw		 = ember.m_CamYaw;
	emberCL.m_CamPitch		 = ember.m_CamPitch;
	emberCL.m_CamDepthBlur	 = ember.m_CamDepthBlur;
	emberCL.m_BlurCoef		 = ember.BlurCoef();

	for (size_t i = 0; i < ember.TotalXformCount() && i < xformsCL.size(); i++)
	{
		Xform<T>* xform = ember.GetTotalXform(i);
		xformsCL[i].m_A = xform->m_Affine.A();
		xformsCL[i].m_B = xform->m_Affine.B();
		xformsCL[i].m_C = xform->m_Affine.C();
		xformsCL[i].m_D = xform->m_Affine.D();
		xformsCL[i].m_E = xform->m_Affine.E();
		xformsCL[i].m_F = xform->m_Affine.F();
		xformsCL[i].m_PostA = xform->m_Post.A();
		xformsCL[i].m_PostB = xform->m_Post.B();
		xformsCL[i].m_PostC = xform->m_Post.C();
		xformsCL[i].m_PostD = xform->m_Post.D();
		xformsCL[i].m_PostE = xform->m_Post.E();
		xformsCL[i].m_PostF = xform->m_Post.F();
		xformsCL[i].m_DirectColor = xform->m_DirectColor;
		xformsCL[i].m_ColorSpeedCache = xform->ColorSpeedCache();
		xformsCL[i].m_OneMinusColorCache = xform->OneMinusColorCache();
		xformsCL[i].m_Opacity = xform->m_Opacity;
		xformsCL[i].m_VizAdjusted = xform->VizAdjusted();

		for (size_t varIndex = 0; varIndex < xform->TotalVariationCount() && varIndex < MAX_CL_VARS; varIndex++)//Assign all variation weights for this xform, with a max of MAX_CL_VARS.
			xformsCL[i].m_VariationWeights[varIndex] = xform->GetVariation(varIndex)->m_Weight;
	}
}

/// <summary>
/// Convert the host side CarToRas object into the CarToRasCL member
/// for passing to OpenCL.
/// </summary>
/// <param name="carToRas">The CarToRas object to convert</param>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::ConvertCarToRas(const CarToRas<T>& carToRas)
{
	m_CarToRasCL.m_RasWidth = uint(carToRas.RasWidth());
	m_CarToRasCL.m_PixPerImageUnitW = carToRas.PixPerImageUnitW();
	m_CarToRasCL.m_RasLlX = carToRas.RasLlX();
	m_CarToRasCL.m_PixPerImageUnitH = carToRas.PixPerImageUnitH();
	m_CarToRasCL.m_RasLlY = carToRas.RasLlY();
	m_CarToRasCL.m_CarLlX = carToRas.CarLlX();
	m_CarToRasCL.m_CarLlY = carToRas.CarLlY();
	m_CarToRasCL.m_CarUrX = carToRas.CarUrX();
	m_CarToRasCL.m_CarUrY = carToRas.CarUrY();
}

/// <summary>
/// Fill a seeds buffer for all devices, each of which gets passed to its
/// respective device when launching the iteration kernel.
/// The range of each seed will be spaced to ensure no duplicates are added.
/// Note, WriteBuffer() must be called after this to actually copy the
/// data from the host to the device.
/// </summary>
template <typename T, typename bucketT>
void RendererCL<T, bucketT>::FillSeeds()
{
	if (!m_Devices.empty())
	{
		double start, delta = std::floor(double(std::numeric_limits<uint>::max()) / (IterGridKernelCount() * 2 * m_Devices.size()));
		m_Seeds.resize(m_Devices.size());
		start = delta;

		for (size_t device = 0; device < m_Devices.size(); device++)
		{
			m_Seeds[device].resize(IterGridKernelCount());

			for (auto& seed : m_Seeds[device])
			{
				seed.x = uint(m_Rand[0].template Frand<double>(start, start + delta));
				start += delta;
				seed.y = uint(m_Rand[0].template Frand<double>(start, start + delta));
				start += delta;
			}
		}
	}
}

template EMBERCL_API class RendererCL<float, float>;

#ifdef DO_DOUBLE
	template EMBERCL_API class RendererCL<double, float>;
#endif
}
