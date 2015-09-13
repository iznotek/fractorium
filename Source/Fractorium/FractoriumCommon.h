#pragma once

#include "FractoriumPch.h"

/// <summary>
/// Fractorium global utility functions.
/// </summary>

/// <summary>
/// Setup a spinner to be placed in a table cell.
/// Due to a serious compiler bug in MSVC, this must be declared as an outside function instead of a static member of Fractorium.
/// The reason is that the default arguments of type valType will not be interpreted correctly by the compiler.
/// If the bug is ever fixed, put it back as a static member function.
/// </summary>
/// <param name="table">The table the spinner belongs to</param>
/// <param name="receiver">The receiver object</param>
/// <param name="row">The row in the table where this spinner resides</param>
/// <param name="col">The col in the table where this spinner resides</param>
/// <param name="spinBox">Double pointer to spin box which will hold the spinner upon exit</param>
/// <param name="height">The height of the spinner</param>
/// <param name="min">The minimum value of the spinner</param>
/// <param name="max">The maximum value of the spinner</param>
/// <param name="step">The step of the spinner</param>
/// <param name="signal">The signal the spinner emits</param>
/// <param name="slot">The slot to receive the signal</param>
/// <param name="incRow">Whether to increment the row value</param>
/// <param name="val">The default value for the spinner</param>
/// <param name="doubleClickZero">When the spinner has a value of zero and is double clicked, assign this value</param>
/// <param name="doubleClickNonZero">When the spinner has a value of non-zero and is double clicked, assign this value</param>
template<typename spinType, typename valType>
static void SetupSpinner(QTableWidget* table, const QObject* receiver, int& row, int col, spinType*& spinBox, int height, valType min, valType max, valType step, const char* signal, const char* slot, bool incRow = true, valType val = 0, valType doubleClickZero = -999, valType doubleClickNonZero = -999)
{
	spinBox = new spinType(table, height, step);
	spinBox->setRange(min, max);
	spinBox->setValue(val);

	if (col >= 0)
		table->setCellWidget(row, col, spinBox);

	if (string(signal) != "" && string(slot) != "")
		receiver->connect(spinBox, signal, receiver, slot, Qt::QueuedConnection);

	if (doubleClickNonZero != -999 && doubleClickZero != -999)
	{
		spinBox->DoubleClick(true);
		spinBox->DoubleClickZero(valType(doubleClickZero));
		spinBox->DoubleClickNonZero(valType(doubleClickNonZero));
	}

	if (incRow)
		row++;
}

/// <summary>
/// Wrapper around QWidget::setTabOrder() to return the second widget.
/// This makes it easy to chain multiple calls without having to retype
/// all of them if the order changes or if a new widget is inserted.
/// </summary>
/// <param name="p">The parent widget that w1 and w2 belong to</param>
/// <param name="w1">The widget to come first in the tab order</param>
/// <param name="w2">The widget to come second in the tab order</param>
static QWidget* SetTabOrder(QWidget* p, QWidget* w1, QWidget* w2)
{
	p->setTabOrder(w1, w2);
	return w2;
}

/// <summary>
/// Wrapper around QLocale::system().toDouble().
/// </summary>
/// <param name="s">The string to convert</param>
/// <param name="ok">Pointer to boolean which stores the success value of the conversion</param>
/// <returns>The converted value if successful, else 0.</returns>
static double ToDouble(const QString &s, bool *ok)
{
	return QLocale::system().toDouble(s, ok);
}

/// <summary>
/// Wrapper around QLocale::system().toString().
/// </summary>
/// <param name="s">The value to convert</param>
/// <returns>The string value if successful, else "".</returns>
template <typename T>
static QString ToString(T val)
{
	return QLocale::system().toString(val);
}

/// <summary>
/// Force a QString to end with the specified value.
/// </summary>
/// <param name="s">The string to append a suffix to</param>
/// <param name="e">The suffix to append</param>
/// <returns>The original string value if it already ended in e, else the original value appended with e.</returns>
template <typename T>
static QString MakeEnd(const QString& s, T e)
{
	if (!s.endsWith(e))
		return s + e;
	else
		return s;
}

