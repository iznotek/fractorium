#pragma once

#include "EmberCommonPch.h"

/// <summary>
/// Global utility classes and functions that are common to all programs that use
/// Ember and its derivatives.
/// </summary>

/// <summary>
/// Derivation of the RenderCallback class to do custom printing action
/// whenever the progress function is internally called inside of Ember
/// and its derivatives.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class RenderProgress : public RenderCallback
{
public:
	/// <summary>
	/// Constructor that initializes the state to zero.
	/// </summary>
	RenderProgress()
	{
		Clear();
	}

	/// <summary>
	/// The progress function which will be called from inside the renderer.
	/// </summary>
	/// <param name="ember">The ember currently being rendered</param>
	/// <param name="foo">An extra dummy parameter</param>
	/// <param name="fraction">The progress fraction from 0-100</param>
	/// <param name="stage">The stage of iteration. 1 is iterating, 2 is density filtering, 2 is final accumulation.</param>
	/// <param name="etaMs">The estimated milliseconds to completion of the current stage</param>
	/// <returns>1 since this is intended to run in an environment where the render runs to completion, unlike interactive rendering.</returns>
	virtual int ProgressFunc(Ember<T>& ember, void* foo, double fraction, int stage, double etaMs)
	{
		if (stage == 0 || stage == 1)
		{
			if (m_LastStage != stage)
				cout << endl;

			cout << "\r" << string(m_S.length() * 2, ' ');//Clear what was previously here, * 2 just to be safe because the end parts of previous strings might be longer.
			m_SS.str("");//Begin new output.
			m_SS << "\rStage = " << (stage ? "filtering" : "iterating");
			m_SS << ", progress = " << int(fraction) << "%";
			m_SS << ", eta = " << t.Format(etaMs);
			m_S = m_SS.str();
			cout << m_S;
		}

		m_LastStage = stage;
		return 1;
	}

	/// <summary>
	/// Reset the state.
	/// </summary>
	void Clear()
	{
		m_LastStage = 0;
		m_LastLength = 0;
		m_SS.clear();
		m_S.clear();
	}

private:
	int m_LastStage;
	int m_LastLength;
	stringstream m_SS;
	string m_S;
	Timing t;
};

/// <summary>
/// Wrapper for parsing an ember Xml file, storing the embers in a vector and printing
/// any errors that occurred.
/// Template argument expected to be float or double.
/// </summary>
/// <param name="parser">The parser to use</param>
/// <param name="filename">The full path and name of the file</param>
/// <param name="embers">Storage for the embers read from the file</param>
/// <param name="useDefaults">True to use defaults if they are not present in the file, else false to use invalid values as placeholders to indicate the values were not present. Default: true.</param>
/// <returns>True if success, else false.</returns>
template <typename T>
static bool ParseEmberFile(XmlToEmber<T>& parser, const string& filename, vector<Ember<T>>& embers, bool useDefaults = true)
{
	if (!parser.Parse(filename.c_str(), embers, useDefaults))
	{
		cout << "Error parsing flame file " << filename << ", returning without executing." << endl;
		return false;
	}

	if (embers.empty())
	{
		cout << "Error: No data present in file " << filename << ". Aborting." << endl;
		return false;
	}

	return true;
}

/// <summary>
/// Wrapper for parsing palette Xml file and initializing it's private static members,
/// and printing any errors that occurred.
/// Template argument expected to be float or double.
/// </summary>
/// <param name="filename">The full path and name of the file</param>
/// <returns>True if success, else false.</returns>
template <typename T>
static bool InitPaletteList(const string& filename)
{
	PaletteList<T> paletteList;//Even though this is local, the members are static so they will remain.
	bool added = paletteList.Add(filename);

	if (!added || !paletteList.Size())
	{
		cout << "Error parsing palette file " << filename << ". Reason: " << endl;
		cout << paletteList.ErrorReportString() << endl << "Returning without executing." << endl;
		return false;
	}

	return true;
}

/// <summary>
/// Convert an RGBA buffer to an RGB buffer.
/// The two buffers can point to the same memory location if needed.
/// </summary>
/// <param name="rgba">The RGBA buffer</param>
/// <param name="rgb">The RGB buffer</param>
/// <param name="width">The width of the image in pixels</param>
/// <param name="height">The height of the image in pixels</param>
static void RgbaToRgb(vector<byte>& rgba, vector<byte>& rgb, size_t width, size_t height)
{
	if (rgba.data() != rgb.data())//Only resize the destination buffer if they are different.
		rgb.resize(width * height * 3);

	for (size_t i = 0, j = 0; i < (width * height * 4); i += 4, j += 3)
	{
		rgb[j]	   = rgba[i];
		rgb[j + 1] = rgba[i + 1];
		rgb[j + 2] = rgba[i + 2];
	}
}

