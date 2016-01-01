#include "FractoriumPch.h"
#include "FractoriumEmberController.h"
#include "FinalRenderEmberController.h"
#include "FinalRenderDialog.h"
#include "Fractorium.h"

/// <summary>
/// Constructor which accepts a pointer to the final render dialog.
/// It passes a pointer to the main window to the base and initializes members.
/// </summary>
/// <param name="finalRender">Pointer to the final render dialog</param>
FinalRenderEmberControllerBase::FinalRenderEmberControllerBase(FractoriumFinalRenderDialog* finalRenderDialog)
	: FractoriumEmberControllerBase(finalRenderDialog->m_Fractorium)
{
	m_Run = false;
	m_PreviewRun = false;
	m_ImageCount = 0;
	m_FinishedImageCount.store(0);
	m_FinalRenderDialog = finalRenderDialog;
	m_Settings = m_Fractorium->m_Settings;
}

/// <summary>
/// Cancel the render by calling Abort().
/// This will block until the cancelling is actually finished.
/// It should never take longer than a few milliseconds because the
/// renderer checks the m_Abort flag in many places during the process.
/// </summary>
template <typename T>
void FinalRenderEmberController<T>::CancelRender()
{
	if (m_Result.isRunning())
	{
		tbb::task_group g;
		g.run([&]
		{
			m_Run = false;

			if (m_Renderer.get())
			{
				m_Renderer->Abort();

				while (m_Renderer->InRender())
					QApplication::processEvents();

				m_Renderer->EnterRender();
				m_Renderer->EnterFinalAccum();
				m_Renderer->LeaveFinalAccum();
				m_Renderer->LeaveRender();
			}
			else
			{
				for (auto& renderer : m_Renderers)
				{
					renderer->Abort();

					while (renderer->InRender())
						QApplication::processEvents();

					renderer->EnterRender();
					renderer->EnterFinalAccum();
					renderer->LeaveFinalAccum();
					renderer->LeaveRender();
				}
			}
		});
		g.wait();

		while (m_Result.isRunning())
			QApplication::processEvents();

		m_FinalRenderDialog->ui.FinalRenderTextOutput->append("Render canceled.");
	}
}

/// <summary>
/// Create a new renderer based on the options selected on the GUI.
/// If a renderer matching the options has already been created, no action is taken.
/// </summary>
/// <returns>True if a valid renderer is created or if no action is taken, else false.</returns>
bool FinalRenderEmberControllerBase::CreateRendererFromGUI()
{
	bool useOpenCL = m_Info->Ok() && m_FinalRenderDialog->OpenCL();
	auto v = Devices(m_FinalRenderDialog->Devices());
	return CreateRenderer((useOpenCL && !v.empty()) ? OPENCL_RENDERER : CPU_RENDERER,
						  v,
						  false);//Not shared.
}

/// <summary>
/// Thin wrapper around invoking a call to append text to the output.
/// </summary>
/// <param name="s">The string to append</param>
void FinalRenderEmberControllerBase::Output(const QString& s)
{
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderTextOutput, "append", Qt::QueuedConnection, Q_ARG(const QString&, s));
}

