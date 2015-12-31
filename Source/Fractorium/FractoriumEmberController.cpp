#include "FractoriumPch.h"
#include "FractoriumEmberController.h"
#include "Fractorium.h"
#include "GLEmberController.h"

/// <summary>
/// Constructor which initializes the non-templated members contained in this class.
/// The renderer, other templated members and GUI setup will be done in the templated derived controller class.
/// </summary>
/// <param name="fractorium">Pointer to the main window.</param>
FractoriumEmberControllerBase::FractoriumEmberControllerBase(Fractorium* fractorium)
{
	Timing t;
	m_Rendering = false;
	m_Shared = true;
	m_FailedRenders = 0;
	m_UndoIndex = 0;
	m_RenderType = CPU_RENDERER;
	m_OutputTexID = 0;
	m_SubBatchCount = 1;//Will be ovewritten by the options on first render.
	m_Fractorium = fractorium;
	m_RenderTimer = nullptr;
	m_RenderRestartTimer = nullptr;
	m_Info = OpenCLInfo::Instance();
	m_Rand = QTIsaac<ISAAC_SIZE, ISAAC_INT>(ISAAC_INT(t.Tic()), ISAAC_INT(t.Tic() * 2), ISAAC_INT(t.Tic() * 3));//Ensure a different rand seed on each instance.
	m_RenderTimer = new QTimer(m_Fractorium);
	m_RenderTimer->setInterval(0);
	m_Fractorium->connect(m_RenderTimer, SIGNAL(timeout()), SLOT(IdleTimer()));
	m_RenderRestartTimer = new QTimer(m_Fractorium);
	m_Fractorium->connect(m_RenderRestartTimer, SIGNAL(timeout()), SLOT(StartRenderTimer()));
}

/// <summary>
/// Destructor which stops rendering and deletes the timers.
/// All other memory is cleared automatically through the use of STL.
/// </summary>
FractoriumEmberControllerBase::~FractoriumEmberControllerBase()
{
	StopRenderTimer(true);

	if (m_RenderTimer)
	{
		m_RenderTimer->stop();
		delete m_RenderTimer;
		m_RenderTimer = nullptr;
	}

	if (m_RenderRestartTimer)
	{
		m_RenderRestartTimer->stop();
		delete m_RenderRestartTimer;
		m_RenderRestartTimer = nullptr;
	}
}