/// <summary>
/// Calculate the number of strips required if the needed amount of memory
/// is greater than the system memory, or greater than what the user wants to allow.
/// </summary>
/// <param name="mem">Amount of memory required</param>
/// <param name="memAvailable">Amount of memory available on the system</param>
/// <param name="useMem">The maximum amount of memory to use. Use max if 0.</param>
/// <returns>The number of strips to use</returns>
static uint CalcStrips(double memRequired, double memAvailable, double useMem)
{
	uint strips;

	if (useMem > 0)
		memAvailable = useMem;
	else
		memAvailable *= 0.8;

	if (memAvailable >= memRequired)
		return 1;

	strips = uint(ceil(memRequired / memAvailable));
	return strips;
}

/// <summary>
/// Given a numerator and a denominator, find the next highest denominator that divides
/// evenly into the numerator.
/// </summary>
/// <param name="numerator">The numerator</param>
/// <param name="denominator">The denominator</param>
/// <returns>The next highest divisor if found, else 1.</returns>
template <typename T>
static T NextHighestEvenDiv(T numerator, T denominator)
{
	T result = 1;
	T numDiv2 = numerator / 2;

	do
	{
		denominator++;

		if (numerator % denominator == 0)
		{
			result = denominator;
			break;
		}
	}
	while (denominator <= numDiv2);

	return result;
}

/// <summary>
/// Given a numerator and a denominator, find the next lowest denominator that divides
/// evenly into the numerator.
/// </summary>
/// <param name="numerator">The numerator</param>
/// <param name="denominator">The denominator</param>
/// <returns>The next lowest divisor if found, else 1.</returns>
template <typename T>
static T NextLowestEvenDiv(T numerator, T denominator)
{
	T result = 1;
	T numDiv2 = numerator / 2;
	denominator--;

	if (denominator > numDiv2)
		denominator = numDiv2;

	while (denominator >= 1)
	{
		if (numerator % denominator == 0)
		{
			result = denominator;
			break;
		}

		denominator--;
	}

	return result;
}

/// <summary>
/// Wrapper for converting a vector of absolute device indices to a vector
/// of platform,device index pairs.
/// </summary>
/// <param name="selectedDevices">The vector of absolute device indices to convert</param>
/// <returns>The converted vector of platform,device index pairs</returns>
static vector<pair<size_t, size_t>> Devices(const vector<size_t>& selectedDevices)
{
	vector<pair<size_t, size_t>> vec;
	auto info = OpenCLInfo::Instance();
	auto& devices = info->DeviceIndices();
	vec.reserve(selectedDevices.size());

	for (size_t i = 0; i < selectedDevices.size(); i++)
	{
		auto index = selectedDevices[i];

		if (index < devices.size())
			vec.push_back(devices[index]);
	}

	return vec;
}

/// <summary>
/// Wrapper for creating a renderer of the specified type.
/// </summary>
/// <param name="renderType">Type of renderer to create</param>
/// <param name="devices">The vector of platform/device indices to use</param>
/// <param name="shared">True if shared with OpenGL, else false.</param>
/// <param name="texId">The texture ID of the shared OpenGL texture if shared</param>
/// <param name="errorReport">The error report for holding errors if anything goes wrong</param>
/// <returns>A pointer to the created renderer if successful, else false.</returns>
template <typename T>
static Renderer<T, float>* CreateRenderer(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared, GLuint texId, EmberReport& errorReport)
{
	string s;
	unique_ptr<Renderer<T, float>> renderer;

	try
	{
		if (renderType == OPENCL_RENDERER && !devices.empty())
		{
			s = "OpenCL";
			renderer = unique_ptr<Renderer<T, float>>(new RendererCL<T, float>(devices, shared, texId));

			if (!renderer.get() || !renderer->Ok())
			{
				if (renderer.get())
					errorReport.AddToReport(renderer->ErrorReport());

				errorReport.AddToReport("Error initializing OpenCL renderer, using CPU renderer instead.");
				renderer = unique_ptr<Renderer<T, float>>(new Renderer<T, float>());
			}
		}
		else
		{
			s = "CPU";
			renderer = unique_ptr<Renderer<T, float>>(new Renderer<T, float>());
		}
	}
	catch (const std::exception& e)
	{
		errorReport.AddToReport("Error creating " + s + " renderer: " + e.what() + "\n");
	}
	catch (...)
	{
		errorReport.AddToReport("Error creating " + s + " renderer.\n");
	}

	return renderer.release();
}

