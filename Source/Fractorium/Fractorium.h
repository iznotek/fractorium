#pragma once

#include "ui_Fractorium.h"
#include "FractoriumCommon.h"
#include "GLWidget.h"
#include "EmberTreeWidgetItem.h"
#include "VariationTreeWidgetItem.h"
#include "StealthComboBox.h"
#include "TableWidget.h"
#include "FinalRenderDialog.h"
#include "OptionsDialog.h"
#include "VariationsDialog.h"
#include "AboutDialog.h"
#include "CurvesGraphicsView.h"
#include "DoubleSpinBoxTableItemDelegate.h"

/// <summary>
/// Fractorium class.
/// </summary>

class GLWidget;
class QssDialog;
class FractoriumOptionsDialog;
class FractoriumVariationsDialog;
class FractoriumFinalRenderDialog;
class FractoriumAboutDialog;
class GLEmberControllerBase;
class FractoriumEmberControllerBase;
class FinalRenderEmberControllerBase;
template <typename T> class GLEmberController;
template <typename T> class FractoriumEmberController;
template <typename T> class FinalRenderEmberController;

/// <summary>
/// Fractorium is the main window for the interactive renderer. The main viewable area
/// is a derivation of QGLWidget named GLWidget. The design uses the concept of a controller
/// to allow for both polymorphism and templating to coexist. Most processing functionality
/// is contained within the controller, and the GUI events just call the appropriate controller
/// member functions.
/// The rendering takes place on a timer with
/// a period of 0 which means it will trigger an event whenever the event input queue is idle.
/// As it's rendering, if the user changes anything on the GUI, a re-render will trigger. Since
/// certain parameters don't require a full render, the minimum necessary processing will be ran.
/// When the user changes something on the GUI, the required processing action is added to a vector.
/// Upon the next execution of the idle timer function, the most significant action will be extracted
/// and applied to the renderer. The state change vector is then cleared.
/// On the left side of the window is a dock widget which contains all controls needed for
/// manipulating embers. It's tabs can be floated, dragged, docked and nested elsewhere.
/// Qt takes very long to create file dialog windows, so they are kept as members and initialized
/// upon first use with lazy instantiation and then kept around for the remainder of the program.
/// Additional dialogs are for the about box, options, and final rendering out to a file.
/// While all class member variables and functions are declared in this .h file, the implementation
/// for them is far too lengthy to put in a single .cpp file. So general functionality is placed in
/// Fractorium.cpp and the other functional areas are each broken out into their own files.
/// The order of the functions in each .cpp file should roughly match the order they appear in the .h file.
/// Future todo list:
///		Implement more rendering types.
///		Add support for animation previewing.
///		Add support for full animation editing and rendering.
///		Possibly add some features from JWildFire.
/// </summary>
class Fractorium : public QMainWindow
{
	Q_OBJECT

	friend GLWidget;
	friend QssDialog;
	friend FractoriumOptionsDialog;
	friend FractoriumFinalRenderDialog;
	friend FractoriumAboutDialog;
	friend GLEmberControllerBase;
	friend GLEmberController<float>;
	friend FractoriumEmberControllerBase;
	friend FractoriumEmberController<float>;
	friend FinalRenderEmberControllerBase;
	friend FinalRenderEmberController<float>;

#ifdef DO_DOUBLE
	friend GLEmberController<double>;
	friend FractoriumEmberController<double>;
	friend FinalRenderEmberController<double>;
#endif

public:
	Fractorium(QWidget* p = 0);
	~Fractorium();

	//Geometry.
	void SetCenter(float x, float y);
	void SetRotation(double rot, bool stealth);
	void SetScale(double scale);
	void SetCoordinateStatus(int rasX, int rasY, float worldX, float worldY);
	void CenterScrollbars();

	//Xforms.
	void CurrentXform(uint i);

	//Xforms Affine.
	bool DrawAllPre();
	bool DrawAllPost();
	bool LocalPivot();

public slots:
	//Dock.
	void OnDockTopLevelChanged(bool topLevel);
	void dockLocationChanged(Qt::DockWidgetArea area);

	//Menu.
	void OnActionNewFlock(bool checked);//File.
	void OnActionNewEmptyFlameInCurrentFile(bool checked);
	void OnActionNewRandomFlameInCurrentFile(bool checked);
	void OnActionCopyFlameInCurrentFile(bool checked);
	void OnActionOpen(bool checked);
	void OnActionSaveCurrentAsXml(bool checked);
	void OnActionSaveEntireFileAsXml(bool checked);
	void OnActionSaveCurrentScreen(bool checked);
	void OnActionSaveCurrentToOpenedFile(bool checked);
	void OnActionExit(bool checked);

