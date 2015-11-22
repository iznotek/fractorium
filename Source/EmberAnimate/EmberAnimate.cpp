#include "EmberCommonPch.h"
#include "EmberAnimate.h"
#include "JpegUtils.h"

/// <summary>
/// The core of the EmberAnimate.exe program.
/// Template argument expected to be float or double.
/// </summary>
/// <param name="opt">A populated EmberOptions object which specifies all program options to be used</param>
/// <returns>True if success, else false.</returns>
template <typename T>
bool EmberAnimate(EmberOptions& opt)
{
	OpenCLInfo& info(OpenCLInfo::Instance());

	std::cout.imbue(std::locale(""));

	if (opt.DumpArgs())
		cout << opt.GetValues(OPT_USE_ANIMATE) << endl;

	if (opt.OpenCLInfo())
	{
		cout << "\nOpenCL Info: " << endl;
		cout << info.DumpInfo();
		return true;
	}

	//Regular variables.
	Timing t;
	bool unsorted = false;
	uint channels, padding;
	size_t i, firstUnsortedIndex = 0;
	string inputPath = GetPath(opt.Input());
	vector<Ember<T>> embers;
	XmlToEmber<T> parser;
	EmberToXml<T> emberToXml;
	EmberReport emberReport;
	const vector<pair<size_t, size_t>> devices = Devices(opt.Devices());
	std::atomic<size_t> atomfTime;
	vector<std::thread> threadVec;
	unique_ptr<RenderProgress<T>> progress;
	vector<unique_ptr<Renderer<T, float>>> renderers;
	vector<string> errorReport;
	CriticalSection verboseCs;

	if (opt.EmberCL())
	{
		renderers = CreateRenderers<T>(OPENCL_RENDERER, devices, false, 0, emberReport);
		errorReport = emberReport.ErrorReport();

		if (!errorReport.empty())
			emberReport.DumpErrorReport();

		if (!renderers.size() || renderers.size() != devices.size())
		{
			cout << "Only created " << renderers.size() << " renderers out of " << devices.size() << " requested, exiting." << endl;
			return false;
		}

		if (opt.DoProgress())
		{
			progress = unique_ptr<RenderProgress<T>>(new RenderProgress<T>());
			renderers[0]->Callback(progress.get());
		}

		cout << "Using OpenCL to render." << endl;

		if (opt.Verbose())
		{
			for (auto& device : devices)
			{
				cout << "Platform: " << info.PlatformName(device.first) << endl;
				cout << "Device: " << info.DeviceName(device.first, device.second) << endl;
			}
		}

		if (opt.ThreadCount() > 1)
			cout << "Cannot specify threads with OpenCL, using 1 thread." << endl;

		opt.ThreadCount(1);

		for (auto& r : renderers)
			r->ThreadCount(opt.ThreadCount(), opt.IsaacSeed() != "" ? opt.IsaacSeed().c_str() : nullptr);

		if (opt.BitsPerChannel() != 8)
		{
			cout << "Bits per channel cannot be anything other than 8 with OpenCL, setting to 8." << endl;
			opt.BitsPerChannel(8);
		}
	}
	else
	{
		unique_ptr<Renderer<T, float>> tempRenderer(CreateRenderer<T>(CPU_RENDERER, devices, false, 0, emberReport));
		errorReport = emberReport.ErrorReport();

		if (!errorReport.empty())
			emberReport.DumpErrorReport();

		if (!tempRenderer.get())
		{
			cout << "Renderer creation failed, exiting." << endl;
			return false;
		}

		if (opt.DoProgress())
		{
			progress = unique_ptr<RenderProgress<T>>(new RenderProgress<T>());
			tempRenderer->Callback(progress.get());
		}

		if (opt.ThreadCount() == 0)
		{
			cout << "Using " << Timing::ProcessorCount() << " automatically detected threads." << endl;
			opt.ThreadCount(Timing::ProcessorCount());
		}
		else
		{
			cout << "Using " << opt.ThreadCount() << " manually specified threads." << endl;
		}

		tempRenderer->ThreadCount(opt.ThreadCount(), opt.IsaacSeed() != "" ? opt.IsaacSeed().c_str() : nullptr);
		renderers.push_back(std::move(tempRenderer));
	}

	if (!InitPaletteList<T>(opt.PalettePath()))
		return false;

	if (!ParseEmberFile(parser, opt.Input(), embers))
		return false;

	if (embers.size() <= 1)
	{
		cout << "Read " << embers.size() << " embers from file. At least 2 required to animate, exiting." << endl;
		return false;
	}

	if (opt.Format() != "jpg" &&
		opt.Format() != "png" &&
		opt.Format() != "ppm" &&
		opt.Format() != "bmp")
	{
		cout << "Format must be jpg, png, ppm, or bmp not " << opt.Format() << ". Setting to jpg." << endl;
	}

	channels = opt.Format() == "png" ? 4 : 3;

	if (opt.BitsPerChannel() == 16 && opt.Format() != "png")
	{
		cout << "Support for 16 bits per channel images is only present for the png format. Setting to 8." << endl;
		opt.BitsPerChannel(8);
	}
	else if (opt.BitsPerChannel() != 8 && opt.BitsPerChannel() != 16)
	{
		cout << "Unexpected bits per channel specified " << opt.BitsPerChannel() << ". Setting to 8." << endl;
		opt.BitsPerChannel(8);
	}

	if (opt.InsertPalette() && opt.BitsPerChannel() != 8)
	{
		cout << "Inserting palette only supported with 8 bits per channel, insertion will not take place." << endl;
		opt.InsertPalette(false);
	}

	if (opt.AspectRatio() < 0)
	{
		cout << "Invalid pixel aspect ratio " << opt.AspectRatio() << endl << ". Must be positive, setting to 1." << endl;
		opt.AspectRatio(1);
	}

	if (opt.Dtime() < 1)
	{
		cout << "Warning: dtime must be positive, not " << opt.Dtime() << ". Setting to 1." << endl;
		opt.Dtime(1);
	}

	if (opt.Frame())
	{
		if (opt.Time())
		{
			cout << "Cannot specify both time and frame." << endl;
			return false;
		}

		if (opt.FirstFrame() || opt.LastFrame())
		{
			cout << "Cannot specify both frame and begin or end." << endl;
			return false;
		}

		opt.FirstFrame(opt.Frame());
		opt.LastFrame(opt.Frame());
	}

	if (opt.Time())
	{
		if (opt.FirstFrame() || opt.LastFrame())
		{
			cout << "Cannot specify both time and begin or end." << endl;
			return false;
		}

		opt.FirstFrame(opt.Time());
		opt.LastFrame(opt.Time());
	}

	//Prep all embers, by ensuring they:
	//-Are sorted by time.
	//-Do not have a dimension of 0.
	//-Do not have a memory requirement greater than max uint.
	//-Have quality and size scales applied, if present.
	//-Have equal dimensions.
	for (i = 0; i < embers.size(); i++)
	{
		if (i > 0 && embers[i].m_Time <= embers[i - 1].m_Time)
		{
			if (!unsorted)
				firstUnsortedIndex = i;

			unsorted = true;
		}

		if (i > 0 && embers[i].m_Time == embers[i - 1].m_Time)
		{
			cout << "Image " << i << " time of " << embers[i].m_Time << " equaled previous image time of " << embers[i - 1].m_Time << ". Adjusting up by 1." << endl;
			embers[i].m_Time++;
		}

		if (opt.Supersample() > 0)
			embers[i].m_Supersample = opt.Supersample();

		if (opt.SubBatchSize() != DEFAULT_SBS)
			embers[i].m_SubBatchSize = opt.SubBatchSize();

		embers[i].m_Quality *= T(opt.QualityScale());
		embers[i].m_FinalRasW = size_t(T(embers[i].m_FinalRasW) * opt.SizeScale());
		embers[i].m_FinalRasH = size_t(T(embers[i].m_FinalRasH) * opt.SizeScale());
		embers[i].m_PixelsPerUnit *= T(opt.SizeScale());

		//Cast to double in case the value exceeds 2^32.
		double imageMem = double(channels) * double(embers[i].m_FinalRasW)
			   * double(embers[i].m_FinalRasH) * double(renderers[0]->BytesPerChannel());
		double maxMem = pow(2.0, double((sizeof(void*) * 8) - 1));

		if (imageMem > maxMem)//Ensure the max amount of memory for a process isn't exceeded.
		{
			cout << "Image " << i << " size > " << maxMem << ". Setting to 1920 x 1080." << endl;
			embers[i].m_FinalRasW = 1920;
			embers[i].m_FinalRasH = 1080;
		}

		if (embers[i].m_FinalRasW == 0 || embers[i].m_FinalRasH == 0)
		{
			cout << "Warning: Output image " << i << " has dimension 0: " << embers[i].m_FinalRasW  << ", " << embers[i].m_FinalRasH << ". Setting to 1920 x 1080." << endl;
			embers[i].m_FinalRasW = 1920;
			embers[i].m_FinalRasH = 1080;
		}

		if ((embers[i].m_FinalRasW != embers[0].m_FinalRasW) ||
			(embers[i].m_FinalRasH != embers[0].m_FinalRasH))
		{
			cout << "Warning: flame " << i << " at time " << embers[i].m_Time << " size mismatch. (" << embers[i].m_FinalRasW << ", " << embers[i].m_FinalRasH <<
				") should be (" << embers[0].m_FinalRasW << ", " << embers[0].m_FinalRasH << "). Setting to " << embers[0].m_FinalRasW << ", " << embers[0].m_FinalRasH << "." << endl;

			embers[i].m_FinalRasW = embers[0].m_FinalRasW;
			embers[i].m_FinalRasH = embers[0].m_FinalRasH;
		}
	}

	if (unsorted)
	{
		cout << "Embers were unsorted by time. First out of order index was " << firstUnsortedIndex << ". Sorting." << endl;
		std::sort(embers.begin(), embers.end(), &CompareEmbers<T>);
	}

	if (!opt.Time() && !opt.Frame())
	{
		if (opt.FirstFrame() == UINT_MAX)
			opt.FirstFrame(size_t(embers[0].m_Time));

		if (opt.LastFrame() == UINT_MAX)
			opt.LastFrame(ClampGte<size_t>(size_t(embers.back().m_Time),//Make sure time - 1 is positive before converting to size_t.
				opt.FirstFrame() + opt.Dtime()));//Make sure the final value is at least first frame + dtime.
	}

	if (!opt.Out().empty())
	{
		cout << "Single output file " << opt.Out() << " specified for multiple images. They would be all overwritten and only the last image will remain, exiting." << endl;
		return false;
	}

	//Final setup steps before running.
	padding = uint(std::log10(double(embers.size()))) + 1;

	for (auto& r : renderers)
	{
		r->SetEmber(embers);
		r->EarlyClip(opt.EarlyClip());
		r->YAxisUp(opt.YAxisUp());
		r->LockAccum(opt.LockAccum());
		r->InsertPalette(opt.InsertPalette());
		r->PixelAspectRatio(T(opt.AspectRatio()));
		r->Transparency(opt.Transparency());
		r->NumChannels(channels);
		r->BytesPerChannel(opt.BitsPerChannel() / 8);
		r->Priority(eThreadPriority(Clamp<intmax_t>(intmax_t(opt.Priority()), intmax_t(eThreadPriority::LOWEST), intmax_t(eThreadPriority::HIGHEST))));
	}

	std::function<void (vector<byte>&, string, EmberImageComments, size_t, size_t, size_t)> saveFunc = [&](vector<byte>& finalImage,
		string filename,//These are copies because this will be launched in a thread.
		EmberImageComments comments,
		size_t w,
		size_t h,
		size_t chan)
	{
		bool writeSuccess = false;
		byte* finalImagep = finalImage.data();

		if ((opt.Format() == "jpg" || opt.Format() == "bmp") && chan == 4)
			RgbaToRgb(finalImage, finalImage, w, h);

		if (opt.Format() == "png")
			writeSuccess = WritePng(filename.c_str(), finalImagep, w, h, opt.BitsPerChannel() / 8, opt.PngComments(), comments, opt.Id(), opt.Url(), opt.Nick());
		else if (opt.Format() == "jpg")
			writeSuccess = WriteJpeg(filename.c_str(), finalImagep, w, h, int(opt.JpegQuality()), opt.JpegComments(), comments, opt.Id(), opt.Url(), opt.Nick());
		else if (opt.Format() == "ppm")
			writeSuccess = WritePpm(filename.c_str(), finalImagep, w, h);
		else if (opt.Format() == "bmp")
			writeSuccess = WriteBmp(filename.c_str(), finalImagep, w, h);

		if (!writeSuccess)
			cout << "Error writing " << filename << endl;
	};
	
	atomfTime.store(opt.FirstFrame());

	std::function<void(size_t)> iterFunc = [&](size_t index)
	{
		size_t ftime, finalImageIndex = 0;
		string filename, flameName;
		RendererBase* renderer = renderers[index].get();
		ostringstream fnstream, os;
		EmberStats stats;
		EmberImageComments comments;
		Ember<T> centerEmber;
		vector<byte> finalImages[2];
		std::thread writeThread;

		os.imbue(std::locale(""));

		while (atomfTime.fetch_add(opt.Dtime()), ((ftime = atomfTime.load()) <= opt.LastFrame()))
		{
			T localTime = T(ftime) - 1;

			if (opt.Verbose() && ((opt.LastFrame() - opt.FirstFrame()) / opt.Dtime() >= 1))
			{
				verboseCs.Enter();
				cout << "Time = " << ftime << " / " << opt.LastFrame() << " / " << opt.Dtime() << endl;
				verboseCs.Leave();
			}

			renderer->Reset();

			if ((renderer->Run(finalImages[finalImageIndex], localTime) != RENDER_OK) || renderer->Aborted() || finalImages[finalImageIndex].empty())
			{
				cout << "Error: image rendering failed, skipping to next image." << endl;
				renderer->DumpErrorReport();//Something went wrong, print errors.
				atomfTime.store(opt.LastFrame() + 1);//Abort all threads if any of them encounter an error.
				break;
			}

			fnstream << inputPath << opt.Prefix() << setfill('0') << setw(padding) << ftime << opt.Suffix() << "." << opt.Format();
			filename = fnstream.str();
			fnstream.str("");

			if (opt.WriteGenome())
			{
				flameName = filename.substr(0, filename.find_last_of('.')) + ".flam3";

				if (opt.Verbose())
				{
					verboseCs.Enter();
					cout << "Writing " << flameName << endl;
					verboseCs.Leave();
				}

				Interpolater<T>::Interpolate(embers, localTime, 0, centerEmber);//Get center flame.
				emberToXml.Save(flameName, centerEmber, opt.PrintEditDepth(), true, opt.IntPalette(), opt.HexPalette(), true, false, false);
				centerEmber.Clear();
			}

			stats = renderer->Stats();
			comments = renderer->ImageComments(stats, opt.PrintEditDepth(), opt.IntPalette(), opt.HexPalette());
			os.str("");
			size_t iterCount = renderer->TotalIterCount(1);
			os << comments.m_NumIters << " / " << iterCount << " (" << std::fixed << std::setprecision(2) << ((double(stats.m_Iters) / double(iterCount)) * 100) << "%)";

			if (opt.Verbose())
			{
				verboseCs.Enter();
				cout << "\nIters ran/requested: " + os.str() << endl;
				if (!opt.EmberCL()) cout << "Bad values: " << stats.m_Badvals << endl;
				cout << "Render time: " << t.Format(stats.m_RenderMs) << endl;
				cout << "Pure iter time: " << t.Format(stats.m_IterMs) << endl;
				cout << "Iters/sec: " << size_t(stats.m_Iters / (stats.m_IterMs / 1000.0)) << endl;
				cout << "Writing " << filename << endl << endl;
				verboseCs.Leave();
			}

			//Run image writing in a thread. Although doing it this way duplicates the final output memory, it saves a lot of time
			//when running with OpenCL. Call join() to ensure the previous thread call has completed.
			if (writeThread.joinable())
				writeThread.join();

			auto threadVecIndex = finalImageIndex;//Cache before launching thread.

			if (opt.ThreadedWrite())//Copies are passed of all but the first parameter to saveFunc(), to avoid conflicting with those values changing when starting the render for the next image.
			{
				writeThread = std::thread(saveFunc, std::ref(finalImages[threadVecIndex]), filename, comments, renderer->FinalRasW(), renderer->FinalRasH(), renderer->NumChannels());
				finalImageIndex ^= 1;//Toggle the index.
			}
			else
				saveFunc(finalImages[threadVecIndex], filename, comments, renderer->FinalRasW(), renderer->FinalRasH(), renderer->NumChannels());//Will always use the first index, thereby not requiring more memory.
		}

		if (writeThread.joinable())//One final check to make sure all writing is done before exiting this thread.
			writeThread.join();
	};

	threadVec.reserve(renderers.size());

	for (size_t r = 0; r < renderers.size(); r++)
	{
		threadVec.push_back(std::thread([&](size_t dev)
		{
			iterFunc(dev);
		}, r));
	}

	for (auto& th : threadVec)
		if (th.joinable())
			th.join();

	t.Toc("\nFinished in: ", true);

	return true;
}

