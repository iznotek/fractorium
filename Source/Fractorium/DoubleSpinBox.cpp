#include "FractoriumPch.h"
#include "DoubleSpinBox.h"

QTimer DoubleSpinBox::m_Timer;

/// <summary>
/// Constructor that passes parent to the base and sets up height and step.
/// Specific focus policy is used to allow the user to hover over the control
/// and change its value using the mouse wheel without explicitly having to click
/// inside of it.
/// </summary>
/// <param name="p">The parent widget. Default: nullptr.</param>
/// <param name="height">The height of the spin box. Default: 16.</param>
/// <param name="step">The step used to increment/decrement the spin box when using the mouse wheel. Default: 0.05.</param>
DoubleSpinBox::DoubleSpinBox(QWidget* p, int h, double step)
	: QDoubleSpinBox(p)
{
	m_Select = false;
	m_DoubleClick = false;
	m_DoubleClickNonZero = 0;
	m_DoubleClickZero = 1;
	m_Step = step;
	m_SmallStep = step / 10.0;
	setSingleStep(step);
	setFrame(false);
	//setAttribute(Qt::WA_PaintOnScreen);
	setButtonSymbols(QAbstractSpinBox::NoButtons);
	setFocusPolicy(Qt::StrongFocus);
	setMinimumHeight(h);//setGeometry() has no effect, so must set both of these instead.
	setMaximumHeight(h);
	setContextMenuPolicy(Qt::PreventContextMenu);
	lineEdit()->installEventFilter(this);
	lineEdit()->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	connect(this, SIGNAL(valueChanged(double)), this, SLOT(OnSpinBoxValueChanged(double)), Qt::QueuedConnection);
}

/// <summary>
/// Set the value of the control without triggering signals.
/// </summary>
/// <param name="d">The value to set it to</param>
void DoubleSpinBox::SetValueStealth(double d)
{
	blockSignals(true);
	setValue(d);
	blockSignals(false);
}

/// <summary>
/// Set whether to respond to double click events.
/// </summary>
/// <param name="b">True if this should respond to double click events, else false.</param>
void DoubleSpinBox::DoubleClick(bool b)
{
	m_DoubleClick = b;
}

/// <summary>
/// Set the value to be used when the user double clicks the spinner while
/// it contains zero.
/// </summary>
/// <param name="val">The value to be used</param>
void DoubleSpinBox::DoubleClickZero(double val)
{
	m_DoubleClickZero = val;
}

/// <summary>
/// Set the value to be used when the user double clicks the spinner while
/// it contains a non-zero value.
/// </summary>
/// <param name="val">The value to be used</param>
void DoubleSpinBox::DoubleClickNonZero(double val)
{
	m_DoubleClickNonZero = val;
}

/// <summary>
/// Get the default step used when the user scrolls.
/// </summary>
double DoubleSpinBox::Step()
{
	return m_Step;
}

/// <summary>
/// Set the default step to be used when the user scrolls.
/// </summary>
/// <param name="step">The step to use for scrolling</param>
void DoubleSpinBox::Step(double step)
{
	m_Step = step;
}

/// <summary>
/// Get the small step to be used when the user holds down shift while scrolling.
/// </summary>
double DoubleSpinBox::SmallStep()
{
	return m_SmallStep;
}

/// <summary>
/// Set the small step to be used when the user holds down shift while scrolling.
/// The default is step / 10, so use this if something else is needed.
/// </summary>
/// <param name="step">The small step to use for scrolling while the shift key is down</param>
void DoubleSpinBox::SmallStep(double step)
{
	m_SmallStep = step;
}

/// <summary>
/// Expose the underlying QLineEdit control to the caller.
/// </summary>
/// <returns>A pointer to the QLineEdit</returns>
QLineEdit* DoubleSpinBox::lineEdit()
{
	return QDoubleSpinBox::lineEdit();
}

/// <summary>
/// Another workaround for the persistent text selection bug in Qt.
/// </summary>
void DoubleSpinBox::OnSpinBoxValueChanged(double d)
{
	lineEdit()->deselect();//Gets rid of nasty "feature" that always has text selected.
}

/// <summary>
/// Called while the timer is activated due to the right mouse button being held down.
/// </summary>
void DoubleSpinBox::OnTimeout()
{
	int xdistance = m_MouseMovePoint.x() - m_MouseDownPoint.x();
	int ydistance = m_MouseMovePoint.y() - m_MouseDownPoint.y();
	int distance = abs(xdistance) > abs(ydistance) ? xdistance : ydistance;
	double scale, val;
	double d = value();
	bool shift = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	//bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
	double amount = (m_SmallStep + m_Step) * 0.5;

	if (shift)
	{
		//qDebug() << "Shift pressed";
		scale = 0.0001;
	}
	/*else if (ctrl)
	{
		qDebug() << "Control pressed";
		scale = 0.01;
	}*/
	else
		scale = 0.001;

	val = d + (distance * amount * scale);
	setValue(val);

	//qDebug() << "Timer on, orig val: " << d << ", new val: " << val << ", distance " << distance;
}

