#include "FractoriumPch.h"
#include "OptionsDialog.h"
#include "Fractorium.h"

/// <summary>
/// Constructor that takes a pointer to the settings object and the parent widget.
/// </summary>
/// <param name="settings">A pointer to the settings object to use</param>
/// <param name="p">The parent widget. Default: nullptr.</param>
/// <param name="f">The window flags. Default: 0.</param>
FractoriumOptionsDialog::FractoriumOptionsDialog(FractoriumSettings* settings, QWidget* p, Qt::WindowFlags f)
	: QDialog(p, f),
	m_Info(OpenCLInfo::Instance())
{
	int i, row = 0, spinHeight = 20;

	ui.setupUi(this);
	m_Settings = settings;
	QTableWidget* table = ui.OptionsXmlSavingTable;
	ui.ThreadCountSpin->setRange(1, Timing::ProcessorCount());
	connect(ui.OpenCLCheckBox, SIGNAL(stateChanged(int)),		 this, SLOT(OnOpenCLCheckBoxStateChanged(int)),	 Qt::QueuedConnection);
	connect(ui.DeviceTable,	   SIGNAL(cellChanged(int, int)),	 this, SLOT(OnDeviceTableCellChanged(int, int)), Qt::QueuedConnection);

	SetupSpinner<SpinBox, int>(table, this, row, 1, m_XmlTemporalSamplesSpin, spinHeight,  1,	1000, 100, "", "", true, 1000);
	SetupSpinner<SpinBox, int>(table, this, row, 1, m_XmlQualitySpin,		  spinHeight,  1, 200000,  50, "", "", true, 1000);
	SetupSpinner<SpinBox, int>(table, this, row, 1, m_XmlSupersampleSpin,	  spinHeight,  1,	   4,   1, "", "", true,    2);

	m_IdEdit = new QLineEdit(ui.OptionsIdentityTable);
	ui.OptionsIdentityTable->setCellWidget(0, 1, m_IdEdit);

	m_UrlEdit = new QLineEdit(ui.OptionsIdentityTable);
	ui.OptionsIdentityTable->setCellWidget(1, 1, m_UrlEdit);

	m_NickEdit = new QLineEdit(ui.OptionsIdentityTable);
	ui.OptionsIdentityTable->setCellWidget(2, 1, m_NickEdit);

	table = ui.DeviceTable;
	table->clearContents();
	table->setRowCount(0);

	if (m_Info.Ok() && !m_Info.Devices().empty())
	{
		SetupDeviceTable(table, m_Settings->Devices());

		for (i = 0; i < table->rowCount(); i++)
			if (auto radio = qobject_cast<QRadioButton*>(table->cellWidget(i, 1)))
				connect(radio, SIGNAL(toggled(bool)), this, SLOT(OnDeviceTableRadioToggled(bool)), Qt::QueuedConnection);
	}
	else
	{
		ui.DeviceTable->setEnabled(false);
		ui.OpenCLCheckBox->setChecked(false);
		ui.OpenCLCheckBox->setEnabled(false);
		ui.OpenCLSubBatchSpin->setEnabled(false);
		ui.OpenCLFilteringDERadioButton->setEnabled(false);
		ui.OpenCLFilteringLogRadioButton->setEnabled(false);
		ui.InteraciveGpuFilteringGroupBox->setEnabled(false);
	}

	DataToGui();
	OnOpenCLCheckBoxStateChanged(ui.OpenCLCheckBox->isChecked());
}

/// <summary>
/// GUI settings wrapper functions, getters only.
/// </summary>

bool FractoriumOptionsDialog::EarlyClip() { return ui.EarlyClipCheckBox->isChecked(); }
bool FractoriumOptionsDialog::YAxisUp() { return ui.YAxisUpCheckBox->isChecked(); }
bool FractoriumOptionsDialog::Transparency() { return ui.TransparencyCheckBox->isChecked(); }
bool FractoriumOptionsDialog::ContinuousUpdate() { return ui.ContinuousUpdateCheckBox->isChecked(); }
bool FractoriumOptionsDialog::OpenCL() { return ui.OpenCLCheckBox->isChecked(); }
bool FractoriumOptionsDialog::Double() { return ui.DoublePrecisionCheckBox->isChecked(); }
bool FractoriumOptionsDialog::ShowAllXforms() { return ui.ShowAllXformsCheckBox->isChecked(); }
bool FractoriumOptionsDialog::AutoUnique() { return ui.AutoUniqueCheckBox->isChecked(); }
uint FractoriumOptionsDialog::ThreadCount() { return ui.ThreadCountSpin->value(); }

/// <summary>
/// The check state of one of the OpenCL devices was changed.
/// This does a special check to always ensure at least one device,
/// as well as one primary is checked.
/// </summary>
/// <param name="row">The row of the cell</param>
/// <param name="col">The column of the cell</param>
void FractoriumOptionsDialog::OnDeviceTableCellChanged(int row, int col)
{
	if (auto item = ui.DeviceTable->item(row, col))
		HandleDeviceTableCheckChanged(ui.DeviceTable, row, col);
}

/// <summary>
/// The primary device radio button selection was changed.
/// If the device was specified as primary, but was not selected
/// for inclusion, it will automatically be selected for inclusion.
/// </summary>
/// <param name="checked">The state of the radio button</param>
void FractoriumOptionsDialog::OnDeviceTableRadioToggled(bool checked)
{
	int row;
	auto s = sender();
	auto table = ui.DeviceTable;
	QRadioButton* radio = nullptr;

	if (s)
	{
		for (row = 0; row < table->rowCount(); row++)
			if (radio = qobject_cast<QRadioButton*>(table->cellWidget(row, 1)))
				if (s == radio)
				{
					HandleDeviceTableCheckChanged(ui.DeviceTable, row, 1);
					break;
				}
	}
}

