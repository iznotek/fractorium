#pragma once

#include "RendererBase.h"
#include "Iterator.h"
#include "SpatialFilter.h"
#include "TemporalFilter.h"
#include "Interpolate.h"
#include "CarToRas.h"
#include "EmberToXml.h"

/// <summary>
/// Renderer.
/// </summary>

namespace EmberNs
{
/// <summary>
/// Renderer is the main class where all of the execution takes place.
/// It is intended that the program have one instance of it that it
/// keeps around for its duration. After a user sets up an ember, it's passed
/// in to be rendered.
/// This class derives from EmberReport, so the caller is able
/// to retrieve a text dump of error information if any errors occur.
/// The final image output vector is also passed in because the calling code has more
/// use for it than this class does.
/// Several functions are made virtual and have a default CPU-based implementation
/// that roughly matches what flam3 did. However they can be overridden in derived classes
/// to provide alternative rendering implementations, such as using the GPU.
/// Since this is a templated class, it's supposed to be entirely implemented in this .h file.
/// However, VC++ 2010 has very crippled support for lambdas, which Renderer makes use of.
/// If too many lambdas are used in a .h file, it will crash the compiler when another library
/// tries to link to it. To work around the bug, only declarations are here and all implementations
/// are in the .cpp file. It's unclear at what point it starts/stops working. But it seems that once
/// enough code is placed in the .h file, the compiler crashes. So for the sake of consistency, everything
/// is moved to the .cpp, even simple getters. One drawback however, is that the class must be
/// explicitly exported at the bottom of the file.
/// Also, despite explicitly doing this, the compiler throws a C4661 warning
/// for every single function in this class, saying it can't find the implementation. This warning
/// can be safely ignored.
/// Template argument T expected to be float or double.
/// Template argument bucketT must always be float.
/// </summary>
template <typename T, typename bucketT>
class EMBER_API Renderer : public RendererBase
{
//using EmberReport::m_ErrorReport;
public:
	Renderer();
	virtual ~Renderer();

	//Non-virtual processing functions.
	void AddEmber(Ember<T>& ember);
	bool AssignIterator();

	//Virtual processing functions overriden from RendererBase.
	virtual void ComputeBounds() override;
	virtual void ComputeQuality() override;
	virtual void ComputeCamera() override;
	virtual void SetEmber(Ember<T>& ember, eProcessAction action = FULL_RENDER) override;
	virtual void SetEmber(vector<Ember<T>>& embers) override;
	virtual bool CreateDEFilter(bool& newAlloc) override;
	virtual bool CreateSpatialFilter(bool& newAlloc) override;
	virtual bool CreateTemporalFilter(bool& newAlloc) override;
	virtual size_t HistBucketSize() const override { return sizeof(tvec4<bucketT, glm::defaultp>); }
	virtual eRenderStatus Run(vector<byte>& finalImage, double time = 0, size_t subBatchCountOverride = 0, bool forceOutput = false, size_t finalOffset = 0) override;
	virtual EmberImageComments ImageComments(const EmberStats& stats, size_t printEditDepth = 0, bool intPalette = false, bool hexPalette = true) override;

protected:
	//New virtual functions to be overridden in derived renderers that use the GPU, but not accessed outside.
	virtual void MakeDmap(T colorScalar);
	virtual bool Alloc(bool histOnly = false);
	virtual bool ResetBuckets(bool resetHist = true, bool resetAccum = true);
	virtual eRenderStatus LogScaleDensityFilter(bool forceOutput = false);
	virtual eRenderStatus GaussianDensityFilter();
	virtual eRenderStatus AccumulatorToFinalImage(vector<byte>& pixels, size_t finalOffset);
	virtual eRenderStatus AccumulatorToFinalImage(byte* pixels, size_t finalOffset);
	virtual EmberStats Iterate(size_t iterCount, size_t temporalSample);

public:
	//Non-virtual render properties, getters and setters.
	inline T PixelAspectRatio() const;
	void PixelAspectRatio(T pixelAspectRatio);

	//Non-virtual renderer properties, getters only.
	inline T                              Scale()               const;
	inline T                              PixelsPerUnitX()      const;
	inline T                              PixelsPerUnitY()      const;
	inline bucketT                        K1()                  const;
	inline bucketT                        K2()                  const;
	inline const CarToRas<T>&             CoordMap()            const;
	inline tvec4<bucketT, glm::defaultp>* HistBuckets();
	inline tvec4<bucketT, glm::defaultp>* AccumulatorBuckets();
	inline SpatialFilter<bucketT>*        GetSpatialFilter();
	inline TemporalFilter<T>*             GetTemporalFilter();

