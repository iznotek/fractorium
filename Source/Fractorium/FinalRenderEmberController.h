#pragma once

#include "FractoriumSettings.h"
#include "FractoriumEmberController.h"

/// <summary>
/// FinalRenderEmberControllerBase and FinalRenderEmberController<T> classes.
/// </summary>

/// <summary>
/// FractoriumEmberController and Fractorium need each other, but each can't include the other.
/// So Fractorium includes this file, and Fractorium is declared as a forward declaration here.
/// </summary>
class Fractorium;
class FractoriumFinalRenderDialog;
//class FractoriumEmberControllerBase;

/// <summary>
/// Used to hold the options specified in the current state of the Gui for performing the final render.
/// </summary>
struct FinalRenderGuiState
{
	bool m_EarlyClip;
	bool m_YAxisUp;
	bool m_AlphaChannel;
	bool m_Transparency;
	bool m_OpenCL;
	bool m_Double;
	bool m_SaveXml;
	bool m_DoAll;
	bool m_DoSequence;
	bool m_KeepAspect;
	eScaleType m_Scale;
	QString m_Path;
	QString m_Ext;
	QString m_Prefix;
	QString m_Suffix;
	QList<QVariant> m_Devices;
	uint m_ThreadCount;
	int m_ThreadPriority;
	double m_WidthScale;
	double m_HeightScale;
	double m_Quality;
	uint m_TemporalSamples;
	uint m_Supersample;
	uint m_Strips;
};

/// <summary>
/// FinalRenderEmberControllerBase serves as a non-templated base class with virtual
/// functions which will be overridden in a derived class that takes a template parameter.
/// Although not meant to be used as an interactive renderer, it derives from FractoriumEmberControllerBase
/// to access a few of its members to avoid having to redefine them here.
/// </summary>
class FinalRenderEmberControllerBase : public FractoriumEmberControllerBase
{
	friend FractoriumFinalRenderDialog;

public:
	FinalRenderEmberControllerBase(FractoriumFinalRenderDialog* finalRenderDialog);
	virtual ~FinalRenderEmberControllerBase() { }

	virtual void SyncCurrentToGui() { }
	virtual void SyncGuiToEmbers(size_t widthOverride = 0, size_t heightOverride = 0) { }
	virtual void SyncCurrentToSizeSpinners(bool scale, bool size) { }
	virtual void ResetProgress(bool total = true) { }
	virtual tuple<size_t, size_t, size_t> SyncAndComputeMemory() { return tuple<size_t, size_t, size_t>(0, 0, 0); }
	virtual double OriginalAspect() { return 1; }
	virtual QString ComposePath(const QString& name) { return ""; }
	virtual void CancelRender() { }

	bool CreateRendererFromGUI();
	void Output(const QString& s);

protected:
	bool m_Run;
	bool m_PreviewRun;
	size_t m_ImageCount;
	std::atomic<size_t> m_FinishedImageCount;

	QFuture<void> m_Result;
	QFuture<void> m_FinalPreviewResult;
	std::function<void (void)> m_FinalRenderFunc;
	std::function<void (void)> m_FinalPreviewRenderFunc;
	
	FractoriumSettings* m_Settings;
	FractoriumFinalRenderDialog* m_FinalRenderDialog;
	FinalRenderGuiState m_GuiState;
	CriticalSection m_PreviewCs, m_ProgressCs;
	Timing m_RenderTimer;
	Timing m_TotalTimer;
};

/// <summary>
/// Templated derived class which implements all interaction functionality between the embers
/// of a specific template type and the final render dialog.
/// </summary>
template<typename T>
class FinalRenderEmberController : public FinalRenderEmberControllerBase
{
public:
	FinalRenderEmberController(FractoriumFinalRenderDialog* finalRender);
	virtual ~FinalRenderEmberController() { }

	//Virtual functions overridden from FractoriumEmberControllerBase.
	virtual void SetEmberFile(const EmberFile<float>& emberFile) override;
	virtual void CopyEmberFile(EmberFile<float>& emberFile, std::function<void(Ember<float>& ember)> perEmberOperation/* = [&](Ember<float>& ember) { }*/) override;
#ifdef DO_DOUBLE
	virtual void SetEmberFile(const EmberFile<double>& emberFile) override;
	virtual void CopyEmberFile(EmberFile<double>& emberFile, std::function<void(Ember<double>& ember)> perEmberOperation/* = [&](Ember<double>& ember) { }*/) override;
#endif
	virtual void SetEmber(size_t index) override;
	virtual bool Render() override;
	virtual bool CreateRenderer(eRendererType renderType, const vector<pair<size_t, size_t>>& devices, bool shared = true) override;
	virtual int ProgressFunc(Ember<T>& ember, void* foo, double fraction, int stage, double etaMs) override;
	virtual size_t Index() const override { return m_Ember->m_Index; }
	virtual uint SizeOfT() const override { return sizeof(T); }

	//Virtual functions overridden from FinalRenderEmberControllerBase.
	virtual void SyncCurrentToGui() override;
	virtual void SyncGuiToEmbers(size_t widthOverride = 0, size_t heightOverride = 0) override;
	virtual void SyncCurrentToSizeSpinners(bool scale, bool size) override;
	virtual void ResetProgress(bool total = true)  override;
	virtual tuple<size_t, size_t, size_t> SyncAndComputeMemory() override;
	virtual double OriginalAspect() override { return double(m_Ember->m_OrigFinalRasW) / m_Ember->m_OrigFinalRasH; }
	virtual QString Name() const override { return QString::fromStdString(m_Ember->m_Name); }
	virtual QString ComposePath(const QString& name) override;
	virtual void CancelRender() override;

	//Non Virtual functions.
	EmberNs::Renderer<T, float>* FirstOrDefaultRenderer();

protected:
	void CancelPreviewRender();
	void HandleFinishedProgress();
	void SaveCurrentRender(Ember<T>& ember);
	void SaveCurrentRender(Ember<T>& ember, const EmberImageComments& comments, vector<byte>& pixels, size_t width, size_t height, size_t channels, size_t bpc);
	void RenderComplete(Ember<T>& ember);
	void RenderComplete(Ember<T>& ember, const EmberStats& stats, Timing& renderTimer);
	void SyncGuiToEmber(Ember<T>& ember, size_t widthOverride = 0, size_t heightOverride = 0);
	bool SyncGuiToRenderer();
	void SetProgressComplete(int val);

	Ember<T>* m_Ember;
	Ember<T> m_PreviewEmber;
	EmberFile<T> m_EmberFile;
	EmberToXml<T> m_XmlWriter;
	unique_ptr<EmberNs::Renderer<T, float>> m_FinalPreviewRenderer;
	vector<unique_ptr<EmberNs::Renderer<T, float>>> m_Renderers;
};