/// <summary>
/// Constructor which accepts a pointer to the final render dialog and passes it to the base.
/// The main final rendering lambda function is constructed here.
/// </summary>
/// <param name="finalRender">Pointer to the final render dialog</param>
template<typename T>
FinalRenderEmberController<T>::FinalRenderEmberController(FractoriumFinalRenderDialog* finalRender)
	: FinalRenderEmberControllerBase(finalRender)
{
	m_FinalPreviewRenderer = unique_ptr<EmberNs::Renderer<T, float>>(new EmberNs::Renderer<T, float>());
	m_FinalPreviewRenderer->Callback(nullptr);
	m_FinalPreviewRenderer->NumChannels(4);
	m_FinalPreviewRenderFunc = [&]()
	{
		m_PreviewCs.Enter();//Thread prep.
		m_PreviewRun = true;
		m_FinalPreviewRenderer->Abort();
		T scalePercentage;
		size_t maxDim = 100;
		QLabel* widget = m_FinalRenderDialog->ui.FinalRenderPreviewLabel;

		//Determine how to scale the scaled ember to fit in the label with a max of 100x100.
		if (m_Ember->m_FinalRasW >= m_Ember->m_FinalRasH)
			scalePercentage = T(maxDim) / m_Ember->m_FinalRasW;
		else
			scalePercentage = T(maxDim) / m_Ember->m_FinalRasH;

		m_PreviewEmber = *m_Ember;
		m_PreviewEmber.m_Quality = 100;
		m_PreviewEmber.m_TemporalSamples = 1;
		m_PreviewEmber.m_FinalRasW = std::max<size_t>(1, std::min<size_t>(maxDim, size_t(scalePercentage * m_Ember->m_FinalRasW)));//Ensure neither is zero.
		m_PreviewEmber.m_FinalRasH = std::max<size_t>(1, std::min<size_t>(maxDim, size_t(scalePercentage * m_Ember->m_FinalRasH)));
		m_PreviewEmber.m_PixelsPerUnit = scalePercentage * m_Ember->m_PixelsPerUnit;
		m_FinalPreviewRenderer->EarlyClip(m_FinalRenderDialog->EarlyClip());
		m_FinalPreviewRenderer->YAxisUp(m_FinalRenderDialog->YAxisUp());
		m_FinalPreviewRenderer->Transparency(m_FinalRenderDialog->Transparency());
		m_FinalPreviewRenderer->SetEmber(m_PreviewEmber);
		m_FinalPreviewRenderer->PrepFinalAccumVector(m_PreviewFinalImage);//Must manually call this first because it could be erroneously made smaller due to strips if called inside Renderer::Run().
		auto strips = VerifyStrips(m_PreviewEmber.m_FinalRasH, m_FinalRenderDialog->Strips(),
		[&](const string & s) { }, [&](const string & s) { }, [&](const string & s) { });
		StripsRender<T>(m_FinalPreviewRenderer.get(), m_PreviewEmber, m_PreviewFinalImage, 0, strips, m_FinalRenderDialog->YAxisUp(),
		[&](size_t strip) { },//Pre strip.
		[&](size_t strip) { },//Post strip.
		[&](size_t strip) { },//Error.
		[&](Ember<T>& finalEmber)//Final strip.
		{
			QImage image(finalEmber.m_FinalRasW, finalEmber.m_FinalRasH, QImage::Format_RGBA8888);//The label wants RGBA.
			memcpy(image.scanLine(0), m_PreviewFinalImage.data(), finalEmber.m_FinalRasW * finalEmber.m_FinalRasH * 4);//Memcpy the data in.
			QPixmap pixmap = QPixmap::fromImage(image);
			QMetaObject::invokeMethod(widget, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, pixmap));
		});
		m_PreviewRun = false;
		m_PreviewCs.Leave();
	};
	//The main rendering function which will be called in a Qt thread.
	//A backup Xml is made before the rendering process starts just in case it crashes before finishing.
	//If it finishes successfully, delete the backup file.
	m_FinalRenderFunc = [&]()
	{
		m_Run = true;
		m_TotalTimer.Tic();//Begin timing for progress of all operations.
		m_GuiState = m_FinalRenderDialog->State();//Cache render settings from the GUI before running.
		size_t i;
		bool doAll = m_GuiState.m_DoAll && m_EmberFile.Size() > 1;
		size_t currentStripForProgress = 0;//Sort of a hack to get the strip value to the progress function.
		QString path = doAll ? ComposePath(QString::fromStdString(m_EmberFile.m_Embers[0].m_Name)) : ComposePath(Name());
		QString backup = path + "_backup.flame";

		//Save backup Xml.
		if (doAll)
			m_XmlWriter.Save(backup.toStdString().c_str(), m_EmberFile.m_Embers, 0, true, false, true);
		else
			m_XmlWriter.Save(backup.toStdString().c_str(), *m_Ember, 0, true, false, true);

		m_FinishedImageCount.store(0);
		SyncGuiToRenderer();
		FirstOrDefaultRenderer()->m_ProgressParameter = reinterpret_cast<void*>(&currentStripForProgress);//When animating, only the first (primary) device has a progress parameter.
		m_GuiState.m_Strips = VerifyStrips(m_Ember->m_FinalRasH, m_GuiState.m_Strips,
		[&](const string & s) { Output(QString::fromStdString(s)); }, //Greater than height.
		[&](const string & s) { Output(QString::fromStdString(s)); }, //Mod height != 0.
		[&](const string & s) { Output(QString::fromStdString(s) + "\n"); }); //Final strips value to be set.
		ResetProgress();

		//The rendering process is different between doing a single image, and doing multiple.
		if (doAll)
		{
			m_ImageCount = m_EmberFile.Size();

			//Different action required for rendering as animation or not.
			if (m_GuiState.m_DoSequence && !m_Renderers.empty())
			{
				Ember<T>* firstEmber = &m_EmberFile.m_Embers[0];

				//Need to loop through and set all w, h, q, ts, ss and t vals.
				for (i = 0; i < m_EmberFile.Size() && m_Run; i++)
				{
					SyncGuiToEmber(m_EmberFile.m_Embers[i], firstEmber->m_FinalRasW, firstEmber->m_FinalRasH);

					if (i > 0)
					{
						if (m_EmberFile.m_Embers[i].m_Time <= m_EmberFile.m_Embers[i - 1].m_Time)
							m_EmberFile.m_Embers[i].m_Time  = m_EmberFile.m_Embers[i - 1].m_Time + 1;
					}
					else if (i == 0)
					{
						m_EmberFile.m_Embers[i].m_Time = 0;
					}

					m_EmberFile.m_Embers[i].m_TemporalSamples = m_GuiState.m_TemporalSamples;
				}

				std::atomic<size_t> atomfTime;
				vector<std::thread> threadVec;
				//Not supporting strips with animation.
				//Shouldn't be a problem because animations will be at max 4k x 2k which will take about 1GB
				//even when using double precision, which most cards at the time of this writing already exceed.
				m_GuiState.m_Strips = 1;
				atomfTime.store(0);
				std::function<void(size_t)> iterFunc = [&](size_t index)
				{
					size_t ftime;
					size_t finalImageIndex = 0;
					std::thread writeThread;
					vector<byte> finalImages[2];
					EmberStats stats;
					EmberImageComments comments;
					Timing renderTimer;
					auto renderer = m_Renderers[index].get();
					renderer->SetEmber(m_EmberFile.m_Embers);//Copy all embers to the local storage inside the renderer.

					//Render each image, cancelling if m_Run ever gets set to false.
					while (atomfTime.fetch_add(1), ((ftime = atomfTime.load() - 1) < m_EmberFile.Size()) && m_Run)//Needed to set 1 to claim this iter from other threads, so decrement it to be zero-indexed here.
					{
						T localTime = T(ftime);
						Output("Image " + ToString(ftime + 1ULL) + ":\n" + ComposePath(QString::fromStdString(m_EmberFile.m_Embers[ftime].m_Name)));
						renderer->Reset();//Have to manually set this since the ember is not set each time through.
						renderTimer.Tic();//Toc() is called in RenderComplete().

						//Can't use strips render here. Run() must be called directly for animation.
						if (renderer->Run(finalImages[finalImageIndex], localTime) != eRenderStatus::RENDER_OK)
						{
							Output("Rendering failed.\n");
							m_Fractorium->ErrorReportToQTextEdit(renderer->ErrorReport(), m_FinalRenderDialog->ui.FinalRenderTextOutput, false);//Internally calls invoke.
							atomfTime.store(m_EmberFile.Size() + 1);//Abort all threads if any of them encounter an error.
							break;
						}
						else
						{
							if (writeThread.joinable())
								writeThread.join();

							stats = renderer->Stats();
							comments = renderer->ImageComments(stats, 0, false, true);
							writeThread = std::thread([&](size_t tempTime, size_t threadFinalImageIndex)
							{
								SaveCurrentRender(m_EmberFile.m_Embers[tempTime],
												  comments,//These all don't change during the renders, so it's ok to access them in the thread.
												  finalImages[threadFinalImageIndex],
												  renderer->FinalRasW(),
												  renderer->FinalRasH(),
												  renderer->NumChannels(),
												  renderer->BytesPerChannel());
							}, ftime, finalImageIndex);
							m_FinishedImageCount.fetch_add(1);
							RenderComplete(m_EmberFile.m_Embers[ftime], stats, renderTimer);

							if (!index)//Only first device has a progress callback, so it also makes sense to only manually set the progress on the first device as well.
								HandleFinishedProgress();
						}

						finalImageIndex ^= 1;//Toggle the index.
					}

					if (writeThread.joinable())//One final check to make sure all writing is done before exiting this thread.
						writeThread.join();
				};
				threadVec.reserve(m_Renderers.size());

				for (size_t r = 0; r < m_Renderers.size(); r++)
				{
					threadVec.push_back(std::thread([&](size_t index)
					{
						iterFunc(index);
					}, r));
				}

				for (auto& th : threadVec)
					if (th.joinable())
						th.join();

				HandleFinishedProgress();//One final check that all images were finished.
			}
			else if (m_Renderer.get())//Make sure a renderer was created and render all images, but not as an animation sequence (without temporal samples motion blur).
			{
				//Render each image, cancelling if m_Run ever gets set to false.
				for (i = 0; i < m_EmberFile.Size() && m_Run; i++)
				{
					Output("Image " + ToString<qulonglong>(m_FinishedImageCount.load() + 1) + ":\n" + ComposePath(QString::fromStdString(m_EmberFile.m_Embers[i].m_Name)));
					m_EmberFile.m_Embers[i].m_TemporalSamples = 1;//No temporal sampling.
					m_Renderer->SetEmber(m_EmberFile.m_Embers[i]);
					m_Renderer->PrepFinalAccumVector(m_FinalImage);//Must manually call this first because it could be erroneously made smaller due to strips if called inside Renderer::Run().
					m_Stats.Clear();
					Memset(m_FinalImage);
					m_RenderTimer.Tic();//Toc() is called in RenderComplete().
					StripsRender<T>(m_Renderer.get(), m_EmberFile.m_Embers[i], m_FinalImage, 0, m_GuiState.m_Strips, m_GuiState.m_YAxisUp,
					[&](size_t strip) { currentStripForProgress = strip; },//Pre strip.
					[&](size_t strip) { m_Stats += m_Renderer->Stats(); },//Post strip.
					[&](size_t strip)//Error.
					{
						Output("Rendering failed.\n");
						m_Fractorium->ErrorReportToQTextEdit(m_Renderer->ErrorReport(), m_FinalRenderDialog->ui.FinalRenderTextOutput, false);//Internally calls invoke.
					},
					[&](Ember<T>& finalEmber)
					{
						m_FinishedImageCount.fetch_add(1);
						SaveCurrentRender(finalEmber);
						RenderComplete(finalEmber);
						HandleFinishedProgress();
					});//Final strip.
				}
			}
			else
			{
				Output("No renderer present, aborting.");
			}
		}
		else if (m_Renderer.get())//Render a single image.
		{
			m_ImageCount = 1;
			m_Ember->m_TemporalSamples = 1;
			m_Renderer->SetEmber(*m_Ember);
			m_Renderer->PrepFinalAccumVector(m_FinalImage);//Must manually call this first because it could be erroneously made smaller due to strips if called inside Renderer::Run().
			m_Stats.Clear();
			Memset(m_FinalImage);
			Output(ComposePath(QString::fromStdString(m_Ember->m_Name)));
			m_RenderTimer.Tic();//Toc() is called in RenderComplete().
			StripsRender<T>(m_Renderer.get(), *m_Ember, m_FinalImage, 0, m_GuiState.m_Strips, m_GuiState.m_YAxisUp,
			[&](size_t strip) { currentStripForProgress = strip; },//Pre strip.
			[&](size_t strip) { m_Stats += m_Renderer->Stats(); },//Post strip.
			[&](size_t strip)//Error.
			{
				Output("Rendering failed.\n");
				m_Fractorium->ErrorReportToQTextEdit(m_Renderer->ErrorReport(), m_FinalRenderDialog->ui.FinalRenderTextOutput, false);//Internally calls invoke.
			},
			[&](Ember<T>& finalEmber)
			{
				m_FinishedImageCount.fetch_add(1);
				SaveCurrentRender(finalEmber);
				RenderComplete(finalEmber);
				HandleFinishedProgress();
			});//Final strip.
		}
		else
		{
			Output("No renderer present, aborting.");
		}

		QString totalTimeString = "All renders completed in: " + QString::fromStdString(m_TotalTimer.Format(m_TotalTimer.Toc())) + ".";
		Output(totalTimeString);
		QFile::remove(backup);
		m_Run = false;
	};
}

