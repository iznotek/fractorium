#include "FractoriumPch.h"
#include "QssTextEdit.h"

QssTextEdit::QssTextEdit(QWidget* parent) :
    QTextEdit(parent)
{
    setTabStopWidth(fontMetrics().width(QLatin1Char(' '))*4);
    setAcceptRichText(false);
    new CssHighlighter(document());
}