/// <summary>
/// Constructor which passes the main window parameter to the base, initializes the templated members contained in this class.
/// Then sets up the parts of the GUI that require templated Widgets, such as the variations tree and the palette table.
/// Note the renderer is not setup here automatically. Instead, it must be manually created by the caller later.
/// </summary>
/// <param name="fractorium">Pointer to the main window.</param>
template <typename T>
FractoriumEmberController<T>::FractoriumEmberController(Fractorium* fractorium)
	: FractoriumEmberControllerBase(fractorium)
{
	m_PreviewRun = false;
	m_PreviewRunning = false;
	m_SheepTools = unique_ptr<SheepTools<T, float>>(new SheepTools<T, float>(
					   QString(QApplication::applicationDirPath() + "flam3-palettes.xml").toLocal8Bit().data(),
					   new EmberNs::Renderer<T, float>()));
	m_GLController = unique_ptr<GLEmberController<T>>(new GLEmberController<T>(fractorium, fractorium->ui.GLDisplay, this));
	m_PreviewRenderer = unique_ptr<EmberNs::Renderer<T, float>>(new EmberNs::Renderer<T, float>());

	//Initial combo change event to fill the palette table will be called automatically later.

	//Look hard for a palette.
	if (!(InitPaletteList(QDir::currentPath().toLocal8Bit().data()) ||
			InitPaletteList(QDir::homePath().toLocal8Bit().data()) ||
			InitPaletteList(QCoreApplication::applicationDirPath().toLocal8Bit().data()) ||
			InitPaletteList(QString("/usr/local/share/fractorium").toLocal8Bit().data()) ||
			InitPaletteList(QString("/usr/share/fractorium").toLocal8Bit().data())) )
	{
		throw "No palettes found, exiting.";
	}

	BackgroundChanged(QColor(0, 0, 0));//Default to black.
	ClearUndo();
	m_PreviewRenderer->Callback(nullptr);
	m_PreviewRenderer->NumChannels(4);
	m_PreviewRenderer->EarlyClip(m_Fractorium->m_Settings->EarlyClip());
	m_PreviewRenderer->YAxisUp(m_Fractorium->m_Settings->YAxisUp());
	m_PreviewRenderer->SetEmber(m_Ember);//Give it an initial ember, will be updated many times later.
	//m_PreviewRenderer->ThreadCount(1);//For debugging.
	m_PreviewRenderFunc = [&](uint start, uint end)
	{
		while (m_PreviewRun || m_PreviewRunning)
		{
		}

		m_PreviewRun = true;
		m_PreviewRunning = true;
		m_PreviewRenderer->ThreadCount(std::max(1u, Timing::ProcessorCount() - 1));//Leave one processor free so the GUI can breathe.
		QTreeWidget* tree = m_Fractorium->ui.LibraryTree;

		if (QTreeWidgetItem* top = tree->topLevelItem(0))
		{
			for (size_t i = start; m_PreviewRun && i < end && i < m_EmberFile.Size(); i++)
			{
				Ember<T> ember = m_EmberFile.m_Embers[i];
				ember.SyncSize();
				ember.SetSizeAndAdjustScale(PREVIEW_SIZE, PREVIEW_SIZE, false, SCALE_WIDTH);
				ember.m_TemporalSamples = 1;
				ember.m_Quality = 25;
				ember.m_Supersample = 1;
				m_PreviewRenderer->SetEmber(ember);

				if (m_PreviewRenderer->Run(m_PreviewFinalImage) == RENDER_OK)
				{
					if (EmberTreeWidgetItem<T>* treeItem = dynamic_cast<EmberTreeWidgetItem<T>*>(top->child(i)))
					{
						//It is critical that Qt::BlockingQueuedConnection is passed because this is running on a different thread than the UI.
						//This ensures the events are processed in order as each preview is updated, and that control does not return here
						//until the update is complete.
						QMetaObject::invokeMethod(m_Fractorium, "SetLibraryTreeItemData", Qt::BlockingQueuedConnection,
												  Q_ARG(EmberTreeWidgetItemBase*, dynamic_cast<EmberTreeWidgetItemBase*>(treeItem)),
												  Q_ARG(vector<byte>&, m_PreviewFinalImage),
												  Q_ARG(uint, PREVIEW_SIZE),
												  Q_ARG(uint, PREVIEW_SIZE));
						//treeItem->SetImage(m_PreviewFinalImage, PREVIEW_SIZE, PREVIEW_SIZE);
					}
				}
			}
		}

		m_PreviewRun = false;
		m_PreviewRunning = false;
	};
}

/// <summary>
/// Empty destructor that does nothing.
/// </summary>
template <typename T>
FractoriumEmberController<T>::~FractoriumEmberController() { }

/// <summary>
/// Setters for embers, ember files and palettes which convert between float and double types.
/// These are used to preserve the current ember/file when switching between renderers.
/// Note that some precision will be lost when going from double to float.
/// </summary>
template <typename T> void FractoriumEmberController<T>::SetEmber(const Ember<float>& ember, bool verbatim) { SetEmberPrivate<float>(ember, verbatim); }
template <typename T> void FractoriumEmberController<T>::CopyEmber(Ember<float>& ember, std::function<void(Ember<float>& ember)> perEmberOperation) { ember = m_Ember; perEmberOperation(ember); }
template <typename T> void FractoriumEmberController<T>::SetEmberFile(const EmberFile<float>& emberFile) { m_EmberFile = emberFile; }
template <typename T> void FractoriumEmberController<T>::CopyEmberFile(EmberFile<float>& emberFile, std::function<void(Ember<float>& ember)> perEmberOperation)
{
	emberFile.m_Filename = m_EmberFile.m_Filename;
	CopyVec(emberFile.m_Embers, m_EmberFile.m_Embers, perEmberOperation);
}

