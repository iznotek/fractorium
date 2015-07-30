#pragma once

#include "ui_VariationsDialog.h"
#include "FractoriumSettings.h"

/// <summary>
/// FractoriumVariationsDialog class.
/// </summary>

/// <summary>
/// The variations filter dialog displays several columns
/// with the different types of variations shown as checkboxes.
/// This is used to filter the variations that are shown in the main window
/// because the list is very long.
/// The results are stored in a map and returned.
/// These are used in conjunction with the filter edit box to filter what's shown.
/// </summary>
class FractoriumVariationsDialog : public QDialog
{
	Q_OBJECT
public:
	FractoriumVariationsDialog(FractoriumSettings* settings, QWidget* p = nullptr, Qt::WindowFlags f = nullptr);
	void ForEachCell(std::function<void(QTableWidgetItem* cb)> func);
	void ForEachSelectedCell(std::function<void(QTableWidgetItem* cb)> func);
	void SyncSettings();
	const QMap<QString, QVariant>& Map();

public slots:
	void OnSelectAllButtonClicked(bool checked);
	void OnInvertSelectionButtonClicked(bool checked);
	void OnSelectNoneButtonClicked(bool checked);
	void OnVariationsTableItemChanged(QTableWidgetItem* item);
	virtual void accept() override;
	virtual void reject() override;

protected:
	virtual void showEvent(QShowEvent* e) override;

private:
	void DataToGui();
	void GuiToData();
	void Populate();
	void SetCheckFromMap(QTableWidgetItem* cb, const Variation<float>* var);
	VariationList<float> m_VariationList;
	QMap<QString, QVariant> m_Vars;
	FractoriumSettings* m_Settings;
	Ui::VariationsDialog ui;
};
