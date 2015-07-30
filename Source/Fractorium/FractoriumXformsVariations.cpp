#include "FractoriumPch.h"
#include "Fractorium.h"

/// <summary>
/// Initialize the xforms variations UI.
/// </summary>
void Fractorium::InitXformsVariationsUI()
{
	auto tree = ui.VariationsTree;

	tree->clear();
	tree->header()->setSectionsClickable(true);
	connect(tree->header(),					SIGNAL(sectionClicked(int)),		 this, SLOT(OnTreeHeaderSectionClicked(int)));
	connect(ui.VariationsFilterLineEdit,	SIGNAL(textChanged(const QString&)), this, SLOT(OnVariationsFilterLineEditTextChanged(const QString&)));
	connect(ui.VariationsFilterClearButton, SIGNAL(clicked(bool)),				 this, SLOT(OnVariationsFilterClearButtonClicked(bool)));
	connect(ui.ActionVariationsDialog,		SIGNAL(triggered(bool)),			 this, SLOT(OnActionVariationsDialog(bool)), Qt::QueuedConnection);

	//Setting dimensions in the designer with a layout is futile, so must hard code here.
	tree->setColumnWidth(0, 160);
	tree->setColumnWidth(1, 23);
}

/// <summary>
/// Show the variations filter dialog.
/// </summary>
/// <param name="checked">Ignored</param>
void Fractorium::OnActionVariationsDialog(bool checked)
{
	if (m_VarDialog->exec())
		Filter();
}

/// <summary>
/// Apply the text passed in, in conjuction with the selections from
/// the variations filter dialog to only show variations whose names
/// contain the substring and are selected.
/// Called when the user types in the variation filter text box and
/// when the variations dialog exits.
/// </summary>
/// <param name="text">The text to filter on</param>
template <typename T>
void FractoriumEmberController<T>::Filter(const QString& text)
{
	auto& ids = m_Fractorium->m_VarDialog->Map();
	auto tree = m_Fractorium->ui.VariationsTree;
	auto xform = CurrentXform();

	tree->setUpdatesEnabled(false);

	for (uint i = 0; i < uint(tree->topLevelItemCount()); i++)
	{
		if (auto item = dynamic_cast<VariationTreeWidgetItem*>(tree->topLevelItem(i)))
		{
			auto varName = item->text(0);
			
			if (xform && xform->GetVariationById(item->Id()))//If it's present then show it no matter what the filter is.
			{
				item->setHidden(false);
			}
			else if (ids.contains(varName))//If the varation is the map of all variations, which is should always be, consider it as well as the filter text.
			{
				item->setHidden(!varName.contains(text, Qt::CaseInsensitive) || !ids[varName].toBool());
			}
			else//Wasn't present, which should never happen, so just consider filter text.
			{
				item->setHidden(!varName.contains(text, Qt::CaseInsensitive));
			}
		}
	}

	m_Fractorium->OnTreeHeaderSectionClicked(m_Fractorium->m_VarSortMode);//Must re-sort every time the filter changes.
	tree->setUpdatesEnabled(true);
}

void Fractorium::Filter()
{
	m_Controller->Filter(ui.VariationsFilterLineEdit->text());
}

/// <summary>
/// Dynamically populate the variation tree widget with VariationTreeWidgetItem and VariationTreeDoubleSpinBox
/// templated with the correct type.
/// This will clear any previous contents.
/// Called upon initialization, or controller type change.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::SetupVariationTree()
{
	T fMin = TLOW;
	T fMax = TMAX;
	QSize hint0(75, 16);
	QSize hint1(30, 16);
	auto tree = m_Fractorium->ui.VariationsTree;

	tree->clear();
	tree->blockSignals(true);

	for (size_t i = 0; i < m_VariationList.Size(); i++)
	{
		auto var = m_VariationList.GetVariation(i);
		auto parVar = dynamic_cast<const ParametricVariation<T>*>(var);

		//First add the variation, with a spinner for its weight.
		auto item = new VariationTreeWidgetItem(var->VariationId(), tree);
		auto spinBox = new VariationTreeDoubleSpinBox(tree, item, var->VariationId(), "");

		item->setText(0, QString::fromStdString(var->Name()));
		item->setSizeHint(0, hint0);
		item->setSizeHint(1, hint1);
		spinBox->setRange(fMin, fMax);
		spinBox->DoubleClick(true);
		spinBox->DoubleClickZero(1);
		spinBox->DoubleClickNonZero(0);
		spinBox->SmallStep(0.001);
		spinBox->setDecimals(4);
		tree->setItemWidget(item, 1, spinBox);
		m_Fractorium->connect(spinBox, SIGNAL(valueChanged(double)), SLOT(OnVariationSpinBoxValueChanged(double)), Qt::QueuedConnection);

		//Check to see if the variation was parametric, and add a tree entry with a spinner for each parameter.
		if (parVar)
		{
			auto params = parVar->Params();

			for (size_t j = 0; j < parVar->ParamCount(); j++)
			{
				if (!params[j].IsPrecalc())
				{
					auto paramWidget = new VariationTreeWidgetItem(var->VariationId(), item);
					auto varSpinBox = new VariationTreeDoubleSpinBox(tree, paramWidget, parVar->VariationId(), params[j].Name());

					paramWidget->setText(0, params[j].Name().c_str());
					paramWidget->setSizeHint(0, hint0);
					paramWidget->setSizeHint(1, hint1);
					varSpinBox->setRange(params[j].Min(), params[j].Max());
					varSpinBox->setValue(params[j].ParamVal());
					varSpinBox->DoubleClick(true);
					varSpinBox->DoubleClickZero(1);
					varSpinBox->DoubleClickNonZero(0);

					if (params[j].Type() == INTEGER || params[j].Type() == INTEGER_NONZERO)
					{
						varSpinBox->setSingleStep(1);
						varSpinBox->Step(1);
						varSpinBox->SmallStep(1);
					}

					varSpinBox->setDecimals(4);
					tree->setItemWidget(paramWidget, 1, varSpinBox);
					m_Fractorium->connect(varSpinBox, SIGNAL(valueChanged(double)), SLOT(OnVariationSpinBoxValueChanged(double)), Qt::QueuedConnection);
				}
			}
		}
	}

	Filter("");
	tree->blockSignals(false);
}

