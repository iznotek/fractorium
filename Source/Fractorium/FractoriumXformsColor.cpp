#include "FractoriumPch.h"
#include "Fractorium.h"

/// <summary>
/// Initialize the xforms color UI.
/// </summary>
void Fractorium::InitXformsColorUI()
{
	int spinHeight = 20, row = 0;
	m_XformColorValueItem = new QTableWidgetItem();
	ui.XformColorIndexTable->setItem(0, 0, m_XformColorValueItem);
	m_PaletteRefItem = new QTableWidgetItem();
	ui.XformPaletteRefTable->setItem(0, 0, m_PaletteRefItem);
	ui.XformPaletteRefTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	connect(ui.XformPaletteRefTable->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), this, SLOT(OnXformRefPaletteResized(int, int, int)), Qt::QueuedConnection);
	SetupSpinner<DoubleSpinBox, double>(ui.XformColorIndexTable,  this, row, 1, m_XformColorIndexSpin,  spinHeight,  0, 1, 0.01, SIGNAL(valueChanged(double)), SLOT(OnXformColorIndexChanged(double)),  false,   0,   1,   0);
	SetupSpinner<DoubleSpinBox, double>(ui.XformColorValuesTable, this, row, 1, m_XformColorSpeedSpin,  spinHeight, -1, 1,  0.1, SIGNAL(valueChanged(double)), SLOT(OnXformColorSpeedChanged(double)),  true,  0.5, 0.5, 0.5);
	SetupSpinner<DoubleSpinBox, double>(ui.XformColorValuesTable, this, row, 1, m_XformOpacitySpin,	    spinHeight,  0, 1,  0.1, SIGNAL(valueChanged(double)), SLOT(OnXformOpacityChanged(double)),	    true,    1,   1,   0);
	SetupSpinner<DoubleSpinBox, double>(ui.XformColorValuesTable, this, row, 1, m_XformDirectColorSpin, spinHeight,  0, 1,  0.1, SIGNAL(valueChanged(double)), SLOT(OnXformDirectColorChanged(double)),	true,	 1,   1,   0);
	m_XformColorIndexSpin->setDecimals(3);
	m_XformColorSpeedSpin->setDecimals(3);
	m_XformOpacitySpin->setDecimals(3);
	m_XformDirectColorSpin->setDecimals(3);
	connect(ui.XformColorScroll,  SIGNAL(valueChanged(int)),							this, SLOT(OnXformScrollColorIndexChanged(int)),			Qt::QueuedConnection);
	connect(ui.SoloXformCheckBox, SIGNAL(stateChanged(int)),							this, SLOT(OnSoloXformCheckBoxStateChanged(int)),			Qt::QueuedConnection);
	connect(ui.ResetCurvesButton, SIGNAL(clicked(bool)),								this, SLOT(OnResetCurvesButtonClicked(bool)),				Qt::QueuedConnection);
	connect(ui.CurvesView,		  SIGNAL(PointChangedSignal(int, int, const QPointF&)), this, SLOT(OnCurvesPointChanged(int, int, const QPointF&)), Qt::QueuedConnection);
	connect(ui.CurvesAllRadio,    SIGNAL(toggled(bool)),								this, SLOT(OnCurvesAllRadioButtonToggled(bool)),			Qt::QueuedConnection);
	connect(ui.CurvesRedRadio,    SIGNAL(toggled(bool)),								this, SLOT(OnCurvesRedRadioButtonToggled(bool)),			Qt::QueuedConnection);
	connect(ui.CurvesGreenRadio,  SIGNAL(toggled(bool)),								this, SLOT(OnCurvesGreenRadioButtonToggled(bool)),		    Qt::QueuedConnection);
	connect(ui.CurvesBlueRadio,   SIGNAL(toggled(bool)),								this, SLOT(OnCurvesBlueRadioButtonToggled(bool)),		    Qt::QueuedConnection);
}