template <typename T> void FractoriumEmberController<T>::SetTempPalette(const Palette<float>& palette) { m_TempPalette = palette; }
template <typename T> void FractoriumEmberController<T>::CopyTempPalette(Palette<float>& palette) { palette = m_TempPalette; }
#ifdef DO_DOUBLE
template <typename T> void FractoriumEmberController<T>::SetEmber(const Ember<double>& ember, bool verbatim) { SetEmberPrivate<double>(ember, verbatim); }
template <typename T> void FractoriumEmberController<T>::CopyEmber(Ember<double>& ember, std::function<void(Ember<double>& ember)> perEmberOperation) { ember = m_Ember; perEmberOperation(ember); }
template <typename T> void FractoriumEmberController<T>::SetEmberFile(const EmberFile<double>& emberFile) { m_EmberFile = emberFile; }
template <typename T> void FractoriumEmberController<T>::CopyEmberFile(EmberFile<double>& emberFile, std::function<void(Ember<double>& ember)> perEmberOperation)
{
	emberFile.m_Filename = m_EmberFile.m_Filename;
	CopyVec(emberFile.m_Embers, m_EmberFile.m_Embers, perEmberOperation);
}

template <typename T> void FractoriumEmberController<T>::SetTempPalette(const Palette<double>& palette) { m_TempPalette = palette; }
template <typename T> void FractoriumEmberController<T>::CopyTempPalette(Palette<double>& palette) { palette = m_TempPalette; }
#endif
template <typename T> Ember<T>* FractoriumEmberController<T>::CurrentEmber() { return &m_Ember; }

template <typename T>
void FractoriumEmberController<T>::ConstrainDimensions(Ember<T>& ember)
{
	ember.m_FinalRasW = std::min<int>(m_Fractorium->ui.GLDisplay->MaxTexSize(), ember.m_FinalRasW);
	ember.m_FinalRasH = std::min<int>(m_Fractorium->ui.GLDisplay->MaxTexSize(), ember.m_FinalRasH);
}

/// <summary>
/// Set the ember at the specified index from the currently opened file as the current Ember.
/// Clears the undo state.
/// Resets the rendering process.
/// </summary>
/// <param name="index">The index in the file from which to retrieve the ember</param>
template <typename T>
void FractoriumEmberController<T>::SetEmber(size_t index)
{
	if (index < m_EmberFile.Size())
	{
		if (QTreeWidgetItem* top = m_Fractorium->ui.LibraryTree->topLevelItem(0))
		{
			for (uint i = 0; i < top->childCount(); i++)
			{
				if (EmberTreeWidgetItem<T>* emberItem = dynamic_cast<EmberTreeWidgetItem<T>*>(top->child(i)))
					emberItem->setSelected(i == index);
			}
		}

		ClearUndo();
		SetEmber(m_EmberFile.m_Embers[index]);
	}
}

/// <summary>
/// Wrapper to call a function, then optionally add the requested action to the rendering queue.
/// </summary>
/// <param name="func">The function to call</param>
/// <param name="updateRender">True to update renderer, else false. Default: true.</param>
/// <param name="action">The action to add to the rendering queue. Default: FULL_RENDER.</param>
template <typename T>
void FractoriumEmberController<T>::Update(std::function<void (void)> func, bool updateRender, eProcessAction action)
{
	func();

	if (updateRender)
		UpdateRender(action);
}

