#pragma once

#include "EmberFile.h"
#include "DoubleSpinBox.h"
#include "GLEmberController.h"

/// <summary>
/// FractoriumEmberControllerBase and FractoriumEmberController<T> classes.
/// </summary>

/// <summary>
/// An enum representing the type of edit being done.
/// </summary>
enum eEditUndoState : uint { REGULAR_EDIT, UNDO_REDO, EDIT_UNDO };

/// <summary>
/// An enum representing which xforms an update should be applied to.
/// </summary>
enum eXformUpdate : uint { UPDATE_CURRENT, UPDATE_SELECTED, UPDATE_SELECTED_EXCEPT_FINAL, UPDATE_ALL, UPDATE_ALL_EXCEPT_FINAL };

/// <summary>
/// FractoriumEmberController and Fractorium need each other, but each can't include the other.
/// So Fractorium includes this file, and Fractorium is declared as a forward declaration here.
/// </summary>
class Fractorium;
#define PREVIEW_SIZE 256
#define UNDO_SIZE 128

/// <summary>
/// FractoriumEmberControllerBase serves as a non-templated base class with virtual
/// functions which will be overridden in a derived class that takes a template parameter.
/// The controller serves as a way to access both the Fractorium GUI and the underlying ember
/// objects through an interface that doesn't require template argument, but does allow
/// templated objects to be used underneath.
/// Note that there are a few functions which access a templated object, so for those both
/// versions for float and double must be provided, then overridden in the templated derived
/// class. It's definitely a design flaw, but C++ doesn't offer any alternative since
/// templated virtual functions are not supported.
/// The functions not implemented in this file can be found in the GUI files which use them.
/// </summary>
class FractoriumEmberControllerBase : public RenderCallback
{
public:
	FractoriumEmberControllerBase(Fractorium* fractorium);
	virtual ~FractoriumEmberControllerBase();

	//Embers.
	virtual void SetEmber(const Ember<float>& ember, bool verbatim = false) { }
	virtual void CopyEmber(Ember<float>& ember, std::function<void(Ember<float>& ember)> perEmberOperation/* = [&](Ember<float>& ember) { }*/) { }//Uncomment default lambdas once LLVM fixes a crash in their compiler with default lambda parameters.//TODO
	virtual void SetEmberFile(const EmberFile<float>& emberFile) { }
	virtual void CopyEmberFile(EmberFile<float>& emberFile, std::function<void(Ember<float>& ember)> perEmberOperation/* = [&](Ember<float>& ember) { }*/) { }
	virtual void SetTempPalette(const Palette<float>& palette) { }
	virtual void CopyTempPalette(Palette<float>& palette) { }
#ifdef DO_DOUBLE
	virtual void SetEmber(const Ember<double>& ember, bool verbatim = false) { }
	virtual void CopyEmber(Ember<double>& ember, std::function<void(Ember<double>& ember)> perEmberOperation/* = [&](Ember<double>& ember) { }*/) { }
	virtual void SetEmberFile(const EmberFile<double>& emberFile) { }
	virtual void CopyEmberFile(EmberFile<double>& emberFile, std::function<void(Ember<double>& ember)> perEmberOperation/* = [&](Ember<double>& ember) { }*/) { }
	virtual void SetTempPalette(const Palette<double>& palette) { }
	virtual void CopyTempPalette(Palette<double>& palette) { }
#endif
	virtual void SetEmber(size_t index) { }
	//virtual void Clear() { }
	virtual void AddXform() { }
	virtual void AddLinkedXform() { }
	virtual void DuplicateXform() { }
	virtual void ClearXform() { }
	virtual void DeleteXforms() { }
	virtual void AddFinalXform() { }
	virtual bool UseFinalXform() { return false; }
	virtual size_t XformCount() const { return 0; }
	virtual size_t TotalXformCount() const { return 0; }
	virtual QString Name() const { return ""; }
	virtual void Name(const string& s) { }
	virtual size_t FinalRasW() const { return 0; }
	virtual void FinalRasW(size_t w) { }
	virtual size_t FinalRasH() const { return 0; }
	virtual void FinalRasH(size_t h) { }
	virtual size_t Index() const { return 0; }
	virtual void AddSymmetry(int sym, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) { }
	virtual void CalcNormalizedWeights() { }