/// <summary>
/// Set the color index of the selected xforms.
/// Update the color index scrollbar to match.
/// Called when spinner in the color index cell in the palette ref table is changed.
/// Optionally resets the rendering process.
/// </summary>
/// <param name="d">The color index, 0-1/</param>
/// <param name="updateRender">True to reset the rendering process, else don't.</param>
template <typename T>
void FractoriumEmberController<T>::XformColorIndexChanged(double d, bool updateRender)
{
	auto scroll = m_Fractorium->ui.XformColorScroll;
	int scrollVal = d * scroll->maximum();
	scroll->blockSignals(true);
	scroll->setValue(scrollVal);
	scroll->blockSignals(false);
	SetCurrentXformColorIndex(d, updateRender);
}

void Fractorium::OnXformColorIndexChanged(double d) { OnXformColorIndexChanged(d, true); }
void Fractorium::OnXformColorIndexChanged(double d, bool updateRender) { m_Controller->XformColorIndexChanged(d, updateRender); }

/// <summary>
/// Set the color index of the current xform.
/// Will trigger an update which will cause the color index cell in the palette ref table to match.
/// Called when color index scrollbar is changed.
/// Resets the rendering process.
/// </summary>
/// <param name="d">The color index, 0-1.</param>
template <typename T>
void FractoriumEmberController<T>::XformScrollColorIndexChanged(int d)
{
	m_Fractorium->m_XformColorIndexSpin->setValue(d / double(m_Fractorium->ui.XformColorScroll->maximum()));//Will trigger an update.
}

void Fractorium::OnXformScrollColorIndexChanged(int d) { m_Controller->XformScrollColorIndexChanged(d); }

/// <summary>
/// Set the color speed of the selected xforms.
/// Called when xform color speed spinner is changed.
/// Resets the rendering process.
/// </summary>
/// <param name="d">The color speed, -1-1.</param>
template <typename T>
void FractoriumEmberController<T>::XformColorSpeedChanged(double d) { UpdateXform([&] (Xform<T>* xform) { xform->m_ColorSpeed = d; }, eXformUpdate::UPDATE_SELECTED); }
void Fractorium::OnXformColorSpeedChanged(double d) { m_Controller->XformColorSpeedChanged(d); }

/// <summary>
/// Set the opacity of the selected xforms.
/// Called when xform opacity spinner is changed.
/// Resets the rendering process.
/// </summary>
/// <param name="d">The opacity, 0-1.</param>
template <typename T>
void FractoriumEmberController<T>::XformOpacityChanged(double d) { UpdateXform([&] (Xform<T>* xform) { xform->m_Opacity = d; }, eXformUpdate::UPDATE_SELECTED); }
void Fractorium::OnXformOpacityChanged(double d) { m_Controller->XformOpacityChanged(d); }

/// <summary>
/// Set the direct color percentage of the selected xforms.
/// Called when xform direct color spinner is changed.
/// Note this only affects xforms that include a dc_ variation.
/// Resets the rendering process.
/// </summary>
/// <param name="d">The direct color percentage, 0-1.</param>
template <typename T>
void FractoriumEmberController<T>::XformDirectColorChanged(double d) { UpdateXform([&] (Xform<T>* xform) { xform->m_DirectColor = d; }, eXformUpdate::UPDATE_SELECTED); }
void Fractorium::OnXformDirectColorChanged(double d) { m_Controller->XformDirectColorChanged(d); }

/// <summary>
/// Set whether the current xform should be rendered solo.
/// If checked, current is solo, if unchecked, none are solo.
/// Solo means that all other xforms will have their opacity temporarily
/// set to zero while rendering so that only the effect of current xform is visible.
/// This will not permanently alter the ember, as the temporary opacity values will be applied
/// right before rendering and reset right after.
/// Called when solo xform check box is checked.
/// Resets the rendering process.
/// </summary>
/// <param name="state">The state of the checkbox</param>
void Fractorium::OnSoloXformCheckBoxStateChanged(int state)
{
	if (state == Qt::Checked)
	{
		ui.CurrentXformCombo->setProperty("soloxform", ui.CurrentXformCombo->currentIndex());
		ui.SoloXformCheckBox->setText("Solo (" + ToString(ui.CurrentXformCombo->currentIndex() + 1) + ")");
	}
	else if (state == Qt::Unchecked)
	{
		ui.CurrentXformCombo->setProperty("soloxform", -1);
		ui.SoloXformCheckBox->setText("Solo");
	}

	m_Controller->UpdateRender();
}

