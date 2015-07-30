#include "FractoriumPch.h"
#include "VariationsDialog.h"

/// <summary>
/// Constructor that takes a parent widget and passes it to the base, then
/// sets up the GUI.
/// </summary>
/// <param name="settings">Pointer to the global settings object to use</param>
/// <param name="p">The parent widget. Default: nullptr.</param>
/// <param name="f">The window flags. Default: 0.</param>
FractoriumVariationsDialog::FractoriumVariationsDialog(FractoriumSettings* settings, QWidget* p, Qt::WindowFlags f)
	: QDialog(p, f),
	m_Settings(settings)
{
	ui.setupUi(this);

	auto table = ui.VariationsTable;

	m_Vars = m_Settings->Variations();
	Populate();
	OnSelectAllButtonClicked(true);
	table->verticalHeader()->setSectionsClickable(true);
	table->horizontalHeader()->setSectionsClickable(true);
	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	connect(table, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(OnVariationsTableItemChanged(QTableWidgetItem*)), Qt::QueuedConnection);
	connect(ui.SelectAllButton, SIGNAL(clicked(bool)), this, SLOT(OnSelectAllButtonClicked(bool)), Qt::QueuedConnection);
	connect(ui.InvertSelectionButton, SIGNAL(clicked(bool)), this, SLOT(OnInvertSelectionButtonClicked(bool)), Qt::QueuedConnection);
	connect(ui.SelectNoneButton, SIGNAL(clicked(bool)), this, SLOT(OnSelectNoneButtonClicked(bool)), Qt::QueuedConnection);
}

/// <summary>
/// A wrapper to iterate over every table widget item and perform the passed in function on it.
/// </summary>
/// <param name="func">Function to call on each object</param>
void FractoriumVariationsDialog::ForEachCell(std::function<void(QTableWidgetItem* cb)> func)
{
	auto table = ui.VariationsTable;
	auto rows = table->rowCount();
	auto cols = table->columnCount();

	table->model()->blockSignals(true);

	for (auto row = 0; row < rows; row++)
		for (auto col = 0; col < cols; col++)
			if (auto cb = table->item(row, col))
				func(cb);

	table->model()->blockSignals(false);
	table->model()->layoutChanged();
}

/// <summary>
/// A wrapper to iterate over every selected table widget item and perform the passed in function on it.
/// </summary>
/// <param name="func">Function to call on each object</param>
void FractoriumVariationsDialog::ForEachSelectedCell(std::function<void(QTableWidgetItem* cb)> func)
{
	auto table = ui.VariationsTable;
	QList<QTableWidgetItem*> selectedItems = table->selectedItems();

	table->model()->blockSignals(true);

	foreach(QTableWidgetItem* item, selectedItems)
		if (item)
			func(item);

	table->model()->blockSignals(false);
	table->model()->layoutChanged();
}

/// <summary>
/// Copy the values of the checkboxes to the map.
/// </summary>
void FractoriumVariationsDialog::SyncSettings()
{
	QMap<QString, QVariant> m;

	ForEachCell([&](QTableWidgetItem* cb)
	{
		if (!cb->text().isEmpty())
			m[cb->text()] = cb->checkState() == Qt::CheckState::Checked;
	});

	m_Settings->Variations(m);
}

/// <summary>
/// Return a const reference to the map.
/// This will contains the state of the checkboxes after
/// the user clicks ok.
/// </summary>
const QMap<QString, QVariant>& FractoriumVariationsDialog::Map()
{
	return m_Vars;
}

/// <summary>
/// Check all of the checkboxes.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumVariationsDialog::OnSelectAllButtonClicked(bool checked)
{
	ForEachCell([&](QTableWidgetItem* cb) { cb->setCheckState(Qt::CheckState::Checked); });
}

/// <summary>
/// Invert the selection state of the checkboxes.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumVariationsDialog::OnInvertSelectionButtonClicked(bool checked)
{
	ForEachCell([&](QTableWidgetItem* cb)
	{
		if (cb->checkState() != Qt::CheckState::Checked)
			cb->setCheckState(Qt::CheckState::Checked);
		else
			cb->setCheckState(Qt::CheckState::Unchecked);
	});
}