/// <summary>
/// Virtual functions overridden from FractoriumEmberControllerBase.
/// </summary>

/// <summary>
/// Setters for embers and ember files which convert between float and double types.
/// These are used to preserve the current ember/file when switching between renderers.
/// Note that some precision will be lost when going from double to float.
/// </summary>
template <typename T> void FinalRenderEmberController<T>::SetEmberFile(const EmberFile<float>& emberFile)
{
	m_EmberFile = emberFile;

	if (m_EmberFile.Size())
		m_Ember = &(m_EmberFile.m_Embers[0]);
}
template <typename T> void FinalRenderEmberController<T>::CopyEmberFile(EmberFile<float>& emberFile, std::function<void(Ember<float>& ember)> perEmberOperation)
{
	emberFile.m_Filename = m_EmberFile.m_Filename;
	CopyVec(emberFile.m_Embers, m_EmberFile.m_Embers, perEmberOperation);
}

#ifdef DO_DOUBLE
template <typename T> void FinalRenderEmberController<T>::SetEmberFile(const EmberFile<double>& emberFile)
{
	m_EmberFile = emberFile;

	if (m_EmberFile.Size())
		m_Ember = &(m_EmberFile.m_Embers[0]);
}
template <typename T> void FinalRenderEmberController<T>::CopyEmberFile(EmberFile<double>& emberFile, std::function<void(Ember<double>& ember)> perEmberOperation)
{
	emberFile.m_Filename = m_EmberFile.m_Filename;
	CopyVec(emberFile.m_Embers, m_EmberFile.m_Embers, perEmberOperation);
}
#endif

