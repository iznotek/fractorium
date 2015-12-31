#include "FractoriumPch.h"
#include "FinalRenderDialog.h"
#include "Fractorium.h"

/// <summary>
/// Constructor which sets up the GUI for the final rendering dialog.
/// Settings used to populate widgets with initial values.
/// This function contains the render function as a lambda.
/// </summary>
/// <param name="settings">Pointer to the global settings object to use</param>
/// <param name="p">The parent widget</param>
/// <param name="f">The window flags. Default: 0.</param>
FractoriumFinalRenderDialog::FractoriumFinalRenderDialog(FractoriumSettings* settings, QWidget* p, Qt::WindowFlags f)
	: QDialog(p, f)
{
	ui.setupUi(this);
	int row = 0, spinHeight = 20;
	uint i;
	double dmax = numeric_limits<double>::max();
	QTableWidget* table = ui.FinalRenderParamsTable;
	QTableWidgetItem* item = nullptr;
	m_Info = OpenCLInfo::Instance();
	m_Fractorium = qobject_cast<Fractorium*>(p);
	m_Settings = settings;
	ui.FinalRenderThreadCountSpin->setRange(1, Timing::ProcessorCount());
	connect(ui.FinalRenderEarlyClipCheckBox,	   SIGNAL(stateChanged(int)),		 this, SLOT(OnEarlyClipCheckBoxStateChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderYAxisUpCheckBox,	       SIGNAL(stateChanged(int)),		 this, SLOT(OnYAxisUpCheckBoxStateChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderTransparencyCheckBox,	   SIGNAL(stateChanged(int)),		 this, SLOT(OnTransparencyCheckBoxStateChanged(int)),	 Qt::QueuedConnection);
	connect(ui.FinalRenderOpenCLCheckBox,		   SIGNAL(stateChanged(int)),		 this, SLOT(OnOpenCLCheckBoxStateChanged(int)),		     Qt::QueuedConnection);
	connect(ui.FinalRenderDoublePrecisionCheckBox, SIGNAL(stateChanged(int)),		 this, SLOT(OnDoublePrecisionCheckBoxStateChanged(int)), Qt::QueuedConnection);
	connect(ui.FinalRenderDoAllCheckBox,		   SIGNAL(stateChanged(int)),		 this, SLOT(OnDoAllCheckBoxStateChanged(int)),			 Qt::QueuedConnection);
	connect(ui.FinalRenderDoSequenceCheckBox,	   SIGNAL(stateChanged(int)),		 this, SLOT(OnDoSequenceCheckBoxStateChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderCurrentSpin,			   SIGNAL(valueChanged(int)),		 this, SLOT(OnCurrentSpinChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderApplyToAllCheckBox,	   SIGNAL(stateChanged(int)),		 this, SLOT(OnApplyAllCheckBoxStateChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderKeepAspectCheckBox,	   SIGNAL(stateChanged(int)),		 this, SLOT(OnKeepAspectCheckBoxStateChanged(int)),		 Qt::QueuedConnection);
	connect(ui.FinalRenderScaleNoneRadioButton,	   SIGNAL(toggled(bool)),			 this, SLOT(OnScaleRadioButtonChanged(bool)),			 Qt::QueuedConnection);
	connect(ui.FinalRenderScaleWidthRadioButton,   SIGNAL(toggled(bool)),			 this, SLOT(OnScaleRadioButtonChanged(bool)),			 Qt::QueuedConnection);
	connect(ui.FinalRenderScaleHeightRadioButton,  SIGNAL(toggled(bool)),			 this, SLOT(OnScaleRadioButtonChanged(bool)),			 Qt::QueuedConnection);
	connect(ui.FinalRenderDeviceTable,			   SIGNAL(cellChanged(int, int)),	 this, SLOT(OnDeviceTableCellChanged(int, int)),		 Qt::QueuedConnection);
	SetupSpinner<DoubleSpinBox, double>(ui.FinalRenderSizeTable, this, row, 1, m_WidthScaleSpin,  spinHeight, 0.001, 99.99, 0.1, SIGNAL(valueChanged(double)), SLOT(OnWidthScaleChanged(double)), true, 1.0, 1.0, 1.0);
	SetupSpinner<DoubleSpinBox, double>(ui.FinalRenderSizeTable, this, row, 1, m_HeightScaleSpin, spinHeight, 0.001, 99.99, 0.1, SIGNAL(valueChanged(double)), SLOT(OnHeightScaleChanged(double)), true, 1.0, 1.0, 1.0);
	m_WidthScaleSpin->setDecimals(3);
	m_HeightScaleSpin->setDecimals(3);
	m_WidthScaleSpin->setSuffix(" ( )");
	m_HeightScaleSpin->setSuffix(" ( )");
	m_WidthScaleSpin->SmallStep(0.001);
	m_HeightScaleSpin->SmallStep(0.001);
	row = 0;
	SetupSpinner<DoubleSpinBox, double>(table, this, row, 1, m_QualitySpin,		    spinHeight,  1, dmax, 50, SIGNAL(valueChanged(double)), SLOT(OnQualityChanged(double)),	     true, 1000, 1000, 1000);
	SetupSpinner<SpinBox, int>         (table, this, row, 1, m_TemporalSamplesSpin, spinHeight,  1, 5000, 50, SIGNAL(valueChanged(int)),    SLOT(OnTemporalSamplesChanged(int)), true, 1000, 1000, 1000);
	SetupSpinner<SpinBox, int>         (table, this, row, 1, m_SupersampleSpin,	    spinHeight,	 1,	   4,  1, SIGNAL(valueChanged(int)),    SLOT(OnSupersampleChanged(int)),	 true,    2,	1,	  1);
	SetupSpinner<SpinBox, int>         (table, this, row, 1, m_StripsSpin,			spinHeight,	 1,	  64,  1, SIGNAL(valueChanged(int)),    SLOT(OnStripsChanged(int)),		     true,    1,	1,	  1);
	m_MemoryCellIndex = row++;//Memory usage.
	m_ItersCellIndex = row++;//Iters.
	m_PathCellIndex = row;
	QStringList comboList;
	comboList.append("jpg");
	comboList.append("png");
	m_Tbcw = new TwoButtonComboWidget("...", "Open", comboList, 22, 40, 22, table);
	table->setCellWidget(row, 1, m_Tbcw);
	table->item(row++, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	connect(m_Tbcw->m_Button1, SIGNAL(clicked(bool)),			 this, SLOT(OnFileButtonClicked(bool)),			Qt::QueuedConnection);
	connect(m_Tbcw->m_Button2, SIGNAL(clicked(bool)),			 this, SLOT(OnShowFolderButtonClicked(bool)),   Qt::QueuedConnection);
	connect(m_Tbcw->m_Combo,   SIGNAL(currentIndexChanged(int)), this, SLOT(OnExtIndexChanged(int)), Qt::QueuedConnection);
	m_PrefixEdit = new QLineEdit(table);
	table->setCellWidget(row++, 1, m_PrefixEdit);
	m_SuffixEdit = new QLineEdit(table);
	table->setCellWidget(row++, 1, m_SuffixEdit);
	connect(m_PrefixEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnPrefixChanged(const QString&)), Qt::QueuedConnection);
	connect(m_SuffixEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnSuffixChanged(const QString&)), Qt::QueuedConnection);
	ui.FinalRenderStartButton->disconnect(SIGNAL(clicked(bool)));
	connect(ui.FinalRenderStartButton, SIGNAL(clicked(bool)), this, SLOT(OnRenderClicked(bool)),		  Qt::QueuedConnection);
	connect(ui.FinalRenderStopButton,  SIGNAL(clicked(bool)), this, SLOT(OnCancelRenderClicked(bool)), Qt::QueuedConnection);
	table = ui.FinalRenderDeviceTable;
	table->clearContents();
	table->setRowCount(0);

	if (m_Info->Ok() && !m_Info->Devices().empty())
	{
		SetupDeviceTable(table, m_Settings->FinalDevices());

		for (int i = 0; i < table->rowCount(); i++)
			if (auto radio = qobject_cast<QRadioButton*>(table->cellWidget(i, 1)))
				connect(radio, SIGNAL(toggled(bool)), this, SLOT(OnDeviceTableRadioToggled(bool)), Qt::QueuedConnection);

		ui.FinalRenderOpenCLCheckBox->setChecked(m_Settings->FinalOpenCL());
	}
	else
	{
		table->setEnabled(false);
		ui.FinalRenderOpenCLCheckBox->setChecked(false);
		ui.FinalRenderOpenCLCheckBox->setEnabled(false);
	}

	ui.FinalRenderEarlyClipCheckBox->setChecked(		  m_Settings->FinalEarlyClip());
	ui.FinalRenderYAxisUpCheckBox->setChecked(			  m_Settings->FinalYAxisUp());
	ui.FinalRenderTransparencyCheckBox->setChecked(		  m_Settings->FinalTransparency());
	ui.FinalRenderDoublePrecisionCheckBox->setChecked(	  m_Settings->FinalDouble());
	ui.FinalRenderSaveXmlCheckBox->setChecked(			  m_Settings->FinalSaveXml());
	ui.FinalRenderDoAllCheckBox->setChecked(			  m_Settings->FinalDoAll());
	ui.FinalRenderDoSequenceCheckBox->setChecked(		  m_Settings->FinalDoSequence());
	ui.FinalRenderKeepAspectCheckBox->setChecked(		  m_Settings->FinalKeepAspect());
	ui.FinalRenderThreadCountSpin->setValue(			  m_Settings->FinalThreadCount());
#ifdef _WIN32
	ui.FinalRenderThreadPriorityComboBox->setCurrentIndex(m_Settings->FinalThreadPriority() + 2);
#else
	auto tpc = ui.FinalRenderThreadPriorityComboBox->count() - 1;

	if (m_Settings->FinalThreadPriority() == THREAD_PRIORITY_LOWEST)
		ui.FinalRenderThreadPriorityComboBox->setCurrentIndex(0);
	else if (m_Settings->FinalThreadPriority() == THREAD_PRIORITY_HIGHEST)
		ui.FinalRenderThreadPriorityComboBox->setCurrentIndex(tpc);
	else
		ui.FinalRenderThreadPriorityComboBox->setCurrentIndex(Clamp<int>(m_Settings->FinalThreadPriority() / 25, 0, tpc));

#endif
	m_QualitySpin->setValue(m_Settings->FinalQuality());
	m_TemporalSamplesSpin->setValue(m_Settings->FinalTemporalSamples());
	m_SupersampleSpin->setValue(m_Settings->FinalSupersample());
	m_StripsSpin->setValue(m_Settings->FinalStrips());
	Scale(eScaleType(m_Settings->FinalScale()));

	if (m_Settings->FinalExt() == "jpg")
		m_Tbcw->m_Combo->setCurrentIndex(0);
	else
		m_Tbcw->m_Combo->setCurrentIndex(1);

	//Explicitly call these to enable/disable the appropriate controls.
	OnOpenCLCheckBoxStateChanged(ui.FinalRenderOpenCLCheckBox->isChecked());
	OnDoAllCheckBoxStateChanged(ui.FinalRenderDoAllCheckBox->isChecked());
	OnDoSequenceCheckBoxStateChanged(ui.FinalRenderDoSequenceCheckBox->isChecked());
	QSize s = size();
	int desktopHeight = qApp->desktop()->availableGeometry().height();
	s.setHeight(std::min(s.height(), int(double(desktopHeight * 0.90))));
	setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, s, qApp->desktop()->availableGeometry()));
	ui.FinalRenderThreadHorizontalLayout->setAlignment(Qt::AlignLeft);
	ui.FinalRenderThreadHorizontalLayout->setAlignment(ui.FinalRenderThreadCountSpin, Qt::AlignLeft);
	ui.FinalRenderThreadHorizontalLayout->setAlignment(ui.FinalRenderThreadPriorityLabel, Qt::AlignLeft);
	ui.FinalRenderThreadHorizontalLayout->setAlignment(ui.FinalRenderThreadPriorityComboBox, Qt::AlignLeft);
	QWidget* w = SetTabOrder(this, ui.FinalRenderEarlyClipCheckBox, ui.FinalRenderYAxisUpCheckBox);
	//Update these with new controls.
	w = SetTabOrder(this, w, ui.FinalRenderTransparencyCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderOpenCLCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderDoublePrecisionCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderSaveXmlCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderDoAllCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderDoSequenceCheckBox);
	w = SetTabOrder(this, w, ui.FinalRenderCurrentSpin);
	w = SetTabOrder(this, w, ui.FinalRenderDeviceTable);
	w = SetTabOrder(this, w, ui.FinalRenderThreadCountSpin);
	w = SetTabOrder(this, w, ui.FinalRenderThreadPriorityComboBox);
	w = SetTabOrder(this, w, ui.FinalRenderApplyToAllCheckBox);
	w = SetTabOrder(this, w, m_WidthScaleSpin);
	w = SetTabOrder(this, w, m_HeightScaleSpin);
	w = SetTabOrder(this, w, ui.FinalRenderScaleNoneRadioButton);
	w = SetTabOrder(this, w, ui.FinalRenderScaleWidthRadioButton);
	w = SetTabOrder(this, w, ui.FinalRenderScaleHeightRadioButton);
	w = SetTabOrder(this, w, ui.FinalRenderKeepAspectCheckBox);
	w = SetTabOrder(this, w, m_QualitySpin);
	w = SetTabOrder(this, w, m_TemporalSamplesSpin);
	w = SetTabOrder(this, w, m_SupersampleSpin);
	w = SetTabOrder(this, w, m_StripsSpin);
	w = SetTabOrder(this, w, m_Tbcw);
	w = SetTabOrder(this, w, m_Tbcw->m_Combo);
	w = SetTabOrder(this, w, m_Tbcw->m_Button1);
	w = SetTabOrder(this, w, m_Tbcw->m_Button2);
	w = SetTabOrder(this, w, m_PrefixEdit);
	w = SetTabOrder(this, w, m_SuffixEdit);
	w = SetTabOrder(this, w, ui.FinalRenderTextOutput);
	w = SetTabOrder(this, w, ui.FinalRenderStartButton);
	w = SetTabOrder(this, w, ui.FinalRenderStopButton);
	w = SetTabOrder(this, w, ui.FinalRenderCloseButton);
}

/// <summary>
/// GUI settings wrapper functions, getters only.
/// </summary>

bool FractoriumFinalRenderDialog::EarlyClip() { return ui.FinalRenderEarlyClipCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::YAxisUp() { return ui.FinalRenderYAxisUpCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::Transparency() { return ui.FinalRenderTransparencyCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::OpenCL() { return ui.FinalRenderOpenCLCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::Double() { return ui.FinalRenderDoublePrecisionCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::SaveXml() { return ui.FinalRenderSaveXmlCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::DoAll() { return ui.FinalRenderDoAllCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::DoSequence() { return ui.FinalRenderDoSequenceCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::KeepAspect() { return ui.FinalRenderKeepAspectCheckBox->isChecked(); }
bool FractoriumFinalRenderDialog::ApplyToAll() { return ui.FinalRenderApplyToAllCheckBox->isChecked(); }
QString FractoriumFinalRenderDialog::Ext() { return m_Tbcw->m_Combo->currentIndex() == 0 ? "jpg" : "png"; }
QString FractoriumFinalRenderDialog::Path() { return ui.FinalRenderParamsTable->item(m_PathCellIndex, 1)->text(); }
void FractoriumFinalRenderDialog::Path(const QString& s) { ui.FinalRenderParamsTable->item(m_PathCellIndex, 1)->setText(s); }
QString FractoriumFinalRenderDialog::Prefix() { return m_PrefixEdit->text(); }
QString FractoriumFinalRenderDialog::Suffix() { return m_SuffixEdit->text(); }
uint FractoriumFinalRenderDialog::Current() { return ui.FinalRenderCurrentSpin->value(); }
uint FractoriumFinalRenderDialog::ThreadCount() { return ui.FinalRenderThreadCountSpin->value(); }
#ifdef _WIN32
int FractoriumFinalRenderDialog::ThreadPriority() { return ui.FinalRenderThreadPriorityComboBox->currentIndex() - 2; }
#else
int FractoriumFinalRenderDialog::ThreadPriority()
{
	if (ui.FinalRenderThreadPriorityComboBox->currentIndex() == 0)
		return THREAD_PRIORITY_LOWEST;
	else if (ui.FinalRenderThreadPriorityComboBox->currentIndex() == (ui.FinalRenderThreadPriorityComboBox->count() - 1))
		return THREAD_PRIORITY_HIGHEST;
	else
		return ui.FinalRenderThreadPriorityComboBox->currentIndex() * 25;
}
#endif
double FractoriumFinalRenderDialog::WidthScale() { return m_WidthScaleSpin->value(); }
double FractoriumFinalRenderDialog::HeightScale() { return m_HeightScaleSpin->value(); }
double FractoriumFinalRenderDialog::Quality() { return m_QualitySpin->value(); }
uint FractoriumFinalRenderDialog::TemporalSamples() { return m_TemporalSamplesSpin->value(); }
uint FractoriumFinalRenderDialog::Supersample() { return m_SupersampleSpin->value(); }
uint FractoriumFinalRenderDialog::Strips() { return m_StripsSpin->value(); }
QList<QVariant> FractoriumFinalRenderDialog::Devices() { return DeviceTableToSettings(ui.FinalRenderDeviceTable); }

/// <summary>
/// Capture the current state of the Gui.
/// Used to hold options for performing the final render.
/// </summary>
/// <returns>The state of the Gui as a struct</returns>
FinalRenderGuiState FractoriumFinalRenderDialog::State()
{
	FinalRenderGuiState state;
	state.m_EarlyClip = EarlyClip();
	state.m_YAxisUp = YAxisUp();
	state.m_Transparency = Transparency();
	state.m_OpenCL = OpenCL();
	state.m_Double = Double();
	state.m_SaveXml = SaveXml();
	state.m_DoAll = DoAll();
	state.m_DoSequence = DoSequence();
	state.m_KeepAspect = KeepAspect();
	state.m_Scale = Scale();
	state.m_Path = Path();
	state.m_Ext = Ext();
	state.m_Prefix = Prefix();
	state.m_Suffix = Suffix();
	state.m_Devices = Devices();
	state.m_ThreadCount = ThreadCount();
	state.m_ThreadPriority = ThreadPriority();
	state.m_WidthScale = WidthScale();
	state.m_HeightScale = HeightScale();
	state.m_Quality = Quality();
	state.m_TemporalSamples = TemporalSamples();
	state.m_Supersample = Supersample();
	state.m_Strips = Strips();
	return state;
}

/// <summary>
/// Return the type of scaling desired based on what radio button has been selected.
/// </summary>
/// <returns>The type of scaling as an eScaleType enum</returns>
eScaleType FractoriumFinalRenderDialog::Scale()
{
	if (ui.FinalRenderScaleNoneRadioButton->isChecked())
		return SCALE_NONE;
	else if (ui.FinalRenderScaleWidthRadioButton->isChecked())
		return SCALE_WIDTH;
	else if (ui.FinalRenderScaleHeightRadioButton->isChecked())
		return SCALE_HEIGHT;
	else
		return SCALE_NONE;
}

/// <summary>
/// Set the type of scaling desired which will select the corresponding radio button.
/// </summary>
/// <param name="scale">The type of scaling to use</param>
void FractoriumFinalRenderDialog::Scale(eScaleType scale)
{
	ui.FinalRenderScaleNoneRadioButton->blockSignals(true);

	if (scale == SCALE_NONE)
		ui.FinalRenderScaleNoneRadioButton->setChecked(true);
	else if (scale == SCALE_WIDTH)
		ui.FinalRenderScaleWidthRadioButton->setChecked(true);
	else if (scale == SCALE_HEIGHT)
		ui.FinalRenderScaleHeightRadioButton->setChecked(true);
	else
		ui.FinalRenderScaleNoneRadioButton->setChecked(true);

	ui.FinalRenderScaleNoneRadioButton->blockSignals(false);
}

/// <summary>
/// Simple wrapper to put moving the cursor to the end in a signal
/// so it can be called from a thread.
/// </summary>
void FractoriumFinalRenderDialog::MoveCursorToEnd()
{
	ui.FinalRenderTextOutput->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
}

/// <summary>
/// Whether to use early clipping before spatial filtering.
/// </summary>
/// <param name="index">True to early clip, else don't.</param>
void FractoriumFinalRenderDialog::OnEarlyClipCheckBoxStateChanged(int state)
{
	SetMemory();
}

/// <summary>
/// Whether the positive Y axis of the final output image is up.
/// </summary>
/// <param name="yup">True if the positive y axis is up, else false.</param>
void FractoriumFinalRenderDialog::OnYAxisUpCheckBoxStateChanged(int state)
{
	SetMemory();
}

/// <summary>
/// Whether to use transparency in png images.
/// </summary>
/// <param name="index">True to use transparency, else don't.</param>
void FractoriumFinalRenderDialog::OnTransparencyCheckBoxStateChanged(int state)
{
	SetMemory();
}

/// <summary>
/// Set whether to use OpenCL in the rendering process or not.
/// Also disable or enable the CPU and OpenCL related controls based on the state passed in.
/// </summary>
/// <param name="state">Use OpenCL if state == Qt::Checked, else don't.</param>
void FractoriumFinalRenderDialog::OnOpenCLCheckBoxStateChanged(int state)
{
	bool checked = state == Qt::Checked;
	ui.FinalRenderDeviceTable->setEnabled(checked);
	ui.FinalRenderThreadCountSpin->setEnabled(!checked);
	ui.FinalRenderThreadPriorityLabel->setEnabled(!checked);
	ui.FinalRenderThreadPriorityComboBox->setEnabled(!checked);
	SetMemory();
}

/// <summary>
/// Set whether to use double or single precision in the rendering process or not.
/// This will recreate the entire controller.
/// </summary>
/// <param name="state">Use double if state == Qt::Checked, else float.</param>
void FractoriumFinalRenderDialog::OnDoublePrecisionCheckBoxStateChanged(int state)
{
	SetMemory();
}

/// <summary>
/// The do all checkbox was changed.
/// If checked, render all embers available in the currently opened file, else
/// only render the current ember.
/// </summary>
/// <param name="state">The state of the checkbox</param>
void FractoriumFinalRenderDialog::OnDoAllCheckBoxStateChanged(int state)
{
	if (!state)
		ui.FinalRenderDoSequenceCheckBox->setChecked(false);

	ui.FinalRenderDoSequenceCheckBox->setEnabled(ui.FinalRenderDoAllCheckBox->isChecked());
}

/// <summary>
/// The do sequence checkbox was changed.
/// If checked, render all embers available in the currently opened file as an animation sequence, else
/// render them individually.
/// </summary>
/// <param name="state">The state of the checkbox</param>
void FractoriumFinalRenderDialog::OnDoSequenceCheckBoxStateChanged(int state)
{
	bool checked = ui.FinalRenderDoSequenceCheckBox->isChecked();
	m_TemporalSamplesSpin->setEnabled(checked);

	if (checked)
		m_StripsSpin->setValue(1);

	m_StripsSpin->setEnabled(!checked);
	SetMemory();
}

/// <summary>
/// The current ember spinner was changed, update fields.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnCurrentSpinChanged(int d)
{
	m_Controller->SetEmber(d - 1);
	m_Controller->SyncCurrentToGui();
	SetMemory();
}

/// <summary>
/// The apply all checkbox was changed.
/// If checked, set values for all embers in the file to the values specified in the GUI.
/// </summary>
/// <param name="state">The state of the checkbox</param>
void FractoriumFinalRenderDialog::OnApplyAllCheckBoxStateChanged(int state)
{
	if (state && m_Controller.get())
		m_Controller->SyncGuiToEmbers();
}

/// <summary>
/// The width spinner was changed, recompute required memory.
/// If the aspect ratio checkbox is checked, set the value of
/// the height spinner as well to be in proportion.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnWidthScaleChanged(double d)
{
	if (ui.FinalRenderKeepAspectCheckBox->isChecked() && m_Controller.get())
		m_HeightScaleSpin->SetValueStealth(m_WidthScaleSpin->value());

	if (SetMemory())
		m_Controller->SyncCurrentToSizeSpinners(false, true);
}

/// <summary>
/// The height spinner was changed, recompute required memory.
/// If the aspect ratio checkbox is checked, set the value of
/// the width spinner as well to be in proportion.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnHeightScaleChanged(double d)
{
	if (ui.FinalRenderKeepAspectCheckBox->isChecked() && m_Controller.get())
		m_WidthScaleSpin->SetValueStealth(m_HeightScaleSpin->value());

	if (SetMemory())
		m_Controller->SyncCurrentToSizeSpinners(false, true);
}

/// <summary>
/// Whether to keep the aspect ratio of the desired width and height the same
/// as that of the original width and height.
/// </summary>
/// <param name="checked">The state of the checkbox</param>
void FractoriumFinalRenderDialog::OnKeepAspectCheckBoxStateChanged(int state)
{
	if (state && m_Controller.get())
		m_HeightScaleSpin->SetValueStealth(m_WidthScaleSpin->value());

	SetMemory();
}

/// <summary>
/// The scaling method radio button selection was changed.
/// </summary>
/// <param name="checked">The state of the radio button</param>
void FractoriumFinalRenderDialog::OnScaleRadioButtonChanged(bool checked)
{
	if (checked)
		SetMemory();
}

/// <summary>
/// The check state of one of the OpenCL devices was changed.
/// This does a special check to always ensure at least one device,
/// as well as one primary is checked.
/// </summary>
/// <param name="row">The row of the cell</param>
/// <param name="col">The column of the cell</param>
void FractoriumFinalRenderDialog::OnDeviceTableCellChanged(int row, int col)
{
	if (auto item = ui.FinalRenderDeviceTable->item(row, col))
	{
		HandleDeviceTableCheckChanged(ui.FinalRenderDeviceTable, row, col);
		SetMemory();
	}
}

/// <summary>
/// The primary device radio button selection was changed.
/// If the device was specified as primary, but was not selected
/// for inclusion, it will automatically be selected for inclusion.
/// </summary>
/// <param name="checked">The state of the radio button</param>
void FractoriumFinalRenderDialog::OnDeviceTableRadioToggled(bool checked)
{
	int row;
	auto s = sender();
	auto table = ui.FinalRenderDeviceTable;
	QRadioButton* radio = nullptr;

	if (s)
	{
		for (row = 0; row < table->rowCount(); row++)
			if (radio = qobject_cast<QRadioButton*>(table->cellWidget(row, 1)))
				if (s == radio)
				{
					HandleDeviceTableCheckChanged(ui.FinalRenderDeviceTable, row, 1);
					break;
				}
	}

	if (checked)
		SetMemory();
}

/// <summary>
/// The quality spinner was changed, recompute required memory.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnQualityChanged(double d)
{
	SetMemory();
}

/// <summary>
/// The temporal samples spinner was changed, recompute required memory.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnTemporalSamplesChanged(int d)
{
	SetMemory();
}

/// <summary>
/// The supersample spinner was changed, recompute required memory.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnSupersampleChanged(int d)
{
	SetMemory();
}

/// <summary>
/// The supersample spinner was changed, recompute required memory.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnStripsChanged(int d)
{
	SetMemory();
}

/// <summary>
/// If a single ember is being rendered, show the save file dialog.
/// If a more than one is being rendered, show the save folder dialog.
/// Called when the ... file button is clicked.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumFinalRenderDialog::OnFileButtonClicked(bool checked)
{
	bool doAll = ui.FinalRenderDoAllCheckBox->isChecked();
	QString s = m_Fractorium->SetupSaveFolderDialog();

	if (Exists(s))
	{
		m_Settings->SaveFolder(s);//Any time they exit the box with a valid value, preserve it in the settings.
		Path(m_Controller->ComposePath(m_Controller->Name()));//And update the GUI.
		SetMemory();
	}
}

/// <summary>
/// Show the folder where the last rendered image was saved to.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumFinalRenderDialog::OnShowFolderButtonClicked(bool checked)
{
	QString s = m_Settings->SaveFolder();

	if (Exists(s))
		QDesktopServices::openUrl(QUrl::fromLocalFile(s));
	else
		QDesktopServices::openUrl(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation)[0]);
}

/// <summary>
/// Change the extension of the output image, which also may change the
/// number of channels used in the final output buffer.
/// </summary>
/// <param name="d">Ignored</param>
void FractoriumFinalRenderDialog::OnExtIndexChanged(int d)
{
	if (SetMemory())
		Path(m_Controller->ComposePath(m_Controller->Name()));
}

/// <summary>
/// Change the prefix prepended to the output file name.
/// </summary>
/// <param name="s">Ignored</param>
void FractoriumFinalRenderDialog::OnPrefixChanged(const QString& s)
{
	Path(m_Controller->ComposePath(m_Controller->Name()));
}

/// <summary>
/// Change the suffix appended to the output file name.
/// </summary>
/// <param name="s">Ignored</param>
void FractoriumFinalRenderDialog::OnSuffixChanged(const QString& s)
{
	Path(m_Controller->ComposePath(m_Controller->Name()));
}

/// <summary>
/// Start the render process.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumFinalRenderDialog::OnRenderClicked(bool checked)
{
	if (CreateControllerFromGUI(true))
		m_Controller->Render();
}

/// <summary>
/// Cancel the render.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumFinalRenderDialog::OnCancelRenderClicked(bool checked)
{
	if (m_Controller.get())
		m_Controller->CancelRender();
}

/// <summary>
/// Uses the options and the ember to populate widgets.
/// Called when the dialog is initially shown.
/// </summary>
/// <param name="e">The event</param>
void FractoriumFinalRenderDialog::showEvent(QShowEvent* e)
{
	if (CreateControllerFromGUI(true))
	{
		int index = m_Fractorium->m_Controller->Index() + 1;
#ifdef DO_DOUBLE
		Ember<double> ed;
		EmberFile<double> efi;
		m_Fractorium->m_Controller->CopyEmberFile(efi, [&](Ember<double>& ember)
		{
			ember.SyncSize();
			ember.m_Quality = m_Settings->FinalQuality();
			ember.m_Supersample = m_Settings->FinalSupersample();
		});//Copy the whole file, will take about 0.2ms per ember in the file.
#else
		Ember<float> ed;
		EmberFile<float> efi;
		m_Fractorium->m_Controller->CopyEmberFile(efi, [&](Ember<float>& ember)
		{
			ember.SyncSize();
			ember.m_Quality = m_Settings->FinalQuality();
			ember.m_Supersample = m_Settings->FinalSupersample();
			ember.m_TemporalSamples = m_Settings->FinalTemporalSamples();
		});//Copy the whole file, will take about 0.2ms per ember in the file.
#endif
		m_Controller->SetEmberFile(efi);//Copy the temp file into the final render controller.
		ui.FinalRenderCurrentSpin->setMaximum(efi.Size());
		ui.FinalRenderCurrentSpin->blockSignals(true);
		ui.FinalRenderCurrentSpin->setValue(index);//Set the currently selected ember to the one that was being edited.
		ui.FinalRenderCurrentSpin->blockSignals(false);
		OnCurrentSpinChanged(index);//Force update in case the ember was new, but at the same index as the previous one.
		m_Controller->m_ImageCount = 0;
		SetMemory();
		m_Controller->ResetProgress();
		QString s = m_Settings->SaveFolder();

		if (Exists(s))
			Path(m_Controller->ComposePath(m_Controller->Name()));//Update the GUI.
	}

	ui.FinalRenderTextOutput->clear();
	QDialog::showEvent(e);
}

/// <summary>
/// Close the dialog without running, or if running, cancel and exit.
/// Settings will not be saved.
/// Control will be returned to Fractorium::OnActiOn().
/// </summary>
void FractoriumFinalRenderDialog::reject()
{
	if (m_Controller.get())
	{
		m_Controller->CancelRender();
		m_Controller->DeleteRenderer();
	}

	QDialog::reject();
}

/// <summary>
/// Create the controller from the options and optionally its underlying renderer.
/// </summary>
/// <returns>True if successful, else false.</returns>
bool FractoriumFinalRenderDialog::CreateControllerFromGUI(bool createRenderer)
{
	bool ok = true;
	int index = Current() - 1;
#ifdef DO_DOUBLE
	size_t elementSize = Double() ? sizeof(double) : sizeof(float);
#else
	size_t elementSize = sizeof(float);
#endif

	if (!m_Controller.get() || (m_Controller->SizeOfT() != elementSize))
	{
#ifdef DO_DOUBLE
		Ember<double> ed;
		Ember<double> orig;
		EmberFile<double> efd;
#else
		Ember<float> ed;
		Ember<float> orig;
		EmberFile<float> efd;
#endif

		//First check if a controller has already been created, and if so, save its embers and gracefully shut it down.
		if (m_Controller.get())
		{
#ifdef DO_DOUBLE
			m_Controller->CopyEmberFile(efd, [&](Ember<double>& ember) { });//Convert float to double or save double verbatim;
#else
			m_Controller->CopyEmberFile(efd, [&](Ember<float>& ember) { });//Convert float to double or save double verbatim;
#endif
			m_Controller->Shutdown();
		}

		//Create a float or double controller based on the GUI.
#ifdef DO_DOUBLE

		if (Double())
			m_Controller = unique_ptr<FinalRenderEmberControllerBase>(new FinalRenderEmberController<double>(this));
		else
#endif
			m_Controller = unique_ptr<FinalRenderEmberControllerBase>(new FinalRenderEmberController<float>(this));

		//Restore the ember and ember file.
		if (m_Controller.get())
		{
			m_Controller->SetEmberFile(efd);//Convert float to double or set double verbatim;
			m_Controller->SetEmber(index);
		}
	}

	if (m_Controller.get())
	{
		if (createRenderer)
			return m_Controller->CreateRendererFromGUI();
		else
			return true;
	}
	else
		return false;
}

/// <summary>
/// Compute the amount of memory needed via call to SyncAndComputeMemory(), then
/// assign the result to the table cell as text.
/// Report errors if not enough memory is available for any of the selected devices.
/// </summary>
/// <returns>True devices and a controller is present, else false.</returns>
bool FractoriumFinalRenderDialog::SetMemory()
{
	if (isVisible() && CreateControllerFromGUI(true))
	{
		bool error = false;
		tuple<size_t, size_t, size_t> p = m_Controller->SyncAndComputeMemory();
		QString s;
		ui.FinalRenderParamsTable->item(m_MemoryCellIndex, 1)->setText(ToString<qulonglong>(get<1>(p)));
		ui.FinalRenderParamsTable->item(m_ItersCellIndex, 1)->setText(ToString<qulonglong>(get<2>(p)));

		if (OpenCL() && !m_Wrappers.empty())
		{
			auto devices = Devices();

			for (size_t i = 0; i < m_Wrappers.size(); i++)
			{
				if (devices.contains(int(i)))
				{
					size_t histSize = get<0>(p);
					size_t totalSize = get<1>(p);
					size_t maxAlloc = m_Wrappers[i].MaxAllocSize();
					size_t totalAvail = m_Wrappers[i].GlobalMemSize();
					QString temp;

					if (histSize > maxAlloc)
					{
						temp = "Histogram/Accumulator memory size of " + ToString<qulonglong>(histSize) +
							   " is greater than the max OpenCL allocation size of " + ToString<qulonglong>(maxAlloc);
					}

					if (totalSize > totalAvail)
					{
						temp += "\n\nTotal required memory size of " + ToString<qulonglong>(totalSize) +
								" is greater than the max OpenCL available memory of " + ToString<qulonglong>(totalAvail);
					}

					if (!temp.isEmpty())
					{
						error = true;
						s += QString::fromStdString(m_Wrappers[i].DeviceName()) + ":\n" + temp + "\n\n";
					}
				}
			}

			if (!s.isEmpty())
				s += "Rendering will most likely fail.";

			ui.FinalRenderTextOutput->setText(s);
		}

		if (!error)
			ui.FinalRenderTextOutput->clear();

		return true;
	}

	return false;
}