	void OnActionUndo(bool checked);//Edit.
	void OnActionRedo(bool checked);
	void OnActionCopyXml(bool checked);
	void OnActionCopyAllXml(bool checked);
	void OnActionPasteXmlAppend(bool checked);
	void OnActionPasteXmlOver(bool checked);
	void OnActionCopySelectedXforms(bool checked);
	void OnActionPasteSelectedXforms(bool checked);

	void OnActionResetWorkspace(bool checked);//View

	void OnActionAddReflectiveSymmetry(bool checked);//Tools.
	void OnActionAddRotationalSymmetry(bool checked);
	void OnActionAddBothSymmetry(bool checked);
	void OnActionFlatten(bool checked);
	void OnActionUnflatten(bool checked);
	void OnActionClearFlame(bool checked);
	void OnActionRenderPreviews(bool checked);
	void OnActionStopRenderingPreviews(bool checked);
	void OnActionFinalRender(bool checked);
	void OnFinalRenderClose(int result);
	void OnActionOptions(bool checked);
	void OnActionVariationsDialog(bool checked);

	void OnActionAbout(bool checked);//Help.

	//Toolbar.
	void OnActionCpu(bool checked);
	void OnActionCL(bool checked);
	void OnActionSP(bool checked);
	void OnActionDP(bool checked);
	void OnActionStyle(bool checked);

	//Library.
	void OnEmberTreeItemChanged(QTreeWidgetItem* item, int col);
	void OnEmberTreeItemDoubleClicked(QTreeWidgetItem* item, int col);
	void OnDelete(const pair<size_t, QTreeWidgetItem*>& p);

	//Params.
	void OnBrightnessChanged(double d);//Color.
	void OnGammaChanged(double d);
	void OnGammaThresholdChanged(double d);
	void OnVibrancyChanged(double d);
	void OnHighlightPowerChanged(double d);
	void OnBackgroundColorButtonClicked(bool checked);
	void OnColorSelected(const QColor& color);
	void OnPaletteModeComboCurrentIndexChanged(int index);
	void OnWidthChanged(int d);//Geometry.
	void OnHeightChanged(int d);
	void OnCenterXChanged(double d);
	void OnCenterYChanged(double d);
	void OnScaleChanged(double d);
	void OnZoomChanged(double d);
	void OnRotateChanged(double d);
	void OnZPosChanged(double d);
	void OnPerspectiveChanged(double d);
	void OnPitchChanged(double d);
	void OnYawChanged(double d);
	void OnDepthBlurChanged(double d);
	void OnSpatialFilterWidthChanged(double d);//Filter.
	void OnSpatialFilterTypeComboCurrentIndexChanged(const QString& text);
	void OnTemporalFilterWidthChanged(double d);
	void OnTemporalFilterTypeComboCurrentIndexChanged(const QString& text);
	void OnDEFilterMinRadiusWidthChanged(double d);
	void OnDEFilterMaxRadiusWidthChanged(double d);
	void OnDEFilterCurveWidthChanged(double d);
	void OnSbsChanged(int d);//Iteration.
	void OnFuseChanged(int d);
	void OnQualityChanged(double d);
	void OnSupersampleChanged(int d);
	void OnTemporalSamplesChanged(int d);
	void OnAffineInterpTypeComboCurrentIndexChanged(int index);
	void OnInterpTypeComboCurrentIndexChanged(int index);

	//Xforms.
	void OnCurrentXformComboChanged(int index);
	void OnAddXformButtonClicked(bool checked);
	void OnAddLinkedXformButtonClicked(bool checked);
	void OnDuplicateXformButtonClicked(bool checked);
	void OnClearXformButtonClicked(bool checked);
	void OnDeleteXformButtonClicked(bool checked);
	void OnAddFinalXformButtonClicked(bool checked);
	void OnXformWeightChanged(double d);
	void OnEqualWeightButtonClicked(bool checked);
	void OnXformNameChanged(int row, int col);

	//Xforms Affine.
	void OnPreAffineRowDoubleClicked(int logicalIndex);
	void OnPreAffineColDoubleClicked(int logicalIndex);
	void OnPostAffineRowDoubleClicked(int logicalIndex);
	void OnPostAffineColDoubleClicked(int logicalIndex);