/// <summary>
/// Set every spinner in the variation tree, including params, to zero.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::ClearVariationsTree()
{
	QTreeWidget* tree = m_Fractorium->ui.VariationsTree;

	for (uint i = 0; i < tree->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* item = tree->topLevelItem(i);
		auto* spinBox = dynamic_cast<VariationTreeDoubleSpinBox*>(tree->itemWidget(item, 1));

		spinBox->SetValueStealth(0);

		for (uint j = 0; j < item->childCount(); j++)//Iterate through all of the children, which will be the params.
		{
			if ((spinBox = dynamic_cast<VariationTreeDoubleSpinBox*>(tree->itemWidget(item->child(j), 1))))//Cast the child widget to the VariationTreeDoubleSpinBox type.
				spinBox->SetValueStealth(0);
		}
	}
}

/// <summary>
/// Copy the value of a variation or param spinner to its corresponding value
/// in the currently selected xform.
/// Called when any spinner in the variations tree is changed.
/// Resets the rendering process.
/// </summary>
/// <param name="d">The spinner value</param>
template <typename T>
void FractoriumEmberController<T>::VariationSpinBoxValueChanged(double d)//Would be awesome to make this work for all.//TODO
{
	auto objSender = m_Fractorium->sender();
	auto tree = m_Fractorium->ui.VariationsTree;
	auto sender = dynamic_cast<VariationTreeDoubleSpinBox*>(objSender);
	auto xform = m_Ember.GetTotalXform(m_Fractorium->ui.CurrentXformCombo->currentIndex());//Will retrieve normal xform or final if needed.

	if (sender && xform)
	{
		auto var = m_VariationList.GetVariation(sender->GetVariationId());//The variation attached to the sender, for reference only.
		auto parVar = dynamic_cast<const ParametricVariation<T>*>(var);//The parametric cast of that variation.
		auto xformVar = xform->GetVariationById(var->VariationId());//The corresponding variation in the currently selected xform.
		auto widgetItem = sender->WidgetItem();
		bool isParam = parVar && sender->IsParam();

		if (isParam)
		{
			//Do not take action if the xform doesn't contain the variation which this param is part of.
			if (ParametricVariation<T>* xformParVar = dynamic_cast<ParametricVariation<T>*>(xformVar))//The parametric cast of the xform's variation.
			{
				if (xformParVar->SetParamVal(sender->ParamName().c_str(), d))
				{
					UpdateRender();
				}
			}
		}
		else
		{
			//If they spun down to zero, and it wasn't a parameter item,
			//and the current xform contained the variation, then remove the variation.
			if (IsNearZero(d))
			{
				if (xformVar)
					xform->DeleteVariationById(var->VariationId());

				widgetItem->setBackgroundColor(0, QColor(255, 255, 255));//Ensure background is always white if weight goes to zero.
			}
			else
			{
				if (xformVar)//The xform already contained this variation, which means they just went from a non-zero weight to another non-zero weight (the simple case).
				{
					xformVar->m_Weight = d;
				}
				else
				{
					//If the item wasn't a param and the xform did not contain this variation,
					//it means they went from zero to a non-zero weight, so add a new copy of this xform.
					auto newVar = var->Copy();//Create a new one with default values.

					newVar->m_Weight = d;
					xform->AddVariation(newVar);
					widgetItem->setBackgroundColor(0, QColor(200, 200, 200));//Set background to gray when a variation has non-zero weight in this xform.

					//If they've added a new parametric variation, then grab the values currently in the spinners
					//for the child parameters and assign them to the newly added variation.
					if (parVar)
					{
						auto newParVar = dynamic_cast<ParametricVariation<T>*>(newVar);
					
						for (int i = 0; i < widgetItem->childCount(); i++)//Iterate through all of the children, which will be the params.
						{
							auto childItem = widgetItem->child(i);//Get the child.
							auto itemWidget = tree->itemWidget(childItem, 1);//Get the widget for the child.

							if (auto spinBox = dynamic_cast<VariationTreeDoubleSpinBox*>(itemWidget))//Cast the widget to the VariationTreeDoubleSpinBox type.
							{
								string s = childItem->text(0).toStdString();//Use the name of the child, and the value of the spinner widget to assign the param.

								newParVar->SetParamVal(s.c_str(), spinBox->value());
							}
						}
					}
				}
			}

			UpdateRender();
		}
	}
}

