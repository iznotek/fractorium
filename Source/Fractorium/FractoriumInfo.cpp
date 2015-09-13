#include "FractoriumPch.h"
#include "Fractorium.h"

/// <summary>
/// Initialize the info UI.
/// </summary>
void Fractorium::InitInfoUI()
{
	auto treeHeader = ui.SummaryTreeWidget->header();
	auto tableHeader = ui.SummaryTableWidget->horizontalHeader();

	treeHeader->setVisible(true);
	treeHeader->setSectionsClickable(true);
	treeHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
	connect(treeHeader, SIGNAL(sectionClicked(int)), this, SLOT(OnSummaryTreeHeaderSectionClicked(int)), Qt::QueuedConnection);
	connect(tableHeader, SIGNAL(sectionResized(int, int, int)), this, SLOT(OnSummaryTableHeaderResized(int, int, int)), Qt::QueuedConnection);
	SetFixedTableHeader(ui.SummaryTableWidget->verticalHeader());

	ui.SummaryTableWidget->setItem(0, 0, m_InfoNameItem = new QTableWidgetItem(""));
	ui.SummaryTableWidget->setItem(1, 0, m_InfoPaletteItem = new QTableWidgetItem(""));
	ui.SummaryTableWidget->setItem(2, 0, m_Info3dItem = new QTableWidgetItem(""));
	ui.SummaryTableWidget->setItem(3, 0, m_InfoXaosItem = new QTableWidgetItem(""));
	ui.SummaryTableWidget->setItem(4, 0, m_InfoXformCountItem = new QTableWidgetItem(""));
	ui.SummaryTableWidget->setItem(5, 0, m_InfoFinalXformItem = new QTableWidgetItem(""));
}

/// <summary>
/// Called when the palette cell of the summary table is resized in response
/// to a resizing of the Info dock.
/// </summary>
/// <param name="logicalIndex">Ignored</param>
/// <param name="oldSize">Ignored</param>
/// <param name="newSize">Ignored</param>
void Fractorium::OnSummaryTableHeaderResized(int logicalIndex, int oldSize, int newSize)
{
	QPixmap pixmap = QPixmap::fromImage(m_Controller->FinalPaletteImage());//Create a QPixmap out of the QImage, will be empty on startup.
	SetPaletteTableItem(&pixmap, ui.SummaryTableWidget, m_InfoPaletteItem, 1, 0);
}

/// <summary>
/// Expand or collapse the summary tree depending on the column index clicked.
/// 0: collapse, 1: expand.
/// </summary>
/// <param name="logicalIndex">The column which was clicked</param>
void Fractorium::OnSummaryTreeHeaderSectionClicked(int logicalIndex)
{
	auto tree = ui.SummaryTreeWidget;

	if (logicalIndex)
		tree->expandAll();
	else
		tree->collapseAll();
}