	void OnX1Changed(double d);
	void OnX2Changed(double d);
	void OnY1Changed(double d);
	void OnY2Changed(double d);
	void OnO1Changed(double d);
	void OnO2Changed(double d);

	void OnFlipHorizontalButtonClicked(bool checked);
	void OnFlipVerticalButtonClicked(bool checked);
	void OnRotate90CButtonClicked(bool checked);
	void OnRotate90CcButtonClicked(bool checked);
	void OnRotateCButtonClicked(bool checked);
	void OnRotateCcButtonClicked(bool checked);
	void OnMoveUpButtonClicked(bool checked);
	void OnMoveDownButtonClicked(bool checked);
	void OnMoveLeftButtonClicked(bool checked);
	void OnMoveRightButtonClicked(bool checked);
	void OnScaleDownButtonClicked(bool checked);
	void OnScaleUpButtonClicked(bool checked);
	void OnResetAffineButtonClicked(bool checked);

	void OnAffineGroupBoxToggled(bool on);
	void OnAffineDrawAllCurrentRadioButtonToggled(bool checked);
	void OnPolarAffineCheckBoxStateChanged(int state);

	//Xforms Color.
	void OnXformColorIndexChanged(double d);
	void OnXformColorIndexChanged(double d, bool updateRender);
	void OnXformScrollColorIndexChanged(int d);
	void OnXformColorSpeedChanged(double d);
	void OnXformOpacityChanged(double d);
	void OnXformDirectColorChanged(double d);
	void OnSoloXformCheckBoxStateChanged(int state);
	void OnXformRefPaletteResized(int logicalIndex, int oldSize, int newSize);
	void OnResetCurvesButtonClicked(bool checked);
	void OnCurvesPointChanged(int curveIndex, int pointIndex, const QPointF& point);
	void OnCurvesAllRadioButtonToggled(bool checked);
	void OnCurvesRedRadioButtonToggled(bool checked);
	void OnCurvesGreenRadioButtonToggled(bool checked);
	void OnCurvesBlueRadioButtonToggled(bool checked);

	//Xforms Variations.
	void OnVariationSpinBoxValueChanged(double d);
	void OnTreeHeaderSectionClicked(int);
	void OnVariationsFilterLineEditTextChanged(const QString& text);
	void OnVariationsFilterClearButtonClicked(bool checked);

	//Xforms Selection.
	void OnXformsSelectAllButtonClicked(bool checked);
	void OnXformsSelectNoneButtonClicked(bool checked);

	//Xaos.
	void OnXaosChanged(double d);
	void OnClearXaosButtonClicked(bool checked);
	void OnRandomXaosButtonClicked(bool checked);
	void OnXaosRowDoubleClicked(int logicalIndex);
	void OnXaosColDoubleClicked(int logicalIndex);
	void OnXaosTableModelDataChanged(const QModelIndex& indexA, const QModelIndex& indexB);

	//Palette.
	void OnPaletteFilenameComboChanged(const QString& text);
	void OnPaletteAdjust(int d);
	void OnPaletteCellClicked(int row, int col);
	void OnPaletteCellDoubleClicked(int row, int col);
	void OnPaletteRandomSelectButtonClicked(bool checked);
	void OnPaletteRandomAdjustButtonClicked(bool checked);
	void OnPaletteFilterLineEditTextChanged(const QString& text);
	void OnPaletteFilterClearButtonClicked(bool checked);
	void OnPaletteHeaderSectionClicked(int col);

	//Info.
	void OnSummaryTableHeaderResized(int logicalIndex, int oldSize, int newSize);
	void OnSummaryTreeHeaderSectionClicked(int logicalIndex);

	//Rendering/progress.
	void StartRenderTimer();
	void IdleTimer();
	bool ControllersOk();
	void ShowCritical(const QString& title, const QString& text, bool invokeRequired = false);