	//Menu.
	virtual void NewFlock(size_t count) { }//File.
	virtual void NewEmptyFlameInCurrentFile() { }
	virtual void NewRandomFlameInCurrentFile() { }
	virtual void CopyFlameInCurrentFile() { }
	virtual void OpenAndPrepFiles(const QStringList& filenames, bool append) { }
	virtual void SaveCurrentAsXml() { }
	virtual void SaveEntireFileAsXml() { }
	virtual void SaveCurrentToOpenedFile() { }
	virtual void Undo() { }//Edit.
	virtual void Redo() { }
	virtual void CopyXml() { }
	virtual void CopyAllXml() { }
	virtual void PasteXmlAppend() { }
	virtual void PasteXmlOver() { }
	virtual void CopySelectedXforms() { }
	virtual void PasteSelectedXforms() { }
	virtual void AddReflectiveSymmetry() { }//Tools.
	virtual void AddRotationalSymmetry() { }
	virtual void AddBothSymmetry() { }
	virtual void Flatten() { }
	virtual void Unflatten() { }
	virtual void ClearFlame() { }

	//Toolbar.

	//Library.
	virtual void SyncNames() { }
	virtual void SyncPointers() { }
	virtual void FillLibraryTree(int selectIndex = -1) { }
	virtual void UpdateLibraryTree() { }
	virtual void EmberTreeItemChanged(QTreeWidgetItem* item, int col) { }
	virtual void EmberTreeItemDoubleClicked(QTreeWidgetItem* item, int col) { }
	virtual void RenderPreviews(uint start = UINT_MAX, uint end = UINT_MAX) { }
	virtual void StopPreviewRender() { }
	virtual void Delete(const pair<size_t, QTreeWidgetItem*>& p) { }

	//Params.
	virtual void SetCenter(double x, double y) { }
	virtual void FillParamTablesAndPalette() { }
	virtual void BrightnessChanged(double d) { }
	virtual void GammaChanged(double d) { }
	virtual void GammaThresholdChanged(double d) { }
	virtual void VibrancyChanged(double d) { }
	virtual void HighlightPowerChanged(double d) { }
	virtual void PaletteModeChanged(uint i) { }
	virtual void WidthChanged(uint i) { }
	virtual void HeightChanged(uint i) { }
	virtual void CenterXChanged(double d) { }
	virtual void CenterYChanged(double d) { }
	virtual void ScaleChanged(double d) { }
	virtual void ZoomChanged(double d) { }
	virtual void RotateChanged(double d) { }
	virtual void ZPosChanged(double d) { }
	virtual void PerspectiveChanged(double d) { }
	virtual void PitchChanged(double d) { }
	virtual void YawChanged(double d) { }
	virtual void DepthBlurChanged(double d) { }
	virtual void SpatialFilterWidthChanged(double d) { }
	virtual void SpatialFilterTypeChanged(const QString& text) { }
	virtual void TemporalFilterWidthChanged(double d) { }
	virtual void TemporalFilterTypeChanged(const QString& text) { }
	virtual void DEFilterMinRadiusWidthChanged(double d) { }
	virtual void DEFilterMaxRadiusWidthChanged(double d) { }
	virtual void DEFilterCurveWidthChanged(double d) { }
	virtual void SbsChanged(int d) { }
	virtual void FuseChanged(int d) { }
	virtual void QualityChanged(double d) { }
	virtual void SupersampleChanged(int d) { }
	virtual void TemporalSamplesChanged(int d) { }
	virtual void AffineInterpTypeChanged(int i) { }
	virtual void InterpTypeChanged(int i) { }
	virtual void BackgroundChanged(const QColor& color) { }
	virtual void ClearColorCurves() { }
	virtual void ColorCurveChanged(int curveIndex, int pointInxed, const QPointF& point) { }

	//Xforms.
	virtual void CurrentXformComboChanged(int index) { }
	virtual void XformWeightChanged(double d) { }
	virtual void EqualizeWeights() { }
	virtual void XformNameChanged(int row, int col) { }
	virtual void FillXforms(int index = 0) { }