/// <summary>
/// Wrapper for creating a vector of renderers of the specified type for each passed in device.
/// If shared is true, only the first renderer will be shared with OpenGL.
/// Although a fallback GPU renderer will be created if a failure occurs, it doesn't really
/// make sense since the concept of devices only applies to OpenCL renderers.
/// </summary>
/// <param name="renderType">Type of renderer to create</param>
/// <param name="devices">The vector of platform/device indices to use</param>
/// <param name="shared">True if shared with OpenGL, else false.</param>
/// <param name="texId">The texture ID of the shared OpenGL texture if shared</param>
/// <param name="errorReport">The error report for holding errors if anything goes wrong</param>
/// <returns>The vector of created renderers if successful, else false.</returns>
template <typename T>
static vector<unique_ptr<Renderer<T, float>>> CreateRenderers(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared, GLuint texId, EmberReport& errorReport)
{
	string s;
	vector<unique_ptr<Renderer<T, float>>> v;

	try
	{
		if (renderType == OPENCL_RENDERER && !devices.empty())
		{
			s = "OpenCL";
			v.reserve(devices.size());

			for (size_t i = 0; i < devices.size(); i++)
			{
				vector<pair<size_t, size_t>> tempDevices{ devices[i] };
				auto renderer = unique_ptr<Renderer<T, float>>(new RendererCL<T, float>(tempDevices, !i ? shared : false, texId));

				if (!renderer.get() || !renderer->Ok())
				{
					ostringstream os;

					if (renderer.get())
						errorReport.AddToReport(renderer->ErrorReport());

					os << "Error initializing OpenCL renderer for platform " << devices[i].first << ", " << devices[i].second;
					errorReport.AddToReport(os.str());
				}
				else
					v.push_back(std::move(renderer));
			}
		}
		else
		{
			s = "CPU";
			v.push_back(std::move(unique_ptr<Renderer<T, float>>(::CreateRenderer<T>(CPU_RENDERER, devices, shared, texId, errorReport))));
		}
	}
	catch (const std::exception& e)
	{
		errorReport.AddToReport("Error creating " + s + " renderer: " + e.what() + "\n");
	}
	catch (...)
	{
		errorReport.AddToReport("Error creating " + s + " renderer.\n");
	}

	if (v.empty() && s != "CPU")//OpenCL creation failed and CPU creation has not been attempted, so just create one CPU renderer and place it in the vector.
	{
		try
		{
			s = "CPU";
			v.push_back(std::move(unique_ptr<Renderer<T, float>>(::CreateRenderer<T>(CPU_RENDERER, devices, shared, texId, errorReport))));
		}
		catch (const std::exception& e)
		{
			errorReport.AddToReport("Error creating fallback" + s + " renderer: " + e.what() + "\n");
		}
		catch (...)
		{
			errorReport.AddToReport("Error creating fallback " + s + " renderer.\n");
		}
	}

	return v;
}