/// <summary>
/// Set the ember at the specified index from the currently opened file as the current Ember.
/// Clears the undo state.
/// Resets the rendering process.
/// </summary>
/// <param name="index">The index in the file from which to retrieve the ember</param>
template <typename T>
void FinalRenderEmberController<T>::SetEmber(size_t index)
{
	if (index < m_EmberFile.Size())
	{
		m_Ember = &(m_EmberFile.m_Embers[index]);
		SyncCurrentToGui();
	}
	else if (m_EmberFile.Size() > 1)
	{
		m_Ember = &(m_EmberFile.m_Embers[0]);//Should never happen.
	}
}

/// <summary>
/// Start the final rendering process.
/// Create the needed renderer from the GUI if it has not been created yet.
/// </summary>
/// <returns></returns>
template<typename T>
bool FinalRenderEmberController<T>::Render()
{
	QString filename = m_FinalRenderDialog->Path();

	if (filename == "")
	{
		m_Fractorium->ShowCritical("File Error", "Please enter a valid path and filename for the output.");
		return false;
	}

	if (CreateRendererFromGUI())
	{
		m_FinalRenderDialog->ui.FinalRenderTextOutput->setText("Preparing all parameters.\n");
		//Note that a Qt thread must be used, rather than a tbb task.
		//This is because tbb does a very poor job of allocating thread resources
		//and dedicates an entire core just to this thread which does nothing waiting for the
		//parallel iteration loops inside of the CPU renderer to finish. The result is that
		//the renderer ends up using ThreadCount - 1 to iterate, instead of ThreadCount.
		//By using a Qt thread here, and tbb inside the renderer, all cores can be maxed out.
		m_Result = QtConcurrent::run(m_FinalRenderFunc);
		m_Settings->sync();
		return true;
	}
	else
		return false;
}

