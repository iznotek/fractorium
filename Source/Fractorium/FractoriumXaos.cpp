#include "FractoriumPch.h"
#include "Fractorium.h"

/// <summary>
/// Initialize the xforms xaos UI.
/// </summary>
void Fractorium::InitXaosUI()
{
	int spinHeight = 20;

	ui.XaosTableView->verticalHeader()->setSectionsClickable(true);
	ui.XaosTableView->horizontalHeader()->setSectionsClickable(true);

	m_XaosSpinBox = new DoubleSpinBox(nullptr, spinHeight, 0.1);
	m_XaosSpinBox->setFixedWidth(35);
	m_XaosSpinBox->DoubleClick(true);
	m_XaosSpinBox->DoubleClickZero(1);
	m_XaosSpinBox->DoubleClickNonZero(0);

	m_XaosTableModel = nullptr;
	m_XaosTableItemDelegate = new DoubleSpinBoxTableItemDelegate(m_XaosSpinBox, this);

	connect(m_XaosSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnXaosChanged(double)), Qt::QueuedConnection);
	connect(ui.ClearXaosButton, SIGNAL(clicked(bool)), this, SLOT(OnClearXaosButtonClicked(bool)), Qt::QueuedConnection);
	connect(ui.RandomXaosButton, SIGNAL(clicked(bool)), this, SLOT(OnRandomXaosButtonClicked(bool)), Qt::QueuedConnection);
	connect(ui.XaosTableView->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(OnXaosRowDoubleClicked(int)), Qt::QueuedConnection);
	connect(ui.XaosTableView->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(OnXaosColDoubleClicked(int)), Qt::QueuedConnection);
}

/// <summary>
/// Fill the xaos table with the values from the ember.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::FillXaos()
{
	for (int i = 0, count = int(XformCount()); i < count; i++)
	{
		if (auto xform = m_Ember.GetXform(i))
		{
			for (int j = 0; j < count; j++)
			{
				QModelIndex index = m_Fractorium->m_XaosTableModel->index(i, j, QModelIndex());
				m_Fractorium->m_XaosTableModel->setData(index, xform->Xaos(j));
			}
		}
	}
}

/// <summary>
/// Create and return a xaos name string.
/// </summary>
/// <param name="i">The index of the xform whose xaos will be used</param>
/// <returns>The xaos name string</returns>
template <typename T>
QString FractoriumEmberController<T>::MakeXaosNameString(uint i)
{
	Xform<T>* xform = m_Ember.GetXform(i);
	QString name;

	//if (xform)
	//{
	//	int indexPlus1 = m_Ember.GetXformIndex(xform) + 1;//GUI is 1 indexed to avoid confusing the user.
	//	int curr = m_Fractorium->ui.CurrentXformCombo->currentIndex() + 1;
	//
	//	if (indexPlus1 != -1)
	//	{
	//		if (m_Fractorium->ui.XaosToRadio->isChecked())
	//			name = QString("From ") + ToString(curr) + QString(" To ") + ToString(indexPlus1);
	//		else
	//			name = QString("From ") + ToString(indexPlus1) + QString(" To ") + ToString(curr);
	//
	//		//if (xform->m_Name != "")
	//		//	name = name + " (" + QString::fromStdString(xform->m_Name) + ")";
	//	}
	//}

	return name;
}

/// <summary>
/// Set the xaos value.
/// Called when any xaos spinner is changed.
/// It actually gets called multiple times as the user clicks around the
/// xaos table due to how QTableView passes events to and from its model.
/// To filter out spurrious events, the value is checked against the existing
/// xaos value.
/// Resets the rendering process.
/// </summary>
/// <param name="sender">The DoubleSpinBox that triggered this event</param>
template <typename T>
void FractoriumEmberController<T>::XaosChanged(int x, int y, double val)
{
	if (Xform<T>* xform = m_Ember.GetXform(x))
		if (!IsClose<T>(val, xform->Xaos(y), 1e-10))//Ensure it actually changed.
			Update([&] { xform->SetXaos(y, val); });
}