/// <summary>
/// Wrapper to call a function on the specified xforms, then optionally add the requested action to the rendering queue.
/// If no xforms are selected via the checkboxes, and the update type is UPDATE_SELECTED, then the function will be called only on the currently selected xform.
/// </summary>
/// <param name="func">The function to call</param>
/// <param name="updateType">Whether to apply this update operation on the current, all or selected xforms. Default: UPDATE_CURRENT.</param>
/// <param name="updateRender">True to update renderer, else false. Default: true.</param>
/// <param name="action">The action to add to the rendering queue. Default: FULL_RENDER.</param>
template <typename T>
void FractoriumEmberController<T>::UpdateXform(std::function<void(Xform<T>*)> func, eXformUpdate updateType, bool updateRender, eProcessAction action)
{
	size_t i = 0;
	bool isCurrentFinal = m_Ember.IsFinalXform(CurrentXform());
	bool doFinal = updateType != eXformUpdate::UPDATE_SELECTED_EXCEPT_FINAL && updateType != eXformUpdate::UPDATE_ALL_EXCEPT_FINAL;

	switch (updateType)
	{
		case eXformUpdate::UPDATE_CURRENT:
		{
			if (Xform<T>* xform = CurrentXform())
				func(xform);
		}
		break;

		case eXformUpdate::UPDATE_SELECTED:
		case eXformUpdate::UPDATE_SELECTED_EXCEPT_FINAL:
		{
			bool anyUpdated = false;

			while (Xform<T>* xform = (doFinal ? m_Ember.GetTotalXform(i) : m_Ember.GetXform(i)))
			{
				if (QLayoutItem* child = m_Fractorium->m_XformsSelectionLayout->itemAt(i))
				{
					if (auto* w = qobject_cast<QCheckBox*>(child->widget()))
					{
						if (w->isChecked())
						{
							func(xform);
							anyUpdated = true;
						}
					}
				}

				i++;
			}

			if (!anyUpdated)//None were selected, so just apply to the current.
				if (doFinal || !isCurrentFinal)//If do final, call func regardless. If not, only call if current is not final.
					if (Xform<T>* xform = CurrentXform())
						func(xform);
		}
		break;

		case eXformUpdate::UPDATE_ALL:
		{
			while (Xform<T>* xform = m_Ember.GetTotalXform(i++))
				func(xform);
		}
		break;

		case eXformUpdate::UPDATE_ALL_EXCEPT_FINAL:
		default:
		{
			while (Xform<T>* xform = m_Ember.GetXform(i++))
				func(xform);
		}
		break;
	}

	if (updateRender)
		UpdateRender(action);
}

/// <summary>
/// Set the current ember, but use GUI values for the fields which make sense to
/// keep the same between ember selection changes.
/// Note the extra template parameter U allows for assigning ember of different types.
/// Resets the rendering process.
/// </summary>
/// <param name="ember">The ember to set as the current</param>
/// <param name="verbatim">If true, do not overwrite temporal samples, quality or supersample value, else overwrite.</param>
template <typename T>
template <typename U>
void FractoriumEmberController<T>::SetEmberPrivate(const Ember<U>& ember, bool verbatim)
{
	if (ember.m_Name != m_Ember.m_Name)
		m_LastSaveCurrent = "";

	size_t w = m_Ember.m_FinalRasW;//Cache values for use below.
	size_t h = m_Ember.m_FinalRasH;
	m_Ember = ember;

	if (!verbatim)
	{
		//m_Ember.SetSizeAndAdjustScale(m_Fractorium->ui.GLDisplay->width(), m_Fractorium->ui.GLDisplay->height(), true, SCALE_WIDTH);
		m_Ember.m_TemporalSamples = 1;//Change once animation is supported.
		m_Ember.m_Quality = m_Fractorium->m_QualitySpin->value();
		m_Ember.m_Supersample = m_Fractorium->m_SupersampleSpin->value();
	}

	static EmberToXml<T> writer;//Save parameters of last full render just in case there is a crash.
	string filename = "last.flame";
	writer.Save(filename.c_str(), m_Ember, 0, true, false, true);
	m_GLController->ResetMouseState();
	FillXforms();//Must do this first because the palette setup in FillParamTablesAndPalette() uses the xforms combo.
	FillParamTablesAndPalette();
	FillSummary();

	//If a resize happened, this won't do anything because the new size is not reflected in the scroll area yet.
	//However, it will have been taken care of in SyncSizes() in that case, so it's ok.
	//This is for when a new ember with the same size was loaded. If it was larger than the scroll area, and was scrolled, re-center it.
	if (m_Ember.m_FinalRasW == w && m_Ember.m_FinalRasH == h)
		m_Fractorium->CenterScrollbars();
}

template class FractoriumEmberController<float>;

#ifdef DO_DOUBLE
	template class FractoriumEmberController<double>;
#endif