/// <summary>
/// Stop rendering and initialize a new renderer, using the specified type and the options on the final render dialog.
/// </summary>
/// <param name="renderType">The type of render to create</param>
/// <param name="devices">The platform,device index pairs of the devices to use</param>
/// <param name="shared">True if shared with OpenGL, else false. Always false in this case.</param>
/// <returns>True if nothing went wrong, else false.</returns>
template <typename T>
bool FinalRenderEmberController<T>::CreateRenderer(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared)
{
	bool ok = true;
	bool deviceDiff = false;
	//uint channels = m_FinalRenderDialog->Ext().endsWith("png", Qt::CaseInsensitive) ? 4 : 3;
	bool renderTypeMismatch = (m_Renderer.get() && (m_Renderer->RendererType() != renderType)) ||
							  (!m_Renderers.empty() && (m_Renderers[0]->RendererType() != renderType));
	CancelRender();

	if ((!m_FinalRenderDialog->DoSequence() && (!m_Renderer.get() || !m_Renderer->Ok())) ||
			(m_FinalRenderDialog->DoSequence() && m_Renderers.empty()) ||
			renderTypeMismatch ||
			!Equal(m_Devices, devices))
	{
		EmberReport emberReport;
		vector<string> errorReport;
		m_Devices = devices;//Store values for re-creation later on.
		m_OutputTexID = 0;//Don't care about tex ID when doing final render.
		m_Shared = shared;//So shared is of course false.

		if (m_FinalRenderDialog->DoSequence())
		{
			m_Renderer.reset();
			m_Renderers = ::CreateRenderers<T>(renderType, m_Devices, shared, m_OutputTexID, emberReport);
		}
		else
		{
			m_Renderers.clear();
			m_Renderer = unique_ptr<EmberNs::RendererBase>(::CreateRenderer<T>(renderType, m_Devices, shared, m_OutputTexID, emberReport));
		}

		errorReport = emberReport.ErrorReport();

		if (!errorReport.empty())
		{
			ok = false;
			m_Fractorium->ShowCritical("Renderer Creation Error", "Could not create requested renderer, fallback CPU renderer created. See info tab for details.");
			m_Fractorium->ErrorReportToQTextEdit(errorReport, m_Fractorium->ui.InfoRenderingTextEdit);
		}
	}

	return SyncGuiToRenderer() && ok;
}

/// <summary>
/// Progress function.
/// Take special action to sync options upon finishing.
/// Note this is only called on the primary renderer.
/// </summary>
/// <param name="ember">The ember currently being rendered</param>
/// <param name="foo">An extra dummy parameter</param>
/// <param name="fraction">The progress fraction from 0-100</param>
/// <param name="stage">The stage of iteration. 1 is iterating, 2 is density filtering, 2 is final accumulation.</param>
/// <param name="etaMs">The estimated milliseconds to completion of the current stage</param>
/// <returns>0 if the user has clicked cancel, else 1 to continue rendering.</returns>
template <typename T>
int FinalRenderEmberController<T>::ProgressFunc(Ember<T>& ember, void* foo, double fraction, int stage, double etaMs)
{
	static int count = 0;
	size_t strip = *(reinterpret_cast<size_t*>(FirstOrDefaultRenderer()->m_ProgressParameter));
	double fracPerStrip = ceil(100.0 / m_GuiState.m_Strips);
	double stripsfrac = ceil(fracPerStrip * strip) + ceil(fraction / m_GuiState.m_Strips);
	int intFract = int(stripsfrac);

	if (stage == 0)
		QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderIterationProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, intFract));
	else if (stage == 1)
		QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderFilteringProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, intFract));
	else if (stage == 2)
		QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderAccumProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, intFract));

	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderImageCountLabel, "setText", Qt::QueuedConnection, Q_ARG(const QString&, ToString<qulonglong>(m_FinishedImageCount.load() + 1) + " / " + ToString<qulonglong>(m_ImageCount) + " Eta: " + QString::fromStdString(m_RenderTimer.Format(etaMs))));
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderTextOutput, "update", Qt::QueuedConnection);
	return m_Run ? 1 : 0;
}

/// <summary>
/// Virtual functions overridden from FinalRenderEmberControllerBase.
/// </summary>

/// <summary>
/// Copy current ember values to widgets.
/// </summary>
template <typename T>
void FinalRenderEmberController<T>::SyncCurrentToGui()
{
	SyncCurrentToSizeSpinners(true, true);
	m_FinalRenderDialog->ui.FinalRenderCurrentSpin->setSuffix("  " + Name());
	m_FinalRenderDialog->Scale(m_Ember->ScaleType());
	m_FinalRenderDialog->m_QualitySpin->SetValueStealth(m_Ember->m_Quality);
	m_FinalRenderDialog->m_SupersampleSpin->SetValueStealth(m_Ember->m_Supersample);
	m_FinalRenderDialog->Path(ComposePath(Name()));
}

/// <summary>
/// Copy GUI values to either the current ember, or all embers in the file
/// depending on whether Render All is checked.
/// </summary>
/// <param name="widthOverride">Width override to use instead of scaling the original width</param>
/// <param name="heightOverride">Height override to use instead of scaling the original height</param>
template <typename T>
void FinalRenderEmberController<T>::SyncGuiToEmbers(size_t widthOverride, size_t heightOverride)
{
	if (m_FinalRenderDialog->ApplyToAll())
	{
		for (auto& ember : m_EmberFile.m_Embers)
			SyncGuiToEmber(ember, widthOverride, heightOverride);
	}
	else
	{
		SyncGuiToEmber(*m_Ember, widthOverride, heightOverride);
	}
}

