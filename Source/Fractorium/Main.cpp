#include "FractoriumPch.h"
#include "Fractorium.h"
#include <QtWidgets/QApplication>

/// <summary>
/// Main program entry point for Fractorium.exe.
/// </summary>
/// <param name="argc">The number of command line arguments passed</param>
/// <param name="argv">The command line arguments passed</param>
/// <returns>0 if successful, else 1.</returns>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

#ifdef TEST_CL
	QMessageBox::critical(QApplication::desktop(), "Error", "Fractorium cannot be run in test mode, undefine TEST_CL first.");
	return 1;
#endif

#ifdef ISAAC_FLAM3_DEBUG
	QMessageBox::critical(QApplication::desktop(), "Error", "Fractorium cannot be run in test mode, undefine ISAAC_FLAM3_DEBUG first.");
	return 1;
#endif

	//Required for large allocs, else GPU memory usage will be severely limited to small sizes.
	//This must be done in the application and not in the EmberCL DLL.
#ifdef WIN32
	_putenv_s("GPU_MAX_ALLOC_PERCENT", "100");
#else
	putenv(const_cast<char*>("GPU_MAX_ALLOC_PERCENT=100"));
#endif
	
#ifndef WIN32
	a.setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 3px; margin-top: 1.1em; background-color: transparent; } \n"
	"QTabBar::tab { height: 2.8ex; } \n"
	"QGroupBox::title "
	"{"
	 "  background-color: transparent;"
	 "  subcontrol-origin: margin; "
	 //"  left: 3px; "
	 "  subcontrol-position: top left;"
	 "  padding: 0 3px 0 3px;"
	 //"    padding: 2px;"
	 "} \n"
	 "QComboBox { margin-top: 0px; padding-bottom: 0px; }"
	 );
#endif

	int rv = -1;

	try
	{
		//a.setStyle(QStyleFactory::create("Fusion"));
		//QPalette darkPalette;
		/*darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
		darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

		darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);;*/

		//darkPalette.setColor(QPalette::, Qt::lightGray);
		//darkPalette.setColor(QPalette::Window, Qt::darkGray);
		//darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::red);
		//darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::blue);//Works for disabled buttons, but not for disabled menus.

		//a.setPalette(darkPalette);
		//a.setStyleSheet("QToolTip { color: #ffffff; background-color: darkgray; border: 1px solid white; }");
		//a.setStyleSheet("QTableWidget { border-color: darkgray; }")
		//QString s;

		//s = "QTableView, QSpinBox, QDoubleSpinBox, QGroupBox, QTreeWidget { background-color: darkGray; } ";
		//s += "QComboBox, QTextEdit, QLineEdit { background - color: lightGray; } ";
		//s += "QTabWidget { window-color: darkGray; } ";
		//a.setStyleSheet("{ color: rgb(85, 170, 0); }");
		//a.setStyleSheet("GLWidget { background-color: darkgray; }");
		//a.setStyleSheet("QTableView, QDoubleSpinBox { background-color: darkgray; }");//Works!
		//a.setStyleSheet(s);//Works!
		//a.setStyleSheet("QTableView, QSpinBox, QDoubleSpinBox, QTreeWidget, QTreeWidgetItem { background-color: darkgray; }");//QTreeWidgetItem not needed.
		//a.setStyleSheet("QTableView, DoubleSpinBox { background-color: darkgray; }");//Works!

		Fractorium w;
		w.show();
		a.installEventFilter(&w);
		rv = a.exec();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(0, "Fatal Error", QString::fromStdString(e.what()));
	}
	catch (const char* e)
	{
		QMessageBox::critical(0, "Fatal Error", e);
	}

	return rv;
}

