#pragma once

#include "ui_OptionsDialog.h"
#include "FractoriumSettings.h"
#include "SpinBox.h"

/// <summary>
/// FractoriumOptionsDialog class.
/// </summary>

class Fractorium;//Forward declaration since Fractorium uses this dialog.

/// <summary>
/// The options dialog allows the user to save various preferences
/// between program runs.
/// It has a pointer to a FractoriumSettings object which is assigned
/// in the constructor. The main window holds the object as a member and the
/// pointer to it here is just for convenience.
/// </summary>
class FractoriumOptionsDialog : public QDialog
{
	Q_OBJECT

	friend Fractorium;

public:
	FractoriumOptionsDialog(FractoriumSettings* settings, QWidget* p = 0, Qt::WindowFlags f = 0);

public slots:
	void OnOpenCLCheckBoxStateChanged(int state);
	void OnDeviceTableCellChanged(int row, int col);
	void OnDeviceTableRadioToggled(bool checked);
	virtual void accept() override;
	virtual void reject() override;

protected:
	virtual void showEvent(QShowEvent* e) override;

private:
	bool EarlyClip();
	bool YAxisUp();
	bool AlphaChannel();
	bool Transparency();
	bool ContinuousUpdate();
	bool OpenCL();
	bool Double();
	bool ShowAllXforms();
	bool AutoUnique();
	uint ThreadCount();
	void DataToGui();
	void GuiToData();

	Ui::OptionsDialog ui;
	OpenCLInfo& m_Info;
	SpinBox* m_XmlTemporalSamplesSpin;
	SpinBox* m_XmlQualitySpin;
	SpinBox* m_XmlSupersampleSpin;
	QLineEdit* m_IdEdit;
	QLineEdit* m_UrlEdit;
	QLineEdit* m_NickEdit;
	FractoriumSettings* m_Settings;
};