/// <summary>
/// Copy GUI values to the renderers.
/// </summary>
template <typename T>
bool FinalRenderEmberController<T>::SyncGuiToRenderer()
{
	bool ok = true;
	uint channels = m_FinalRenderDialog->Ext().endsWith("png", Qt::CaseInsensitive) ? 4 : 3;

	if (m_Renderer.get())
	{
		if (m_Renderer->RendererType() == OPENCL_RENDERER)
			channels = 4;//Always using 4 since the GL texture is RGBA.

		m_Renderer->Callback(this);
		m_Renderer->NumChannels(channels);
		m_Renderer->EarlyClip(m_FinalRenderDialog->EarlyClip());
		m_Renderer->YAxisUp(m_FinalRenderDialog->YAxisUp());
		m_Renderer->ThreadCount(m_FinalRenderDialog->ThreadCount());
		m_Renderer->Priority((eThreadPriority)m_FinalRenderDialog->ThreadPriority());
		m_Renderer->Transparency(m_FinalRenderDialog->Transparency());
	}
	else if (!m_Renderers.empty())
	{
		for (size_t i = 0; i < m_Renderers.size(); i++)
		{
			if (m_Renderers[i]->RendererType() == OPENCL_RENDERER)
				channels = 4;//Always using 4 since the GL texture is RGBA.

			m_Renderers[i]->Callback(!i ? this : nullptr);
			m_Renderers[i]->NumChannels(channels);
			m_Renderers[i]->EarlyClip(m_FinalRenderDialog->EarlyClip());
			m_Renderers[i]->YAxisUp(m_FinalRenderDialog->YAxisUp());
			m_Renderers[i]->ThreadCount(m_FinalRenderDialog->ThreadCount());
			m_Renderers[i]->Priority((eThreadPriority)m_FinalRenderDialog->ThreadPriority());
			m_Renderers[i]->Transparency(m_FinalRenderDialog->Transparency());
		}
	}
	else
	{
		ok = false;
		m_Fractorium->ShowCritical("Renderer Creation Error", "No renderer present, aborting. See info tab for details.");
	}

	return ok;
}

/// <summary>
/// Set values for scale spinners based on the ratio of the original dimensions to the current dimensions
/// of the current ember. Also update the size suffix text.
/// </summary>
/// <param name="scale">Whether to update the scale values</param>
/// <param name="size">Whether to update the size suffix text</param>
template <typename T>
void FinalRenderEmberController<T>::SyncCurrentToSizeSpinners(bool scale, bool size)
{
	if (scale)
	{
		m_FinalRenderDialog->m_WidthScaleSpin->SetValueStealth(double(m_Ember->m_FinalRasW) / m_Ember->m_OrigFinalRasW);//Work backward to determine the scale.
		m_FinalRenderDialog->m_HeightScaleSpin->SetValueStealth(double(m_Ember->m_FinalRasH) / m_Ember->m_OrigFinalRasH);
	}

	if (size)
	{
		m_FinalRenderDialog->m_WidthScaleSpin->setSuffix(" (" + ToString<qulonglong>(m_Ember->m_FinalRasW) + ")");
		m_FinalRenderDialog->m_HeightScaleSpin->setSuffix(" (" + ToString<qulonglong>(m_Ember->m_FinalRasH) + ")");
	}
}

/// <summary>
/// Reset the progress bars.
/// </summary>
/// <param name="total">True to reset render image and total progress bars, else false to only do iter, filter and accum bars.</param>
template <typename T>
void FinalRenderEmberController<T>::ResetProgress(bool total)
{
	if (total)
	{
		QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderImageCountLabel, "setText",  Qt::QueuedConnection, Q_ARG(const QString&, "0 / " + ToString<qulonglong>(m_ImageCount)));
		QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderTotalProgress,   "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
	}

	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderIterationProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderFilteringProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderAccumProgress,     "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
}

/// <summary>
/// Set various parameters in the renderers and current ember with the values
/// specified in the widgets and compute the amount of memory required to render.
/// This includes the memory needed for the final output image.
/// </summary>
/// <returns>If successful, memory required in bytes, else zero.</returns>
template <typename T>
tuple<size_t, size_t, size_t> FinalRenderEmberController<T>::SyncAndComputeMemory()
{
	size_t iterCount;
	pair<size_t, size_t> p(0, 0);
	size_t strips;
	bool b = false;
	uint channels = m_FinalRenderDialog->Ext() == "png" ? 4 : 3;//4 channels for Png, else 3.
	SyncGuiToEmbers();

	if (m_Renderer.get())
	{
		strips = VerifyStrips(m_Ember->m_FinalRasH, m_FinalRenderDialog->Strips(),
		[&](const string & s) {}, [&](const string & s) {}, [&](const string & s) {});
		m_Renderer->SetEmber(*m_Ember);
		m_Renderer->CreateSpatialFilter(b);
		m_Renderer->CreateTemporalFilter(b);
		m_Renderer->NumChannels(channels);
		m_Renderer->ComputeBounds();
		m_Renderer->ComputeQuality();
		m_Renderer->ComputeCamera();
		CancelPreviewRender();
		m_FinalPreviewRenderFunc();
		p = m_Renderer->MemoryRequired(strips, true, m_FinalRenderDialog->DoSequence());
		iterCount = m_Renderer->TotalIterCount(strips);
	}
	else if (!m_Renderers.empty())
	{
		for (auto& renderer : m_Renderers)
		{
			renderer->SetEmber(*m_Ember);
			renderer->CreateSpatialFilter(b);
			renderer->CreateTemporalFilter(b);
			renderer->NumChannels(channels);
			renderer->ComputeBounds();
			renderer->ComputeQuality();
			renderer->ComputeCamera();
		}

		CancelPreviewRender();
		m_FinalPreviewRenderFunc();
		strips = 1;
		p = m_Renderers[0]->MemoryRequired(1, true, m_FinalRenderDialog->DoSequence());
		iterCount = m_Renderers[0]->TotalIterCount(strips);
	}

	m_FinalRenderDialog->m_StripsSpin->setSuffix(" (" + ToString<qulonglong>(strips) + ")");
	return tuple<size_t, size_t, size_t>(p.first, p.second, iterCount);
}