/// <summary>
/// Redraw the palette ref table.
/// Called on resize.
/// </summary>
/// <param name="logicalIndex">Ignored</param>
/// <param name="oldSize">Ignored</param>
/// <param name="newSize">Ignored</param>
void Fractorium::OnXformRefPaletteResized(int logicalIndex, int oldSize, int newSize)
{
	QPixmap pixmap = QPixmap::fromImage(m_Controller->FinalPaletteImage());
	SetPaletteTableItem(&pixmap, ui.XformPaletteRefTable, m_PaletteRefItem, 0, 0);
}

/// <summary>
/// Reset the color curve values in the current ember to their default state and also update the curves control.
/// Called when ResetCurvesButton is clicked.
/// Resets the rendering process at either ACCUM_ONLY by default, or FILTER_AND_ACCUM when using early clip.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::ClearColorCurves()
{
	Update([&]
	{
		m_Ember.m_Curves.Init();
	}, true, m_Renderer->EarlyClip() ? eProcessAction::FILTER_AND_ACCUM : eProcessAction::ACCUM_ONLY);
	FillCurvesControl();
}

void Fractorium::OnResetCurvesButtonClicked(bool checked) { m_Controller->ClearColorCurves(); }

/// <summary>
/// Set the coordinate of the curve point.
/// Called when the position of any of the points in the curves editor is is changed.
/// Resets the rendering process at either ACCUM_ONLY by default, or FILTER_AND_ACCUM when using early clip.
/// </summary>
/// <param name="curveIndex">The curve index, 0-1/</param>
/// <param name="pointIndex">The point index within the selected curve, 1-2.</param>
/// <param name="point">The new coordinate of the point in terms of the curves control rect.</param>
template <typename T>
void FractoriumEmberController<T>::ColorCurveChanged(int curveIndex, int pointIndex, const QPointF& point)
{
	Update([&]
	{
		m_Ember.m_Curves.m_Points[curveIndex][pointIndex].x = point.x();
		m_Ember.m_Curves.m_Points[curveIndex][pointIndex].y = point.y();
	}, true, m_Renderer->EarlyClip() ? eProcessAction::FILTER_AND_ACCUM : eProcessAction::ACCUM_ONLY);
}

void Fractorium::OnCurvesPointChanged(int curveIndex, int pointIndex, const QPointF& point) { m_Controller->ColorCurveChanged(curveIndex, pointIndex, point); }

/// <summary>
/// Set the top most points in the curves control, which makes it easier to
/// select a point by putting it on top of all the others.
/// Called when the any of the curve color radio buttons are toggled.
/// </summary>
/// <param name="curveIndex">The curve index, 0-1/</param>
void Fractorium::OnCurvesAllRadioButtonToggled(bool checked)   { if (checked) ui.CurvesView->SetTop(CurveIndex::ALL);   }
void Fractorium::OnCurvesRedRadioButtonToggled(bool checked)   { if (checked) ui.CurvesView->SetTop(CurveIndex::RED);   }
void Fractorium::OnCurvesGreenRadioButtonToggled(bool checked) { if (checked) ui.CurvesView->SetTop(CurveIndex::GREEN); }
void Fractorium::OnCurvesBlueRadioButtonToggled(bool checked)  { if (checked) ui.CurvesView->SetTop(CurveIndex::BLUE);  }