/// <summary>
/// Fill the summary tree with values from the current ember.
/// This is meant to be a rough summary by containing only the most relevant
/// values from the ember.
/// It's also meant to be used in a fire-and-forget way. Once the tree is filled
/// individual nodes are never referenced again.
/// The entire tree is cleared and refilled for every field change.
/// This would seem inefficient, but it appears to update with no flicker.
/// If this ever presents a problem in the future, revisit with a more
/// intelligent design.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::FillSummary()
{
	int p = 3;
	int vp = 4;
	int vlen = 7;
	char pc = 'f';
	size_t x = 0, total = m_Ember.TotalXformCount();
	Xform<T>* xform = nullptr;
	QColor color;
	auto table = m_Fractorium->ui.SummaryTableWidget;
	auto tree = m_Fractorium->ui.SummaryTreeWidget;
	QVariantList states;
	QTreeWidgetItemIterator it(tree);

	while (*it)
	{
		if (!(*it)->parent())//Top level only.
			states += (*it)->isExpanded();

		++it;
	}

	tree->blockSignals(true);
	tree->clear();
	m_Fractorium->m_InfoNameItem->setText(m_Ember.m_Name.c_str());
	m_Fractorium->m_Info3dItem->setText(m_Ember.ProjBits() ? "Yes" : "No");
	m_Fractorium->m_InfoXaosItem->setText(m_Ember.XaosPresent() ? "Yes" : "No");
	m_Fractorium->m_InfoXformCountItem->setText(QString::number(m_Ember.XformCount()));
	m_Fractorium->m_InfoFinalXformItem->setText(m_Ember.UseFinalXform() ? "Yes" : "No");

	QPixmap pixmap = QPixmap::fromImage(m_FinalPaletteImage);//Create a QPixmap out of the QImage.
	QSize size(table->columnWidth(0), table->rowHeight(1) + 1);
	m_Fractorium->m_InfoPaletteItem->setData(Qt::DecorationRole, pixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

	for (x = 0; x < total && (xform = m_Ember.GetTotalXform(x)); x++)
	{
		size_t i = 0;
		QString as = "Pre";
		auto item1 = new QTreeWidgetItem(tree);
		intmax_t linkedIndex = IsXformLinked(m_Ember, xform);
		QString linked = (linkedIndex != -1) ? (" Linked to " + QString::number(linkedIndex + 1)) : "";
		auto index = m_Ember.GetXformIndex(xform);
		m_Ember.CalcNormalizedWeights(m_NormalizedWeights);
		xform->SetPrecalcFlags();//Needed for HasPost() below.

		if (!m_Ember.IsFinalXform(xform) && index != -1)
		{
			item1->setText(0, "Xform " +
				QString::number(x + 1) +
				" (" + QLocale::system().toString(xform->m_Weight, pc, p) + ") (" +
				QLocale::system().toString(double(m_NormalizedWeights[index]), pc, p) + ") " +
				linked);
		}
		else
			item1->setText(0, "Final xform");

		item1->setText(1, xform->m_Name.c_str());

		auto affineItem = new QTreeWidgetItem(item1);
		affineItem->setText(0, "Affine");
		
		if (xform->m_Affine.IsZero())
			as += " Empty";
		else if (xform->m_Affine.IsID())
			as += " ID";

		if (xform->HasPost())
		{
			as += ", Post";

			if (xform->m_Post.IsZero())
				as += " Empty";//Don't need to check further for IsID() because post is not included if it's ID.
		}

		affineItem->setText(1, as);

		auto colorIndexItem = new QTreeWidgetItem(item1);
		colorIndexItem->setText(0, "Color index");
		colorIndexItem->setText(1, QLocale::system().toString(xform->m_ColorX, pc, p));
		color = ColorIndexToQColor(xform->m_ColorX);
		color.setAlphaF(xform->m_Opacity);
		colorIndexItem->setBackgroundColor(1, color);
		colorIndexItem->setTextColor(1, VisibleColor(color));

		auto colorSpeedItem = new QTreeWidgetItem(item1);
		colorSpeedItem->setText(0, "Color speed");
		colorSpeedItem->setText(1, QLocale::system().toString(xform->m_ColorSpeed, pc, p));

		auto opacityItem = new QTreeWidgetItem(item1);
		opacityItem->setText(0, "Opacity");
		opacityItem->setText(1, QLocale::system().toString(xform->m_Opacity, pc, p));

		auto dcItem = new QTreeWidgetItem(item1);
		dcItem->setText(0, "Direct color");
		dcItem->setText(1, QLocale::system().toString(xform->m_DirectColor, pc, p));

		while (auto var = xform->GetVariation(i++))
		{
			auto vitem = new QTreeWidgetItem(item1);

			vitem->setText(0, QString::fromStdString(var->Name()));
			vitem->setText(1, QLocale::system().toString(var->m_Weight, pc, vp).rightJustified(vlen, ' '));

			if (auto parVar = dynamic_cast<ParametricVariation<T>*>(var))
			{
				auto params = parVar->Params();

				for (auto j = 0; j < parVar->ParamCount(); j++)
				{
					if (!params[j].IsPrecalc())
					{
						auto pitem = new QTreeWidgetItem(vitem);

						pitem->setText(0, params[j].Name().c_str());
						pitem->setText(1, QLocale::system().toString(params[j].ParamVal(), pc, vp).rightJustified(vlen, ' '));
					}
				}
			}
		}

		auto item2 = new QTreeWidgetItem(tree);//Empty item in between xforms.
	}

	QTreeWidgetItemIterator it2(tree);

	if (!states.isEmpty())
	{
		while (*it2)
		{
			if (!(*it2)->parent())//Top level only.
			{
				if (!states.isEmpty())
					(*it2)->setExpanded(states.takeFirst().toBool());
				else
					(*it2)->setExpanded(true);//Expand any remainder when going from lesser to greater number of xforms.
			}

			++it2;
		}
	}
	else
		tree->expandAll();

	tree->blockSignals(false);
}

void Fractorium::FillSummary()
{
	m_Controller->FillSummary();
}

/// <summary>
/// Update the histogram bounds display labels.
/// This shows the user the actual bounds of what's
/// being rendered. Mostly of engineering interest.
/// </summary>
void Fractorium::UpdateHistogramBounds()
{
	if (RendererBase* r = m_Controller->Renderer())
	{
		sprintf_s(m_ULString, 32, "UL: %3.3f, %3.3f",   r->LowerLeftX(), r->UpperRightY());//These bounds include gutter padding.
		sprintf_s(m_URString, 32, "UR: %3.3f, %3.3f",  -r->LowerLeftX(), r->UpperRightY());
		sprintf_s(m_LRString, 32, "LR: %3.3f, %3.3f",  -r->LowerLeftX(), r->LowerLeftY());
		sprintf_s(m_LLString, 32, "LL: %3.3f, %3.3f",   r->LowerLeftX(), r->LowerLeftY());
		sprintf_s(m_WHString, 32, "W x H: %4lu x %4lu", r->SuperRasW(),  r->SuperRasH());

		ui.InfoBoundsLabelUL->setText(QString(m_ULString));
		ui.InfoBoundsLabelUR->setText(QString(m_URString));
		ui.InfoBoundsLabelLR->setText(QString(m_LRString));
		ui.InfoBoundsLabelLL->setText(QString(m_LLString));
		ui.InfoBoundsLabelWH->setText(QString(m_WHString));

		ui.InfoBoundsTable->item(0, 1)->setText(ToString<qulonglong>(r->GutterWidth()));

		if (r->GetDensityFilter())
		{
			uint deWidth = (r->GetDensityFilter()->FilterWidth() * 2) + 1;

			sprintf_s(m_DEString, 16, "%d x %d", deWidth, deWidth);
			ui.InfoBoundsTable->item(1, 1)->setText(QString(m_DEString));
		}
		else
			ui.InfoBoundsTable->item(1, 1)->setText("N/A");
	}
}

/// <summary>
/// Fill the passed in QTextEdit with the vector of strings.
/// Optionally clear first.
/// Serves as a convenience function because the error reports coming
/// from Ember and EmberCL use vector<string>.
/// Use invokeMethod() in case this is called from a thread.
/// </summary>
/// <param name="errors">The vector of error strings</param>
/// <param name="textEdit">The QTextEdit to fill</param>
/// <param name="clear">Clear if true, else don't.</param>
void Fractorium::ErrorReportToQTextEdit(const vector<string>& errors, QTextEdit* textEdit, bool clear)
{
	if (clear)
		QMetaObject::invokeMethod(textEdit, "clear", Qt::QueuedConnection);

	for (auto& error : errors)
		QMetaObject::invokeMethod(textEdit, "append", Qt::QueuedConnection, Q_ARG(const QString&, QString::fromStdString(error) + "\n"));
}

template class FractoriumEmberController<float>;

#ifdef DO_DOUBLE
template class FractoriumEmberController<double>;
#endif