	//Xforms Affine.
	virtual void AffineSetHelper(double d, int index, bool pre) { }
	virtual void FlipXforms(bool horizontal, bool vertical, bool pre) { }
	virtual void RotateXformsByAngle(double angle, bool pre) { }
	virtual void MoveXforms(double x, double y, bool pre) { }
	virtual void ScaleXforms(double scale, bool pre) { }
	virtual void ResetXformsAffine(bool pre) { }
	virtual void FillBothAffines() { }

	//Xforms Color.
	virtual void XformColorIndexChanged(double d, bool updateRender) { }
	virtual void XformScrollColorIndexChanged(int d) { }
	virtual void XformColorSpeedChanged(double d) { }
	virtual void XformOpacityChanged(double d) { }
	virtual void XformDirectColorChanged(double d) { }
	virtual QColor ColorIndexToQColor(double d) { return QColor(); }

	//Xforms Variations.
	virtual void Filter(const QString& text) { }
	virtual void SetupVariationTree() { }
	virtual void ClearVariationsTree() { }
	virtual void VariationSpinBoxValueChanged(double d) { }
	virtual void FilteredVariations() { }

	//Xforms Selection.

	//Xaos.
	virtual void FillXaos() { }
	virtual QString MakeXaosNameString(uint i) { return ""; }
	virtual void XaosChanged(int x, int y, double val) { }
	virtual void ClearXaos() { }
	virtual void RandomXaos() { }

	//Palette.
	virtual int  InitPaletteList(const string& s) { return 0; }
	virtual bool FillPaletteTable(const string& s) { return false; }
	virtual void ApplyPaletteToEmber() { }
	virtual void PaletteAdjust() { }
	virtual void PaletteCellClicked(int row, int col) { }
	QImage& FinalPaletteImage() { return m_FinalPaletteImage; }

	//Info.
	virtual void FillSummary() { }

	//Rendering/progress.
	virtual bool Render() { return false; }
	virtual bool CreateRenderer(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared = true) { return false; }
	virtual uint SizeOfT() const { return 0; }
	virtual void ClearUndo() { }
	virtual GLEmberControllerBase* GLController() { return nullptr; }
	bool RenderTimerRunning();
	void StartRenderTimer();
	void DelayedStartRenderTimer();
	void StopRenderTimer(bool wait);
	void ClearFinalImages();
	void Shutdown();
	void UpdateRender(eProcessAction action = FULL_RENDER);
	void DeleteRenderer();
	void SaveCurrentRender(const QString& filename, const EmberImageComments& comments, vector<byte>& pixels, size_t width, size_t height, size_t channels, size_t bpc);
	RendererBase* Renderer() { return m_Renderer.get(); }
	vector<byte>* FinalImage() { return &(m_FinalImage); }
	vector<byte>* PreviewFinalImage() { return &m_PreviewFinalImage; }
	EmberStats Stats() { return m_Stats; }

protected:
	//Rendering/progress.
	void AddProcessAction(eProcessAction action);
	eProcessAction CondenseAndClearProcessActions();
	eProcessState ProcessState() { return m_Renderer.get() ? m_Renderer->ProcessState() : NONE; }

	//Non-templated members.
	bool m_Rendering;
	bool m_Shared;
	bool m_LastEditWasUndoRedo;
	vector<pair<size_t, size_t>> m_Devices;
	uint m_SubBatchCount;
	uint m_FailedRenders;
	uint m_UndoIndex;
	eRendererType m_RenderType;
	eEditUndoState m_EditState;
	GLuint m_OutputTexID;
	Timing m_RenderElapsedTimer;
	EmberStats m_Stats;
	QImage m_FinalPaletteImage;
	QString m_LastSaveAll;
	QString m_LastSaveCurrent;
	string m_CurrentPaletteFilePath;
	CriticalSection m_Cs;
	std::thread m_WriteThread;
	vector<byte> m_FinalImage;
	vector<byte> m_PreviewFinalImage;
	vector<eProcessAction> m_ProcessActions;
	vector<eVariationId> m_FilteredVariations;
	unique_ptr<EmberNs::RendererBase> m_Renderer;
	QTIsaac<ISAAC_SIZE, ISAAC_INT> m_Rand;
	Fractorium* m_Fractorium;
	QTimer* m_RenderTimer;
	QTimer* m_RenderRestartTimer;
	shared_ptr<OpenCLInfo> m_Info;
};