	//Can't have a template function be a slot.
	void SetLibraryTreeItemData(EmberTreeWidgetItemBase* item, vector<byte>& v, uint w, uint h);

public:
	//template<typename spinType, typename valType>//See below.
	//static void SetupSpinner(QTableWidget* table, const QObject* receiver, int& row, int col, spinType*& spinBox, int height, valType min, valType max, valType step, const char* signal, const char* slot, bool incRow = true, valType val = 0, valType doubleClickZero = -999, valType doubleClickNonZero = -999);
	static void SetupAffineSpinner(QTableWidget* table, const QObject* receiver, int row, int col, DoubleSpinBox*& spinBox, int height, double min, double max, double step, double prec, const char* signal, const char* slot);
	static void SetupCombo(QTableWidget* table, const QObject* receiver, int& row, int col, StealthComboBox*& comboBox, const vector<string>& vals, const char* signal, const char* slot, Qt::ConnectionType connectionType = Qt::QueuedConnection);
	static void SetFixedTableHeader(QHeaderView* header, QHeaderView::ResizeMode mode = QHeaderView::Fixed);

protected:
	virtual bool eventFilter(QObject* o, QEvent* e) override;
	virtual void resizeEvent(QResizeEvent* e) override;
	virtual void closeEvent(QCloseEvent* e) override;
	virtual void dragEnterEvent(QDragEnterEvent* e) override;
	virtual void dragMoveEvent(QDragMoveEvent* e) override;
	virtual void dropEvent(QDropEvent* e) override;
	virtual void showEvent(QShowEvent* e) override;

private:
	void InitMenusUI();
	void InitToolbarUI();
	void InitParamsUI();
	void InitXformsUI();
	void InitXformsColorUI();
	void InitXformsAffineUI();
	void InitXformsVariationsUI();
	void InitXformsSelectUI();
	void InitXaosUI();
	void InitPaletteUI();
	void InitLibraryUI();
	void InitInfoUI();
	void SetTabOrders();

	void ToggleTableRow(QTableView* table, int logicalIndex);
	void ToggleTableCol(QTableView* table, int logicalIndex);

	//Embers.
	bool HaveFinal();

	//Toolbar.
	void SyncOptionsToToolbar();

	//Library.
	pair<size_t, QTreeWidgetItem*> GetCurrentEmberIndex();

	//Params.

	//Xforms.

	//Xforms Color.

	//Xforms Affine.

	//Xforms Variations.
	void Filter();
	void Filter(const QString& text);

	//Xforms Selection.
	void ClearXformsSelections();
	void ForEachXformCheckbox(std::function<void(int, QCheckBox*)> func);

	//Xaos.
	void FillXaosTable();

	//Palette.
	void ResetPaletteControls();
	void SetPaletteFileComboIndex(const string& filename);
	void SetPaletteTableItem(QPixmap* pixmap, QTableWidget* table, QTableWidgetItem* item, int row, int col);

	//Info.
	void FillSummary();
	void UpdateHistogramBounds();
	void ErrorReportToQTextEdit(const vector<string>& errors, QTextEdit* textEdit, bool clear = true);
	void SetTableWidgetBackgroundColor();

	//Rendering/progress.
	void ShutdownAndRecreateFromOptions();
	bool CreateRendererFromOptions();
	bool CreateControllerFromOptions();

	//Dialogs.
	QStringList SetupOpenXmlDialog();
	QString SetupSaveXmlDialog(const QString& defaultFilename);
	QString SetupSaveImageDialog(const QString& defaultFilename);
	QString SetupSaveFolderDialog();
	QColorDialog* m_ColorDialog;
	FractoriumFinalRenderDialog* m_FinalRenderDialog;
	FractoriumOptionsDialog* m_OptionsDialog;
	FractoriumVariationsDialog* m_VarDialog;
	FractoriumAboutDialog* m_AboutDialog;

	//Params.
	DoubleSpinBox* m_BrightnessSpin;//Color.
	DoubleSpinBox* m_GammaSpin;
	DoubleSpinBox* m_GammaThresholdSpin;
	DoubleSpinBox* m_VibrancySpin;
	DoubleSpinBox* m_HighlightSpin;
	QPushButton* m_BackgroundColorButton;
	StealthComboBox* m_PaletteModeCombo;
	SpinBox* m_WidthSpin;//Geometry.
	SpinBox* m_HeightSpin;
	DoubleSpinBox* m_CenterXSpin;
	DoubleSpinBox* m_CenterYSpin;
	DoubleSpinBox* m_ScaleSpin;
	DoubleSpinBox* m_ZoomSpin;
	DoubleSpinBox* m_RotateSpin;
	DoubleSpinBox* m_ZPosSpin;
	DoubleSpinBox* m_PerspectiveSpin;
	DoubleSpinBox* m_PitchSpin;
	DoubleSpinBox* m_YawSpin;
	DoubleSpinBox* m_DepthBlurSpin;
	DoubleSpinBox* m_SpatialFilterWidthSpin;//Filter.
	StealthComboBox* m_SpatialFilterTypeCombo;
	DoubleSpinBox* m_TemporalFilterWidthSpin;
	StealthComboBox* m_TemporalFilterTypeCombo;
	DoubleSpinBox* m_DEFilterMinRadiusSpin;
	DoubleSpinBox* m_DEFilterMaxRadiusSpin;
	DoubleSpinBox* m_DECurveSpin;
	SpinBox* m_SbsSpin;//Iteration.
	SpinBox* m_FuseSpin;
	DoubleSpinBox* m_QualitySpin;
	SpinBox* m_SupersampleSpin;
	SpinBox* m_TemporalSamplesSpin;
	StealthComboBox* m_AffineInterpTypeCombo;
	StealthComboBox* m_InterpTypeCombo;