/// <summary>
/// Look up the passed in index in the current ember's palette
/// and return the QColor equivalent.
/// </summary>
/// <param name="d">The palette index to look up, 0-1.</param>
/// <returns>The palette color at the given index as a QColor</returns>
template <typename T>
QColor FractoriumEmberController<T>::ColorIndexToQColor(double d)
{
	v4T entry = m_Ember.m_Palette[Clamp<int>(d * COLORMAP_LENGTH_MINUS_1, 0, m_Ember.m_Palette.Size())];
	entry.r *= 255;
	entry.g *= 255;
	entry.b *= 255;
	QRgb rgb = uint(entry.r) << 16 | uint(entry.g) << 8 | uint(entry.b);
	return QColor::fromRgb(rgb);
}

/// <summary>
/// Set the selected xforms color index to the passed in value.
/// Set the color cell in the palette ref table.
/// </summary>
/// <param name="d">The index value to set, 0-1.</param>
template <typename T>
void FractoriumEmberController<T>::SetCurrentXformColorIndex(double d, bool updateRender)
{
	UpdateXform([&] (Xform<T>* xform)
	{
		xform->m_ColorX = Clamp<T>(d, 0, 1);
		//Grab the current color from the index and assign it to the first cell of the first table.
		m_Fractorium->ui.XformColorIndexTable->item(0, 0)->setBackgroundColor(ColorIndexToQColor(xform->m_ColorX)/*QColor::fromRgb(rgb)*/);
	}, eXformUpdate::UPDATE_SELECTED, updateRender);
}

/// <summary>
/// Set the points in the curves control to the values of the curve points in the current ember.
/// </summary>
template <typename T>
void FractoriumEmberController<T>::FillCurvesControl()
{
	m_Fractorium->ui.CurvesView->blockSignals(true);

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 1; j < 3; j++)//Only do middle points.
		{
			QPointF point(m_Ember.m_Curves.m_Points[i][j].x, m_Ember.m_Curves.m_Points[i][j].y);
			m_Fractorium->ui.CurvesView->Set(i, j, point);
		}
	}

	m_Fractorium->ui.CurvesView->blockSignals(false);
	m_Fractorium->ui.CurvesView->update();
}

/// <summary>
/// Set the color index, speed and opacity spinners with the values of the passed in xform.
/// Set the cells of the palette ref table as well.
/// </summary>
/// <param name="xform">The xform whose values will be copied to the GUI</param>
template <typename T>
void FractoriumEmberController<T>::FillColorWithXform(Xform<T>* xform)
{
	m_Fractorium->m_XformColorIndexSpin->SetValueStealth(xform->m_ColorX);
	m_Fractorium->m_XformColorSpeedSpin->SetValueStealth(xform->m_ColorSpeed);
	m_Fractorium->m_XformOpacitySpin->SetValueStealth(xform->m_Opacity);
	m_Fractorium->m_XformDirectColorSpin->SetValueStealth(xform->m_DirectColor);
	FillCurvesControl();
	m_Fractorium->OnXformColorIndexChanged(xform->m_ColorX, false);//Had to call stealth before to avoid doing an update, now manually update related controls, still without doing an update.
}

/// <summary>
/// Set the cell at the row and column in the passed in table to the passed in pixmap.
/// </summary>
/// <param name="pixmap">The pixmap to assign</param>
/// <param name="table">The table whose cell will be filled with the image</param>
/// <param name="item">The QTableWidgetItem in the cell</param>
/// <param name="row">The row of the cell</param>
/// <param name="col">The column of the cell</param>
void Fractorium::SetPaletteTableItem(QPixmap* pixmap, QTableWidget* table, QTableWidgetItem* item, int row, int col)
{
	if (pixmap && !pixmap->isNull())
	{
		QSize size(table->columnWidth(col), table->rowHeight(row) + 1);
		item->setData(Qt::DecorationRole, pixmap->scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	}
}

template class FractoriumEmberController<float>;

#ifdef DO_DOUBLE
template class FractoriumEmberController<double>;
#endif