/// <summary>
/// Main program entry point for EmberAnimate.exe.
/// </summary>
/// <param name="argc">The number of command line arguments passed</param>
/// <param name="argv">The command line arguments passed</param>
/// <returns>0 if successful, else 1.</returns>
int _tmain(int argc, _TCHAR* argv[])
{
	bool b = false;
	EmberOptions opt;

	//Required for large allocs, else GPU memory usage will be severely limited to small sizes.
	//This must be done in the application and not in the EmberCL DLL.
#ifdef WIN32
	_putenv_s("GPU_MAX_ALLOC_PERCENT", "100");
#else
	putenv(const_cast<char*>("GPU_MAX_ALLOC_PERCENT=100"));
#endif

	if (!opt.Populate(argc, argv, OPT_USE_ANIMATE))
	{

#ifdef DO_DOUBLE
		if (opt.Bits() == 64)
		{
			b = EmberAnimate<double>(opt);
		}
		else
#endif
		if (opt.Bits() == 33)
		{
			b = EmberAnimate<float>(opt);
		}
		else if (opt.Bits() == 32)
		{
			cout << "Bits 32/int histogram no longer supported. Using bits == 33 (float)." << endl;
			b = EmberAnimate<float>(opt);
		}
	}

	return b ? 0 : 1;
}
