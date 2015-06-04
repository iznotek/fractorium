#pragma once

#include "FractoriumPch.h"

/// <summary>
/// PaletteTableWidgetItem class.
/// </summary>

/// <summary>
/// A thin derivation of QTableWidgetItem which keeps a pointer to a palette object.
/// The lifetime of the palette object must be greater than or equal to
/// the lifetime of this object.
/// </summary>
class PaletteTableWidgetItemBase : public QTableWidgetItem
{
public:
	PaletteTableWidgetItemBase()
	{
	}

	virtual size_t Index() const { return 0; }
};

template <typename T>
class PaletteTableWidgetItem : public PaletteTableWidgetItemBase
{
public:
	PaletteTableWidgetItem(Palette<T>* palette)
		: m_Palette(palette)
	{
	}

	virtual size_t Index() const override { return m_Palette->m_Index; }
	Palette<T>* GetPalette() const { return m_Palette; }

private:
	Palette<T>* m_Palette;
};