/// <summary>
/// Disable or enable the CPU and OpenCL related controls based on the state passed in.
/// Called when the state of the OpenCL checkbox is changed.
/// </summary>
/// <param name="state">The state of the checkbox</param>
void FractoriumOptionsDialog::OnOpenCLCheckBoxStateChanged(int state)
{
	bool checked = state == Qt::Checked;

	ui.DeviceTable->setEnabled(checked);
	ui.ThreadCountSpin->setEnabled(!checked);
	ui.CpuSubBatchSpin->setEnabled(!checked);
	ui.OpenCLSubBatchSpin->setEnabled(checked);
	ui.CpuFilteringDERadioButton->setEnabled(!checked);
	ui.CpuFilteringLogRadioButton->setEnabled(!checked);
	ui.OpenCLFilteringDERadioButton->setEnabled(checked);
	ui.OpenCLFilteringLogRadioButton->setEnabled(checked);
	ui.InteraciveGpuFilteringGroupBox->setEnabled(checked);
}

/// <summary>
/// Save all settings on the GUI to the settings object.
/// Called when the user clicks ok.
/// Not called if cancelled or closed with the X.
/// </summary>
void FractoriumOptionsDialog::accept()
{
	GuiToData();
	QDialog::accept();
}

/// <summary>
/// Restore all GUI items to what was originally in the settings object.
/// Called when the user clicks cancel or closes with the X.
/// </summary>
void FractoriumOptionsDialog::reject()
{
	DataToGui();
	QDialog::reject();
}

/// <summary>
/// Copy the state of the map to the checkboxes and show the dialog.
/// </summary>
/// <param name="e">Event, passed to base.</param>
void FractoriumOptionsDialog::showEvent(QShowEvent* e)
{
	DataToGui();
	QDialog::showEvent(e);
}

/// <summary>
/// Copy the state of the Gui to the settings object.
/// </summary>
void FractoriumOptionsDialog::GuiToData()
{
	//Interactive rendering.
	m_Settings->EarlyClip(EarlyClip());
	m_Settings->YAxisUp(YAxisUp());
	m_Settings->Transparency(Transparency());
	m_Settings->ContinuousUpdate(ContinuousUpdate());
	m_Settings->OpenCL(OpenCL());
	m_Settings->Double(Double());
	m_Settings->ShowAllXforms(ShowAllXforms());
	m_Settings->ThreadCount(ThreadCount());
	m_Settings->CpuSubBatch(ui.CpuSubBatchSpin->value());
	m_Settings->OpenCLSubBatch(ui.OpenCLSubBatchSpin->value());
	m_Settings->CpuDEFilter(ui.CpuFilteringDERadioButton->isChecked());
	m_Settings->OpenCLDEFilter(ui.OpenCLFilteringDERadioButton->isChecked());
	m_Settings->Devices(DeviceTableToSettings(ui.DeviceTable));

	//Xml saving.
	m_Settings->XmlTemporalSamples(m_XmlTemporalSamplesSpin->value());
	m_Settings->XmlQuality(m_XmlQualitySpin->value());
	m_Settings->XmlSupersample(m_XmlSupersampleSpin->value());
	m_Settings->SaveAutoUnique(AutoUnique());

	//Identity.
	m_Settings->Id(m_IdEdit->text());
	m_Settings->Url(m_UrlEdit->text());
	m_Settings->Nick(m_NickEdit->text());
}

/// <summary>
/// Copy the state of the settings object to the Gui.
/// </summary>
void FractoriumOptionsDialog::DataToGui()
{
	//Interactive rendering.
	auto devices = m_Settings->Devices();

	ui.EarlyClipCheckBox->setChecked(m_Settings->EarlyClip());
	ui.YAxisUpCheckBox->setChecked(m_Settings->YAxisUp());
	ui.TransparencyCheckBox->setChecked(m_Settings->Transparency());
	ui.ContinuousUpdateCheckBox->setChecked(m_Settings->ContinuousUpdate());
	ui.OpenCLCheckBox->setChecked(m_Settings->OpenCL());
	ui.DoublePrecisionCheckBox->setChecked(m_Settings->Double());
	ui.ShowAllXformsCheckBox->setChecked(m_Settings->ShowAllXforms());
	ui.ThreadCountSpin->setValue(m_Settings->ThreadCount());
	ui.CpuSubBatchSpin->setValue(m_Settings->CpuSubBatch());
	ui.OpenCLSubBatchSpin->setValue(m_Settings->OpenCLSubBatch());
	SettingsToDeviceTable(ui.DeviceTable, devices);

	if (m_Settings->CpuDEFilter())
		ui.CpuFilteringDERadioButton->setChecked(true);
	else
		ui.CpuFilteringLogRadioButton->setChecked(true);

	if (m_Settings->OpenCLDEFilter())
		ui.OpenCLFilteringDERadioButton->setChecked(true);
	else
		ui.OpenCLFilteringLogRadioButton->setChecked(true);

	//Xml saving.
	m_XmlTemporalSamplesSpin->setValue(m_Settings->XmlTemporalSamples());
	m_XmlQualitySpin->setValue(m_Settings->XmlQuality());
	m_XmlSupersampleSpin->setValue(m_Settings->XmlSupersample());
	ui.AutoUniqueCheckBox->setChecked(m_Settings->SaveAutoUnique());

	//Identity.
	m_IdEdit->setText(m_Settings->Id());
	m_UrlEdit->setText(m_Settings->Url());
	m_NickEdit->setText(m_Settings->Nick());
}
