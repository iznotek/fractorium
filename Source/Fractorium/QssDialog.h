/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once

#include "Fractorium.h"
#include "FractoriumCommon.h"
#include "QssTextEdit.h"
#include "qcssparser.h"

/// <summary>
/// The code in this file did not originate in Fractorium.
/// It was taken either in whole or in part from the source code
/// of Qt Creator. Their license applies.
/// </summary>

namespace Ui
{
	class QssDialog;
}

/// <summary>
/// A dialog for editing the stylesheet used in the application.
/// This is meant to be used in the following way:
///		On first run, no stylesheet is present/selected, so a basic style
///		is used on startup. This style differs slightly between Windows and Linux. See BaseStyle() for details.
///		If the user clicks Save as default or ok to exit the dialog, the text of this stylesheet will
///		be saved to the application settings folder in the file default.qss.
///		On all subsequent runs, the main window will detect the presence of default.qss and load it.
///		The user can load a different stylesheet from disk, such as dark.qss which comes with the installation.
///		They can save this back to disk (under a different name because dark.qss is made read only by the installer),
///		however it will not become the default until they explicitly click the Save as default button or click ok.
///		The other buttons Basic, Medium and Advanced produce an empty style sheet that gives access to various controls.
///			Basic: Just the base style.
///			Medium: Basic + every type of control in the application.
///			Advanced: Medium + the name of every individual control in the application. It is not intended that the user fill
///				out a custom style for every single control. Rather, it's to make them aware of the names of the controls in the
///				event they want to set some custom styling for a specific control.
///		For all practical purposes, the user will probably start with dark.qss, edit what they need and save to a new stylesheet,
//		then set that one as the default.
/// </summary>
class QssDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QssDialog(Fractorium* parent);
	~QssDialog();

	QString Text() const;
	void SetText(const QString& t);
	QList<QString> GetClassNames(bool includeObjectNames);
	static bool IsStyleSheetValid(const QString& styleSheet);

public slots:
	virtual void accept() override;
	virtual void reject() override;

protected:
	virtual void showEvent(QShowEvent* e) override;

private slots:
	void SlotTextChanged();
	void SlotAddColor(const QString& p);
	void SlotAddGeom(const QString& p);
	void SlotAddBorder(const QString& p);
	void SlotSetTheme(const QString& s);
	void SlotAddFont();
	void SlotApplyCss();

	void LoadButton_clicked();
	void SaveButton_clicked();
	void BasicButton_clicked();
	void MediumButton_clicked();
	void AdvancedButton_clicked();

private:
	void SaveAsDefault();
	void InsertCssProperty(const QString &name, const QString &value);
	void SetupFileDialog();
	QString OpenFile();
	QString SaveFile();

	QStyle* m_Theme;
	QStyle* m_LastTheme;
	QString m_LastStyle;
	QAction* m_AddColorAction;
	QAction* m_AddGeomAction;
	QAction* m_AddBorderAction;
	QAction* m_AddFontAction;
	QAction* m_AddStyleAction;
	QSignalMapper* m_ColorActionMapper;
	QSignalMapper* m_GeomActionMapper;
	QSignalMapper* m_BorderActionMapper;
	QSignalMapper* m_StyleActionMapper;
	QHash<QString, QString> m_ColorMap;
	QHash<QString, QString> m_GeomMap;
	QHash<QString, QString> m_BorderMap;
	QHash<QString, QString> m_StyleMap;
	QTimer* m_ApplyTimer;
	Fractorium* m_Parent;
	QFileDialog* m_FileDialog;
	Ui::QssDialog *ui;
};