void Fractorium::OnXaosChanged(double d)
{
	if (auto* senderSpinBox = qobject_cast<DoubleSpinBox*>(this->sender()))
	{
		auto p = senderSpinBox->property("tableindex").toPoint();

		m_Controller->XaosChanged(p.x(), p.y(), d);
	}
}

void Fractorium::OnXaosTableModelDataChanged(const QModelIndex& indexA, const QModelIndex& indexB)
{
	m_Controller->XaosChanged(indexA.row(), indexA.column(), indexA.data().toDouble());
}

/// <summary>
/// Clear xaos table, recreate all spinners based on the xaos used in the current ember.
/// </summary>
void Fractorium::FillXaosTable()
{
	int count = int(m_Controller->XformCount());
	QStringList hl, vl;
	auto oldModel = m_XaosTableModel;

	hl.reserve(count);
	vl.reserve(count);
	m_XaosTableModel = new QStandardItemModel(count, count, this);
	connect(m_XaosTableModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), SLOT(OnXaosTableModelDataChanged(QModelIndex, QModelIndex)));
	ui.XaosTableView->blockSignals(true);

	for (int i = 0; i < count; i++)
	{
		auto s = QString::number(i + 1);

		hl.push_back("F" + s);
		vl.push_back("T" + s);
	}

	m_XaosTableModel->setHorizontalHeaderLabels(hl);
	m_XaosTableModel->setVerticalHeaderLabels(vl);

	ui.XaosTableView->setModel(m_XaosTableModel);
	ui.XaosTableView->setItemDelegate(m_XaosTableItemDelegate);
	ui.XaosTableView->resizeRowsToContents();
	ui.XaosTableView->resizeColumnsToContents();

	SetTabOrder(this, ui.ClearXaosButton, ui.RandomXaosButton);
	ui.XaosTableView->blockSignals(false);
	
	if (oldModel)
		delete oldModel;
}

/// <summary>
/// Clear all xaos from the current ember.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::ClearXaos()
{
	Update([&] { m_Ember.ClearXaos(); });
	FillXaos();
}

void Fractorium::OnClearXaosButtonClicked(bool checked) { m_Controller->ClearXaos(); }

/// <summary>
/// Set all xaos values to random numbers.
/// There is a 50% chance they're set to 0 or 1, and
/// 50% that they're 0-3.
/// Resets the rendering process.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::RandomXaos()
{
	Update([&]
	{
		for (size_t i = 0; i < m_Ember.XformCount(); i++)
		{
			if (auto* xform = m_Ember.GetXform(i))
			{
				for (size_t j = 0; j < m_Ember.XformCount(); j++)
				{
					if (m_Rand.RandBit())
						xform->SetXaos(j, T(m_Rand.RandBit()));
					else
						xform->SetXaos(j, m_Rand.Frand<T>(0, 3));
				}
			}
		}
	});
	
	FillXaos();
}

void Fractorium::OnRandomXaosButtonClicked(bool checked) { m_Controller->RandomXaos(); }

/// <summary>
/// Toggle all xaos values in one row.
/// Resets the rendering process.
/// </summary>
/// <param name="logicalIndex">The index of the row that was double clicked</param>
void Fractorium::OnXaosRowDoubleClicked(int logicalIndex)
{
	ToggleTableRow(ui.XaosTableView, logicalIndex);
}

/// <summary>
/// Toggle all xaos values in one column.
/// Resets the rendering process.
/// </summary>
/// <param name="logicalIndex">The index of the column that was double clicked</param>
void Fractorium::OnXaosColDoubleClicked(int logicalIndex)
{
	ToggleTableCol(ui.XaosTableView, logicalIndex);
}

template class FractoriumEmberController<float>;

#ifdef DO_DOUBLE
	template class FractoriumEmberController<double>;
#endif
