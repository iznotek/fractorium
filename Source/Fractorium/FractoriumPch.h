#define GL_GLEXT_PROTOTYPES 1
#define XFORM_COLOR_COUNT 14

#undef QT_OPENGL_ES_2//Make absolutely sure OpenGL ES is not used.
#define QT_NO_OPENGL_ES_2

#ifndef WIN32
#include <QtWidgets>
#endif

#include "Renderer.h"
#include "RendererCL.h"
#include "VariationList.h"
#include "OpenCLWrapper.h"
#include "XmlToEmber.h"
#include "EmberToXml.h"
#include "SheepTools.h"
#include "JpegUtils.h"
#include "EmberCommon.h"

#ifdef WIN32
#include <QtWidgets>
#endif

#include <deque>
#include "qfunctions.h"
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFont>
#include <QFontDialog>
#include <QFontMetrics>
#include <QFuture>
#include <QGraphicsView>
#include <QIcon>
#include <QImageReader>
#include <QItemDelegate>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndex>
#include <qopenglfunctions_2_0.h>
#include <QOpenGLWidget>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QWheelEvent>
#include <QtConcurrentRun>
#include <QtCore/QMultiHash>
#include <QtCore/QPair>
#include <QtCore/QSharedData>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/qglobal.h>
#include <QtGui/QFont>
#include <QtGui/QPalette>
#include <QtGui/QSyntaxHighlighter>
#include <QtWidgets/QMainWindow>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#ifndef WIN32
    #undef Bool
#endif

using namespace std;
using namespace EmberNs;
using namespace EmberCLns;