/// <summary>
/// Templated derived class which implements all interaction functionality between the embers
/// of a specific template type and the GUI.
/// Switching between template arguments requires complete re-creation of the controller and the
/// underlying renderer. Switching between CPU and OpenCL only requires re-creation of the renderer.
/// </summary>
template<typename T>
class FractoriumEmberController : public FractoriumEmberControllerBase
{
public:
	FractoriumEmberController(Fractorium* fractorium);
	virtual ~FractoriumEmberController();

	//Embers.
	virtual void SetEmber(const Ember<float>& ember, bool verbatim = false) override;
	virtual void CopyEmber(Ember<float>& ember, std::function<void(Ember<float>& ember)> perEmberOperation/* = [&](Ember<float>& ember) { }*/) override;
	virtual void SetEmberFile(const EmberFile<float>& emberFile) override;
	virtual void CopyEmberFile(EmberFile<float>& emberFile, std::function<void(Ember<float>& ember)> perEmberOperation/* = [&](Ember<float>& ember) { }*/) override;
	virtual void SetTempPalette(const Palette<float>& palette) override;
	virtual void CopyTempPalette(Palette<float>& palette) override;
#ifdef DO_DOUBLE
	virtual void SetEmber(const Ember<double>& ember, bool verbatim = false) override;
	virtual void CopyEmber(Ember<double>& ember, std::function<void(Ember<double>& ember)> perEmberOperation/* = [&](Ember<double>& ember) { }*/) override;
	virtual void SetEmberFile(const EmberFile<double>& emberFile) override;
	virtual void CopyEmberFile(EmberFile<double>& emberFile, std::function<void(Ember<double>& ember)> perEmberOperation/* = [&](Ember<double>& ember) { }*/) override;
	virtual void SetTempPalette(const Palette<double>& palette) override;
	virtual void CopyTempPalette(Palette<double>& palette) override;
#endif
	virtual void SetEmber(size_t index) override;
	//virtual void Clear() override { }
	virtual void AddXform() override;
	virtual void AddLinkedXform() override;
	virtual void DuplicateXform() override;
	virtual void ClearXform() override;
	virtual void DeleteXforms() override;
	virtual void AddFinalXform() override;
	virtual bool UseFinalXform() override { return m_Ember.UseFinalXform(); }
	//virtual bool IsFinal(uint i) { return false; }
	virtual size_t XformCount() const override { return m_Ember.XformCount(); }
	virtual size_t TotalXformCount() const override { return m_Ember.TotalXformCount(); }
	virtual QString Name() const override { return QString::fromStdString(m_Ember.m_Name); }
	virtual void Name(const string& s) override { m_Ember.m_Name = s; }
	virtual size_t FinalRasW() const override { return m_Ember.m_FinalRasW; }
	virtual void FinalRasW(size_t w) override { m_Ember.m_FinalRasW = w; }
	virtual size_t FinalRasH() const override { return m_Ember.m_FinalRasH; }
	virtual void FinalRasH(size_t h) override { m_Ember.m_FinalRasH = h; }
	virtual size_t Index() const override { return m_Ember.m_Index; }
	virtual void AddSymmetry(int sym, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override { m_Ember.AddSymmetry(sym, rand); }
	virtual void CalcNormalizedWeights() override { m_Ember.CalcNormalizedWeights(m_NormalizedWeights); }
	void ConstrainDimensions(Ember<T>& ember);
	Ember<T>* CurrentEmber();

	//Menu.
	virtual void NewFlock(size_t count) override;
	virtual void NewEmptyFlameInCurrentFile() override;
	virtual void NewRandomFlameInCurrentFile() override;
	virtual void CopyFlameInCurrentFile() override;
	virtual void OpenAndPrepFiles(const QStringList& filenames, bool append) override;
	virtual void SaveCurrentAsXml() override;
	virtual void SaveEntireFileAsXml() override;
	virtual void SaveCurrentToOpenedFile() override;
	virtual void Undo() override;
	virtual void Redo() override;
	virtual void CopyXml() override;
	virtual void CopyAllXml() override;
	virtual void PasteXmlAppend() override;
	virtual void PasteXmlOver() override;
	virtual void CopySelectedXforms() override;
	virtual void PasteSelectedXforms() override;
	virtual void AddReflectiveSymmetry() override;
	virtual void AddRotationalSymmetry() override;
	virtual void AddBothSymmetry() override;
	virtual void Flatten() override;
	virtual void Unflatten() override;
	virtual void ClearFlame() override;

	//Toolbar.

	//Library.
	virtual void SyncNames() override;
	virtual void SyncPointers() override;
	virtual void FillLibraryTree(int selectIndex = -1) override;
	virtual void UpdateLibraryTree() override;
	virtual void Delete(const pair<size_t, QTreeWidgetItem*>& p) override;
	virtual void EmberTreeItemChanged(QTreeWidgetItem* item, int col) override;
	virtual void EmberTreeItemDoubleClicked(QTreeWidgetItem* item, int col) override;
	virtual void RenderPreviews(uint start = UINT_MAX, uint end = UINT_MAX) override;
	virtual void StopPreviewRender() override;

	//Params.
	virtual void SetCenter(double x, double y) override;
	virtual void FillParamTablesAndPalette() override;
	virtual void BrightnessChanged(double d) override;
	virtual void GammaChanged(double d) override;
	virtual void GammaThresholdChanged(double d) override;
	virtual void VibrancyChanged(double d) override;
	virtual void HighlightPowerChanged(double d) override;
	virtual void PaletteModeChanged(uint i) override;
	virtual void WidthChanged(uint i) override;
	virtual void HeightChanged(uint i) override;
	virtual void CenterXChanged(double d) override;
	virtual void CenterYChanged(double d) override;
	virtual void ScaleChanged(double d) override;
	virtual void ZoomChanged(double d) override;
	virtual void RotateChanged(double d) override;
	virtual void ZPosChanged(double d) override;
	virtual void PerspectiveChanged(double d) override;
	virtual void PitchChanged(double d) override;
	virtual void YawChanged(double d) override;
	virtual void DepthBlurChanged(double d) override;
	virtual void SpatialFilterWidthChanged(double d) override;
	virtual void SpatialFilterTypeChanged(const QString& text) override;
	virtual void TemporalFilterWidthChanged(double d) override;
	virtual void TemporalFilterTypeChanged(const QString& text) override;
	virtual void DEFilterMinRadiusWidthChanged(double d) override;
	virtual void DEFilterMaxRadiusWidthChanged(double d) override;
	virtual void DEFilterCurveWidthChanged(double d) override;
	virtual void SbsChanged(int d) override;
	virtual void FuseChanged(int d) override;
	virtual void QualityChanged(double d) override;
	virtual void SupersampleChanged(int d) override;
	virtual void TemporalSamplesChanged(int d) override;
	virtual void AffineInterpTypeChanged(int index) override;
	virtual void InterpTypeChanged(int index) override;
	virtual void BackgroundChanged(const QColor& col) override;
	virtual void ClearColorCurves() override;
	virtual void ColorCurveChanged(int curveIndex, int pointInxed, const QPointF& point) override;

	//Xforms.
	virtual void CurrentXformComboChanged(int index) override;
	virtual void XformWeightChanged(double d) override;
	virtual void EqualizeWeights() override;
	virtual void XformNameChanged(int row, int col) override;
	virtual void FillXforms(int index = 0) override;
	void FillWithXform(Xform<T>* xform);
	Xform<T>* CurrentXform();

	//Xforms Affine.
	virtual void AffineSetHelper(double d, int index, bool pre) override;
	virtual void FlipXforms(bool horizontal, bool vertical, bool pre) override;
	virtual void RotateXformsByAngle(double angle, bool pre) override;
	virtual void MoveXforms(double x, double y, bool pre) override;
	virtual void ScaleXforms(double scale, bool pre) override;
	virtual void ResetXformsAffine(bool pre) override;
	virtual void FillBothAffines() override;
	void FillAffineWithXform(Xform<T>* xform, bool pre);

	//Xforms Color.
	virtual void XformColorIndexChanged(double d, bool updateRender) override;
	virtual void XformScrollColorIndexChanged(int d) override;
	virtual void XformColorSpeedChanged(double d) override;
	virtual void XformOpacityChanged(double d) override;
	virtual void XformDirectColorChanged(double d) override;
	virtual QColor ColorIndexToQColor(double d) override;
	void FillColorWithXform(Xform<T>* xform);

	//Xforms Variations.
	virtual void Filter(const QString& text) override;
	virtual void SetupVariationTree() override;
	virtual void ClearVariationsTree() override;
	virtual void VariationSpinBoxValueChanged(double d) override;
	virtual void FilteredVariations() override;
	void FillVariationTreeWithXform(Xform<T>* xform);

	//Xforms Xaos.
	virtual void FillXaos() override;
	virtual QString MakeXaosNameString(uint i) override;
	virtual void XaosChanged(int x, int y, double val) override;
	virtual void ClearXaos() override;
	virtual void RandomXaos() override;

	//Palette.
	virtual int  InitPaletteList(const string& s) override;
	virtual bool FillPaletteTable(const string& s) override;
	virtual void ApplyPaletteToEmber() override;
	virtual void PaletteAdjust() override;
	virtual void PaletteCellClicked(int row, int col) override;

	//Info.
	virtual void FillSummary() override;

	//Rendering/progress.
	virtual bool Render() override;
	virtual bool CreateRenderer(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared = true) override;
	virtual uint SizeOfT() const override { return sizeof(T); }
	virtual int ProgressFunc(Ember<T>& ember, void* foo, double fraction, int stage, double etaMs) override;
	virtual void ClearUndo() override;
	virtual GLEmberControllerBase* GLController() override { return m_GLController.get(); }

private:
	//Embers.
	void ApplyXmlSavingTemplate(Ember<T>& ember);
	template <typename U> void SetEmberPrivate(const Ember<U>& ember, bool verbatim);

	//Params.
	void ParamsToEmber(Ember<T>& ember);

	//Xforms.
	void SetNormalizedWeightText(Xform<T>* xform);
	bool IsFinal(Xform<T>* xform);

	//Xforms Color.
	void SetCurrentXformColorIndex(double d, bool updateRender);
	void FillCurvesControl();

	//Xforms Selection.
	QString MakeXformCaption(size_t i);
	bool XformCheckboxAt(int i, std::function<void(QCheckBox*)> func);
	bool XformCheckboxAt(Xform<T>* xform, std::function<void(QCheckBox*)> func);
	void UpdateXform(std::function<void(Xform<T>*)> func, eXformUpdate updateType = eXformUpdate::UPDATE_CURRENT, bool updateRender = true, eProcessAction action = FULL_RENDER);

	//Palette.
	void UpdateAdjustedPaletteGUI(Palette<T>& palette);

	//Rendering/progress.
	void Update(std::function<void (void)> func, bool updateRender = true, eProcessAction action = FULL_RENDER);
	bool SyncSizes();

	//Templated members.
	bool m_PreviewRun;
	bool m_PreviewRunning;
	vector<T> m_TempOpacities;
	vector<T> m_NormalizedWeights;
	Ember<T> m_Ember;
	EmberFile<T> m_EmberFile;
	deque<Ember<T>> m_UndoList;
	vector<Xform<T>> m_CopiedXforms;
	Xform<T> m_CopiedFinalXform;
	Palette<T> m_TempPalette;
	PaletteList<T> m_PaletteList;
	VariationList<T> m_VariationList;
	unique_ptr<SheepTools<T, float>> m_SheepTools;
	unique_ptr<GLEmberController<T>> m_GLController;
	unique_ptr<EmberNs::Renderer<T, float>> m_PreviewRenderer;
	QFuture<void> m_PreviewResult;
	std::function<void (uint, uint)> m_PreviewRenderFunc;
};