	//Xforms.
	DoubleSpinBox* m_XformWeightSpin;
	SpinnerButtonWidget* m_XformWeightSpinnerButtonWidget;
	QFormLayout* m_XformsSelectionLayout;
	QVector<QCheckBox*> m_XformSelections;

	//Xforms Color.
	QTableWidgetItem* m_XformColorValueItem;
	QTableWidgetItem* m_PaletteRefItem;
	DoubleSpinBox* m_XformColorIndexSpin;
	DoubleSpinBox* m_XformColorSpeedSpin;
	DoubleSpinBox* m_XformOpacitySpin;
	DoubleSpinBox* m_XformDirectColorSpin;

	//Xforms Affine.
	DoubleSpinBox* m_PreX1Spin;//Pre.
	DoubleSpinBox* m_PreX2Spin;
	DoubleSpinBox* m_PreY1Spin;
	DoubleSpinBox* m_PreY2Spin;
	DoubleSpinBox* m_PreO1Spin;
	DoubleSpinBox* m_PreO2Spin;

	DoubleSpinBox* m_PostX1Spin;//Post.
	DoubleSpinBox* m_PostX2Spin;
	DoubleSpinBox* m_PostY1Spin;
	DoubleSpinBox* m_PostY2Spin;
	DoubleSpinBox* m_PostO1Spin;
	DoubleSpinBox* m_PostO2Spin;

	DoubleSpinBox* m_PreSpins[6];
	DoubleSpinBox* m_PostSpins[6];

	//Xaos.
	DoubleSpinBox* m_XaosSpinBox;
	QStandardItemModel* m_XaosTableModel;
	DoubleSpinBoxTableItemDelegate* m_XaosTableItemDelegate;

	//Palette.
	SpinBox* m_PaletteHueSpin;
	SpinBox* m_PaletteSaturationSpin;
	SpinBox* m_PaletteBrightnessSpin;
	SpinBox* m_PaletteContrastSpin;
	SpinBox* m_PaletteBlurSpin;
	SpinBox* m_PaletteFrequencySpin;

	//Info.
	QTableWidgetItem* m_InfoNameItem;
	QTableWidgetItem* m_InfoPaletteItem;
	QTableWidgetItem* m_Info3dItem;
	QTableWidgetItem* m_InfoXaosItem;
	QTableWidgetItem* m_InfoXformCountItem;
	QTableWidgetItem* m_InfoFinalXformItem;

	//Files.
	QFileDialog* m_FileDialog;
	QFileDialog* m_FolderDialog;
	QssDialog* m_QssDialog;
	QString m_LastSaveAll;
	QString m_LastSaveCurrent;
	QString m_Style;
	QStyle* m_Theme;
	QString m_SettingsPath;
	//QMenu* m_FileTreeMenu;

	QProgressBar* m_ProgressBar;
	QLabel* m_RenderStatusLabel;
	QLabel* m_CoordinateStatusLabel;
	FractoriumSettings* m_Settings;
	char m_ULString[32];
	char m_URString[32];
	char m_LRString[32];
	char m_LLString[32];
	char m_WHString[64];
	char m_DEString[16];
	char m_CoordinateString[128];
	QColor m_XformComboColors[XFORM_COLOR_COUNT], m_FinalXformComboColor;
	QIcon m_XformComboIcons[XFORM_COLOR_COUNT], m_FinalXformComboIcon;
	vector<QDockWidget*> m_Docks;

	int m_FontSize;
	int m_VarSortMode;
	int m_PaletteSortMode;
	int m_PreviousPaletteRow;
	shared_ptr<OpenCLInfo> m_Info;
	unique_ptr<FractoriumEmberControllerBase> m_Controller;
	Ui::FractoriumClass ui;
};