/// <summary>
/// Uncheck all of the checkboxes.
/// </summary>
/// <param name="checked">Ignored</param>
void FractoriumVariationsDialog::OnSelectNoneButtonClicked(bool checked)
{
	ForEachCell([&](QTableWidgetItem* cb) { cb->setCheckState(Qt::CheckState::Unchecked); });
}

/// <summary>
/// Create all checkboxes and check them according to the map.
/// </summary>
void FractoriumVariationsDialog::Populate()
{
	auto table = ui.VariationsTable;
	auto size = std::max<size_t>(std::max<size_t>(m_VariationList.RegSize(), m_VariationList.PreSize()), m_VariationList.PostSize());

	table->setRowCount(size);

	for (size_t i = 0; i < size; i++)
	{
		if (auto pre = m_VariationList.GetVariation(i, eVariationType::VARTYPE_PRE))
		{
			auto cb = new QTableWidgetItem(pre->Name().c_str());
			table->setItem(i, 0, cb);
			SetCheckFromMap(cb, pre);
		}

		if (auto reg = m_VariationList.GetVariation(i, eVariationType::VARTYPE_REG))
		{
			auto cb = new QTableWidgetItem(reg->Name().c_str());
			table->setItem(i, 1, cb);
			SetCheckFromMap(cb, reg);
		}
		
		if (auto post = m_VariationList.GetVariation(i, eVariationType::VARTYPE_POST))
		{
			auto cb = new QTableWidgetItem(post->Name().c_str());
			table->setItem(i, 2, cb);
			SetCheckFromMap(cb, post);
		}
	}
}

/// <summary>
/// Called when a checkbox changes state.
/// There is a slight hack here to apply the change to all selected checkboxes
/// if ctrl is pressed.
/// Otherwise it will only apply to the checkbox that was clicked.
/// </summary>
/// <param name="item"></param>
void FractoriumVariationsDialog::OnVariationsTableItemChanged(QTableWidgetItem* item)
{
	bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);

	if (ctrl)
		ForEachSelectedCell([&](QTableWidgetItem* cb) { cb->setCheckState(item->checkState()); });
}

/// <summary>
/// Called when the user clicks ok.
/// Copy the state of the checkboxes to the map.
/// </summary>
void FractoriumVariationsDialog::accept()
{
	GuiToData();
	QDialog::accept();
}

/// <summary>
/// Called when the user clicks cancel.
/// Reset the state of the the checkboxes to what the map previously was
/// when the dialog was shown.
/// </summary>
void FractoriumVariationsDialog::reject()
{
	DataToGui();
	QDialog::reject();
}

/// <summary>
/// Copy the state of the map to the checkboxes and show the dialog.
/// </summary>
/// <param name="e">Event, passed to base.</param>
void FractoriumVariationsDialog::showEvent(QShowEvent* e)
{
	DataToGui();
	QDialog::showEvent(e);
}

/// <summary>
/// Copy the values in the map to the state of the checkboxes.
/// </summary>
void FractoriumVariationsDialog::DataToGui()
{
	ForEachCell([&](QTableWidgetItem* cb)
	{
		if (auto var = m_VariationList.GetVariation(cb->text().toStdString()))
			SetCheckFromMap(cb, var);
	});
}

/// <summary>
/// Copy the state of the checkboxes to the map.
/// </summary>
void FractoriumVariationsDialog::GuiToData()
{
	ForEachCell([&](QTableWidgetItem* cb)
	{
		if (auto var = m_VariationList.GetVariation(cb->text().toStdString()))
			m_Vars[cb->text()] = (cb->checkState() == Qt::Checked);
	});
}

/// <summary>
/// Set the state of the passed in table item checkbox based on the boolean contained
/// in the map for the passed in variation.
/// </summary>
/// <param name="cb">The checkbox to check</param>
/// <param name="var">That variation to be looked up in the map</param>
void FractoriumVariationsDialog::SetCheckFromMap(QTableWidgetItem* cb, const Variation<float>* var)
{
	if (!m_Vars.contains(var->Name().c_str()))
	{
		cb->setCheckState(Qt::Checked);
	}
	else
	{
		bool chk = m_Vars[var->Name().c_str()].toBool();

		cb->setCheckState(chk ? Qt::Checked : Qt::Unchecked);
	}
}