/// <summary>
/// Check if a path is not empty and exists on the file system.
/// </summary>
/// <param name="s">The path to check</param>
/// <returns>True if s was not empty and existed, else false.</returns>
static bool Exists(const QString& s)
{
	 return s != "" && QDir(s).exists();
}

/// <summary>
/// Convert a color to one that is displayable on any background.
/// </summary>
/// <param name="color">The color to convert</param>
/// <returns>The converted color</returns>
static QColor VisibleColor(const QColor& color)
{
	int threshold = 105;
	int delta = (color.red()   * 0.299) + //Magic numbers gotten from a Stack Overflow post.
		(color.green() * 0.587) +
		(color.blue()  * 0.114);

	QColor textColor = (255 - delta < threshold) ? QColor(0, 0, 0) : QColor(255, 255, 255);
	return textColor;
}

/// <summary>
/// Determine whether an xform in an ember is linked to any other xform
/// in the ember.
/// </summary>
/// <param name="ember">The ember which contains the xform</param>
/// <param name="xform">The xform to inspect</param>
/// <returns>The index of the xform that the xform argument is linked to, else -1</returns>
template <typename T>
static intmax_t IsXformLinked(Ember<T>& ember, Xform<T>* xform)
{
	auto count = ember.XformCount();
	auto index = ember.GetXformIndex(xform);
	intmax_t linked = -1;
	size_t toOneCount = 0;
	size_t toZeroCount = 0;
	size_t toOneIndex = 0;
	size_t fromOneCount = 0;
	size_t fromZeroCount = 0;
	size_t fromOneIndex = 0;

	if (index >= 0)
	{
		for (auto i = 0; i < count; i++)
		{
			if (xform->Xaos(i) == 0)
				toZeroCount++;
			else if (xform->Xaos(i) == 1)
			{
				toOneIndex = i;
				toOneCount++;
			}
		}

		if ((toZeroCount == (count - 1)) && toOneCount == 1)
		{
			for (auto i = 0; i < count; i++)
			{
				if (auto fromXform = ember.GetXform(i))
				{
					if (fromXform->Xaos(toOneIndex) == 0)
						fromZeroCount++;
					else if (fromXform->Xaos(toOneIndex) == 1)
					{
						fromOneIndex = i;
						fromOneCount++;
					}
				}
			}

			if ((fromZeroCount == (count - 1)) && fromOneCount == 1)
			{
				linked = toOneIndex;
			}
		}
	}

	return linked;
}

/// <summary>
/// Convert the passed in QList<QVariant> of absolute device indices to a vector<pair<size_t, size_t>> of platform,device
/// index pairs.
/// </summary>
/// <param name="selectedDevices">The absolute device indices</param>
/// <returns>The converted device vector of platform,device index pairs</returns>
static vector<pair<size_t, size_t>> Devices(const QList<QVariant>& selectedDevices)
{
	vector<pair<size_t, size_t>> vec;
	OpenCLInfo& info = OpenCLInfo::Instance();
	auto& devices = info.DeviceIndices();

	vec.reserve(selectedDevices.size());

	for (size_t i = 0; i < selectedDevices.size(); i++)
	{
		auto index = selectedDevices[i].toUInt();

		if (index < devices.size())
			vec.push_back(devices[index]);
	}

	return vec;
}