/// <summary>
/// Compose a final output path given a base name.
/// This includes the base path, the prefix, the name, the suffix and the
/// extension.
/// </summary>
/// <param name="name">The base filename to compose a full path for</param>
/// <returns>The fully composed path</returns>
template <typename T>
QString FinalRenderEmberController<T>::ComposePath(const QString& name)
{
	QString path = MakeEnd(m_Settings->SaveFolder(), '/');//Base path.
	QString full = path + m_FinalRenderDialog->Prefix() + name + m_FinalRenderDialog->Suffix() + "." + m_FinalRenderDialog->Ext();
	return EmberFile<T>::UniqueFilename(full);
}

/// <summary>
/// Non-virtual functions declared in FinalRenderEmberController<T>.
/// </summary>

/// <summary>
/// Return either m_Renderer in the case of running a CPU renderer, else
/// m_Renderers[0] in the case of running OpenCL.
/// </summary>
/// <returns>The primary renderer</returns>
template <typename T>
EmberNs::Renderer<T, float>* FinalRenderEmberController<T>::FirstOrDefaultRenderer()
{
	if (m_Renderer.get())
		return dynamic_cast<EmberNs::Renderer<T, float>*>(m_Renderer.get());
	else if (!m_Renderers.empty())
		return dynamic_cast<EmberNs::Renderer<T, float>*>(m_Renderers[0].get());
	else
	{
		throw "No final renderer, exiting.";
		return nullptr;
	}
}

/// <summary>
/// Stop the preview renderer.
/// This is meant to only be called programatically and never by the user.
/// </summary>
template <typename T>
void FinalRenderEmberController<T>::CancelPreviewRender()
{
	m_FinalPreviewRenderer->Abort();

	while (m_FinalPreviewRenderer->InRender()) { QApplication::processEvents(); }

	while (m_PreviewRun) { QApplication::processEvents(); }

	while (m_FinalPreviewResult.isRunning()) { QApplication::processEvents(); }
}

/// <summary>
/// Save the output of the render.
/// </summary>
/// <param name="ember">The ember whose rendered output will be saved</param>
template<typename T>
void FinalRenderEmberController<T>::SaveCurrentRender(Ember<T>& ember)
{
	auto comments = m_Renderer->ImageComments(m_Stats, 0, false, true);
	SaveCurrentRender(ember, comments, m_FinalImage, m_Renderer->FinalRasW(), m_Renderer->FinalRasH(), m_Renderer->NumChannels(), m_Renderer->BytesPerChannel());
}

/// <summary>
/// Save the output of the render.
/// </summary>
/// <param name="ember">The ember whose rendered output will be saved</param>
/// <param name="comments">The comments to save in the png or jpg</param>
/// <param name="pixels">The buffer containing the pixels</param>
/// <param name="width">The width in pixels of the image</param>
/// <param name="height">The height in pixels of the image</param>
/// <param name="channels">The number of channels, 3 or 4.</param>
/// <param name="bpc">The bytes per channel, almost always 1.</param>
template<typename T>
void FinalRenderEmberController<T>::SaveCurrentRender(Ember<T>& ember, const EmberImageComments& comments, vector<byte>& pixels, size_t width, size_t height, size_t channels, size_t bpc)
{
	QString filename = ComposePath(QString::fromStdString(ember.m_Name));
	FractoriumEmberControllerBase::SaveCurrentRender(filename, comments, pixels, width, height, channels, bpc);
}

/// <summary>
/// Action to take when rendering an image completes.
/// Thin wrapper around the function of the same name that takes more arguments.
/// Just passes m_Renderer and m_FinalImage.
/// </summary>
/// <param name="ember">The ember currently being rendered</param>
template<typename T>
void FinalRenderEmberController<T>::RenderComplete(Ember<T>& ember)
{
	if (auto renderer = dynamic_cast<EmberNs::Renderer<T, float>*>(m_Renderer.get()))
		RenderComplete(ember, m_Stats, m_RenderTimer);
}

/// <summary>
/// Handle setting the appropriate progress bar values when an image render has finished.
/// This handles single image, animations, and strips.
/// </summary>
template<typename T>
void FinalRenderEmberController<T>::HandleFinishedProgress()
{
	auto finishedCountCached = m_FinishedImageCount.load();//Make sure to use the same value throughout this function even if the atomic is changing.

	if (m_FinishedImageCount.load() != m_ImageCount)
		ResetProgress(false);
	else
		SetProgressComplete(100);//Just to be safe.

	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderTotalProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, int((float(finishedCountCached) / float(m_ImageCount)) * 100)));
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderImageCountLabel, "setText", Qt::QueuedConnection, Q_ARG(const QString&, ToString<qulonglong>(finishedCountCached) + " / " + ToString<qulonglong>(m_ImageCount)));
}