/// <summary>
/// Event filter for taking special action on double click events.
/// </summary>
/// <param name="o">The object</param>
/// <param name="e">The eevent</param>
/// <returns>false</returns>
bool DoubleSpinBox::eventFilter(QObject* o, QEvent* e)
{
	QMouseEvent* me = dynamic_cast<QMouseEvent*>(e);

	if (isEnabled() &&
		me &&
		me->type() == QMouseEvent::MouseButtonPress &&
		me->button() == Qt::RightButton)
	{
		m_MouseDownPoint = m_MouseMovePoint = me->pos();
		StartTimer();
		//qDebug() << "Right mouse down";
	//	QPoint pt;
	//
	//	if (QMouseEvent* me = (QMouseEvent*)e)
	//		pt = me->localPos().toPoint();
	//
	//	int pos = lineEdit()->cursorPositionAt(pt);
	//
	//	if (lineEdit()->selectedText() != "")
	//	{
	//		lineEdit()->deselect();
	//		lineEdit()->setCursorPosition(pos);
	//		return true;
	//	}
	//	else if (m_Select)
	//	{
	//		lineEdit()->setCursorPosition(pos);
	//		selectAll();
	//		m_Select = false;
	//		return true;
	//	}
	}
	else if (isEnabled() &&
		me &&
		me->type() == QMouseEvent::MouseButtonRelease &&
		me->button() == Qt::RightButton)
	{
		StopTimer();
		m_MouseDownPoint = m_MouseMovePoint = me->pos();
		//qDebug() << "Right mouse up";
	}
	else if (isEnabled() &&
		me &&
		me->type() == QMouseEvent::MouseMove &&
		QGuiApplication::mouseButtons() & Qt::RightButton)
	{
		m_MouseMovePoint = me->pos();
		qDebug() << "Mouse move while right down. Pt = " << me->pos() << ", global: " << mapToGlobal(me->pos());
	}
	else if (m_DoubleClick && e->type() == QMouseEvent::MouseButtonDblClick && isEnabled())
	{
		if (IsNearZero(value()))
			setValue(m_DoubleClickZero);
		else
			setValue(m_DoubleClickNonZero);
	}
	else
	{
		if (e->type() == QEvent::Wheel)
		{
			//Take special action for shift to reduce the scroll amount. Control already
			//increases it automatically.
			if (QWheelEvent* we = dynamic_cast<QWheelEvent*>(e))
			{
				Qt::KeyboardModifiers mod = we->modifiers();

				if (mod.testFlag(Qt::ShiftModifier))
					setSingleStep(m_SmallStep);
				else
					setSingleStep(m_Step);
			}
		}
	}

	return QDoubleSpinBox::eventFilter(o, e);
}

/// <summary>
/// Called when focus enters the spinner.
/// </summary>
/// <param name="e">The event</param>
void DoubleSpinBox::focusInEvent(QFocusEvent* e)
{
	//lineEdit()->setReadOnly(false);
	StopTimer();
	QDoubleSpinBox::focusInEvent(e);
}

/// <summary>
/// Called when focus leaves the spinner.
/// Qt has a nasty "feature" that leaves the text in a spinner selected
/// and the cursor visible, regardless of whether it has the focus.
/// Manually clear both here.
/// </summary>
/// <param name="e">The event</param>
void DoubleSpinBox::focusOutEvent(QFocusEvent* e)
{
	 //lineEdit()->deselect();//Clear selection when leaving.
	 //lineEdit()->setReadOnly(true);//Clever hack to clear the cursor when leaving.
	StopTimer();
	QDoubleSpinBox::focusOutEvent(e);
}

/// <summary>
/// Called when focus enters the spinner.
/// Must set the focus to make sure key down messages don't erroneously go to the GLWidget.
/// </summary>
/// <param name="e">The event</param>
void DoubleSpinBox::enterEvent(QEvent* e)
{
	//m_Select = true;
	//setFocus();
	StopTimer();
	QDoubleSpinBox::enterEvent(e);
}

/// <summary>
/// Called when focus leaves the spinner.
/// Must clear the focus to make sure key down messages don't erroneously go to the GLWidget.
/// </summary>
/// <param name="e">The event</param>
void DoubleSpinBox::leaveEvent(QEvent* e)
{
	//m_Select = false;
	//clearFocus();.
	StopTimer();
	QDoubleSpinBox::leaveEvent(e);
}

/// <summary>
/// Start the timer in response to the right mouse button being pressed.
/// </summary>
void DoubleSpinBox::StartTimer()
{
	m_Timer.stop();
	connect(&m_Timer, SIGNAL(timeout()), this, SLOT(OnTimeout()));
	m_Timer.start(300);
}

/// <summary>
/// Stop the timer in response to the left mouse button being pressed.
/// </summary>
void DoubleSpinBox::StopTimer()
{
	m_Timer.stop();
	disconnect(&m_Timer, SIGNAL(timeout()), this, SLOT(OnTimeout()));
}