void Fractorium::OnVariationSpinBoxValueChanged(double d) { m_Controller->VariationSpinBoxValueChanged(d); }

/// <summary>
/// Fill the variation tree values from passed in xform and apply the current sorting mode.
/// Called when the currently selected xform changes.
/// </summary>
/// <param name="xform">The xform whose variation values will be used to fill the tree</param>
template <typename T>
void FractoriumEmberController<T>::FillVariationTreeWithXform(Xform<T>* xform)
{
	auto tree = m_Fractorium->ui.VariationsTree;

	tree->blockSignals(true);
	m_Fractorium->Filter();

	for (uint i = 0; i < tree->topLevelItemCount(); i++)
	{
		auto item = dynamic_cast<VariationTreeWidgetItem*>(tree->topLevelItem(i));
		auto var = xform->GetVariationById(item->Id());//See if this variation in the tree was contained in the xform.
		auto parVar = dynamic_cast<ParametricVariation<T>*>(var);//Attempt cast to parametric variation for later.
		auto origParVar = dynamic_cast<const ParametricVariation<T>*>(m_VariationList.GetVariation(item->Id()));

		if (auto spinBox = dynamic_cast<VariationTreeDoubleSpinBox*>(tree->itemWidget(item, 1)))//Get the widget for the item, and cast the widget to the VariationTreeDoubleSpinBox type.
		{
			if (var)//Ensure it's visible, even if it's supposed to be filtered.
				item->setHidden(false);

			spinBox->SetValueStealth(var ? var->m_Weight : 0);//If the variation was present, set the spin box to its weight, else zero.
			item->setBackgroundColor(0, var ? QColor(200, 200, 200) : QColor(255, 255, 255));//Ensure background is always white if the value goes to zero, else gray if var present.

			for (uint j = 0; j < item->childCount(); j++)//Iterate through all of the children, which will be the params if it was a parametric variation.
			{
				T* param = nullptr;
				auto childItem = item->child(j);//Get the child.
				auto childItemWidget = tree->itemWidget(childItem, 1);//Get the widget for the child.

				if (auto childSpinBox = dynamic_cast<VariationTreeDoubleSpinBox*>(childItemWidget))//Cast the widget to the VariationTreeDoubleSpinBox type.
				{
					string s = childItem->text(0).toStdString();//Get the name of the child.

					if (parVar)
					{
						if ((param = parVar->GetParam(s.c_str())))//Retrieve pointer to the param.
							childSpinBox->SetValueStealth(*param);
					}
					else if (origParVar)//Parametric variation was not present in this xform, so set child values to defaults.
					{
						if ((param = origParVar->GetParam(s.c_str())))
							childSpinBox->SetValueStealth(*param);
						else
							childSpinBox->SetValueStealth(0);//Will most likely never happen, but just to be safe.
					}
				}
			}
		}
	}

	tree->blockSignals(false);
	m_Fractorium->OnTreeHeaderSectionClicked(m_Fractorium->m_VarSortMode);
}

/// <summary>
/// Change the sorting to be either by variation ID, or by weight.
/// If sorting by variation ID, repeated clicks will alternate ascending or descending.
/// Called when user clicks the tree headers.
/// </summary>
/// <param name="logicalIndex">Column index of the header clicked. Sort by name if 0, sort by weight if 1.</param>
void Fractorium::OnTreeHeaderSectionClicked(int logicalIndex)
{
	m_VarSortMode = logicalIndex;
	ui.VariationsTree->sortItems(m_VarSortMode, m_VarSortMode == 0 ? Qt::AscendingOrder : Qt::DescendingOrder);

	if (m_VarSortMode == 1)
		ui.VariationsTree->scrollToTop();
}

/// <summary>
/// Apply the text in the variation filter text box to only show variations whose names
/// contain the substring.
/// Called when the user types in the variation filter text box.
/// </summary>
/// <param name="text">The text to filter on</param>
void Fractorium::OnVariationsFilterLineEditTextChanged(const QString& text)
{
	Filter();
}

/// <summary>
/// Clear the variation name filter, which will display all variations.
/// Called when clear variations filter button is clicked.
/// </summary>
/// <param name="checked">Ignored</param>
void Fractorium::OnVariationsFilterClearButtonClicked(bool checked)
{
	ui.VariationsFilterLineEdit->clear();
}

template class FractoriumEmberController<float>;

#ifdef DO_DOUBLE
	template class FractoriumEmberController<double>;
#endif