/// <summary>
/// Action to take when rendering an image completes.
/// </summary>
/// <param name="ember">The ember currently being rendered</param>
/// <param name="stats">The renderer stats</param>
/// <param name="pixels">The timer which was started at the beginning of the render</param>
template<typename T>
void FinalRenderEmberController<T>::RenderComplete(Ember<T>& ember, const EmberStats& stats, Timing& renderTimer)
{
	m_ProgressCs.Enter();
	string renderTimeString = renderTimer.Format(renderTimer.Toc()), totalTimeString;
	QString status, filename = ComposePath(QString::fromStdString(ember.m_Name));
	QString itersString = ToString<qulonglong>(stats.m_Iters);
	QString itersPerSecString = ToString<qulonglong>(size_t(stats.m_Iters / (stats.m_IterMs / 1000.0)));

	if (m_GuiState.m_SaveXml)
	{
		QFileInfo xmlFileInfo(filename);//Create another one in case it was modified for batch rendering.
		QString newPath = xmlFileInfo.absolutePath() + '/' + xmlFileInfo.completeBaseName() + ".flame";
		xmlDocPtr tempEdit = ember.m_Edits;
		ember.m_Edits = m_XmlWriter.CreateNewEditdoc(&ember, nullptr, "edit", m_Settings->Nick().toStdString(), m_Settings->Url().toStdString(), m_Settings->Id().toStdString(), "", 0, 0);
		m_XmlWriter.Save(newPath.toStdString().c_str(), ember, 0, true, false, true);//Note that the ember passed is used, rather than m_Ember because it's what was actually rendered.

		if (tempEdit)
			xmlFreeDoc(tempEdit);
	}

	status = "Pure render time: " + QString::fromStdString(renderTimeString);
	Output(status);
	totalTimeString = renderTimer.Format(renderTimer.Toc());
	status = "Total time: " + QString::fromStdString(totalTimeString) + "\nTotal iters: " + itersString + "\nIters/second: " + itersPerSecString + "\n";
	Output(status);
	QMetaObject::invokeMethod(m_FinalRenderDialog, "MoveCursorToEnd", Qt::QueuedConnection);

	if (m_FinishedImageCount.load() == m_ImageCount)//Finished, save whatever options were specified on the GUI to the settings.
	{
		m_Settings->FinalEarlyClip(m_GuiState.m_EarlyClip);
		m_Settings->FinalYAxisUp(m_GuiState.m_YAxisUp);
		m_Settings->FinalTransparency(m_GuiState.m_Transparency);
		m_Settings->FinalOpenCL(m_GuiState.m_OpenCL);
		m_Settings->FinalDouble(m_GuiState.m_Double);
		m_Settings->FinalDevices(m_GuiState.m_Devices);
		m_Settings->FinalSaveXml(m_GuiState.m_SaveXml);
		m_Settings->FinalDoAll(m_GuiState.m_DoAll);
		m_Settings->FinalDoSequence(m_GuiState.m_DoSequence);
		m_Settings->FinalKeepAspect(m_GuiState.m_KeepAspect);
		m_Settings->FinalScale(uint(m_GuiState.m_Scale));
		m_Settings->FinalExt(m_GuiState.m_Ext);
		m_Settings->FinalThreadCount(m_GuiState.m_ThreadCount);
		m_Settings->FinalThreadPriority(m_GuiState.m_ThreadPriority);
		m_Settings->FinalQuality(m_GuiState.m_Quality);
		m_Settings->FinalTemporalSamples(m_GuiState.m_TemporalSamples);
		m_Settings->FinalSupersample(m_GuiState.m_Supersample);
		m_Settings->FinalStrips(m_GuiState.m_Strips);
	}

	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderTextOutput, "update", Qt::QueuedConnection);
	m_ProgressCs.Leave();
}

/// <summary>
/// Copy widget values to the ember passed in.
/// </summary>
/// <param name="ember">The ember whose values will be modified</param>
/// <param name="widthOverride">Width override to use instead of scaling the original width</param>
/// <param name="heightOverride">Height override to use instead of scaling the original height</param>
template <typename T>
void FinalRenderEmberController<T>::SyncGuiToEmber(Ember<T>& ember, size_t widthOverride, size_t heightOverride)
{
	size_t w;
	size_t h;

	if (widthOverride && heightOverride)
	{
		w = widthOverride;
		h = heightOverride;
	}
	else
	{
		double wScale = m_FinalRenderDialog->m_WidthScaleSpin->value();
		double hScale = m_FinalRenderDialog->m_HeightScaleSpin->value();
		w = ember.m_OrigFinalRasW * wScale;
		h = ember.m_OrigFinalRasH * hScale;
	}

	w = std::max<size_t>(w, 10);
	h = std::max<size_t>(h, 10);
	ember.SetSizeAndAdjustScale(w, h, false, m_FinalRenderDialog->Scale());
	ember.m_Quality = m_FinalRenderDialog->m_QualitySpin->value();
	ember.m_Supersample = m_FinalRenderDialog->m_SupersampleSpin->value();
}

/// <summary>
/// Set the iteration, density filter, and final accumulation progress bars to the same value.
/// Usually 0 or 100.
/// </summary>
/// <param name="val">The value to set them to</param>
template <typename T>
void FinalRenderEmberController<T>::SetProgressComplete(int val)
{
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderIterationProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, val));//Just to be safe.
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderFilteringProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, val));
	QMetaObject::invokeMethod(m_FinalRenderDialog->ui.FinalRenderAccumProgress,		"setValue", Qt::QueuedConnection, Q_ARG(int, val));
}

template class FinalRenderEmberController<float>;

#ifdef DO_DOUBLE
	template class FinalRenderEmberController<double>;
#endif