	//Virtual renderer properties overridden from RendererBase, getters only.
	virtual double ScaledQuality()				   const override;
	virtual double LowerLeftX(bool  gutter = true) const override;
	virtual double LowerLeftY(bool  gutter = true) const override;
	virtual double UpperRightX(bool gutter = true) const override;
	virtual double UpperRightY(bool gutter = true) const override;
	virtual DensityFilterBase* GetDensityFilter()        override;

	//Non-virtual ember wrappers, getters only.
	inline bool              XaosPresent()		   const;
	inline size_t			 Supersample()         const;
	inline size_t			 PaletteIndex()        const;
	inline T                 Time()                const;
	inline T                 Quality()             const;
	inline T                 SpatialFilterRadius() const;
	inline T                 PixelsPerUnit()       const;
	inline T                 Zoom()                const;
	inline T                 CenterX()             const;
	inline T                 CenterY()             const;
	inline T                 Rotate()              const;
	inline T                 Hue()                 const;
	inline bucketT           Brightness()          const;
	inline bucketT           Contrast()            const;
	inline bucketT           Gamma()               const;
	inline bucketT           Vibrancy()            const;
	inline bucketT           GammaThresh()         const;
	inline bucketT           HighlightPower()      const;
	inline Color<T>			 Background()          const;
	inline const Xform<T>*   Xforms()              const;
	inline Xform<T>*         NonConstXforms();
	inline size_t			 XformCount()          const;
	inline const Xform<T>*   FinalXform()          const;
	inline Xform<T>*         NonConstFinalXform();
	inline bool              UseFinalXform()       const;
	inline const Palette<T>* GetPalette()          const;
	inline ePaletteMode      PaletteMode()         const;

	//Virtual ember wrappers overridden from RendererBase, getters only.
	virtual size_t TemporalSamples() const override;
	virtual size_t FinalRasW()       const override;
	virtual size_t FinalRasH()       const override;
	virtual size_t SubBatchSize()    const override;
	virtual size_t FuseCount()		 const override;

	//Non-virtual iterator wrappers.
	const byte* XformDistributions()		const;
	size_t 		XformDistributionsSize()    const;
	Point<T>*	Samples(size_t threadIndex) const;

protected:
	//Non-virtual functions that might be needed by a derived class.
	void PrepFinalAccumVals(Color<bucketT>& background, bucketT& g, bucketT& linRange, bucketT& vibrancy);

	private:
	//Miscellaneous non-virtual functions used only in this class.
	void Accumulate(QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand, Point<T>* samples, size_t sampleCount, const Palette<bucketT>* palette);
	/*inline*/ void AddToAccum(const tvec4<bucketT, glm::defaultp>& bucket, intmax_t i, intmax_t ii, intmax_t j, intmax_t jj);
	template <typename accumT> void GammaCorrection(tvec4<bucketT, glm::defaultp>& bucket, Color<bucketT>& background, bucketT g, bucketT linRange, bucketT vibrancy, bool doAlpha, bool scale, accumT* correctedChannels);
	void CurveAdjust(bucketT& a, const glm::length_t& index);
	void VectorizedLogScale(size_t row, size_t rowEnd);

protected:
	T m_Scale;
	T m_PixelsPerUnitX;
	T m_PixelsPerUnitY;
	T m_PixelAspectRatio;
	T m_LowerLeftX;
	T m_LowerLeftY;
	T m_UpperRightX;
	T m_UpperRightY;
	bucketT m_K1;
	bucketT m_K2;
	bucketT m_Vibrancy;//Accumulate these after each temporal sample.
	bucketT m_Gamma;
	T m_ScaledQuality;
	Color<bucketT> m_Background;//This is a scaled copy of the m_Background member of m_Ember, but with a type of bucketT.
	Affine2D<T> m_RotMat;
	Ember<T> m_Ember;
	Ember<T> m_TempEmber;
	Ember<T> m_LastEmber;
	vector<Ember<T>> m_Embers;
	vector<Ember<T>> m_ThreadEmbers;
	CarToRas<T> m_CarToRas;
	Iterator<T>* m_Iterator;
	unique_ptr<StandardIterator<T>> m_StandardIterator;
	unique_ptr<XaosIterator<T>> m_XaosIterator;
	Palette<bucketT> m_Dmap, m_Csa;
	vector<tvec4<bucketT, glm::defaultp>> m_HistBuckets;
	vector<tvec4<bucketT, glm::defaultp>> m_AccumulatorBuckets;
	unique_ptr<SpatialFilter<bucketT>> m_SpatialFilter;
	unique_ptr<TemporalFilter<T>> m_TemporalFilter;
	unique_ptr<DensityFilter<bucketT>> m_DensityFilter;
	vector<vector<Point<T>>> m_Samples;
	EmberToXml<T> m_EmberToXml;
};

//This class had to be implemented in a cpp file because the compiler was breaking.
//So the explicit instantiation must be declared here rather than in Ember.cpp where
//all of the other classes are done.
}