/// <summary>
/// Setup a table showing all available OpenCL devices on the system.
/// Create checkboxes and radio buttons which allow the user to specify
/// which devices to use, and which one to make the primary device.
/// Used in the options dialog and the final render dialog.
/// </summary>
/// <param name="table">The QTableWidget to setup</param>
/// <param name="settingsDevices">The absolute indices of the devices to use, with the first being the primary.</param>
static void SetupDeviceTable(QTableWidget* table, const QList<QVariant>& settingsDevices)
{
	bool primary = false;
	auto& deviceNames = OpenCLInfo::Instance().AllDeviceNames();

	table->clearContents();
	table->setRowCount(deviceNames.size());

	for (size_t i = 0; i < deviceNames.size(); i++)
	{
		auto checkItem = new QTableWidgetItem();
		auto radio = new QRadioButton();
		auto deviceItem = new QTableWidgetItem(QString::fromStdString(deviceNames[i]));

		table->setItem(i, 0, checkItem);
		table->setCellWidget(i, 1, radio);
		table->setItem(i, 2, deviceItem);

		if (settingsDevices.contains(QVariant::fromValue(i)))
		{
			checkItem->setCheckState(Qt::Checked);

			if (!primary)
			{
				radio->setChecked(true);
				primary = true;
			}
		}
		else
			checkItem->setCheckState(Qt::Unchecked);
	}

	if (!primary && table->rowCount() > 0)//Primary was never set, so just default to the first device and hope it was the one detected as the main display.
	{
		table->item(0, 0)->setCheckState(Qt::Checked);
		qobject_cast<QRadioButton*>(table->cellWidget(0, 1))->setChecked(true);
	}
}

/// <summary>
/// Copy the passed in selected absolute device indices to the controls on the passed in table.
/// Used in the options dialog and the final render dialog.
/// </summary>
/// <param name="table">The QTableWidget to copy values to</param>
/// <param name="settingsDevices">The absolute indices of the devices to use, with the first being the primary.</param>
static void SettingsToDeviceTable(QTableWidget* table, QList<QVariant>& settingsDevices)
{
	if (settingsDevices.empty() && table->rowCount() > 0)
	{
		table->item(0, 0)->setCheckState(Qt::Checked);
		qobject_cast<QRadioButton*>(table->cellWidget(0, 1))->setChecked(true);

		for (int row = 1; row < table->rowCount(); row++)
			if (auto item = table->item(row, 0))
				item->setCheckState(Qt::Unchecked);
	}
	else
	{
		for (int row = 0; row < table->rowCount(); row++)
		{
			if (auto item = table->item(row, 0))
			{
				if (settingsDevices.contains(row))
				{
					item->setCheckState(Qt::Checked);

					if (!settingsDevices.indexOf(QVariant::fromValue(row)))
						if (auto radio = qobject_cast<QRadioButton*>(table->cellWidget(row, 1)))
							radio->setChecked(true);
				}
				else
				{
					item->setCheckState(Qt::Unchecked);
				}
			}
		}
	}
}

/// <summary>
/// Copy the values of the controls on the passed in table to a list of absolute device indices.
/// Used in the options dialog and the final render dialog.
/// </summary>
/// <param name="table">The QTableWidget to copy values from</param>
/// <returns>The list of absolute device indices</returns>
static QList<QVariant> DeviceTableToSettings(QTableWidget* table)
{
	QList<QVariant> devices;
	auto rows = table->rowCount();

	for (int row = 0; row < rows; row++)
	{
		auto checkItem = table->item(row, 0);
		auto radio = qobject_cast<QRadioButton*>(table->cellWidget(row, 1));

		if (checkItem->checkState() == Qt::Checked)
		{
			if (radio && radio->isChecked())
				devices.push_front(row);
			else
				devices.push_back(row);
		}
	}

	return devices;
}

/// <summary>
/// Ensure device selection on the passed in table make sense.
/// </summary>
/// <param name="table">The QTableWidget to setup</param>
/// <param name="row">The row of the cell</param>
/// <param name="col">The column of the cell</param>
static void HandleDeviceTableCheckChanged(QTableWidget* table, int row, int col)
{
	int primaryRow = -1;
	QRadioButton* primaryRadio = nullptr;

	for (int i = 0; i < table->rowCount(); i++)
	{
		if (auto radio = qobject_cast<QRadioButton*>(table->cellWidget(i, 1)))
		{
			if (radio->isChecked())
			{
				primaryRow = i;
				primaryRadio = radio;
				break;
			}
		}
	}

	if (primaryRow == -1) primaryRow = 0;

	if (auto primaryItem = table->item(primaryRow, 0))
		if (primaryItem->checkState() == Qt::Unchecked)
			primaryItem->setCheckState(Qt::Checked);
}