/// <summary>
/// Perform a render which allows for using strips or not.
/// If an error occurs while rendering any strip, the rendering process stops.
/// </summary>
/// <param name="renderer">The renderer to use</param>
/// <param name="ember">The ember to render</param>
/// <param name="finalImage">The vector to place the final output in</param>
/// <param name="time">The time position to use, only valid for animation</param>
/// <param name="strips">The number of strips to use. This must be validated before calling this function.</param>
/// <param name="yAxisUp">True to flip the Y axis, else false.</param>
/// <param name="perStripStart">Function called before the start of the rendering of each strip</param>
/// <param name="perStripFinish">Function called after the end of the rendering of each strip</param>
/// <param name="perStripError">Function called if there is an error rendering a strip</param>
/// <param name="allStripsFinished">Function called when all strips successfully finish rendering</param>
/// <returns>True if all rendering was successful, else false.</returns>
template <typename T>
static bool StripsRender(RendererBase* renderer, Ember<T>& ember, vector<byte>& finalImage, double time, size_t strips, bool yAxisUp,
						 std::function<void(size_t strip)> perStripStart,
						 std::function<void(size_t strip)> perStripFinish,
						 std::function<void(size_t strip)> perStripError,
						 std::function<void(Ember<T>& finalEmber)> allStripsFinished)
{
	bool success = false;
	size_t origHeight, realHeight = ember.m_FinalRasH;
	T centerY = ember.m_CenterY;
	T floatStripH = T(ember.m_FinalRasH) / T(strips);
	T zoomScale = pow(T(2), ember.m_Zoom);
	T centerBase = centerY - ((strips - 1) * floatStripH) / (2 * ember.m_PixelsPerUnit * zoomScale);
	vector<QTIsaac<ISAAC_SIZE, ISAAC_INT>> randVec;
	ember.m_Quality *= strips;
	ember.m_FinalRasH = size_t(ceil(floatStripH));

	if (strips > 1)
		randVec = renderer->RandVec();

	for (size_t strip = 0; strip < strips; strip++)
	{
		size_t stripOffset;

		if (yAxisUp)
			stripOffset = ember.m_FinalRasH * ((strips - strip) - 1) * renderer->FinalRowSize();
		else
			stripOffset = ember.m_FinalRasH * strip * renderer->FinalRowSize();

		ember.m_CenterY = centerBase + ember.m_FinalRasH * T(strip) / (ember.m_PixelsPerUnit * zoomScale);

		if ((ember.m_FinalRasH * (strip + 1)) > realHeight)
		{
			origHeight = ember.m_FinalRasH;
			ember.m_FinalRasH = realHeight - origHeight * strip;
			ember.m_CenterY -= (origHeight - ember.m_FinalRasH) * T(0.5) / (ember.m_PixelsPerUnit * zoomScale);
		}

		perStripStart(strip);

		if (strips > 1)
		{
			renderer->RandVec(randVec);//Use the same vector of ISAAC rands for each strip.
			renderer->SetEmber(ember);//Set one final time after modifications for strips.
		}

		if ((renderer->Run(finalImage, time, 0, false, stripOffset) == RENDER_OK) && !renderer->Aborted() && !finalImage.empty())
		{
			perStripFinish(strip);
		}
		else
		{
			perStripError(strip);
			break;
		}

		if (strip == strips - 1)
			success = true;
	}

	//Restore the ember values to their original values.
	ember.m_Quality /= strips;
	ember.m_FinalRasH = realHeight;
	ember.m_CenterY = centerY;
	renderer->SetEmber(ember);//Further processing will require the dimensions to match the original ember, so re-assign.

	if (success)
		allStripsFinished(ember);

	Memset(finalImage);
	return success;
}

/// <summary>
/// Verify that the specified number of strips is valid for the given height.
/// The passed in error functions will be called if the number of strips needs
/// to be modified for the given height.
/// </summary>
/// <param name="height">The height in pixels of the image to be rendered</param>
/// <param name="strips">The number of strips to split the render into</param>
/// <param name="stripError1">Function called if the number of strips exceeds the height of the image</param>
/// <param name="stripError2">Function called if the number of strips does not divide evently into the height of the image</param>
/// <param name="stripError3">Called if for any reason the number of strips used will differ from the value passed in</param>
/// <returns>The actual number of strips that will be used</returns>
static size_t VerifyStrips(size_t height, size_t strips,
						   std::function<void(const string& s)> stripError1,
						   std::function<void(const string& s)> stripError2,
						   std::function<void(const string& s)> stripError3)
{
	ostringstream os;

	if (strips > height)
	{
		os << "Cannot have more strips than rows: " << strips << " > " << height << ". Setting strips = rows.";
		stripError1(os.str()); os.str("");
		strips = height;
	}

	if (height % strips != 0)
	{
		os << "A strips value of " << strips << " does not divide evenly into a height of " << height << ".";
		stripError2(os.str()); os.str("");
		strips = NextHighestEvenDiv(height, strips);

		if (strips == 1)//No higher divisor, check for a lower one.
			strips = NextLowestEvenDiv(height, strips);

		os << "Setting strips to " << strips << ".";
		stripError3(os.str()); os.str("");
	}

	return strips;
}

/// <summary>
/// Simple macro to print a string if the --verbose options has been specified.
/// </summary>
#define VerbosePrint(s) if (opt.Verbose()) cout << s << endl
