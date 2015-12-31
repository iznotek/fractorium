#pragma once

#include "Variation.h"

namespace EmberNs
{
/// <summary>
/// hexes.
/// </summary>
template <typename T>
class EMBER_API HexesVariation : public ParametricVariation<T>
{
public:
	HexesVariation(T weight = 1.0) : ParametricVariation<T>("hexes", VAR_HEXES, weight)
	{
		Init();
	}

	PARVARCOPY(HexesVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		// Xh = (Xo + sqrt(3) * Yo) / (3 * l)
		static const T AXhXo = T(1.0 / 3.0);
		static const T AXhYo = T(1.7320508075688772935 / 3.0);
		// Now:  Xh = ( AXhXo * Xo + AXhYo * Yo ) / l;
		// Yh = (-Xo + sqrt(3) * Yo) / (3 * l)
		static const T AYhXo = T(-1.0 / 3.0);
		static const T AYhYo = T(1.7320508075688772935 / 3.0);
		// Now:  Yh = ( AYhXo * Xo + AYhYo * Yo ) / l;
		// Xo = 3/2 * l * (Xh - Yh)
		static const T AXoXh = T(1.5);
		static const T AXoYh = T(-1.5);
		// Now:  Xo = ( AXoXh * Xh + AXoYh * Yh ) * l;
		// Yo = sqrt(3)/2 * l * (Xh + Yh)
		static const T AYoXh = T(1.7320508075688772935 / 2.0);
		static const T AYoYh = T(1.7320508075688772935 / 2.0);
		static const v2T offset[4] { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };
		int i = 0;
		T di, dj;
		T XCh, YCh, XCo, YCo, DXo, DYo, L, L1, L2, R, s, trgL;
		v2T u, v;
		v2T P[7];
		//For speed/convenience.
		s = m_Cellsize;

		//Infinite number of small cells? No effect . . .
		if (s == 0)
			return;

		//Get co-ordinates, and convert to hex co-ordinates.
		u.x = helper.In.x;
		u.y = helper.In.y;
		XCh = T(Floor((AXhXo * u.x + AXhYo * u.y) / s));
		YCh = T(Floor((AYhXo * u.x + AYhYo * u.y) / s));

		// Get a set of 4 hex center points, based around the one above
		for (di = XCh; di < XCh + T(1.1); di += 1)
		{
			for (dj = YCh; dj < YCh + T(1.1); dj += 1)
			{
				P[i].x = (AXoXh * di + AXoYh * dj) * s;
				P[i].y = (AYoXh * di + AYoYh * dj) * s;
				i++;
			}
		}

		int q = m_VarFuncs->Closest(&P[0], 4, u);
		//Remake list starting from chosen hex, ensure it is completely surrounded (total 7 points).
		//First adjust centers according to which one was found to be closest.
		XCh += offset[q].x;
		YCh += offset[q].y;
		//First point is central/closest.
		XCo = (AXoXh * XCh + AXoYh * YCh) * s;
		YCo = (AYoXh * XCh + AYoYh * YCh) * s;
		P[0].x = XCo;
		P[0].y = YCo;
		//Next six points are based on hex graph (6 hexes around center). As long as
		//center points are not too distorted from simple hex, this defines all possible edges.
		//In hex co-ords, offsets are: (0,1) (1,1) (1,0) (0,-1) (-1,-1) (-1, 0).
		P[1].x = XCo + (AXoYh) * s;
		P[1].y = YCo + (AYoYh) * s;
		P[2].x = XCo + (AXoXh + AXoYh) * s;
		P[2].y = YCo + (AYoXh + AYoYh) * s;
		P[3].x = XCo + (AXoXh) * s;
		P[3].y = YCo + (AYoXh) * s;
		P[4].x = XCo - AXoYh * s;
		P[4].y = YCo - AYoYh * s;
		P[5].x = XCo - (AXoXh + AXoYh) * s;
		P[5].y = YCo - (AYoXh + AYoYh) * s;
		P[6].x = XCo - AXoXh * s;
		P[6].y = YCo - AYoXh * s;
		L1 = m_VarFuncs->Voronoi(&P[0], 7, 0, u);
		//Delta vector from center of hex.
		DXo = u.x - P[0].x;
		DYo = u.y - P[0].y;
		//Apply "interesting bit" to cell's DXo and DYo co-ordinates.
		//trgL is the defined value of l, independent of any rotation.
		trgL = std::pow(Zeps(L1), m_Power) * m_Scale;//Original added 1e-100, use Zeps to be more precise.
		//Rotate.
		v.x = DXo * m_RotCos + DYo * m_RotSin;
		v.y = -DXo * m_RotSin + DYo * m_RotCos;
		//Measure voronoi distance again.
		u = v + P[0];
		L2 = m_VarFuncs->Voronoi(&P[0], 7, 0, u);
		//Scale to meet target size . . . adjust according to how close
		//we are to the edge.
		//Code here attempts to remove the "rosette" effect caused by
		//scaling between.
		//L is maximum of L1 or L2 . . .
		//When L = 0.8 or higher . . . match trgL/L2 exactly.
		//When L = T(0.5) or less . . . match trgL/L1 exactly.
		L = (L1 > L2) ? L1 : L2;

		if (L < T(0.5))
		{
			R = trgL / L1;
		}
		else
		{
			if (L > T(0.8))
				R = trgL / L2;
			else
				R = ((trgL / L1) * (T(0.8) - L) + (trgL / L2) * (L - T(0.5))) / T(0.3);
		}

		v *= R;
		//Add cell center co-ordinates back in.
		v += P[0];
		//Finally add values in.
		helper.Out.x = m_Weight * v.x;
		helper.Out.y = m_Weight * v.y;
		helper.Out.z = (m_VarType == VARTYPE_REG) ? 0 : helper.In.z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0, varIndex = IndexInXform();
		ss2 << "_" << XformIndexInEmber() << "]";
		string index = ss2.str();
		string cellsize = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string power	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string rotate	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string scale	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string rotsin	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string rotcos	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   << "\t\tint i = 0;\n"
		   << "\t\treal_t di, dj;\n"
		   << "\t\treal_t XCh, YCh, XCo, YCo, DXo, DYo, L, L1, L2, R, s, trgL, Vx, Vy;\n"
		   << "\t\treal2 U;\n"
		   << "\t\treal2 P[7];\n"
		   << "\n"
		   << "\t\ts = " << cellsize << ";\n"
		   << "\n"
		   << "\t\tif (s == 0)\n"
		   << "\t\t	return;\n"
		   << "\n"
		   << "\t\tU.x = vIn.x;\n"
		   << "\t\tU.y = vIn.y;\n"
		   << "\n"
		   << "\t\tXCh = floor((AXhXo * U.x + AXhYo * U.y) / s);\n"
		   << "\t\tYCh = floor((AYhXo * U.x + AYhYo * U.y) / s);\n"
		   << "\n"
		   << "\t\tfor (di = XCh; di < XCh + 1.1; di += 1)\n"
		   << "\t\t{\n"
		   << "\t\t	for (dj = YCh; dj < YCh + 1.1; dj += 1)\n"
		   << "\t\t	{\n"
		   << "\t\t		P[i].x = (AXoXh * di + AXoYh * dj) * s;\n"
		   << "\t\t		P[i].y = (AYoXh * di + AYoYh * dj) * s;\n"
		   << "\t\t		i++;\n"
		   << "\t\t	}\n"
		   << "\t\t}\n"
		   << "\n"
		   << "\t\tint q = Closest(&P[0], 4, &U);\n"
		   << "\n"
		   << "\t\tXCh += offset[q].x;\n"
		   << "\t\tYCh += offset[q].y;\n"
		   << "\n"
		   << "\t\tXCo = (AXoXh * XCh + AXoYh * YCh) * s;\n"
		   << "\t\tYCo = (AYoXh * XCh + AYoYh * YCh) * s;\n"
		   << "\t\tP[0].x = XCo;\n"
		   << "\t\tP[0].y = YCo;\n"
		   << "\n"
		   << "\t\tP[1].x = XCo + (AXoYh)* s;\n"
		   << "\t\tP[1].y = YCo + (AYoYh)* s;\n"
		   << "\t\tP[2].x = XCo + (AXoXh + AXoYh) * s;\n"
		   << "\t\tP[2].y = YCo + (AYoXh + AYoYh) * s;\n"
		   << "\t\tP[3].x = XCo + (AXoXh)* s;\n"
		   << "\t\tP[3].y = YCo + (AYoXh)* s;\n"
		   << "\t\tP[4].x = XCo - AXoYh * s;\n"
		   << "\t\tP[4].y = YCo - AYoYh * s;\n"
		   << "\t\tP[5].x = XCo - (AXoXh + AXoYh) * s;\n"
		   << "\t\tP[5].y = YCo - (AYoXh + AYoYh) * s;\n"
		   << "\t\tP[6].x = XCo - AXoXh * s;\n"
		   << "\t\tP[6].y = YCo - AYoXh * s;\n"
		   << "\n"
		   << "\t\tL1 = Voronoi(&P[0], 7, 0, &U);\n"
		   << "\n"
		   << "\t\tDXo = U.x - P[0].x;\n"
		   << "\t\tDYo = U.y - P[0].y;\n"
		   << "\n"
		   << "\t\ttrgL = pow(Zeps(L1), " << power << ") * " << scale << ";\n"
		   << "\n"
		   << "\t\tVx = DXo * " << rotcos << " + DYo * " << rotsin << ";\n"
		   << "\t\tVy = -DXo * " << rotsin << " + DYo * " << rotcos << ";\n"
		   << "\n"
		   << "\t\tU.x = Vx + P[0].x;\n"
		   << "\t\tU.y = Vy + P[0].y;\n"
		   << "\t\tL2 = Voronoi(&P[0], 7, 0, &U);\n"
		   << "\n"
		   << "\t\tL = (L1 > L2) ? L1 : L2;\n"
		   << "\n"
		   << "\t\tif (L < 0.5)\n"
		   << "\t\t{\n"
		   << "\t\t	R = trgL / L1;\n"
		   << "\t\t}\n"
		   << "\t\telse\n"
		   << "\t\t{\n"
		   << "\t\t	if (L > 0.8)\n"
		   << "\t\t		R = trgL / L2;\n"
		   << "\t\t	else\n"
		   << "\t\t		R = ((trgL / L1) * (0.8 - L) + (trgL / L2) * (L - 0.5)) / 0.3;\n"
		   << "\t\t}\n"
		   << "\n"
		   << "\t\tVx *= R;\n"
		   << "\t\tVy *= R;\n"
		   << "\n"
		   << "\t\tVx += P[0].x;\n"
		   << "\t\tVy += P[0].y;\n"
		   << "\n"
		   << "\t\tvOut.x = xform->m_VariationWeights[" << varIndex << "] * Vx;\n"
		   << "\t\tvOut.y = xform->m_VariationWeights[" << varIndex << "] * Vy;\n"
		   << "\t\tvOut.z = " << ((m_VarType == VARTYPE_REG) ? "0" : "vIn.z") << ";\n"
		   << "\t}\n";
		return ss.str();
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Zeps", "Sqr", "Vratio", "Closest", "Vratio" };
	}

	virtual string OpenCLFuncsString() const override
	{
		return
			"constant real_t AXhXo = (real_t)(1.0 / 3.0);\n"
			"constant real_t AXhYo = (real_t)(1.7320508075688772935 / 3.0);\n"
			"constant real_t AYhXo = (real_t)(-1.0 / 3.0);\n"
			"constant real_t AYhYo = (real_t)(1.7320508075688772935 / 3.0);\n"
			"constant real_t AXoXh = (real_t)(1.5);\n"
			"constant real_t AXoYh = (real_t)(-1.5);\n"
			"constant real_t AYoXh = (real_t)(1.7320508075688772935 / 2.0);\n"
			"constant real_t AYoYh = (real_t)(1.7320508075688772935 / 2.0);\n"
			"constant real2 offset[4] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };\n"
			"\n";
	}

	virtual void Precalc() override
	{
		m_RotSin = std::sin(m_Rotate * M_2PI);
		m_RotCos = std::cos(m_Rotate * M_2PI);
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.push_back(ParamWithName<T>(&m_Cellsize, prefix + "hexes_cellsize", 1));
		m_Params.push_back(ParamWithName<T>(&m_Power,	 prefix + "hexes_power", 1));
		m_Params.push_back(ParamWithName<T>(&m_Rotate,	 prefix + "hexes_rotate", T(0.166)));
		m_Params.push_back(ParamWithName<T>(&m_Scale,	 prefix + "hexes_scale", 1));
		m_Params.push_back(ParamWithName<T>(true, &m_RotSin, prefix + "hexes_rotsin"));//Precalc.
		m_Params.push_back(ParamWithName<T>(true, &m_RotCos, prefix + "hexes_rotcos"));
		m_VarFuncs = VarFuncs<T>::Instance();
	}

private:
	T m_Cellsize;
	T m_Power;
	T m_Rotate;
	T m_Scale;
	T m_RotSin;//Precalc.
	T m_RotCos;
	std::shared_ptr<VarFuncs<T>> m_VarFuncs;
};

/// <summary>
/// nBlur.
/// </summary>
template <typename T>
class EMBER_API NblurVariation : public ParametricVariation<T>
{
	struct RandXyParams
	{
		T NumEdges;
		T RatioHole;
		T CircumCircle;
		T EqualBlur;
		T ExactCalc;
		T MidAngle;
		T AngStart;
		T AngStripes;
		T HasStripes;
		T NegStripes;
		T MaxStripes;
		T Tan90M2;
		T ArcTan1;
		T ArcTan2;
		T RatioStripes;
		T RatioComplement;
		T SpeedCalc1;
		T SpeedCalc2;
		T LenInnerEdges;
		T LenOuterEdges;
		T X;
		T Y;
		T LenXY;
	};

public:
	NblurVariation(T weight = 1.0) : ParametricVariation<T>("nBlur", VAR_NBLUR, weight)
	{
		Init();
	}

	PARVARCOPY(NblurVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		T xTmp, yTmp;
		RandXyParams params;
		params.NumEdges = m_NumEdges;
		params.RatioHole = m_RatioHole;
		params.CircumCircle = m_CircumCircle;
		params.EqualBlur = m_EqualBlur;
		params.ExactCalc = m_ExactCalc;
		params.MidAngle = m_MidAngle;
		params.AngStart = m_AngStart;
		params.AngStripes = m_AngStripes;
		params.HasStripes = m_HasStripes;
		params.NegStripes = m_NegStripes;
		params.MaxStripes = m_MaxStripes;
		params.Tan90M2 = m_Tan90M2;
		params.ArcTan1 = m_ArcTan1;
		params.ArcTan2 = m_ArcTan2;
		params.RatioStripes = m_RatioStripes;
		params.RatioComplement = m_RatioComplement;
		params.SpeedCalc1 = m_SpeedCalc1;
		params.SpeedCalc2 = m_SpeedCalc2;
		RandXY(params, rand);

		if ((m_ExactCalc == 1) && (m_CircumCircle == 0))
			while ((params.LenXY < params.LenInnerEdges) || (params.LenXY > params.LenOuterEdges))
				RandXY(params, rand);

		if ((m_ExactCalc == 1) && (m_CircumCircle == 1))
			while (params.LenXY < params.LenInnerEdges)
				RandXY(params, rand);

		xTmp = params.X;
		yTmp = params.Y;
		params.X = m_Cosa * xTmp - m_Sina * yTmp;
		params.Y = m_Sina * xTmp + m_Cosa * yTmp;
		helper.Out.x = m_AdjustedWeight * params.X;
		helper.Out.y = m_AdjustedWeight * params.Y;
		helper.Out.z = (m_VarType == VARTYPE_REG) ? 0 : helper.In.z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0;
		ss2 << "_" << XformIndexInEmber() << "]";
		string index = ss2.str();
		string numEdges		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string numStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string ratioStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string ratioHole	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string circumCircle	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string adjustToLinear  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string equalBlur	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string exactCalc	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string highlightEdges  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string ratioComplement = "parVars[" + ToUpper(m_Params[i++].Name()) + index;//Precalc.
		string midAngle		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angStart		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string hasStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string negStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string maxStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string absStripes	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string sina			   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string cosa			   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string tan90M2		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string arcTan1		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string arcTan2		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string speedCalc1	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string speedCalc2	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string adjustedWeight  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   << "\t\treal_t xTmp, yTmp;\n"
		   << "\t\tRandXyParams params;\n"
		   << "\n"
		   << "\t\tparams.NumEdges = " << numEdges << ";\n"
		   << "\t\tparams.RatioHole = " << ratioHole << ";\n"
		   << "\t\tparams.CircumCircle = " << circumCircle << ";\n"
		   << "\t\tparams.EqualBlur = " << equalBlur << ";\n"
		   << "\t\tparams.ExactCalc = " << exactCalc << ";\n"
		   << "\t\tparams.MidAngle = " << midAngle << ";\n"
		   << "\t\tparams.AngStart = " << angStart << ";\n"
		   << "\t\tparams.AngStripes = " << angStripes << ";\n"
		   << "\t\tparams.HasStripes = " << hasStripes << ";\n"
		   << "\t\tparams.NegStripes = " << negStripes << ";\n"
		   << "\t\tparams.MaxStripes = " << maxStripes << ";\n"
		   << "\t\tparams.Tan90M2 = " << tan90M2 << ";\n"
		   << "\t\tparams.ArcTan1 = " << arcTan1 << ";\n"
		   << "\t\tparams.ArcTan2 = " << arcTan2 << ";\n"
		   << "\t\tparams.RatioStripes = " << ratioStripes << ";\n"
		   << "\t\tparams.RatioComplement = " << ratioComplement << ";\n"
		   << "\t\tparams.SpeedCalc1 = " << speedCalc1 << ";\n"
		   << "\t\tparams.SpeedCalc2 = " << speedCalc2 << ";\n"
		   << "\n"
		   << "\t\tRandXY(&params, mwc);\n"
		   << "\n"
		   << "\t\tif ((" << exactCalc << " == 1) && (" << circumCircle << " == 0))\n"
		   << "\t\t	while ((params.LenXY < params.LenInnerEdges) || (params.LenXY > params.LenOuterEdges))\n"
		   << "\t\t		RandXY(&params, mwc);\n"
		   << "\n"
		   << "\t\tif ((" << exactCalc << " == 1) && (" << circumCircle << " == 1))\n"
		   << "\t\t	while (params.LenXY < params.LenInnerEdges)\n"
		   << "\t\t		RandXY(&params, mwc);\n"
		   << "\n"
		   << "\t\txTmp = params.X;\n"
		   << "\t\tyTmp = params.Y;\n"
		   << "\n"
		   << "\t\tparams.X = " << cosa << " * xTmp - " << sina << " * yTmp;\n"
		   << "\t\tparams.Y = " << sina << " * xTmp + " << cosa << " * yTmp;\n"
		   << "\n"
		   << "\t\tvOut.x = " << adjustedWeight << " * params.X;\n"
		   << "\t\tvOut.y = " << adjustedWeight << " * params.Y;\n"
		   << "\t\tvOut.z = " << ((m_VarType == VARTYPE_REG) ? "0" : "vIn.z") << ";\n"
		   << "\t}\n";
		return ss.str();
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Swap" };
	}

	virtual string OpenCLFuncsString() const override
	{
		return
			"typedef struct __attribute__ " ALIGN_CL " _RandXyParams\n"
			"{\n"
			"	real_t NumEdges;\n"
			"	real_t RatioHole;\n"
			"	real_t CircumCircle;\n"
			"	real_t EqualBlur;\n"
			"	real_t ExactCalc;\n"
			"	real_t MidAngle;\n"
			"	real_t AngStart;\n"
			"	real_t AngStripes;\n"
			"	real_t HasStripes;\n"
			"	real_t NegStripes;\n"
			"	real_t MaxStripes;\n"
			"	real_t Tan90M2;\n"
			"	real_t ArcTan1;\n"
			"	real_t ArcTan2;\n"
			"	real_t RatioStripes;\n"
			"	real_t RatioComplement;\n"
			"	real_t SpeedCalc1;\n"
			"	real_t SpeedCalc2;\n"
			"	real_t LenInnerEdges;\n"
			"	real_t LenOuterEdges;\n"
			"	real_t X;\n"
			"	real_t Y;\n"
			"	real_t LenXY;\n"
			"} RandXyParams;\n"
			"\n"
			"static void RandXY(RandXyParams* params, uint2* mwc)\n"
			"{\n"
			"	real_t angXY, angMem, angTmp;\n"
			"	real_t ratioTmp, ratioTmpNum, ratioTmpDen;\n"
			"	real_t xTmp, yTmp;\n"
			"	real_t ranTmp;\n"
			"	int count;\n"
			"\n"
			"	if (params->ExactCalc == 1)\n"
			"		angXY = MwcNext01(mwc) * M_2PI;\n"
			"	else\n"
			"		angXY = (atan(params->ArcTan1 * (MwcNext01(mwc) - 0.5)) / params->ArcTan2 + 0.5 + (real_t)(MwcNextRange(mwc, (uint)params->NumEdges))) * params->MidAngle;\n"
			"\n"
			"	params->X = sincos(angXY, &params->Y);\n"
			"	angMem = angXY;\n"
			"\n"
			"	while (angXY > params->MidAngle)\n"
			"		angXY -= params->MidAngle;\n"
			"\n"
			"	if (params->HasStripes == 1)\n"
			"	{\n"
			"		count = 0;\n"
			"		angTmp = params->AngStart;\n"
			"\n"
			"		while (angXY > angTmp)\n"
			"		{\n"
			"			angTmp += params->AngStripes;\n"
			"\n"
			"			if (angTmp > params->MidAngle)\n"
			"				angTmp = params->MidAngle;\n"
			"\n"
			"			count++;\n"
			"		}\n"
			"\n"
			"		if (angTmp != params->MidAngle)\n"
			"			angTmp -= params->AngStart;\n"
			"\n"
			"		if (params->NegStripes == 0)\n"
			"		{\n"
			"			if ((count & 1) == 1)\n"
			"			{\n"
			"				if (angXY > angTmp)\n"
			"				{\n"
			"					angXY = angXY + params->AngStart;\n"
			"					angMem = angMem + params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"					angTmp += params->AngStripes;\n"
			"					count++;\n"
			"				}\n"
			"				else\n"
			"				{\n"
			"					angXY = angXY - params->AngStart;\n"
			"					angMem = angMem - params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"					angTmp -= params->AngStripes;\n"
			"					count--;\n"
			"				}\n"
			"			}\n"
			"\n"
			"			if (((count & 1) == 0) && (params->RatioStripes > 1))\n"
			"			{\n"
			"				if ((angXY > angTmp) && (count != params->MaxStripes))\n"
			"				{\n"
			"					angMem = angMem - angXY + angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					angXY = angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"				}\n"
			"				else\n"
			"				{\n"
			"					angMem = angMem - angXY + angTmp - (angTmp - angXY) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					angXY = angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"				}\n"
			"			}\n"
			"\n"
			"			if (((count & 1) == 0) && (params->RatioStripes < 1))\n"
			"			{\n"
			"				if ((fabs(angXY - angTmp) > params->SpeedCalc2) && (count != params->MaxStripes))\n"
			"				{\n"
			"					if ((angXY - angTmp) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angXY - (angTmp + params->SpeedCalc2)) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp + ratioTmp), &params->Y);\n"
			"						angXY = angTmp + ratioTmp;\n"
			"					}\n"
			"\n"
			"					if ((angTmp - angXY) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angTmp - params->SpeedCalc2 - angXY) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp - ratioTmp), &params->Y);\n"
			"						angXY = angTmp - ratioTmp;\n"
			"					}\n"
			"				}\n"
			"\n"
			"				if (count == params->MaxStripes)\n"
			"				{\n"
			"					if ((angTmp - angXY) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angTmp - params->SpeedCalc2 - angXY) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp - ratioTmp), &params->Y);\n"
			"						angXY = angTmp - ratioTmp;\n"
			"					}\n"
			"				}\n"
			"			}\n"
			"		}\n"
			"		else\n"
			"		{\n"
			"			Swap(&params->RatioStripes, &params->RatioComplement);\n"
			"			Swap(&params->SpeedCalc1, &params->SpeedCalc2);\n"
			"\n"
			"			if ((count & 1) == 0)\n"
			"			{\n"
			"				if ((angXY > angTmp) && (count != params->MaxStripes))\n"
			"				{\n"
			"					angXY = angXY + params->AngStart;\n"
			"					angMem = angMem + params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"					angTmp += params->AngStripes;\n"
			"					count++;\n"
			"				}\n"
			"				else\n"
			"				{\n"
			"					angXY = angXY - params->AngStart;\n"
			"					angMem = angMem - params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"					angTmp -= params->AngStripes;\n"
			"					count--;\n"
			"				}\n"
			"			}\n"
			"\n"
			"			if (((count & 1) == 1) && (params->RatioStripes > 1))\n"
			"			{\n"
			"				if ((angXY > angTmp) && (count != params->MaxStripes))\n"
			"				{\n"
			"					angMem = angMem - angXY + angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					angXY = angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"				}\n"
			"				else\n"
			"				{\n"
			"					angMem = angMem - angXY + angTmp - (angTmp - angXY) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					angXY = angTmp + (angXY - angTmp) / params->AngStart * params->RatioStripes * params->AngStart;\n"
			"					params->X = sincos(angMem, &params->Y);\n"
			"				}\n"
			"			}\n"
			"\n"
			"			if (((count & 1) == 1) && (params->RatioStripes < 1))\n"
			"			{\n"
			"				if ((fabs(angXY - angTmp) > params->SpeedCalc2) && (count != params->MaxStripes))\n"
			"				{\n"
			"					if ((angXY - angTmp) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angXY - (angTmp + params->SpeedCalc2)) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp + ratioTmp), &params->Y);\n"
			"						angXY = angTmp + ratioTmp;\n"
			"					}\n"
			"\n"
			"					if ((angTmp - angXY) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angTmp - params->SpeedCalc2 - angXY) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp - ratioTmp), &params->Y);\n"
			"						angXY = angTmp - ratioTmp;\n"
			"					}\n"
			"				}\n"
			"\n"
			"				if (count == params->MaxStripes)\n"
			"				{\n"
			"					angTmp = params->MidAngle;\n"
			"\n"
			"					if ((angTmp - angXY) > params->SpeedCalc2)\n"
			"					{\n"
			"						ratioTmpNum = (angTmp - params->SpeedCalc2 - angXY) * params->SpeedCalc2;\n"
			"						ratioTmpDen = params->AngStart - params->SpeedCalc2;\n"
			"						ratioTmp = ratioTmpNum / ratioTmpDen;\n"
			"						params->X = sincos((angMem - angXY + angTmp - ratioTmp), &params->Y);\n"
			"						angXY = angTmp - ratioTmp;\n"
			"					}\n"
			"				}\n"
			"			}\n"
			"\n"
			"			Swap(&params->RatioStripes, &params->RatioComplement);\n"
			"			Swap(&params->SpeedCalc1, &params->SpeedCalc2);\n"
			"		}\n"
			"	}\n"
			"\n"
			"	xTmp = params->Tan90M2 / (params->Tan90M2 - tan(angXY));\n"
			"	yTmp = xTmp * tan(angXY);\n"
			"	params->LenOuterEdges = sqrt(SQR(xTmp) + SQR(yTmp));\n"
			"\n"
			"	if (params->ExactCalc == 1)\n"
			"	{\n"
			"		if (params->EqualBlur == 1)\n"
			"			ranTmp = sqrt(MwcNext01(mwc));\n"
			"		else\n"
			"			ranTmp = MwcNext01(mwc);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		if (params->CircumCircle == 1)\n"
			"		{\n"
			"			if (params->EqualBlur == 1)\n"
			"				ranTmp = sqrt(MwcNext01(mwc));\n"
			"			else\n"
			"				ranTmp = MwcNext01(mwc);\n"
			"		}\n"
			"		else\n"
			"		{\n"
			"			if (params->EqualBlur == 1)\n"
			"				ranTmp = sqrt(MwcNext01(mwc)) * params->LenOuterEdges;\n"
			"			else\n"
			"				ranTmp = MwcNext01(mwc) * params->LenOuterEdges;\n"
			"		}\n"
			"	}\n"
			"\n"
			"	params->LenInnerEdges = params->RatioHole * params->LenOuterEdges;\n"
			"\n"
			"	if (params->ExactCalc == 0)\n"
			"	{\n"
			"		if (ranTmp < params->LenInnerEdges)\n"
			"		{\n"
			"			if (params->CircumCircle == 1)\n"
			"			{\n"
			"				if (params->EqualBlur == 1)\n"
			"					ranTmp = params->LenInnerEdges + sqrt(MwcNext01(mwc)) * (1 - params->LenInnerEdges + EPS);\n"
			"				else\n"
			"					ranTmp = params->LenInnerEdges + MwcNext01(mwc) * (1 - params->LenInnerEdges + EPS);\n"
			"			}\n"
			"			else\n"
			"			{\n"
			"				if (params->EqualBlur == 1)\n"
			"					ranTmp = params->LenInnerEdges + sqrt(MwcNext01(mwc)) * (params->LenOuterEdges - params->LenInnerEdges);\n"
			"				else\n"
			"					ranTmp = params->LenInnerEdges + MwcNext01(mwc) * (params->LenOuterEdges - params->LenInnerEdges);\n"
			"			}\n"
			"		}\n"
			"	}\n"
			"\n"
			"	params->X *= ranTmp;\n"
			"	params->Y *= ranTmp;\n"
			"	params->LenXY = sqrt(SQR(params->X) + SQR(params->Y));\n"
			"}\n\n"
			;
	}

	virtual void Precalc() override
	{
		if (m_NumEdges < 3)
			m_NumEdges = 3;

		if (m_NumStripes != 0)
		{
			m_HasStripes = 1;

			if (m_NumStripes < 0)
			{
				m_NegStripes = 1;
				//m_NumStripes *= -1;
			}
			else
				m_NegStripes = 0;
		}
		else
		{
			m_HasStripes = 0;
			m_NegStripes = 0;
		}

		m_AbsStripes = std::fabs(m_NumStripes);
		m_MidAngle = M_2PI / m_NumEdges;

		if (m_HasStripes == 1)
		{
			m_AngStripes = m_MidAngle / (2 * m_AbsStripes);
			m_AngStart = m_AngStripes / 2;
			m_RatioComplement = 2 - m_RatioStripes;
		}

		if ((m_RatioHole > T(0.95)) && (m_ExactCalc == 1) && (m_CircumCircle == 0))
			m_RatioHole = T(0.95);

		m_Tan90M2 = std::tan(T(M_PI_2) + m_MidAngle / 2);
		sincos((m_MidAngle / 2), &m_Sina, &m_Cosa);

		if (m_HighlightEdges <= 0.1)
			m_HighlightEdges = T(0.1);

		if (m_AdjustToLinear == 1)
		{
			if (int(m_NumEdges) % 4 == 0)
			{
				m_AdjustedWeight = m_Weight / (std::sqrt(2 - 2 * std::cos(m_MidAngle * (m_NumEdges / 2 - 1))) / 2);
			}
			else
			{
				m_AdjustedWeight = m_Weight / (std::sqrt(2 - 2 * std::cos(m_MidAngle * std::floor((m_NumEdges / 2)))) / 2);
			}
		}
		else
		{
			m_AdjustedWeight = m_Weight;
		}

		if (m_CircumCircle == 1)
		{
			m_ExactCalc = 0;
			m_HighlightEdges = T(0.1);
		}

		m_SpeedCalc1 = m_RatioComplement * m_AngStart;
		m_SpeedCalc2 = m_RatioStripes * m_AngStart;
		m_MaxStripes = 2 * m_AbsStripes;

		if (m_NegStripes == 0)
		{
			m_ArcTan1 = (13 / std::pow(m_NumEdges, T(1.3))) * m_HighlightEdges;
			m_ArcTan2 = (2 * std::atan(m_ArcTan1 / -2));
		}
		else
		{
			m_ArcTan1 = (T(7.5) / std::pow(m_NumEdges, T(1.3))) * m_HighlightEdges;
			m_ArcTan2 = 2 * std::atan(m_ArcTan1 / -2);
		}
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.reserve(25);
		m_Params.push_back(ParamWithName<T>(&m_NumEdges,	   prefix + "nBlur_numEdges", 3, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_NumStripes,	   prefix + "nBlur_numStripes", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_RatioStripes,   prefix + "nBlur_ratioStripes", 1, REAL, 0, 2));
		m_Params.push_back(ParamWithName<T>(&m_RatioHole,	   prefix + "nBlur_ratioHole", 0, REAL, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_CircumCircle,   prefix + "nBlur_circumCircle", 0, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_AdjustToLinear, prefix + "nBlur_adjustToLinear", 1, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_EqualBlur,	   prefix + "nBlur_equalBlur", 1, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_ExactCalc,	   prefix + "nBlur_exactCalc", 0, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_HighlightEdges, prefix + "nBlur_highlightEdges", 1));
		m_Params.push_back(ParamWithName<T>(true, &m_RatioComplement, prefix + "nBlur_ratioComplement"));//Precalc.
		m_Params.push_back(ParamWithName<T>(true, &m_MidAngle,		  prefix + "nBlur_midAngle"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngStart,		  prefix + "nBlur_angStart"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngStripes,	  prefix + "nBlur_angStripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_HasStripes,	  prefix + "nBlur_hasStripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_NegStripes,	  prefix + "nBlur_negStripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_MaxStripes,	  prefix + "nBlur_maxStripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_AbsStripes,	  prefix + "nBlur_absStripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_Sina,			  prefix + "nBlur_sina"));
		m_Params.push_back(ParamWithName<T>(true, &m_Cosa,			  prefix + "nBlur_cosa"));
		m_Params.push_back(ParamWithName<T>(true, &m_Tan90M2,		  prefix + "nBlur_tan90m2"));
		m_Params.push_back(ParamWithName<T>(true, &m_ArcTan1,		  prefix + "nBlur_arcTan1"));
		m_Params.push_back(ParamWithName<T>(true, &m_ArcTan2,		  prefix + "nBlur_arcTan2"));
		m_Params.push_back(ParamWithName<T>(true, &m_SpeedCalc1,	  prefix + "nBlur_speedCalc1"));
		m_Params.push_back(ParamWithName<T>(true, &m_SpeedCalc2,	  prefix + "nBlur_speedCalc2"));
		m_Params.push_back(ParamWithName<T>(true, &m_AdjustedWeight,  prefix + "nBlur_adjustedWeight"));
	}

private:
	static void RandXY(RandXyParams& params, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand)
	{
		T angXY, angMem, angTmp;
		T ratioTmp, ratioTmpNum, ratioTmpDen;
		T xTmp, yTmp;
		T ranTmp;
		int count;

		if (params.ExactCalc == 1)
			angXY = rand.Frand01<T>() * M_2PI;
		else
			angXY = (std::atan(params.ArcTan1 * (rand.Frand01<T>() - T(0.5))) / params.ArcTan2 + T(0.5) + T(rand.Rand() % glm::uint(params.NumEdges))) * params.MidAngle;

		sincos(angXY, &params.X, &params.Y);
		angMem = angXY;

		while (angXY > params.MidAngle)
			angXY -= params.MidAngle;

		if (params.HasStripes == 1)
		{
			count = 0;
			angTmp = params.AngStart;

			while (angXY > angTmp)
			{
				angTmp += params.AngStripes;

				if (angTmp > params.MidAngle)
					angTmp = params.MidAngle;

				count++;
			}

			if (angTmp != params.MidAngle)
				angTmp -= params.AngStart;

			if (params.NegStripes == 0)
			{
				if ((count & 1) == 1)
				{
					if (angXY > angTmp)
					{
						angXY = angXY + params.AngStart;
						angMem = angMem + params.AngStart;
						sincos(angMem, &params.X, &params.Y);
						angTmp += params.AngStripes;
						count++;
					}
					else
					{
						angXY = angXY - params.AngStart;
						angMem = angMem - params.AngStart;
						sincos(angMem, &params.X, &params.Y);
						angTmp -= params.AngStripes;
						count--;
					}
				}

				if (((count & 1) == 0) && (params.RatioStripes > 1))
				{
					if ((angXY > angTmp) && (count != params.MaxStripes))
					{
						angMem = angMem - angXY + angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						angXY = angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						sincos(angMem, &params.X, &params.Y);
					}
					else
					{
						angMem = angMem - angXY + angTmp - (angTmp - angXY) / params.AngStart * params.RatioStripes * params.AngStart;
						angXY = angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						sincos(angMem, &params.X, &params.Y);
					}
				}

				if (((count & 1) == 0) && (params.RatioStripes < 1))
				{
					if ((std::fabs(angXY - angTmp) > params.SpeedCalc2) && (count != params.MaxStripes))
					{
						if ((angXY - angTmp) > params.SpeedCalc2)
						{
							ratioTmpNum = (angXY - (angTmp + params.SpeedCalc2)) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp + ratioTmp), &params.X, &params.Y);
							angXY = angTmp + ratioTmp;
						}

						if ((angTmp - angXY) > params.SpeedCalc2)
						{
							ratioTmpNum = (angTmp - params.SpeedCalc2 - angXY) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp - ratioTmp), &params.X, &params.Y);
							angXY = angTmp - ratioTmp;
						}
					}

					if (count == params.MaxStripes)
					{
						if ((angTmp - angXY) > params.SpeedCalc2)
						{
							ratioTmpNum = (angTmp - params.SpeedCalc2 - angXY) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp - ratioTmp), &params.X, &params.Y);
							angXY = angTmp - ratioTmp;
						}
					}
				}
			}
			else
			{
				std::swap(params.RatioStripes, params.RatioComplement);
				std::swap(params.SpeedCalc1, params.SpeedCalc2);

				if ((count & 1) == 0)
				{
					if ((angXY > angTmp) && (count != params.MaxStripes))
					{
						angXY = angXY + params.AngStart;
						angMem = angMem + params.AngStart;
						sincos(angMem, &params.X, &params.Y);
						angTmp += params.AngStripes;
						count++;
					}
					else
					{
						angXY = angXY - params.AngStart;
						angMem = angMem - params.AngStart;
						sincos(angMem, &params.X, &params.Y);
						angTmp -= params.AngStripes;
						count--;
					}
				}

				if (((count & 1) == 1) && (params.RatioStripes > 1))
				{
					if ((angXY > angTmp) && (count != params.MaxStripes))
					{
						angMem = angMem - angXY + angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						angXY = angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						sincos(angMem, &params.X, &params.Y);
					}
					else
					{
						angMem = angMem - angXY + angTmp - (angTmp - angXY) / params.AngStart * params.RatioStripes * params.AngStart;
						angXY = angTmp + (angXY - angTmp) / params.AngStart * params.RatioStripes * params.AngStart;
						sincos(angMem, &params.X, &params.Y);
					}
				}

				if (((count & 1) == 1) && (params.RatioStripes < 1))
				{
					if ((std::fabs(angXY - angTmp) > params.SpeedCalc2) && (count != params.MaxStripes))
					{
						if ((angXY - angTmp) > params.SpeedCalc2)
						{
							ratioTmpNum = (angXY - (angTmp + params.SpeedCalc2)) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp + ratioTmp), &params.X, &params.Y);
							angXY = angTmp + ratioTmp;
						}

						if ((angTmp - angXY) > params.SpeedCalc2)
						{
							ratioTmpNum = (angTmp - params.SpeedCalc2 - angXY) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp - ratioTmp), &params.X, &params.Y);
							angXY = angTmp - ratioTmp;
						}
					}

					if (count == params.MaxStripes)
					{
						angTmp = params.MidAngle;

						if ((angTmp - angXY) > params.SpeedCalc2)
						{
							ratioTmpNum = (angTmp - params.SpeedCalc2 - angXY) * params.SpeedCalc2;
							ratioTmpDen = params.AngStart - params.SpeedCalc2;
							ratioTmp = ratioTmpNum / ratioTmpDen;
							sincos((angMem - angXY + angTmp - ratioTmp), &params.X, &params.Y);
							angXY = angTmp - ratioTmp;
						}
					}
				}

				std::swap(params.RatioStripes, params.RatioComplement);
				std::swap(params.SpeedCalc1, params.SpeedCalc2);
			}
		}

		xTmp = params.Tan90M2 / (params.Tan90M2 - std::tan(angXY));
		yTmp = xTmp * std::tan(angXY);
		params.LenOuterEdges = std::sqrt(SQR(xTmp) + SQR(yTmp));

		if (params.ExactCalc == 1)
		{
			if (params.EqualBlur == 1)
				ranTmp = std::sqrt(rand.Frand01<T>());
			else
				ranTmp = rand.Frand01<T>();
		}
		else
		{
			if (params.CircumCircle == 1)
			{
				if (params.EqualBlur == 1)
					ranTmp = std::sqrt(rand.Frand01<T>());
				else
					ranTmp = rand.Frand01<T>();
			}
			else
			{
				if (params.EqualBlur == 1)
					ranTmp = std::sqrt(rand.Frand01<T>()) * params.LenOuterEdges;
				else
					ranTmp = rand.Frand01<T>() * params.LenOuterEdges;
			}
		}

		params.LenInnerEdges = params.RatioHole * params.LenOuterEdges;

		if (params.ExactCalc == 0)
		{
			if (ranTmp < params.LenInnerEdges)
			{
				if (params.CircumCircle == 1)
				{
					if (params.EqualBlur == 1)
						ranTmp = params.LenInnerEdges + std::sqrt(rand.Frand01<T>()) * (1 - params.LenInnerEdges + EPS);
					else
						ranTmp = params.LenInnerEdges + rand.Frand01<T>() * (1 - params.LenInnerEdges + EPS);
				}
				else
				{
					if (params.EqualBlur == 1)
						ranTmp = params.LenInnerEdges + std::sqrt(rand.Frand01<T>()) * (params.LenOuterEdges - params.LenInnerEdges);
					else
						ranTmp = params.LenInnerEdges + rand.Frand01<T>() * (params.LenOuterEdges - params.LenInnerEdges);
				}
			}
		}

		params.X *= ranTmp;
		params.Y *= ranTmp;
		params.LenXY = std::sqrt(SQR(params.X) + SQR(params.Y));
	}

	T m_NumEdges;
	T m_NumStripes;
	T m_RatioStripes;
	T m_RatioHole;
	T m_CircumCircle;
	T m_AdjustToLinear;
	T m_EqualBlur;
	T m_ExactCalc;
	T m_HighlightEdges;
	T m_RatioComplement;//Precalc.
	T m_MidAngle;
	T m_AngStart;
	T m_AngStripes;
	T m_HasStripes;
	T m_NegStripes;
	T m_MaxStripes;
	T m_AbsStripes;
	T m_Sina;
	T m_Cosa;
	T m_Tan90M2;
	T m_ArcTan1;
	T m_ArcTan2;
	T m_SpeedCalc1;
	T m_SpeedCalc2;
	T m_AdjustedWeight;
};

/// <summary>
/// octapol.
/// </summary>
template <typename T>
class EMBER_API OctapolVariation : public ParametricVariation<T>
{
public:
	OctapolVariation(T weight = 1.0) : ParametricVariation<T>("octapol", VAR_OCTAPOL, weight)
	{
		Init();
	}

	PARVARCOPY(OctapolVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		bool clear = false;
		T x = helper.In.x * T(0.15), y = helper.In.y * T(0.15), z = helper.In.z, r = 0, u = 0, v = 0;
		v2T xy = { x, y };
		v2T tempOut = { 0, 0 };

		if ((m_TempRad > 0) && HitsCircleAroundOrigin(m_TempRad, xy, r))
		{
			T rd = std::log(Sqr(r / m_TempRad));
			T phi = std::atan2(y, x);
			tempOut.x = m_Weight * Lerp(x, phi, rd * m_Polarweight);
			tempOut.y = m_Weight * Lerp(y, r, rd * m_Polarweight);
		}
		else if (HitsSquareAroundOrigin(m_St, xy))
		{
			if (HitsRect(m_H, m_K, xy) || HitsRect(m_J, m_D, xy) ||
					HitsRect(m_A, m_J, xy) || HitsRect(m_K, m_E, xy) ||
					HitsTriangle(m_I, m_A, m_H, xy, u, v) ||
					HitsTriangle(m_J, m_B, m_C, xy, u, v) ||
					HitsTriangle(m_L, m_D, m_E, xy, u, v) ||
					HitsTriangle(m_K, m_F, m_G, xy, u, v))
			{
				tempOut.x = m_Weight * x;
				tempOut.y = m_Weight * y;
			}
			else
				clear = true;
		}
		else
			clear = true;

		if (clear)
		{
			if (m_VarType == VARTYPE_PRE)
			{
				helper.m_TransX = 0;
				helper.m_TransY = 0;
			}
			else
			{
				outPoint.m_X = 0;
				outPoint.m_Y = 0;
			}
		}

		helper.Out.x = tempOut.x + (m_Weight * x);
		helper.Out.y = tempOut.y + (m_Weight * y);
		helper.Out.z = m_Weight * z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0, varIndex = IndexInXform();
		ss2 << "_" << XformIndexInEmber();
		string index = ss2.str() + "]";
		string stateIndex = ss2.str();
		string polarweight = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string radius	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string s		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string t		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string tempRad	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string abss		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string abst		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string st		   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string axStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string bxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string cxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string dxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string exStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string fxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string gxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string hxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string ixStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string jxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string kxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex; i += 2;
		string lxStartIndex = ToUpper(m_Params[i].Name()) + stateIndex;
		ss << "\t{\n"
		   << "\t\tbool clear = false;\n"
		   << "\t\treal_t x = vIn.x * 0.15, y = vIn.y * 0.15, z = vIn.z, r = 0, u = 0, v = 0, x2 = 0, y2 = 0;\n"
		   << "\t\treal2 xy = { x, y };\n"
		   << "\t\treal2 tempOut = { 0, 0 };\n"
		   << "\t\treal2 A = { parVars[" << axStartIndex << "], parVars[" << axStartIndex << " + 1] };\n"
		   << "\t\treal2 B = { parVars[" << bxStartIndex << "], parVars[" << bxStartIndex << " + 1] };\n"
		   << "\t\treal2 C = { parVars[" << cxStartIndex << "], parVars[" << cxStartIndex << " + 1] };\n"
		   << "\t\treal2 D = { parVars[" << dxStartIndex << "], parVars[" << dxStartIndex << " + 1] };\n"
		   << "\t\treal2 E = { parVars[" << exStartIndex << "], parVars[" << exStartIndex << " + 1] };\n"
		   << "\t\treal2 F = { parVars[" << fxStartIndex << "], parVars[" << fxStartIndex << " + 1] };\n"
		   << "\t\treal2 G = { parVars[" << gxStartIndex << "], parVars[" << gxStartIndex << " + 1] };\n"
		   << "\t\treal2 H = { parVars[" << hxStartIndex << "], parVars[" << hxStartIndex << " + 1] };\n"
		   << "\t\treal2 I = { parVars[" << ixStartIndex << "], parVars[" << ixStartIndex << " + 1] };\n"
		   << "\t\treal2 J = { parVars[" << jxStartIndex << "], parVars[" << jxStartIndex << " + 1] };\n"
		   << "\t\treal2 K = { parVars[" << kxStartIndex << "], parVars[" << kxStartIndex << " + 1] };\n"
		   << "\t\treal2 L = { parVars[" << lxStartIndex << "], parVars[" << lxStartIndex << " + 1] };\n"
		   << "\n"
		   << "\t\tif ((" << tempRad << " > 0) && HitsCircleAroundOrigin(" << tempRad << ", &xy, &r))\n"
		   << "\t\t{\n"
		   << "\t\t	real_t rd = log(Sqr(r / " << tempRad << "));\n"
		   << "\t\t	real_t phi = atan2(y, x);\n"
		   << "\n"
		   << "\t\t	tempOut.x = xform->m_VariationWeights[" << varIndex << "] * Lerp(x, phi, rd * " << polarweight << ");\n"
		   << "\t\t	tempOut.y = xform->m_VariationWeights[" << varIndex << "] * Lerp(y, r, rd *   " << polarweight << ");\n"
		   << "\t\t}\n"
		   << "\t\telse if (HitsSquareAroundOrigin(" << st << ", &xy))\n"
		   << "\t\t{\n"
		   << "\t\t	if (HitsRect(&H, &K, &xy) || HitsRect(&J, &D, &xy) ||\n"
		   << "\t\t		HitsRect(&A, &J, &xy) || HitsRect(&K, &E, &xy) ||\n"
		   << "\t\t		HitsTriangle(&I, &A, &H, &xy, &u, &v) ||\n"
		   << "\t\t		HitsTriangle(&J, &B, &C, &xy, &u, &v) ||\n"
		   << "\t\t		HitsTriangle(&L, &D, &E, &xy, &u, &v) ||\n"
		   << "\t\t		HitsTriangle(&K, &F, &G, &xy, &u, &v))\n"
		   << "\t\t	{\n"
		   << "\t\t		tempOut.x = xform->m_VariationWeights[" << varIndex << "] * x;\n"
		   << "\t\t		tempOut.y = xform->m_VariationWeights[" << varIndex << "] * y;\n"
		   << "\t\t	}\n"
		   << "\t\t	else\n"
		   << "\t\t		clear = true;\n"
		   << "\t\t}\n"
		   << "\t\telse\n"
		   << "\t\t	clear = true;\n"
		   << "\n"
		   << "\t\tif (clear)\n"
		   << "\t\t{\n";

		if (m_VarType == VARTYPE_PRE)
		{
			ss
					<< "\t\t	transX = 0;\n"
					<< "\t\t	transY = 0;\n";
		}
		else
		{
			ss
					<< "\t\t	outPoint->m_X = 0;\n"
					<< "\t\t	outPoint->m_Y = 0;\n";
		}

		ss
				<< "\t\t}\n"
				<< "\n"
				<< "\t\tvOut.x = tempOut.x + (xform->m_VariationWeights[" << varIndex << "] * x);\n"
				<< "\t\tvOut.y = tempOut.y + (xform->m_VariationWeights[" << varIndex << "] * y);\n"
				<< "\t\tvOut.z = xform->m_VariationWeights[" << varIndex << "] * z;\n"
				<< "\t}\n";
		return ss.str();
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Lerp", "Sqr" };
	}

	virtual string OpenCLFuncsString() const override
	{
		return
			"static int HitsRect(real2* tl, real2* br, real2* p)\n"
			"{\n"
			"	return ((*p).x >= (*tl).x && (*p).y >= (*tl).y && (*p).x <= (*br).x && (*p).y <= (*br).y);\n"
			"}\n"
			"\n"
			"static int HitsSquareAroundOrigin(real_t a, real2* p)\n"
			"{\n"
			"	return (fabs((*p).x) <= a && fabs((*p).y) <= a);\n"
			"}\n"
			"\n"
			"static int HitsCircleAroundOrigin(real_t radius, real2* p, real_t* r)\n"
			"{\n"
			"	if (radius == 0)\n"
			"		return 1;\n"
			"\n"
			"	*r = sqrt(SQR((*p).x) + SQR((*p).y));\n"
			"	return (*r <= radius);\n"
			"}\n"
			"\n"
			"static int HitsTriangle(real2* a, real2* b, real2* c, real2* p, real_t* u, real_t* v)\n"
			"{\n"
			"	real2 v0 = { (*c).x - (*a).x, (*c).y - (*a).y };\n"
			"	real2 v1 = { (*b).x - (*a).x, (*b).y - (*a).y };\n"
			"	real2 v2 = { (*p).x - (*a).x, (*p).y - (*a).y };\n"
			"	real_t d00 = dot(v0, v0);\n"
			"	real_t d01 = dot(v0, v1);\n"
			"	real_t d02 = dot(v0, v2);\n"
			"	real_t d11 = dot(v1, v1);\n"
			"	real_t d12 = dot(v1, v2);\n"
			"	real_t denom = (d00 * d11 - d01 * d01);\n"
			"\n"
			"	if (denom != 0)\n"
			"	{\n"
			"		*u = (d11 * d02 - d01 * d12) / denom;\n"
			"		*v = (d00 * d12 - d01 * d02) / denom;\n"
			"	}\n"
			"	else\n"
			"		*u = *v = 0;\n"
			"\n"
			"	return ((*u + *v) < 1) && (*u > 0) && (*v > 0);\n"
			"}\n\n";
	}

	virtual void Precalc() override
	{
		static const T DENOM_SQRT2 = T(0.707106781);
		m_AbsS = std::fabs(m_S);
		m_AbsT = std::fabs(m_T);
		m_St = m_AbsS * T(0.5) + m_AbsT;
		m_TempRad = DENOM_SQRT2 * m_AbsS * std::fabs(m_Radius);
		m_A = { -T(0.5)* m_AbsS, T(0.5)* m_AbsS + m_AbsT };
		m_B = { T(0.5)* m_AbsS, T(0.5)* m_AbsS + m_AbsT };
		m_C = { m_AbsT, T(0.5)* m_AbsS };
		m_D = { m_AbsT, -T(0.5)* m_AbsS };
		m_E = { T(0.5)* m_AbsS, -T(0.5)* m_AbsS - m_AbsT };
		m_F = { -T(0.5)* m_AbsS, -T(0.5)* m_AbsS - m_AbsT };
		m_G = { -m_AbsT, -T(0.5)* m_AbsS };
		m_H = { -m_AbsT, T(0.5)* m_AbsS };
		m_I = { -T(0.5)* m_AbsS, T(0.5)* m_AbsS };
		m_J = { T(0.5)* m_AbsS, T(0.5)* m_AbsS };
		m_K = { -T(0.5)* m_AbsS, -T(0.5)* m_AbsS };
		m_L = { T(0.5)* m_AbsS, -T(0.5)* m_AbsS };
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.push_back(ParamWithName<T>(&m_Polarweight, prefix + "octapol_polarweight"));
		m_Params.push_back(ParamWithName<T>(&m_Radius,		prefix + "octapol_radius", 1));
		m_Params.push_back(ParamWithName<T>(&m_S,			prefix + "octapol_s", T(0.5)));
		m_Params.push_back(ParamWithName<T>(&m_T,			prefix + "octapol_t", T(0.5)));
		m_Params.push_back(ParamWithName<T>(true, &m_TempRad, prefix + "octapol_rad"));//Precalc.
		m_Params.push_back(ParamWithName<T>(true, &m_AbsS,	  prefix + "octapol_abss"));
		m_Params.push_back(ParamWithName<T>(true, &m_AbsT,	  prefix + "octapol_abst"));
		m_Params.push_back(ParamWithName<T>(true, &m_St,	  prefix + "octapol_absst"));
		m_Params.push_back(ParamWithName<T>(true, &m_A.x,	  prefix + "octapol_ax"));
		m_Params.push_back(ParamWithName<T>(true, &m_A.y,	  prefix + "octapol_ay"));
		m_Params.push_back(ParamWithName<T>(true, &m_B.x,	  prefix + "octapol_bx"));
		m_Params.push_back(ParamWithName<T>(true, &m_B.y,	  prefix + "octapol_by"));
		m_Params.push_back(ParamWithName<T>(true, &m_C.x,	  prefix + "octapol_cx"));
		m_Params.push_back(ParamWithName<T>(true, &m_C.y,	  prefix + "octapol_cy"));
		m_Params.push_back(ParamWithName<T>(true, &m_D.x,	  prefix + "octapol_dx"));
		m_Params.push_back(ParamWithName<T>(true, &m_D.y,	  prefix + "octapol_dy"));
		m_Params.push_back(ParamWithName<T>(true, &m_E.x,	  prefix + "octapol_ex"));
		m_Params.push_back(ParamWithName<T>(true, &m_E.y,	  prefix + "octapol_ey"));
		m_Params.push_back(ParamWithName<T>(true, &m_F.x,	  prefix + "octapol_fx"));
		m_Params.push_back(ParamWithName<T>(true, &m_F.y,	  prefix + "octapol_fy"));
		m_Params.push_back(ParamWithName<T>(true, &m_G.x,	  prefix + "octapol_gx"));
		m_Params.push_back(ParamWithName<T>(true, &m_G.y,	  prefix + "octapol_gy"));
		m_Params.push_back(ParamWithName<T>(true, &m_H.x,	  prefix + "octapol_hx"));
		m_Params.push_back(ParamWithName<T>(true, &m_H.y,	  prefix + "octapol_hy"));
		m_Params.push_back(ParamWithName<T>(true, &m_I.x,	  prefix + "octapol_ix"));
		m_Params.push_back(ParamWithName<T>(true, &m_I.y,	  prefix + "octapol_iy"));
		m_Params.push_back(ParamWithName<T>(true, &m_J.x,	  prefix + "octapol_jx"));
		m_Params.push_back(ParamWithName<T>(true, &m_J.y,	  prefix + "octapol_jy"));
		m_Params.push_back(ParamWithName<T>(true, &m_K.x,	  prefix + "octapol_kx"));
		m_Params.push_back(ParamWithName<T>(true, &m_K.y,	  prefix + "octapol_ky"));
		m_Params.push_back(ParamWithName<T>(true, &m_L.x,	  prefix + "octapol_lx"));
		m_Params.push_back(ParamWithName<T>(true, &m_L.y,	  prefix + "octapol_ly"));
	}

private:
	int HitsRect(v2T& tl, v2T& br, v2T& p)
	{
		return (p.x >= tl.x && p.y >= tl.y && p.x <= br.x && p.y <= br.y);
	}

	int HitsSquareAroundOrigin(T a, v2T& p)
	{
		return (std::fabs(p.x) <= a && std::fabs(p.y) <= a);
	}

	int HitsCircleAroundOrigin(T radius, v2T& p, T& r)
	{
		if (radius == 0)
			return 1;

		r = std::sqrt(SQR(p.x) + SQR(p.y));
		return (r <= radius);
	}

	int HitsTriangle(v2T& a, v2T& b, v2T& c, v2T& p, T& u, T& v)
	{
		v2T v0 = { c.x - a.x, c.y - a.y };
		v2T v1 = { b.x - a.x, b.y - a.y };
		v2T v2 = { p.x - a.x, p.y - a.y };
		T d00 = glm::dot(v0, v0);
		T d01 = glm::dot(v0, v1);
		T d02 = glm::dot(v0, v2);
		T d11 = glm::dot(v1, v1);
		T d12 = glm::dot(v1, v2);
		T denom = (d00 * d11 - d01 * d01);

		if (denom != 0)
		{
			u = (d11 * d02 - d01 * d12) / denom;
			v = (d00 * d12 - d01 * d02) / denom;
		}
		else
			u = v = 0;

		return ((u + v) < 1) && (u > 0) && (v > 0);
	}

	T m_Polarweight;
	T m_Radius;
	T m_S;
	T m_T;
	T m_TempRad;//Precalc.
	T m_AbsS;
	T m_AbsT;
	T m_St;
	v2T m_A;
	v2T m_B;
	v2T m_C;
	v2T m_D;
	v2T m_E;
	v2T m_F;
	v2T m_G;
	v2T m_H;
	v2T m_I;
	v2T m_J;
	v2T m_K;
	v2T m_L;
};

/// <summary>
/// crob.
/// This uses the input point in an extremely rare way since it changes it.
/// </summary>
template <typename T>
class EMBER_API CrobVariation : public ParametricVariation<T>
{
public:
	CrobVariation(T weight = 1.0) : ParametricVariation<T>("crob", VAR_CROB, weight)
	{
		Init();
	}

	PARVARCOPY(CrobVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		T gradTmp, secTmp, xTmp, yTmp;

		if ((helper.In.x < m_LeftBorder) || (helper.In.x > m_RightBorder) || (helper.In.y < m_TopBorder) || (helper.In.y > m_BottomBorder))
		{
			if (m_Blur == 0)
			{
				if (m_VarType == VARTYPE_PRE)//Setting input point.
				{
					helper.m_TransX = 0;
					helper.m_TransY = 0;
				}
				else if (m_VarType == VARTYPE_REG)
				{
					helper.In.x = 0;
					helper.In.y = 0;
				}
				else
				{
					outPoint.m_X = 0;
					outPoint.m_Y = 0;
				}
			}
			else
			{
				secTmp = rand.Frand01<T>();

				if (secTmp < m_SetProb)
				{
					do
					{
						yTmp = m_Top + rand.Frand01<T>() * m_YInt2;
						xTmp = m_Right - pow(rand.Frand01<T>(), m_DirectBlur) * m_RatioBlur * m_MinInt2;
					}
					while ((yTmp - m_Y0c) / (xTmp - m_X0c) < -1);

					if (secTmp < m_SetProbH)
						xTmp = m_Left + m_Right - xTmp;

					if ((secTmp > m_SetProbQ) && (secTmp < m_SetProbTQ))
						yTmp = m_Bottom + m_Top - yTmp;
				}
				else
				{
					do
					{
						xTmp = m_Right - rand.Frand01<T>() * m_XInt2;
						yTmp = m_Top + std::pow(rand.Frand01<T>(), m_DirectBlur) * m_RatioBlur * m_MinInt2;
						gradTmp = (yTmp - m_Y0c) / (xTmp - m_X0c);
					}
					while ((gradTmp <= 0) && (gradTmp > -1));

					if (secTmp > m_SetProbH)
						yTmp = m_Bottom + m_Top - yTmp;

					if ((secTmp > m_SetProbQ) && (secTmp < m_SetProbTQ))
						xTmp = m_Left + m_Right - xTmp;
				}

				if (m_VarType == VARTYPE_PRE)
				{
					helper.m_TransX = xTmp;
					helper.m_TransY = yTmp;
				}
				else if (m_VarType == VARTYPE_REG)
				{
					helper.In.x = xTmp;
					helper.In.y = yTmp;
				}
				else
				{
					outPoint.m_X = xTmp;
					outPoint.m_Y = yTmp;
				}
			}
		}

		if (m_VarType == VARTYPE_PRE)
		{
			helper.Out.x = helper.m_TransX;
			helper.Out.y = helper.m_TransY;
		}
		else if (m_VarType == VARTYPE_REG)
		{
			helper.Out.x = helper.In.x;
			helper.Out.y = helper.In.y;
		}
		else
		{
			helper.Out.x = outPoint.m_X;
			helper.Out.y = outPoint.m_Y;
		}

		helper.Out.z = (m_VarType == VARTYPE_REG) ? 0 : helper.In.z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0;
		ss2 << "_" << XformIndexInEmber() << "]";
		string index = ss2.str();
		string top =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string bottom =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string left =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string right =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string blur =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string ratioBlur =     "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string directBlur =    "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string xInterval =     "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string yInterval =     "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string xInt2 =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string yInt2 =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string minInt2 =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string x0 =			   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string y0 =			   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string x0c =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string y0c =		   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setProb =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setProbH =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setProbQ =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setProbTQ =     "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setCompProb =   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setCompProbH =  "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setCompProbQ =  "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string setCompProbTQ = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string topBorder =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string bottomBorder =  "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string leftBorder =	   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string rightBorder =   "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   << "\t\treal_t gradTmp, secTmp, xTmp, yTmp;\n"
		   << "\n"
		   << "\t\tif ((vIn.x < " << leftBorder << ") || (vIn.x > " << rightBorder << ") || (vIn.y < " << topBorder << ") || (vIn.y > " << bottomBorder << "))\n"
		   << "\t\t{\n"
		   << "\t\t	if (" << blur << " == 0)\n"
		   << "\t\t	{\n";

		if (m_VarType == VARTYPE_PRE)
		{
			ss
					<< "\t\t		transX = 0;\n"
					<< "\t\t		transY = 0;\n";
		}
		else if (m_VarType == VARTYPE_REG)
		{
			ss
					<< "\t\t		vIn.x = 0;\n"
					<< "\t\t		vIn.y = 0;\n";
		}
		else
		{
			ss
					<< "\t\t		outPoint->m_X = 0;\n"
					<< "\t\t		outPoint->m_Y = 0;\n";
		}

		ss
				<< "\t\t	}\n"
				<< "\t\t	else\n"
				<< "\t\t	{\n"
				<< "\t\t		secTmp = MwcNext01(mwc);\n"
				<< "\n"
				<< "\t\t		if (secTmp < " << setProb << ")\n"
				<< "\t\t		{\n"
				<< "\t\t			do\n"
				<< "\t\t			{\n"
				<< "\t\t				yTmp = " << top << " + MwcNext01(mwc) * " << yInt2 << ";\n"
				<< "\t\t				xTmp = " << right << " - pow(MwcNext01(mwc), " << directBlur << ") * " << ratioBlur << " * " << minInt2 << ";\n"
				<< "\t\t			} while ((yTmp - " << y0c << ") / (xTmp - " << x0c << ") < -1);\n"
				<< "\n"
				<< "\t\t			if (secTmp < " << setProbH << ")\n"
				<< "\t\t				xTmp = " << left << " + " << right << " - xTmp;\n"
				<< "\n"
				<< "\t\t			if ((secTmp > " << setProbQ << ") && (secTmp < " << setProbTQ << "))\n"
				<< "\t\t				yTmp = " << bottom << " + " << top << " - yTmp;\n"
				<< "\t\t		}\n"
				<< "\t\t		else\n"
				<< "\t\t		{\n"
				<< "\t\t			do\n"
				<< "\t\t			{\n"
				<< "\t\t				xTmp = " << right << " - MwcNext01(mwc) * " << xInt2 << ";\n"
				<< "\t\t				yTmp = " << top << " + pow(MwcNext01(mwc), " << directBlur << ") * " << ratioBlur << " * " << minInt2 << ";\n"
				<< "\t\t				gradTmp = (yTmp - " << y0c << ") / (xTmp - " << x0c << ");\n"
				<< "\t\t			} while ((gradTmp <= 0) && (gradTmp > -1));\n"
				<< "\n"
				<< "\t\t			if (secTmp > " << setProbH << ")\n"
				<< "\t\t				yTmp = " << bottom << " + " << top << " - yTmp;\n"
				<< "\n"
				<< "\t\t			if ((secTmp > " << setProbQ << ") && (secTmp < " << setProbTQ << "))\n"
				<< "\t\t				xTmp = " << left << " + " << right << " - xTmp;\n"
				<< "\t\t		}\n"
				<< "\n";

		if (m_VarType == VARTYPE_PRE)
		{
			ss
					<< "\t\t		transX = xTmp;\n"
					<< "\t\t		transY = yTmp;\n";
		}
		else if (m_VarType == VARTYPE_REG)
		{
			ss
					<< "\t\t		vIn.x = xTmp;\n"
					<< "\t\t		vIn.y = yTmp;\n";
		}
		else
		{
			ss
					<< "\t\t		outPoint->m_X = xTmp;\n"
					<< "\t\t		outPoint->m_Y = yTmp;\n";
		}

		ss
				<< "\t\t\t}\n"
				<< "\t\t}\n"
				<< "\n";

		if (m_VarType == VARTYPE_PRE)
		{
			ss
					<< "\t\tvOut.x = transX;\n"
					<< "\t\tvOut.y = transY;\n";
		}
		else if (m_VarType == VARTYPE_REG)
		{
			ss
					<< "\t\tvOut.x = vIn.x;\n"
					<< "\t\tvOut.y = vIn.y;\n";
		}
		else
		{
			ss
					<< "\t\tvOut.x = outPoint->m_X;\n"
					<< "\t\tvOut.y = outPoint->m_Y;\n";
		}

		ss
				<< "\t\tvOut.z = " << ((m_VarType == VARTYPE_REG) ? "0" : "vIn.z") << ";\n"
				<< "\t}\n";
		return ss.str();
	}

	virtual void Precalc() override
	{
		if (m_Top > m_Bottom)
			std::swap(m_Top, m_Bottom);

		if (m_Top == m_Bottom)
		{
			m_Top = -1;
			m_Bottom = 1;
		}

		if (m_Left > m_Right)
			std::swap(m_Left, m_Right);

		if (m_Left == m_Right)
		{
			m_Left = -1;
			m_Right = 1;
		}

		if (m_DirectBlur < 0)
			m_DirectBlur = 0;

		if (m_Blur != 0)
			m_Blur = 1;

		m_XInterval = std::fabs(m_Right) - m_Left;
		m_YInterval = std::fabs(m_Bottom) - m_Top;
		m_XInt2 = m_XInterval / 2;
		m_YInt2 = m_YInterval / 2;

		if (m_XInt2 > m_YInt2)
			m_MinInt2 = m_YInt2;
		else
			m_MinInt2 = m_XInt2;

		m_X0 = m_Right - m_XInt2;
		m_Y0 = m_Top + m_YInt2;

		if (m_XInt2 > m_YInt2)
		{
			m_X0c = m_Right - m_MinInt2;
			m_Y0c = m_Y0;
		}
		else if (m_XInt2 < m_YInt2)
		{
			m_X0c = m_X0;
			m_Y0c = m_Top + m_MinInt2;
		}
		else
		{
			m_X0c = m_X0;
			m_Y0c = m_Y0;
		}

		m_SetProb = m_YInterval / (m_XInterval + m_YInterval);
		m_SetProbQ = T(0.25) * m_SetProb;
		m_SetProbH = T(0.50) * m_SetProb;
		m_SetProbTQ = T(0.75) * m_SetProb;
		m_SetCompProb = T(1.0) - m_SetProb;
		m_SetCompProbQ = m_SetProb + T(0.25) * m_SetCompProb;
		m_SetCompProbH = m_SetProb + T(0.50) * m_SetCompProb;
		m_SetCompProbTQ = m_SetProb + T(0.75) * m_SetCompProb;

		if (m_Blur == 0)
		{
			m_TopBorder = m_Top;
			m_BottomBorder = m_Bottom;
			m_LeftBorder = m_Left;
			m_RightBorder = m_Right;
		}
		else
		{
			m_TopBorder = m_Top  + m_MinInt2 * m_RatioBlur;
			m_BottomBorder = m_Bottom - m_MinInt2 * m_RatioBlur;
			m_LeftBorder = m_Left  + m_MinInt2 * m_RatioBlur;
			m_RightBorder = m_Right   - m_MinInt2 * m_RatioBlur;
		}
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.push_back(ParamWithName<T>(&m_Top,		   prefix + "crob_top", -1));
		m_Params.push_back(ParamWithName<T>(&m_Bottom,	   prefix + "crob_bottom", 1));
		m_Params.push_back(ParamWithName<T>(&m_Left,	   prefix + "crob_left", -1));
		m_Params.push_back(ParamWithName<T>(&m_Right,	   prefix + "crob_right", 1));
		m_Params.push_back(ParamWithName<T>(&m_Blur,	   prefix + "crob_blur", 1, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_RatioBlur,  prefix + "crob_ratioBlur", T(0.5), REAL, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_DirectBlur, prefix + "crob_directBlur", 2));
		m_Params.push_back(ParamWithName<T>(true, &m_XInterval,		prefix + "crob_xinterval"));
		m_Params.push_back(ParamWithName<T>(true, &m_YInterval,		prefix + "crob_yinterval"));
		m_Params.push_back(ParamWithName<T>(true, &m_XInt2,			prefix + "crob_xint2"));
		m_Params.push_back(ParamWithName<T>(true, &m_YInt2,			prefix + "crob_yint2"));
		m_Params.push_back(ParamWithName<T>(true, &m_MinInt2,		prefix + "crob_minint2"));
		m_Params.push_back(ParamWithName<T>(true, &m_X0,			prefix + "crob_x0"));
		m_Params.push_back(ParamWithName<T>(true, &m_Y0,			prefix + "crob_y0"));
		m_Params.push_back(ParamWithName<T>(true, &m_X0c,			prefix + "crob_x0c"));
		m_Params.push_back(ParamWithName<T>(true, &m_Y0c,			prefix + "crob_y0c"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetProb,		prefix + "crob_set_prob"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetProbH,		prefix + "crob_set_prob_h"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetProbQ,		prefix + "crob_set_prob_q"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetProbTQ,		prefix + "crob_set_prob_tq"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetCompProb,	prefix + "crob_set_comp_prob"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetCompProbH,	prefix + "crob_set_comp_prob_h"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetCompProbQ,	prefix + "crob_set_comp_prob_q"));
		m_Params.push_back(ParamWithName<T>(true, &m_SetCompProbTQ, prefix + "crob_set_comp_prob_tq"));
		m_Params.push_back(ParamWithName<T>(true, &m_TopBorder,		prefix + "crob_top_border"));
		m_Params.push_back(ParamWithName<T>(true, &m_BottomBorder,	prefix + "crob_bottom_border"));
		m_Params.push_back(ParamWithName<T>(true, &m_LeftBorder,	prefix + "crob_left_border"));
		m_Params.push_back(ParamWithName<T>(true, &m_RightBorder,	prefix + "crob_right_border"));
	}

private:
	T m_Top;
	T m_Bottom;
	T m_Left;
	T m_Right;
	T m_Blur;
	T m_RatioBlur;
	T m_DirectBlur;
	T m_XInterval;//Precalc.
	T m_YInterval;
	T m_XInt2;
	T m_YInt2;
	T m_MinInt2;
	T m_X0;
	T m_Y0;
	T m_X0c;
	T m_Y0c;
	T m_SetProb;
	T m_SetProbH;
	T m_SetProbQ;
	T m_SetProbTQ;
	T m_SetCompProb;
	T m_SetCompProbH;
	T m_SetCompProbQ;
	T m_SetCompProbTQ;
	T m_TopBorder;
	T m_BottomBorder;
	T m_LeftBorder;
	T m_RightBorder;
};

/// <summary>
/// bubbleT3D.
/// </summary>
template <typename T>
class EMBER_API BubbleT3DVariation : public ParametricVariation<T>
{
public:
	BubbleT3DVariation(T weight = 1.0) : ParametricVariation<T>("bubbleT3D", VAR_BUBBLET3D, weight, true)
	{
		Init();
	}

	PARVARCOPY(BubbleT3DVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		T x = helper.In.x, y = helper.In.y, z = helper.In.z;
		T xTmp, yTmp, angTmp, angRot, fac;
		T rad = helper.m_PrecalcSumSquares / 4 + 1;
		T angXY, angZ;
		T c, s;
		angXY = std::atan2(x, y);

		if (angXY < 0)
			angXY += M_2PI;

		if (m_AbsNumberStripes != 0)
		{
			while (angXY > m_AngStrip2)
			{
				angXY -= m_AngStrip2;
			}

			if (m_InvStripes == 0)
			{
				if (angXY > m_AngStrip1)
				{
					if (m_ModusBlur == 0)
					{
						x = 0;
						y = 0;
					}
					else
					{
						if (m_RatioStripes == 1)
						{
							xTmp = m_C * x - m_S * y;
							yTmp = m_S * x + m_C * y;
							x = xTmp;
							y = yTmp;
						}
						else
						{
							angRot = (angXY - m_AngStrip1) / (m_AngStrip2 - m_AngStrip1);
							angRot = angXY - angRot * m_AngStrip1;
							sincos(angRot, &s, &c);
							xTmp = c * x - s * y;
							yTmp = s * x + c * y;
							x = xTmp;
							y = yTmp;
						}
					}
				}
			}
			else
			{
				if (angXY < m_AngStrip1)
				{
					if (m_ModusBlur == 0)
					{
						x = 0;
						y = 0;
					}
					else
					{
						if (m_AbsNumberStripes == 1)
						{
							xTmp = m_C * x - m_S * y;
							yTmp = m_S * x + m_C * y;
							x = xTmp;
							y = yTmp;
						}
						else
						{
							angRot = (angXY - m_AngStrip1) / m_AngStrip1;
							angRot = angXY - angRot * (m_AngStrip2 - m_AngStrip1);
							sincos(angRot, &s, &c);
							xTmp = c * x - s * y;
							yTmp = s * x + c * y;
							x = xTmp;
							y = yTmp;
						}
					}
				}
			}
		}

		x = x / rad;
		y = y / rad;

		if ((x != 0) || (y != 0))
		{
			z = 2 / std::pow(rad, m_ExponentZ) - 1;

			if (m_ExponentZ <= 2)
				angZ = T(M_PI) - std::acos((z / (Sqr(x) + Sqr(y) + Sqr(z))));
			else
				angZ = T(M_PI) - std::atan2(Sqr(Sqr(x) + Sqr(y)), z);
		}
		else
		{
			z = 0;
			angZ = 0;
		}

		if (m_SymmetryZ == 0)
		{
			if (m_InvHole == 0)
			{
				if (angZ > m_AngleHoleTemp)
				{
					if ((m_ModusBlur == 0) || (m_ExponentZ != 1))
					{
						x = 0;
						y = 0;
						z = 0;
					}
					else
					{
						angTmp = (T(M_PI) - angZ) / m_AngHoleComp * m_AngleHoleTemp - T(M_PI_2);
						angZ -= T(M_PI_2);
						fac = std::cos(angTmp) / std::cos(angZ);
						x = x * fac;
						y = y * fac;
						z = z * (std::sin(angTmp) / std::sin(angZ));
					}
				}
			}
			else
			{
				if (angZ < m_AngleHoleTemp)
				{
					if ((m_ModusBlur == 0) || (m_ExponentZ != 1))
					{
						x = 0;
						y = 0;
						z = 0;
					}
					else
					{
						angTmp = T(M_PI) - angZ / m_AngHoleComp * m_AngleHoleTemp - T(M_PI_2);
						angZ -= T(M_PI_2);
						fac = std::cos(angTmp) / std::cos(angZ);
						x = x * fac;
						y = y * fac;
						z = z * (std::sin(angTmp) / std::sin(angZ));
					}
				}
			}
		}
		else
		{
			if ((angZ > m_AngleHoleTemp) || (angZ < (T(M_PI) - m_AngleHoleTemp)))
			{
				if ((m_ModusBlur == 0) || (m_ExponentZ != 1))
				{
					x = 0;
					y = 0;
					z = 0;
				}
				else
				{
					if (angZ > m_AngleHoleTemp)
					{
						angTmp = (T(M_PI) - angZ) / m_AngHoleComp * (T(M_PI) - 2 * m_AngHoleComp) + m_AngHoleComp - T(M_PI_2);
					}
					else
					{
						angTmp = T(M_PI_2) - (angZ / m_AngHoleComp * (T(M_PI) - 2 * m_AngHoleComp) + m_AngHoleComp);
					}

					angZ -= T(M_PI_2);
					fac = std::cos(angTmp) / std::cos(angZ);
					x = x * fac;
					y = y * fac;
					z = z * (std::sin(angTmp) / std::sin(angZ));
				}
			}
		}

		helper.Out.x = m_Weight * x;
		helper.Out.y = m_Weight * y;
		helper.Out.z = m_Weight * z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0, varIndex = IndexInXform();
		ss2 << "_" << XformIndexInEmber();
		string index = ss2.str() + "]";
		string numberStripes	= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string ratioStripes		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angleHole		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string exponentZ		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string symmetryZ		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string modusBlur		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string absNumberStripes = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angHoleTemp		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angStrip			= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angStrip1		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angStrip2		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string invStripes		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string angHoleComp		= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string invHole			= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string c				= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string s				= "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   "\t\treal_t x = vIn.x, y = vIn.y, z = vIn.z;\n"
		   "\t\treal_t xTmp, yTmp, angTmp, angRot, fac;\n"
		   "\t\treal_t rad = precalcSumSquares / 4 + 1;\n"
		   "\t\treal_t angXY, angZ;\n"
		   "\t\treal_t c, s;\n"
		   "\t\t\n"
		   "\t\tangXY = atan2(x, y);\n"
		   "\t\t\n"
		   "\t\tif (angXY < 0)\n"
		   "\t\t	angXY += M_2PI;\n"
		   "\t\t\n"
		   "\t\tif (" << absNumberStripes << " != 0)\n"
		   "\t\t{\n"
		   "\t\t	while (angXY > " << angStrip2 << ")\n"
		   "\t\t	{\n"
		   "\t\t		angXY -= " << angStrip2 << ";\n"
		   "\t\t	}\n"
		   "\t\t\n"
		   "\t\t	if (" << invStripes << " == 0)\n"
		   "\t\t	{\n"
		   "\t\t		if (angXY > " << angStrip1 << ")\n"
		   "\t\t		{\n"
		   "\t\t			if (" << modusBlur << " == 0)\n"
		   "\t\t			{\n"
		   "\t\t				x = 0;\n"
		   "\t\t				y = 0;\n"
		   "\t\t			}\n"
		   "\t\t			else\n"
		   "\t\t			{\n"
		   "\t\t				if (" << ratioStripes << " == 1)\n"
		   "\t\t				{\n"
		   "\t\t					xTmp = " << c << " * x - " << s << " * y;\n"
		   "\t\t					yTmp = " << s << " * x + " << c << " * y;\n"
		   "\t\t					x = xTmp;\n"
		   "\t\t					y = yTmp;\n"
		   "\t\t				}\n"
		   "\t\t				else\n"
		   "\t\t				{\n"
		   "\t\t					angRot = (angXY - " << angStrip1 << ") / (" << angStrip2 << " - " << angStrip1 << ");\n"
		   "\t\t					angRot = angXY - angRot * " << angStrip1 << ";\n"
		   "\t\t					s = sincos(angRot, &c);\n"
		   "\t\t					xTmp = c * x - s * y;\n"
		   "\t\t					yTmp = s * x + c * y;\n"
		   "\t\t					x = xTmp;\n"
		   "\t\t					y = yTmp;\n"
		   "\t\t				}\n"
		   "\t\t			}\n"
		   "\t\t		}\n"
		   "\t\t	}\n"
		   "\t\t	else\n"
		   "\t\t	{\n"
		   "\t\t		if (angXY < " << angStrip1 << ")\n"
		   "\t\t		{\n"
		   "\t\t			if (" << modusBlur << " == 0)\n"
		   "\t\t			{\n"
		   "\t\t				x = 0;\n"
		   "\t\t				y = 0;\n"
		   "\t\t			}\n"
		   "\t\t			else\n"
		   "\t\t			{\n"
		   "\t\t				if (" << absNumberStripes << " == 1)\n"
		   "\t\t				{\n"
		   "\t\t					xTmp = " << c << " * x - " << s << " * y;\n"
		   "\t\t					yTmp = " << s << " * x + " << c << " * y;\n"
		   "\t\t					x = xTmp;\n"
		   "\t\t					y = yTmp;\n"
		   "\t\t				}\n"
		   "\t\t				else\n"
		   "\t\t				{\n"
		   "\t\t					angRot = (angXY - " << angStrip1 << ") / " << angStrip1 << ";\n"
		   "\t\t					angRot = angXY - angRot * (" << angStrip2 << " - " << angStrip1 << ");\n"
		   "\t\t					s = sincos(angRot, &c);\n"
		   "\t\t					xTmp = c * x - s * y;\n"
		   "\t\t					yTmp = s * x + c * y;\n"
		   "\t\t					x = xTmp;\n"
		   "\t\t					y = yTmp;\n"
		   "\t\t				}\n"
		   "\t\t			}\n"
		   "\t\t		}\n"
		   "\t\t	}\n"
		   "\t\t}\n"
		   "\t\t\n"
		   "\t\tx = x / rad;\n"
		   "\t\ty = y / rad;\n"
		   "\t\t\n"
		   "\t\tif ((x != 0) || (y != 0))\n"
		   "\t\t{\n"
		   "\t\t	z = 2 / pow(rad, " << exponentZ << ") - 1;\n"
		   "\t\t\n"
		   "\t\t	if (" << exponentZ << " <= 2)\n"
		   "\t\t		angZ = M_PI - acos((z / (Sqr(x) + Sqr(y) + Sqr(z))));\n"
		   "\t\t	else\n"
		   "\t\t		angZ = M_PI - atan2(Sqr(Sqr(x) + Sqr(y)), z);\n"
		   "\t\t}\n"
		   "\t\telse\n"
		   "\t\t{\n"
		   "\t\t	z = 0;\n"
		   "\t\t	angZ = 0;\n"
		   "\t\t}\n"
		   "\t\t\n"
		   "\t\tif (" << symmetryZ << " == 0)\n"
		   "\t\t{\n"
		   "\t\t	if (" << invHole << " == 0)\n"
		   "\t\t	{\n"
		   "\t\t		if (angZ > " << angHoleTemp << ")\n"
		   "\t\t		{\n"
		   "\t\t			if ((" << modusBlur << " == 0) || (" << exponentZ << " != 1))\n"
		   "\t\t			{\n"
		   "\t\t				x = 0;\n"
		   "\t\t				y = 0;\n"
		   "\t\t				z = 0;\n"
		   "\t\t			}\n"
		   "\t\t			else\n"
		   "\t\t			{\n"
		   "\t\t				angTmp = (M_PI - angZ) / " << angHoleComp << " * " << angHoleTemp << " - M_PI_2;\n"
		   "\t\t				angZ -= M_PI_2;\n"
		   "\t\t				fac = cos(angTmp) / cos(angZ);\n"
		   "\t\t				x = x * fac;\n"
		   "\t\t				y = y * fac;\n"
		   "\t\t				z = z * (sin(angTmp) / sin(angZ));\n"
		   "\t\t			}\n"
		   "\t\t		}\n"
		   "\t\t	}\n"
		   "\t\t	else\n"
		   "\t\t	{\n"
		   "\t\t		if (angZ < " << angHoleTemp << ")\n"
		   "\t\t		{\n"
		   "\t\t			if ((" << modusBlur << " == 0) || (" << exponentZ << " != 1))\n"
		   "\t\t			{\n"
		   "\t\t				x = 0;\n"
		   "\t\t				y = 0;\n"
		   "\t\t				z = 0;\n"
		   "\t\t			}\n"
		   "\t\t			else\n"
		   "\t\t			{\n"
		   "\t\t				angTmp = M_PI - angZ / " << angHoleComp << " * " << angHoleTemp << " - M_PI_2;\n"
		   "\t\t				angZ -= M_PI_2;\n"
		   "\t\t				fac = cos(angTmp) / cos(angZ);\n"
		   "\t\t				x = x * fac;\n"
		   "\t\t				y = y * fac;\n"
		   "\t\t				z = z * (sin(angTmp) / sin(angZ));\n"
		   "\t\t			}\n"
		   "\t\t		}\n"
		   "\t\t	}\n"
		   "\t\t}\n"
		   "\t\telse\n"
		   "\t\t{\n"
		   "\t\t	if ((angZ > " << angHoleTemp << ") || (angZ < (M_PI - " << angHoleTemp << ")))\n"
		   "\t\t	{\n"
		   "\t\t		if ((" << modusBlur << " == 0) || (" << exponentZ << " != 1))\n"
		   "\t\t		{\n"
		   "\t\t			x = 0;\n"
		   "\t\t			y = 0;\n"
		   "\t\t			z = 0;\n"
		   "\t\t		}\n"
		   "\t\t		else\n"
		   "\t\t		{\n"
		   "\t\t			if (angZ > " << angHoleTemp << ")\n"
		   "\t\t			{\n"
		   "\t\t				angTmp = (M_PI - angZ) / " << angHoleComp << " * (M_PI - 2 * " << angHoleComp << ") + " << angHoleComp << " - M_PI_2;\n"
		   "\t\t			}\n"
		   "\t\t			else\n"
		   "\t\t			{\n"
		   "\t\t				angTmp = M_PI_2 - (angZ / " << angHoleComp << " * (M_PI - 2 * " << angHoleComp << ") + " << angHoleComp << ");\n"
		   "\t\t			}\n"
		   "\t\t\n"
		   "\t\t			angZ -= M_PI_2;\n"
		   "\t\t			fac = cos(angTmp) / cos(angZ);\n"
		   "\t\t			x = x * fac;\n"
		   "\t\t			y = y * fac;\n"
		   "\t\t			z = z * (sin(angTmp) / sin(angZ));\n"
		   "\t\t		}\n"
		   "\t\t	}\n"
		   "\t\t}\n"
		   "\t\t\n"
		   "\t\tvOut.x = xform->m_VariationWeights[" << varIndex << "] * x;\n"
		   "\t\tvOut.y = xform->m_VariationWeights[" << varIndex << "] * y;\n"
		   "\t\tvOut.z = xform->m_VariationWeights[" << varIndex << "] * z;\n"
		   "\t}\n";
		return ss.str();
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Sqr" };
	}

	virtual void Precalc() override
	{
		if (m_NumberStripes < 0)
		{
			m_AbsNumberStripes = std::fabs(m_NumberStripes);
			m_InvStripes = 1;
		}
		else
		{
			m_AbsNumberStripes = m_NumberStripes;
			m_InvStripes = 0;
		}

		if (m_AbsNumberStripes != 0)
		{
			m_AngStrip = T(M_PI) / m_AbsNumberStripes;
			m_AngStrip2 = 2 * m_AngStrip;
			sincos(m_AngStrip, &m_S, &m_C);
			ClampRef<T>(m_RatioStripes, T(0.01), T(1.99));
			m_AngStrip1 = m_RatioStripes * m_AngStrip;
		}

		if (m_SymmetryZ == 1)
		{
			if (m_AngleHole < 0)
				m_AngleHoleTemp = std::fabs(m_AngleHole);
			else if (m_AngleHole > T(179.9))
				m_AngleHoleTemp = T(179.9);
			else
				m_AngleHoleTemp = m_AngleHole;
		}
		else
			m_AngleHoleTemp = m_AngleHole;

		if (m_AngleHoleTemp < 0)
		{
			m_AngleHoleTemp = std::fabs(m_AngleHole);
			m_InvHole = 1;
			m_AngleHoleTemp = (m_AngleHoleTemp / 360 * M_2PI) / 2;
		}
		else
		{
			m_InvHole = 0;
			m_AngleHoleTemp = T(M_PI) - (m_AngleHoleTemp / 360 * M_2PI) / 2;
		}

		m_AngHoleComp = T(M_PI) - m_AngleHoleTemp;
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.reserve(14);
		m_Params.push_back(ParamWithName<T>(&m_NumberStripes, prefix + "bubbleT3D_number_of_stripes", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_RatioStripes,  prefix + "bubbleT3D_ratio_of_stripes", 1, REAL, 0, 2));
		m_Params.push_back(ParamWithName<T>(&m_AngleHole,	  prefix + "bubbleT3D_angle_of_hole", 0, REAL, -360, 360));
		m_Params.push_back(ParamWithName<T>(&m_ExponentZ,	  prefix + "bubbleT3D_exponentZ", 1));
		m_Params.push_back(ParamWithName<T>(&m_SymmetryZ,	  prefix + "bubbleT3D_symmetryZ", 0, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(&m_ModusBlur,	  prefix + "bubbleT3D_modusBlur", 0, INTEGER, 0, 1));
		m_Params.push_back(ParamWithName<T>(true, &m_AbsNumberStripes, prefix + "bubbleT3D_abs_number_of_stripes"));//Precalc.
		m_Params.push_back(ParamWithName<T>(true, &m_AngleHoleTemp,	   prefix + "bubbleT3D_ang_hole_temp"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngStrip,		   prefix + "bubbleT3D_ang_strip"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngStrip1,		   prefix + "bubbleT3D_ang_strip1"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngStrip2,		   prefix + "bubbleT3D_ang_strip2"));
		m_Params.push_back(ParamWithName<T>(true, &m_InvStripes,	   prefix + "bubbleT3D_inv_stripes"));
		m_Params.push_back(ParamWithName<T>(true, &m_AngHoleComp,	   prefix + "bubbleT3D_ang_hole_comp"));
		m_Params.push_back(ParamWithName<T>(true, &m_InvHole,		   prefix + "bubbleT3D_inv_hole"));
		m_Params.push_back(ParamWithName<T>(true, &m_C,				   prefix + "bubbleT3D_c"));
		m_Params.push_back(ParamWithName<T>(true, &m_S,				   prefix + "bubbleT3D_s"));
	}

private:
	T m_NumberStripes;
	T m_RatioStripes;
	T m_AngleHole;
	T m_ExponentZ;
	T m_SymmetryZ;
	T m_ModusBlur;
	T m_AbsNumberStripes;//Precalc.
	T m_AngleHoleTemp;
	T m_AngStrip;
	T m_AngStrip1;
	T m_AngStrip2;
	T m_InvStripes;
	T m_AngHoleComp;
	T m_InvHole;
	T m_C;
	T m_S;
};

// -------------------------------------------------------------
// Modes
// "Lagacy" modes from v1
#define MODE_SPHERICAL 0
#define MODE_BUBBLE 1
#define MODE_BLUR_LEGACY 2
// New modes in v2
#define MODE_BLUR_NEW 3
#define MODE_BLUR_ZIGZAG 4
#define MODE_RAWCIRCLE 5
#define MODE_RAWX 6
#define MODE_RAWY 7
#define MODE_RAWXY 8
#define MODE_SHIFTX 9
#define MODE_SHIFTY 10
#define MODE_SHIFTXY 11
#define MODE_SINUSOIDAL 12
#define MODE_SWIRL 13
#define MODE_HYPERBOLIC 14
#define MODE_JULIA 15
#define MODE_DISC 16
#define MODE_RINGS 17
#define MODE_CYLINDER 18
#define MODE_BLUR_RING 19
#define MODE_BLUR_RING2 20
#define MODE_SHIFTTHETA 21

// -------------------------------------------------------------
// Wave types
#define WAVE_SIN 0
#define WAVE_COS 1
#define WAVE_SQUARE 2
#define WAVE_SAW 3
#define WAVE_TRIANGLE 4
#define WAVE_CONCAVE 5
#define WAVE_CONVEX 6
#define WAVE_NGON 7
// New wave types in v2
#define WAVE_INGON 8

// -------------------------------------------------------------
// Layer types
#define LAYER_ADD 0
#define LAYER_MULT 1
#define LAYER_MAX 2
#define LAYER_MIN 3

// -------------------------------------------------------------
// Interpolation types
#define LERP_LINEAR 0
#define LERP_BEZIER 1

// -------------------------------------------------------------
// Sine/Cosine interpretation types
#define SINCOS_MULTIPLY 0
#define SINCOS_MIXIN 1

/// <summary>
/// synth.
/// </summary>
template <typename T>
class EMBER_API SynthVariation : public ParametricVariation<T>
{
public:
	SynthVariation(T weight = 1.0) : ParametricVariation<T>("synth", VAR_SYNTH, weight, true, true, false, true)
	{
		Init();
	}

	PARVARCOPY(SynthVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		T Vx, Vy, radius, theta; // Position vector in cartesian and polar co-ords
		T thetaFactor;          // Evaluation of synth() function for current point
		T s, c, mu;              // Handy temp variables, s & c => sine & cosine, mu = generic temp param
		int synthMode = int(m_SynthMode);
		SynthStruct synth;
		synth.SynthA = m_SynthA;
		synth.SynthB = m_SynthB;
		synth.SynthBPhs = m_SynthBPhs;
		synth.SynthBFrq = m_SynthBFrq;
		synth.SynthBSkew = m_SynthBSkew;
		synth.SynthBType = int(m_SynthBType);
		synth.SynthBLayer = int(m_SynthBLayer);
		synth.SynthC = m_SynthC;
		synth.SynthCPhs = m_SynthCPhs;
		synth.SynthCFrq = m_SynthCFrq;
		synth.SynthCSkew = m_SynthCSkew;
		synth.SynthCType = int(m_SynthCType);
		synth.SynthCLayer = int(m_SynthCLayer);
		synth.SynthD = m_SynthD;
		synth.SynthDPhs = m_SynthDPhs;
		synth.SynthDFrq = m_SynthDFrq;
		synth.SynthDSkew = m_SynthDSkew;
		synth.SynthDType = int(m_SynthDType);
		synth.SynthDLayer = int(m_SynthDLayer);
		synth.SynthE = m_SynthE;
		synth.SynthEPhs = m_SynthEPhs;
		synth.SynthEFrq = m_SynthEFrq;
		synth.SynthESkew = m_SynthESkew;
		synth.SynthEType = int(m_SynthEType);
		synth.SynthELayer = int(m_SynthELayer);
		synth.SynthF = m_SynthF;
		synth.SynthFPhs = m_SynthFPhs;
		synth.SynthFFrq = m_SynthFFrq;
		synth.SynthFSkew = m_SynthFSkew;
		synth.SynthFType = int(m_SynthFType);
		synth.SynthFLayer = int(m_SynthFLayer);
		synth.SynthMix = m_SynthMix;

		switch (synthMode)
		{
			case MODE_SPHERICAL:  // Power YES, Smooth YES
				// Re-write of spherical with synth tweak
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), (m_SynthPower + 1) / 2);
				// Get angle and angular factor
				theta = helper.m_PrecalcAtanxy;
				thetaFactor = SynthValue(synth, theta);
				radius = Interpolate(radius, thetaFactor, synthMode);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			case MODE_BUBBLE:  // Power NO, Smooth YES
				// Re-write of bubble with synth tweak
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = helper.m_PrecalcSqrtSumSquares / (helper.m_PrecalcSumSquares / 4 + 1);
				// Get angle and angular factor
				theta = helper.m_PrecalcAtanxy;
				thetaFactor = SynthValue(synth, theta);
				radius = Interpolate(radius, thetaFactor, synthMode);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			case MODE_BLUR_LEGACY:  // Power YES, Smooth YES
				// "old" blur style, has some problems with moire-style artefacts
				radius = (rand.Frand01<T>() + rand.Frand01<T>() + T(0.002) * rand.Frand01<T>()) / T(2.002);
				theta = M_2PI * rand.Frand01<T>() - T(M_PI);
				Vx = radius * std::sin(theta);
				Vy = radius * std::cos(theta);
				radius = std::pow(Zeps<T>(radius * radius), m_SynthPower / 2);
				// Get angle and angular factor
				thetaFactor = SynthValue(synth, theta);
				radius = m_Weight * Interpolate(radius, thetaFactor, synthMode);
				// Write back to running totals for new vector
				helper.Out.x = Vx * radius;
				helper.Out.y = Vy * radius;
				break;

			case MODE_BLUR_NEW:  // Power YES, Smooth YES
				// Blur style, with normal smoothing function
				// Choose radius randomly, then adjust distribution using pow
				radius = T(0.5) * (rand.Frand01<T>() + rand.Frand01<T>());
				theta = M_2PI * rand.Frand01<T>() - T(M_PI);
				radius = std::pow(Zeps<T>(SQR(radius)), -m_SynthPower / 2);
				// Get angular factor defining the shape
				thetaFactor = SynthValue(synth, theta);
				// Get final radius after synth applied
				radius = Interpolate(radius, thetaFactor, synthMode);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			case MODE_BLUR_ZIGZAG:  // Power YES, Smooth YES
				// Blur effect based on line segment
				// theta is used as x value
				// Vy is y value
				Vy = 1 + T(0.1) * (rand.Frand01<T>() + rand.Frand01<T>() - 1) * m_SynthPower;
				theta = 2 * std::asin((rand.Frand01<T>() - T(0.5)) * 2);
				// Get angular factor defining the shape
				thetaFactor = SynthValue(synth, theta);
				// Get new location
				Vy = Interpolate(Vy, thetaFactor, synthMode);
				// Write to running totals for transform
				helper.Out.x = m_Weight * (theta / T(M_PI));
				helper.Out.y = m_Weight * (Vy - 1);
				break;

			case MODE_RAWCIRCLE:  // Power NO, Smooth YES
				// Get current radius and angle
				radius = helper.m_PrecalcSqrtSumSquares;
				theta = helper.m_PrecalcAtanxy;
				// Calculate new radius
				thetaFactor = SynthValue(synth, theta);
				radius = Interpolate(radius, thetaFactor, synthMode);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			case MODE_RAWX:  // Power NO, Smooth YES
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// x value will be mapped according to synth(y) value
				thetaFactor = SynthValue(synth, Vy);
				// Write to running totals for transform
				helper.Out.x = m_Weight * Interpolate(Vx, thetaFactor, synthMode);
				helper.Out.y = m_Weight * Vy;
				break;

			case MODE_RAWY:  // Power NO, Smooth YES
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// y value will be mapped according to synth(x) value
				thetaFactor = SynthValue(synth, Vx);
				// Write to running totals for transform
				helper.Out.x = m_Weight * Vx;
				helper.Out.y = m_Weight * Interpolate(Vy, thetaFactor, synthMode);
				break;

			case MODE_RAWXY:  // Power NO, Smooth YES
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// x value will be mapped according to synth(y) value
				thetaFactor = SynthValue(synth, Vy);
				helper.Out.x = m_Weight * Interpolate(Vx, thetaFactor, synthMode);
				// y value will be mapped according to synth(x) value
				thetaFactor = SynthValue(synth, Vx);
				helper.Out.y = m_Weight * Interpolate(Vy, thetaFactor, synthMode);
				break;

			case MODE_SHIFTX:  // Power NO, Smooth YES
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// Write to running totals for transform
				helper.Out.x = m_Weight * (Vx + SynthValue(synth, Vy) - 1);
				helper.Out.y = m_Weight * Vy;
				break;

			case MODE_SHIFTY:  // Power NO, Smooth NO
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// Write to running totals for transform
				helper.Out.x = m_Weight * Vx;
				helper.Out.y = m_Weight * (Vy + SynthValue(synth, Vx) - 1);
				break;

			case MODE_SHIFTXY:  // Power NO, Smooth NO
				// Use x and y values directly
				Vx = helper.In.x;
				Vy = helper.In.y;
				// Write to running totals for transform
				helper.Out.x = m_Weight * (Vx + SynthValue(synth, Vy) - 1);
				helper.Out.y = m_Weight * (Vy + SynthValue(synth, Vx) - 1);
				break;

			case MODE_SINUSOIDAL:  // Power NO, Smooth NO
				Vx = helper.In.x;
				Vy = helper.In.y;
				// The default mix=0 is same as normal sin
				helper.Out.x = m_Weight * (SynthValue(synth, Vx) - 1 + (1 - m_SynthMix) * std::sin(Vx));
				helper.Out.y = m_Weight * (SynthValue(synth, Vy) - 1 + (1 - m_SynthMix) * std::sin(Vy));
				break;

			case MODE_SWIRL:  // Power YES, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 2);
				// Synth-modified sine & cosine
				SynthSinCos(synth, radius, s, c, synthMode);
				helper.Out.x = m_Weight * (s * Vx - c * Vy);
				helper.Out.y = m_Weight * (c * Vx + s * Vy);
				break;

			case MODE_HYPERBOLIC:  // Power YES, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 2);
				theta = helper.m_PrecalcAtanxy;
				// Synth-modified sine & cosine
				SynthSinCos(synth, theta, s, c, synthMode);
				helper.Out.x = m_Weight * s / radius;
				helper.Out.y = m_Weight * c * radius;
				break;

			case MODE_JULIA: // Power YES, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 4);
				theta = helper.m_PrecalcAtanxy / 2;

				if (rand.Frand01<T>() < T(0.5))
					theta += T(M_PI);

				// Synth-modified sine & cosine
				SynthSinCos(synth, theta, s, c, synthMode);
				helper.Out.x = m_Weight * radius * c;
				helper.Out.y = m_Weight * radius * s;
				break;

			case MODE_DISC: // Power YES, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				theta = helper.m_PrecalcAtanxy / T(M_PI);
				radius = T(M_PI) * std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 2);
				// Synth-modified sine & cosine
				SynthSinCos(synth, radius, s, c, synthMode);
				helper.Out.x = m_Weight * s * theta;
				helper.Out.y = m_Weight * c * theta;
				break;

			case MODE_RINGS: // Power PARAM, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = helper.m_PrecalcSqrtSumSquares;
				theta = helper.m_PrecalcAtanxy;
				mu = Zeps<T>(SQR(m_SynthPower));
				radius += -2 * mu * int((radius + mu) / (2 * mu)) + radius * (1 - mu);
				SynthSinCos(synth, radius, s, c, synthMode);
				helper.Out.x = m_Weight * s * theta;
				helper.Out.y = m_Weight * c * theta;
				break;

			case MODE_CYLINDER: // Power YES, Smooth WAVE
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 2);
				// Modified sine only used here
				SynthSinCos(synth, Vx, s, c, synthMode);
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * Vy;
				break;

			case MODE_BLUR_RING:  // Power YES, Smooth YES
				// Blur style, with normal smoothing function
				radius = 1 + T(0.1) * (rand.Frand01<T>() + rand.Frand01<T>() - 1) * m_SynthPower;
				theta = M_2PI * rand.Frand01<T>() - T(M_PI);
				// Get angular factor defining the shape
				thetaFactor = SynthValue(synth, theta);
				// Get final radius after synth applied
				radius = Interpolate(radius, thetaFactor, synthMode);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			case MODE_BLUR_RING2:  // Power YES, Smooth NO
				// Simple, same-thickness ring
				// Choose radius randomly, then adjust distribution using pow
				theta = M_2PI * rand.Frand01<T>() - T(M_PI);
				radius = std::pow(Zeps<T>(rand.Frand01<T>()), m_SynthPower);
				// Get final radius after synth applied
				radius = SynthValue(synth, theta) + T(0.1) * radius;
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;

			default:
			case MODE_SHIFTTHETA:  // Power YES, Smooth NO
				// Use (adjusted) radius to move point around circle
				Vx = helper.In.x;
				Vy = helper.In.y;
				radius = std::pow(Zeps<T>(helper.m_PrecalcSumSquares), m_SynthPower / 2);
				theta = helper.m_PrecalcAtanxy - 1 + SynthValue(synth, radius);
				sincos(theta, &s, &c);
				// Write to running totals for transform
				helper.Out.x = m_Weight * radius * s;
				helper.Out.y = m_Weight * radius * c;
				break;
		}

		helper.Out.z = (m_VarType == VARTYPE_REG) ? 0 : helper.In.z;
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0, varIndex = IndexInXform();
		ss2 << "_" << XformIndexInEmber();
		string index = ss2.str() + "]";
		string synthA	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthMode   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthPower  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthMix	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthSmooth = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthB	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthBType  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthBSkew  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthBFrq   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthBPhs   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthBLayer = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthC	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthCType  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthCSkew  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthCFrq   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthCPhs   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthCLayer = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthD	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthDType  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthDSkew  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthDFrq   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthDPhs   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthDLayer = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthE	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthEType  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthESkew  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthEFrq   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthEPhs   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthELayer = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthF	   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthFType  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthFSkew  = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthFFrq   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthFPhs   = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string synthFLayer = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   << "\t\treal_t Vx, Vy, radius, theta;\n"
		   << "\t\treal_t thetaFactor;\n"
		   << "\t\treal_t s, c, mu;\n"
		   << "\t\tint synthMode = (int)" << synthMode << ";\n"
		   << "\t\tSynthStruct synth;\n"
		   << "\n"
		   << "\t\tsynth.SynthA = " << synthA << ";\n"
		   << "\t\tsynth.SynthB = " << synthB << ";\n"
		   << "\t\tsynth.SynthBPhs = " << synthBPhs << ";\n"
		   << "\t\tsynth.SynthBFrq = " << synthBFrq << ";\n"
		   << "\t\tsynth.SynthBSkew = " << synthBSkew << ";\n"
		   << "\t\tsynth.SynthBType = (int)" << synthBType << ";\n"
		   << "\t\tsynth.SynthBLayer = (int)" << synthBLayer << ";\n"
		   << "\t\tsynth.SynthC = " << synthC << ";\n"
		   << "\t\tsynth.SynthCPhs = " << synthCPhs << ";\n"
		   << "\t\tsynth.SynthCFrq = " << synthCFrq << ";\n"
		   << "\t\tsynth.SynthCSkew = " << synthCSkew << ";\n"
		   << "\t\tsynth.SynthCType = (int)" << synthCType << ";\n"
		   << "\t\tsynth.SynthCLayer = (int)" << synthCLayer << ";\n"
		   << "\t\tsynth.SynthD = " << synthD << ";\n"
		   << "\t\tsynth.SynthDPhs = " << synthDPhs << ";\n"
		   << "\t\tsynth.SynthDFrq = " << synthDFrq << ";\n"
		   << "\t\tsynth.SynthDSkew = " << synthDSkew << ";\n"
		   << "\t\tsynth.SynthDType = (int)" << synthDType << ";\n"
		   << "\t\tsynth.SynthDLayer = (int)" << synthDLayer << ";\n"
		   << "\t\tsynth.SynthE = " << synthE << ";\n"
		   << "\t\tsynth.SynthEPhs = " << synthEPhs << ";\n"
		   << "\t\tsynth.SynthEFrq = " << synthEFrq << ";\n"
		   << "\t\tsynth.SynthESkew = " << synthESkew << ";\n"
		   << "\t\tsynth.SynthEType = (int)" << synthEType << ";\n"
		   << "\t\tsynth.SynthELayer = (int)" << synthELayer << ";\n"
		   << "\t\tsynth.SynthF = " << synthF << ";\n"
		   << "\t\tsynth.SynthFPhs = " << synthFPhs << ";\n"
		   << "\t\tsynth.SynthFFrq = " << synthFFrq << ";\n"
		   << "\t\tsynth.SynthFSkew = " << synthFSkew << ";\n"
		   << "\t\tsynth.SynthFType = (int)" << synthFType << ";\n"
		   << "\t\tsynth.SynthFLayer = (int)" << synthFLayer << ";\n"
		   << "\t\tsynth.SynthMix = " << synthMix << ";\n"
		   << "\n"
		   << "\t\tswitch (synthMode)\n"
		   << "\t\t{\n"
		   << "\t\tcase MODE_SPHERICAL:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), (" << synthPower << " + 1) / 2);\n"
		   << "\t\t	theta = precalcAtanxy;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BUBBLE:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = precalcSqrtSumSquares / (precalcSumSquares / 4 + 1);\n"
		   << "\t\t	theta = precalcAtanxy;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BLUR_LEGACY:\n"
		   << "\t\t	radius = (MwcNext01(mwc) + MwcNext01(mwc) + 0.002 * MwcNext01(mwc)) / 2.002;\n"
		   << "\t\t	theta = M_2PI * MwcNext01(mwc) - M_PI;\n"
		   << "\t\t	Vx = radius * sin(theta);\n"
		   << "\t\t	Vy = radius * cos(theta);\n"
		   << "\t\t	radius = pow(Zeps(radius * radius), " << synthPower << " / 2);\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = xform->m_VariationWeights[" << varIndex << "] * Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	vOut.x = Vx * radius;\n"
		   << "\t\t	vOut.y = Vy * radius;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BLUR_NEW:\n"
		   << "\t\t	radius = 0.5 * (MwcNext01(mwc) + MwcNext01(mwc));\n"
		   << "\t\t	theta = M_2PI * MwcNext01(mwc) - M_PI;\n"
		   << "\t\t	radius = pow(Zeps(SQR(radius)), -" << synthPower << " / 2);\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BLUR_ZIGZAG:\n"
		   << "\t\t	Vy = 1 + 0.1 * (MwcNext01(mwc) + MwcNext01(mwc) - 1) * " << synthPower << ";\n"
		   << "\t\t	theta = 2 * asin((MwcNext01(mwc) - 0.5) * 2);\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	Vy = Interpolate(Vy, thetaFactor, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * (theta / M_PI);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * (Vy - 1);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_RAWCIRCLE:\n"
		   << "\t\t	radius = precalcSqrtSumSquares;\n"
		   << "\t\t	theta = precalcAtanxy;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_RAWX:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, Vy);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * Interpolate(Vx, thetaFactor, synthMode);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * Vy;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_RAWY:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, Vx);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * Vx;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * Interpolate(Vy, thetaFactor, synthMode);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_RAWXY:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, Vy);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * Interpolate(Vx, thetaFactor, synthMode);\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, Vx);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * Interpolate(Vy, thetaFactor, synthMode);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SHIFTX:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * (Vx + SynthValue(&synth, Vy) - 1);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * Vy;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SHIFTY:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * Vx;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * (Vy + SynthValue(&synth, Vx) - 1);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SHIFTXY:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * (Vx + SynthValue(&synth, Vy) - 1);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * (Vy + SynthValue(&synth, Vx) - 1);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SINUSOIDAL:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * (SynthValue(&synth, Vx) - 1 + (1 - " << synthMix << ") * sin(Vx));\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * (SynthValue(&synth, Vy) - 1 + (1 - " << synthMix << ") * sin(Vy));\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SWIRL:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), " << synthPower << " / 2);\n"
		   << "\t\t	SynthSinCos(&synth, radius, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * (s * Vx - c * Vy);\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * (c * Vx + s * Vy);\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_HYPERBOLIC:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), " << synthPower << " / 2);\n"
		   << "\t\t	theta = precalcAtanxy;\n"
		   << "\t\t	SynthSinCos(&synth, theta, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * s / radius;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * c * radius;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_JULIA:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), " << synthPower << " / 4);\n"
		   << "\t\t	theta = precalcAtanxy / 2;\n"
		   << "\n"
		   << "\t\t	if (MwcNext01(mwc) < 0.5)\n"
		   << "\t\t		theta += M_PI;\n"
		   << "\n"
		   << "\t\t	SynthSinCos(&synth, theta, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_DISC:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	theta = precalcAtanxy / M_PI;\n"
		   << "\t\t	radius = M_PI * pow(Zeps(precalcSumSquares), " << synthPower << " / 2);\n"
		   << "\t\t	SynthSinCos(&synth, radius, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * s * theta;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * c * theta;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_RINGS:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = precalcSqrtSumSquares;\n"
		   << "\t\t	theta = precalcAtanxy;\n"
		   << "\t\t	mu = Zeps(SQR(" << synthPower << "));\n"
		   << "\t\t	radius += -2 * mu * (int)((radius + mu) / (2 * mu)) + radius * (1 - mu);\n"
		   << "\t\t	SynthSinCos(&synth, radius, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * s * theta;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * c * theta;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_CYLINDER:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), " << synthPower << " / 2);\n"
		   << "\t\t	SynthSinCos(&synth, Vx, &s, &c, synthMode);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * Vy;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BLUR_RING:\n"
		   << "\t\t	radius = 1 + 0.1 * (MwcNext01(mwc) + MwcNext01(mwc) - 1) * " << synthPower << ";\n"
		   << "\t\t	theta = M_2PI * MwcNext01(mwc) - M_PI;\n"
		   << "\t\t	thetaFactor = SynthValue(&synth, theta);\n"
		   << "\t\t	radius = Interpolate(radius, thetaFactor, synthMode);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_BLUR_RING2:\n"
		   << "\t\t	theta = M_2PI * MwcNext01(mwc) - M_PI;\n"
		   << "\t\t	radius = pow(Zeps(MwcNext01(mwc)), " << synthPower << ");\n"
		   << "\t\t	radius = SynthValue(&synth, theta) + 0.1 * radius;\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\n"
		   << "\t\tcase MODE_SHIFTTHETA:\n"
		   << "\t\t	Vx = vIn.x;\n"
		   << "\t\t	Vy = vIn.y;\n"
		   << "\t\t	radius = pow(Zeps(precalcSumSquares), " << synthPower << " / 2);\n"
		   << "\t\t	theta = precalcAtanxy - 1 + SynthValue(&synth, radius);\n"
		   << "\t\t	s = sincos(theta, &c);\n"
		   << "\t\t	vOut.x = xform->m_VariationWeights[" << varIndex << "] * radius * s;\n"
		   << "\t\t	vOut.y = xform->m_VariationWeights[" << varIndex << "] * radius * c;\n"
		   << "\t\t	break;\n"
		   << "\t\t}\n"
		   << "\n"
		   << "\t\tvOut.z = " << ((m_VarType == VARTYPE_REG) ? "0" : "vIn.z") << ";\n"
		   << "\t}\n";
		return ss.str();
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Zeps" };
	}

	virtual string OpenCLFuncsString() const override
	{
		return
			"#define MODE_SPHERICAL 0\n"
			"#define MODE_BUBBLE 1\n"
			"#define MODE_BLUR_LEGACY 2\n"
			"#define MODE_BLUR_NEW 3\n"
			"#define MODE_BLUR_ZIGZAG 4\n"
			"#define MODE_RAWCIRCLE 5\n"
			"#define MODE_RAWX 6\n"
			"#define MODE_RAWY 7\n"
			"#define MODE_RAWXY 8\n"
			"#define MODE_SHIFTX 9\n"
			"#define MODE_SHIFTY 10\n"
			"#define MODE_SHIFTXY 11\n"
			"#define MODE_SINUSOIDAL 12\n"
			"#define MODE_SWIRL 13\n"
			"#define MODE_HYPERBOLIC 14\n"
			"#define MODE_JULIA 15\n"
			"#define MODE_DISC 16\n"
			"#define MODE_RINGS 17\n"
			"#define MODE_CYLINDER 18\n"
			"#define MODE_BLUR_RING 19\n"
			"#define MODE_BLUR_RING2 20\n"
			"#define MODE_SHIFTTHETA 21\n"
			"#define WAVE_SIN 0\n"
			"#define WAVE_COS 1\n"
			"#define WAVE_SQUARE 2\n"
			"#define WAVE_SAW 3\n"
			"#define WAVE_TRIANGLE 4\n"
			"#define WAVE_CONCAVE 5\n"
			"#define WAVE_CONVEX 6\n"
			"#define WAVE_NGON 7\n"
			"#define WAVE_INGON 8\n"
			"#define LAYER_ADD 0\n"
			"#define LAYER_MULT 1\n"
			"#define LAYER_MAX 2\n"
			"#define LAYER_MIN 3\n"
			"#define LERP_LINEAR 0\n"
			"#define LERP_BEZIER 1\n"
			"#define SINCOS_MULTIPLY 0\n"
			"#define SINCOS_MIXIN 1\n"
			"\n"
			"typedef struct __attribute__ " ALIGN_CL " _SynthStruct\n"
			"{\n"
			"	real_t SynthA;\n"
			"	real_t SynthB;\n"
			"	real_t SynthBPhs;\n"
			"	real_t SynthBFrq;\n"
			"	real_t SynthBSkew;\n"
			"	int SynthBType;\n"
			"	int SynthBLayer;\n"
			"	real_t SynthC;\n"
			"	real_t SynthCPhs;\n"
			"	real_t SynthCFrq;\n"
			"	real_t SynthCSkew;\n"
			"	int SynthCType;\n"
			"	int SynthCLayer;\n"
			"	real_t SynthD;\n"
			"	real_t SynthDPhs;\n"
			"	real_t SynthDFrq;\n"
			"	real_t SynthDSkew;\n"
			"	int SynthDType;\n"
			"	int SynthDLayer;\n"
			"	real_t SynthE;\n"
			"	real_t SynthEPhs;\n"
			"	real_t SynthEFrq;\n"
			"	real_t SynthESkew;\n"
			"	int SynthEType;\n"
			"	int SynthELayer;\n"
			"	real_t SynthF;\n"
			"	real_t SynthFPhs;\n"
			"	real_t SynthFFrq;\n"
			"	real_t SynthFSkew;\n"
			"	int SynthFType;\n"
			"	int SynthFLayer;\n"
			"	real_t SynthMix;\n"
			"} SynthStruct;\n"
			"\n"
			"static void SynthValueProc(real_t* synthA, real_t* thetaFactor, real_t theta, real_t* synth, real_t* phs, real_t* frq, real_t* skew, real_t* x, real_t* y, real_t* z, int* type, int* layer)\n"
			"{\n"
			"	if (*synth != 0)\n"
			"	{\n"
			"		*z = *phs + theta * *frq;\n"
			"		*y = *z / M_2PI;\n"
			"		*y -= floor(*y);\n"
			"\n"
			"		if (*skew != 0)\n"
			"		{\n"
			"			*z = 0.5 + 0.5 * *skew;\n"
			"\n"
			"			if (*y > *z)\n"
			"				*y = 0.5 + 0.5 * (*y - *z) / Zeps(1 - *z);\n"
			"			else\n"
			"				*y = 0.5 - 0.5 * (*z - *y) / Zeps(*z);\n"
			"		}\n"
			"\n"
			"		switch (*type)\n"
			"		{\n"
			"			case WAVE_SIN:\n"
			"				*x = sin(*y * M_2PI);\n"
			"				break;\n"
			"			case WAVE_COS:\n"
			"				*x = cos(*y * M_2PI);\n"
			"				break;\n"
			"			case WAVE_SQUARE:\n"
			"				*x = *y > 0.5 ? 1.0 : -1.0;\n"
			"				break;\n"
			"			case WAVE_SAW:\n"
			"				*x = 1 - 2 * *y;\n"
			"				break;\n"
			"			case WAVE_TRIANGLE:\n"
			"				*x = *y > 0.5 ? 3 - 4 * *y : 2 * *y - 1;\n"
			"				break;\n"
			"			case WAVE_CONCAVE:\n"
			"				*x = 8 * (*y - 0.5) * (*y - 0.5) - 1;\n"
			"				break;\n"
			"			case WAVE_CONVEX:\n"
			"				*x = 2 * sqrt(*y) - 1;\n"
			"				break;\n"
			"			case WAVE_NGON:\n"
			"				*y -= 0.5;\n"
			"				*y *= M_2PI / *frq;\n"
			"				*x = 1 / Zeps(cos(*y)) - 1;\n"
			"				break;\n"
			"			case WAVE_INGON:\n"
			"				*y -= 0.5;\n"
			"				*y *= M_2PI / *frq;\n"
			"				*z = cos(*y);\n"
			"				*x = *z / Zeps(1 - *z);\n"
			"				break;\n"
			"		}\n"
			"\n"
			"		switch (*layer)\n"
			"		{\n"
			"			case LAYER_ADD:\n"
			"				*thetaFactor += *synth * *x;\n"
			"				break;\n"
			"			case LAYER_MULT:\n"
			"				*thetaFactor *= (1 + *synth * *x);\n"
			"				break;\n"
			"			case LAYER_MAX:\n"
			"				*z = *synthA + *synth * *x;\n"
			"				*thetaFactor = (*thetaFactor > *z ? *thetaFactor : *z);\n"
			"				break;\n"
			"			case LAYER_MIN:\n"
			"				*z = *synthA + *synth * *x;\n"
			"				*thetaFactor = (*thetaFactor < *z ? *thetaFactor : *z);\n"
			"				break;\n"
			"		}\n"
			"	}\n"
			"}\n"
			"\n"
			"static real_t SynthValue(SynthStruct* s, real_t theta)\n"
			"{\n"
			"	real_t x, y, z;\n"
			"	real_t thetaFactor = s->SynthA;\n"
			"\n"
			"	SynthValueProc(&(s->SynthA), &thetaFactor, theta, &(s->SynthB), &(s->SynthBPhs), &(s->SynthBFrq), &(s->SynthBSkew), &x, &y, &z, &(s->SynthBType), &(s->SynthBLayer));\n"
			"	SynthValueProc(&(s->SynthA), &thetaFactor, theta, &(s->SynthC), &(s->SynthCPhs), &(s->SynthCFrq), &(s->SynthCSkew), &x, &y, &z, &(s->SynthCType), &(s->SynthCLayer));\n"
			"	SynthValueProc(&(s->SynthA), &thetaFactor, theta, &(s->SynthD), &(s->SynthDPhs), &(s->SynthDFrq), &(s->SynthDSkew), &x, &y, &z, &(s->SynthDType), &(s->SynthDLayer));\n"
			"	SynthValueProc(&(s->SynthA), &thetaFactor, theta, &(s->SynthE), &(s->SynthEPhs), &(s->SynthEFrq), &(s->SynthESkew), &x, &y, &z, &(s->SynthEType), &(s->SynthELayer));\n"
			"	SynthValueProc(&(s->SynthA), &thetaFactor, theta, &(s->SynthF), &(s->SynthFPhs), &(s->SynthFFrq), &(s->SynthFSkew), &x, &y, &z, &(s->SynthFType), &(s->SynthFLayer));\n"
			"\n"
			"	return thetaFactor * s->SynthMix + (1 - s->SynthMix);\n"
			"}\n"
			"\n"
			"static real_t BezierQuadMap(real_t x, real_t m)\n"
			"{\n"
			"	real_t a = 1;\n"
			"	real_t t = 0;\n"
			"\n"
			"	if (m < 0) { m = -m; a = -1; }\n"
			"	if (x < 0) { x = -x; a = -a; }\n"
			"\n"
			"	real_t iM = 1e10;\n"
			"\n"
			"	if (m > 1.0e-10)\n"
			"		iM = 1 / m;\n"
			"\n"
			"	real_t L = iM < m * 2 ? m * 2 : iM;\n"
			"\n"
			"	if ((x > L) || (m == 1))\n"
			"		return a * x;\n"
			"\n"
			"	if ((m < 1) && (x <= 1))\n"
			"	{\n"
			"		t = x;\n"
			"\n"
			"		if (fabs(m - 0.5) > 1e-10)\n"
			"			t = (-1 * m + sqrt(m * m + (1 - 2 * m) * x)) / (1 - 2 * m);\n"
			"\n"
			"		return a * (x + (m - 1) * t * t);\n"
			"	}\n"
			"\n"
			"	if ((1 < m) && (x <= 1))\n"
			"	{\n"
			"		t = x;\n"
			"\n"
			"		if (fabs(m - 2) > 1e-10)\n"
			"			t = (-1 * iM + sqrt(iM * iM + (1 - 2 * iM) * x)) / (1 - 2 * iM);\n"
			"\n"
			"		return a * (x + (m - 1) * t * t);\n"
			"	}\n"
			"\n"
			"	if (m < 1)\n"
			"	{\n"
			"		t = sqrt((x - 1) / (L - 1));\n"
			"		return a * (x + (m - 1) * t * t + 2 * (1 - m) * t + (m - 1));\n"
			"	}\n"
			"\n"
			"	t = (1 - m) + sqrt((m - 1) * (m - 1) + (x - 1));\n"
			"	return a * (x + (m - 1) * t * t - 2 * (m - 1) *  t + (m - 1));\n"
			"}\n"
			"\n"
			"static real_t Interpolate(real_t x, real_t m, int lerpType)\n"
			"{\n"
			"	switch (lerpType)\n"
			"	{\n"
			"		case LERP_LINEAR:\n"
			"			return x * m;\n"
			"		case LERP_BEZIER:\n"
			"			return BezierQuadMap(x, m);\n"
			"	}\n"
			"\n"
			"	return x * m;\n"
			"}\n"
			"\n"
			"static void SynthSinCos(SynthStruct* synth, real_t theta, real_t* s, real_t* c, int sineType)\n"
			"{\n"
			"	*s = sincos(theta, c);\n"
			"\n"
			"	switch (sineType)\n"
			"	{\n"
			"		case SINCOS_MULTIPLY:\n"
			"			*s = *s * SynthValue(synth, theta);\n"
			"			*c = *c * SynthValue(synth, theta + M_PI / 2);\n"
			"			break;\n"
			"		case SINCOS_MIXIN:\n"
			"			*s = (1 - synth->SynthMix) * *s + (SynthValue(synth, theta) - 1);\n"
			"			*c = (1 - synth->SynthMix) * *c + (SynthValue(synth, theta + M_PI / 2) - 1);\n"
			"			break;\n"
			"	}\n"
			"\n"
			"	return;\n"
			"}\n\n"
			;
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_Params.clear();
		m_Params.reserve(34);
		m_Params.push_back(ParamWithName<T>(&m_SynthA,		prefix + "synth_a"));
		m_Params.push_back(ParamWithName<T>(&m_SynthMode,	prefix + "synth_mode", 3, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthPower,	prefix + "synth_power", -2));
		m_Params.push_back(ParamWithName<T>(&m_SynthMix,	prefix + "synth_mix"));
		m_Params.push_back(ParamWithName<T>(&m_SynthSmooth, prefix + "synth_smooth", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthB,		prefix + "synth_b"));
		m_Params.push_back(ParamWithName<T>(&m_SynthBType,	prefix + "synth_b_type", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthBSkew,	prefix + "synth_b_skew"));
		m_Params.push_back(ParamWithName<T>(&m_SynthBFrq,	prefix + "synth_b_frq", 1, REAL));
		m_Params.push_back(ParamWithName<T>(&m_SynthBPhs,	prefix + "synth_b_phs"));
		m_Params.push_back(ParamWithName<T>(&m_SynthBLayer, prefix + "synth_b_layer", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthC,		prefix + "synth_c"));
		m_Params.push_back(ParamWithName<T>(&m_SynthCType,	prefix + "synth_c_type", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthCSkew,	prefix + "synth_c_skew"));
		m_Params.push_back(ParamWithName<T>(&m_SynthCFrq,	prefix + "synth_c_frq", 1, REAL));
		m_Params.push_back(ParamWithName<T>(&m_SynthCPhs,	prefix + "synth_c_phs"));
		m_Params.push_back(ParamWithName<T>(&m_SynthCLayer, prefix + "synth_c_layer", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthD,		prefix + "synth_d"));
		m_Params.push_back(ParamWithName<T>(&m_SynthDType,	prefix + "synth_d_type", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthDSkew,	prefix + "synth_d_skew"));
		m_Params.push_back(ParamWithName<T>(&m_SynthDFrq,	prefix + "synth_d_frq", 1, REAL));
		m_Params.push_back(ParamWithName<T>(&m_SynthDPhs,	prefix + "synth_d_phs"));
		m_Params.push_back(ParamWithName<T>(&m_SynthDLayer, prefix + "synth_d_layer", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthE,		prefix + "synth_e"));
		m_Params.push_back(ParamWithName<T>(&m_SynthEType,	prefix + "synth_e_type", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthESkew,	prefix + "synth_e_skew"));
		m_Params.push_back(ParamWithName<T>(&m_SynthEFrq,	prefix + "synth_e_frq", 1, REAL));
		m_Params.push_back(ParamWithName<T>(&m_SynthEPhs,	prefix + "synth_e_phs"));
		m_Params.push_back(ParamWithName<T>(&m_SynthELayer, prefix + "synth_e_layer", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthF,		prefix + "synth_f"));
		m_Params.push_back(ParamWithName<T>(&m_SynthFType,	prefix + "synth_f_type", 0, INTEGER));
		m_Params.push_back(ParamWithName<T>(&m_SynthFSkew,	prefix + "synth_f_skew"));
		m_Params.push_back(ParamWithName<T>(&m_SynthFFrq,	prefix + "synth_f_frq", 1, REAL));
		m_Params.push_back(ParamWithName<T>(&m_SynthFPhs,	prefix + "synth_f_phs"));
		m_Params.push_back(ParamWithName<T>(&m_SynthFLayer, prefix + "synth_f_layer", 0, INTEGER));
	}

private:
	struct SynthStruct
	{
		T SynthA;
		T SynthB;
		T SynthBPhs;
		T SynthBFrq;
		T SynthBSkew;
		int SynthBType;
		int SynthBLayer;
		T SynthC;
		T SynthCPhs;
		T SynthCFrq;
		T SynthCSkew;
		int SynthCType;
		int SynthCLayer;
		T SynthD;
		T SynthDPhs;
		T SynthDFrq;
		T SynthDSkew;
		int SynthDType;
		int SynthDLayer;
		T SynthE;
		T SynthEPhs;
		T SynthEFrq;
		T SynthESkew;
		int SynthEType;
		int SynthELayer;
		T SynthF;
		T SynthFPhs;
		T SynthFFrq;
		T SynthFSkew;
		int SynthFType;
		int SynthFLayer;
		T SynthMix;
	};

	inline void SynthValueProc(T& synthA, T& thetaFactor, T theta, T& synth, T& phs, T& frq, T& skew, T& x, T& y, T& z, int& type, int& layer)
	{
		if (synth != 0)
		{
			z = phs + theta * frq;
			y = z / M_2PI;
			y -= Floor<T>(y);

			// y is in range 0 - 1. Now skew according to synth_f_skew
			if (skew != 0)
			{
				z = T(0.5) + T(0.5) * skew;

				if (y > z)
					y = T(0.5) + T(0.5) * (y - z) / Zeps<T>(1 - z);// y is T(0.5) if equals z, up to 1.0
				else
					y = T(0.5) - T(0.5) * (z - y) / Zeps<T>(z);// y is T(0.5) if equals z, down to 0.0
			}

			switch (type)
			{
				case WAVE_SIN:
					x = std::sin(y * M_2PI);
					break;

				case WAVE_COS:
					x = std::cos(y * M_2PI);
					break;

				case WAVE_SQUARE:
					x = y > T(0.5) ? T(1) : T(-1);
					break;

				case WAVE_SAW:
					x = 1 - 2 * y;
					break;

				case WAVE_TRIANGLE:
					x = y > T(0.5) ? 3 - 4 * y : 2 * y - 1;
					break;

				case WAVE_CONCAVE:
					x = 8 * (y - T(0.5)) * (y - T(0.5)) - 1;
					break;

				case WAVE_CONVEX:
					x = 2 * std::sqrt(y) - 1;
					break;

				case WAVE_NGON:
					y -= T(0.5);
					y *= M_2PI / frq;
					x = 1 / Zeps<T>(std::cos(y)) - 1;
					break;

				default:
				case WAVE_INGON:
					y -= T(0.5);
					y *= M_2PI / frq;
					z = std::cos(y);
					x = z / Zeps<T>(1 - z);
					break;
			}

			switch (layer)
			{
				case LAYER_ADD:
					thetaFactor += synth * x;
					break;

				case LAYER_MULT:
					thetaFactor *= (1 + synth * x);
					break;

				case LAYER_MAX:
					z = synthA + synth * x;
					thetaFactor = (thetaFactor > z ? thetaFactor : z);
					break;

				default:
				case LAYER_MIN:
					z = synthA + synth * x;
					thetaFactor = (thetaFactor < z ? thetaFactor : z);
					break;
			}
		}
	}

	inline T SynthValue(SynthStruct& s, T theta)
	{
		T x, y, z;
		T thetaFactor = s.SynthA;
		SynthValueProc(s.SynthA, thetaFactor, theta, s.SynthB, s.SynthBPhs, s.SynthBFrq, s.SynthBSkew, x, y, z, s.SynthBType, s.SynthBLayer);
		SynthValueProc(s.SynthA, thetaFactor, theta, s.SynthC, s.SynthCPhs, s.SynthCFrq, s.SynthCSkew, x, y, z, s.SynthCType, s.SynthCLayer);
		SynthValueProc(s.SynthA, thetaFactor, theta, s.SynthD, s.SynthDPhs, s.SynthDFrq, s.SynthDSkew, x, y, z, s.SynthDType, s.SynthDLayer);
		SynthValueProc(s.SynthA, thetaFactor, theta, s.SynthE, s.SynthEPhs, s.SynthEFrq, s.SynthESkew, x, y, z, s.SynthEType, s.SynthELayer);
		SynthValueProc(s.SynthA, thetaFactor, theta, s.SynthF, s.SynthFPhs, s.SynthFFrq, s.SynthFSkew, x, y, z, s.SynthFType, s.SynthFLayer);
		// Mix is applied here, assuming 1.0 to be the "flat" line for legacy support
		return thetaFactor * s.SynthMix + (1 - s.SynthMix);
	}

	inline T BezierQuadMap(T x, T m)
	{
		T a = 1; // a is used to control sign of result
		T t = 0; // t is the Bezier curve parameter

		// Simply reflect in the y axis for negative values
		if (m < 0) { m = -m; a = -1; }

		if (x < 0) { x = -x; a = -a; }

		// iM is "inverse m" used in a few places below
		T iM = T(1e10);

		if (m > 1.0e-10)
			iM = 1 / m;

		// L is the upper bound on our curves, where we have rejoined the y = x line
		T L = iM < m * 2 ? m * 2 : iM;

		// "Non Curved"
		// Covers x >= L, or always true if m == 1.0
		// y = x  i.e. not distorted
		if ((x > L) || (m == 1))
			return a * x;

		if ((m < 1) && (x <= 1))
		{
			// Bezier Curve #1
			// Covers 0 <= $m <= 1.0, 0 <= $x <= 1.0
			// Control points are (0,0), (m,m) and (1,m)
			t = x; // Special case when m == 0.5

			if (std::fabs(m - T(0.5)) > 1e-10)
				t = (-1 * m + std::sqrt(m * m + (1 - 2 * m) * x)) / (1 - 2 * m);

			return a * (x + (m - 1) * t * t);
		}

		if ((1 < m) && (x <= 1))
		{
			// Bezier Curve #2
			// Covers m >= 1.0, 0 <= x <= 1.0
			// Control points are (0,0), (iM,iM) and (1,m)
			t = x; // Special case when m == 2

			if (std::fabs(m - 2) > 1e-10)
				t = (-1 * iM + std::sqrt(iM * iM + (1 - 2 * iM) * x)) / (1 - 2 * iM);

			return a * (x + (m - 1) * t * t);
		}

		if (m < 1)
		{
			// Bezier Curve #3
			// Covers 0 <= m <= 1.0, 1 <= x <= L
			// Control points are (1,m), (1,1) and (L,L)
			// (L is x value (>1) where we re-join y = x line, and is maximum( iM, 2 * m )
			t = std::sqrt((x - 1) / (L - 1));
			return a * (x + (m - 1) * t * t + 2 * (1 - m) * t + (m - 1));
		}

		// Curve #4
		// Covers 1.0 <= m, 1 <= x <= L
		// Control points are (1,m), (m,m) and (L,L)
		// (L is x value (>1) where we re-join y = x line, and is maximum( iM, 2 *  m )
		t = (1 - m) + std::sqrt((m - 1) * (m - 1) + (x - 1));
		return a * (x + (m - 1) * t * t - 2 * (m - 1) *  t + (m - 1));
	}

	inline T Interpolate(T x, T m, int lerpType)
	{
		switch (lerpType)
		{
			case LERP_LINEAR:
				return x * m;

			default:
			case LERP_BEZIER:
				return BezierQuadMap(x, m);
		}

		return x * m;
	}

	inline void SynthSinCos(SynthStruct& synth, T theta, T& s, T& c, int sineType)
	{
		sincos(theta, &s, &c);

		switch (sineType)
		{
			case SINCOS_MULTIPLY:
				s = s * SynthValue(synth, theta);
				c = c * SynthValue(synth, theta + T(M_PI) / 2);
				break;

			default:
			case SINCOS_MIXIN:
				s = (1 - m_SynthMix) * s + (SynthValue(synth, theta) - 1);
				c = (1 - m_SynthMix) * c + (SynthValue(synth, theta + T(M_PI) / 2) - 1);
				break;
		}

		return;
	}

	T m_SynthA;
	T m_SynthMode;
	T m_SynthPower;
	T m_SynthMix;
	T m_SynthSmooth;
	T m_SynthB;
	T m_SynthBType;
	T m_SynthBSkew;
	T m_SynthBFrq;
	T m_SynthBPhs;
	T m_SynthBLayer;
	T m_SynthC;
	T m_SynthCType;
	T m_SynthCSkew;
	T m_SynthCFrq;
	T m_SynthCPhs;
	T m_SynthCLayer;
	T m_SynthD;
	T m_SynthDType;
	T m_SynthDSkew;
	T m_SynthDFrq;
	T m_SynthDPhs;
	T m_SynthDLayer;
	T m_SynthE;
	T m_SynthEType;
	T m_SynthESkew;
	T m_SynthEFrq;
	T m_SynthEPhs;
	T m_SynthELayer;
	T m_SynthF;
	T m_SynthFType;
	T m_SynthFSkew;
	T m_SynthFFrq;
	T m_SynthFPhs;
	T m_SynthFLayer;
};

#define CACHE_NUM 10
#define CACHE_WIDTH 21
#define VORONOI_MAXPOINTS 10

/// <summary>
/// crackle.
/// </summary>
template <typename T>
class EMBER_API CrackleVariation : public ParametricVariation<T>
{
public:
	CrackleVariation(T weight = 1.0) : ParametricVariation<T>("crackle", VAR_CRACKLE, weight)
	{
		Init();
	}

	PARVARCOPY(CrackleVariation)

	virtual void Func(IteratorHelper<T>& helper, Point<T>& outPoint, QTIsaac<ISAAC_SIZE, ISAAC_INT>& rand) override
	{
		int di, dj;
		int i = 0;
		T l, r, trgL;
		v2T u, dO;
		glm::ivec2 cv;
		v2T p[VORONOI_MAXPOINTS];

		if (m_CellSize == 0)
			return;

		T blurr = (rand.Frand01<T>() + rand.Frand01<T>()) / 2 + (rand.Frand01<T>() - T(0.5)) / 4;
		T theta = M_2PI * rand.Frand01<T>();
		u.x = blurr * std::sin(theta);
		u.y = blurr * std::cos(theta);
		cv.x = int(std::floor(u.x / m_HalfCellSize));
		cv.y = int(std::floor(u.y / m_HalfCellSize));

		for (di = -1; di < 2; di++)
		{
			for (dj = -1; dj < 2; dj++)
			{
				CachedPosition(m_C, cv.x + di, cv.y + dj, m_Z, m_HalfCellSize, m_Distort, p[i]);
				i++;
			}
		}

		int q = m_VarFuncs->Closest(p, 9, u);
		glm::ivec2 offset[9] = { { -1, -1 }, { -1, 0 }, { -1, 1 },
			{ 0, -1 }, { 0, 0 }, { 0, 1 },
			{ 1, -1 }, { 1, 0 }, { 1, 1 }
		};
		cv += offset[q];
		i = 0;

		for (di = -1; di < 2; di++)
		{
			for (dj = -1; dj < 2; dj++)
			{
				CachedPosition(m_C, cv.x + di, cv.y + dj, m_Z, m_HalfCellSize, m_Distort, p[i]);
				i++;
			}
		}

		l = m_VarFuncs->Voronoi(p, 9, 4, u);
		dO = u - p[4];
		trgL = std::pow(Zeps<T>(l), m_Power) * m_Scale;
		r = trgL / Zeps<T>(l);
		dO *= r;
		dO += p[4];
		helper.Out.x = m_Weight * dO.x;
		helper.Out.y = m_Weight * dO.y;
		helper.Out.z = (m_VarType == VARTYPE_REG) ? 0 : helper.In.z;
	}

	virtual vector<string> OpenCLGlobalFuncNames() const override
	{
		return vector<string> { "Zeps", "Sqr", "Closest", "Vratio", "Voronoi", "SimplexNoise3D" };
	}

	virtual vector<string> OpenCLGlobalDataNames() const override
	{
		return vector<string> { "NOISE_INDEX", "NOISE_POINTS" };
	}

	virtual string OpenCLFuncsString() const override
	{
		//CPU version uses a cache of points if the abs() values are <= 10. However, this crashes on Nvidia GPUs.
		//The problem was traced to the usage of the cache array.
		//No possible solution was found, so it is unused here.
		//The full calculation is recomputed for every point.
		return
			"static void Position(__global real_t* p, __global real3* grad, int x, int y, real_t z, real_t s, real_t d, real2* v)\n"
			"{\n"
			"	real3 e, f;\n"
			"	e.x = x * 2.5;\n"
			"	e.y = y * 2.5;\n"
			"	e.z = z * 2.5;\n"
			"	f.x = y * 2.5 + 30.2;\n"
			"	f.y = x * 2.5 - 12.1;\n"
			"	f.z = z * 2.5 + 19.8;\n"
			"	(*v).x = (x + d * SimplexNoise3D(&e, p, grad)) * s;\n"
			"	(*v).y = (y + d * SimplexNoise3D(&f, p, grad)) * s;\n"
			"}\n"
			"\n";
	}

	virtual string OpenCLString() const override
	{
		ostringstream ss, ss2;
		intmax_t i = 0, varIndex = IndexInXform();
		ss2 << "_" << XformIndexInEmber();
		string index = ss2.str() + "]";
		string cellSize     = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string power        = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string distort      = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string scale        = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string z            = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		string halfCellSize = "parVars[" + ToUpper(m_Params[i++].Name()) + index;
		ss << "\t{\n"
		   << "\t\tint di = -1, dj = -1;\n"
		   << "\t\tint i = 0;\n"
		   << "\t\treal_t l, r, trgL;\n"
		   << "\t\treal2 u, dO;\n"
		   << "\t\tint2 cv;\n"
		   << "\t\treal2 p[" << VORONOI_MAXPOINTS << "];\n"
		   << "\n"
		   << "\t\tif (" << cellSize << " == 0)\n"
		   << "\t\t	return;\n"
		   << "\n"
		   << "\t\treal_t blurr = (MwcNext01(mwc) + MwcNext01(mwc)) / 2 + (MwcNext01(mwc) - 0.5) / 4;\n"
		   << "\t\treal_t theta = M_2PI * MwcNext01(mwc);\n"
		   << "\t\tu.x = blurr * sin(theta);\n"
		   << "\t\tu.y = blurr * cos(theta);\n"
		   << "\t\tcv.x = (int)floor(u.x / " << halfCellSize << ");\n"
		   << "\t\tcv.y = (int)floor(u.y / " << halfCellSize << ");\n"
		   << "\n"
		   << "\t\tfor (di = -1; di < 2; di++)\n"
		   << "\t\t{\n"
		   << "\t\t	for (dj = -1; dj < 2; dj++)\n"
		   << "\t\t	{\n"
		   << "\t\t		Position(globalShared + NOISE_INDEX, (__global real3*)(globalShared + NOISE_POINTS), cv.x + di, cv.y + dj, " << z << ", " << halfCellSize << ", " << distort << ", &p[i]);\n"
		   << "\t\t		i++;\n"
		   << "\t\t	}\n"
		   << "\t\t}\n"
		   << "\n"
		   << "\t\tint q = Closest(p, 9, &u);\n"
		   << "\t\tint2 offset[9] = { { -1, -1 }, { -1, 0 }, { -1, 1 }, \n"
		   << "\t\t{ 0, -1 }, { 0, 0 }, { 0, 1 },\n"
		   << "\t\t{ 1, -1 }, { 1, 0 }, { 1, 1 } };\n"
		   << "\t\tcv += offset[q];\n"
		   << "\t\ti = 0;\n"
		   << "\n"
		   << "\t\tfor (di = -1; di < 2; di++)\n"
		   << "\t\t{\n"
		   << "\t\t	for (dj = -1; dj < 2; dj++)\n"
		   << "\t\t	{\n"
		   << "\t\t		Position(globalShared + NOISE_INDEX, (__global real3*)(globalShared + NOISE_POINTS), cv.x + di, cv.y + dj, " << z << ", " << halfCellSize << ", " << distort << ", &p[i]);\n"
		   << "\t\t		i++;\n"
		   << "\t\t	}\n"
		   << "\t\t}\n"
		   << "\n"
		   << "\t\tl = Voronoi(p, 9, 4, &u);\n"
		   << "\t\tdO = u - p[4];\n"
		   << "\t\ttrgL = pow(Zeps(l), " << power << ") * " << scale << ";\n"
		   << "\t\tr = trgL / Zeps(l);\n"
		   << "\t\tdO *= r;\n"
		   << "\t\tdO += p[4];\n"
		   << "\t\tvOut.x = xform->m_VariationWeights[" << varIndex << "] * dO.x;\n"
		   << "\t\tvOut.y = xform->m_VariationWeights[" << varIndex << "] * dO.y;\n"
		   << "\t\tvOut.z = " << ((m_VarType == VARTYPE_REG) ? "0" : "vIn.z") << ";\n"
		   << "\t}\n";
		return ss.str();
	}

	virtual void Precalc() override
	{
		int x, y;
		m_HalfCellSize = Zeps<T>(m_CellSize / 2);

		for (x = -CACHE_NUM; x <= CACHE_NUM; x++)
			for (y = -CACHE_NUM; y <= CACHE_NUM; y++)
				Position(x, y, m_Z, m_HalfCellSize, m_Distort, m_C[x + CACHE_NUM][y + CACHE_NUM]);
	}

protected:
	void Init()
	{
		string prefix = Prefix();
		m_VarFuncs = VarFuncs<T>::Instance();
		m_Params.clear();
		m_Params.reserve(8);
		m_Params.push_back(ParamWithName<T>(&m_CellSize, prefix + "crackle_cellsize", 1));
		m_Params.push_back(ParamWithName<T>(&m_Power,	 prefix + "crackle_power", T(0.2)));
		m_Params.push_back(ParamWithName<T>(&m_Distort,  prefix + "crackle_distort"));
		m_Params.push_back(ParamWithName<T>(&m_Scale,	 prefix + "crackle_scale", 1));
		m_Params.push_back(ParamWithName<T>(&m_Z,		 prefix + "crackle_z"));
		m_Params.push_back(ParamWithName<T>(true, &m_HalfCellSize, prefix + "crackle_half_cellsize"));
	}

private:
	void Position(int x, int y, T z, T s, T d, v2T& v)
	{
		v3T e, f;
		// Values here are arbitrary, chosen simply to be far enough apart so they do not correlate
		e.x = x * T(2.5);
		e.y = y * T(2.5);
		e.z = z * T(2.5);
		// Cross-over between x and y is intentional
		f.x = y * T(2.5) + T(30.2);
		f.y = x * T(2.5) - T(12.1);
		f.z = z * T(2.5) + T(19.8);
		v.x = (x + d * m_VarFuncs->SimplexNoise3D(e)) * s;
		v.y = (y + d * m_VarFuncs->SimplexNoise3D(f)) * s;
	}

	void CachedPosition(v2T cache[CACHE_WIDTH][CACHE_WIDTH], int x, int y, T z, T s, T d, v2T& v)
	{
		if (std::abs(x) <= CACHE_NUM && std::abs(y) <= CACHE_NUM)
			v = cache[x + CACHE_NUM][y + CACHE_NUM];
		else
			Position(x, y, z, s, d, v);
	}

	T m_CellSize;
	T m_Power;
	T m_Distort;
	T m_Scale;
	T m_Z;
	T m_HalfCellSize;//Precalc
	v2T m_C[CACHE_WIDTH][CACHE_WIDTH];//Not kept as a precalc because it crashes Nvidia GPUs.
	std::shared_ptr<VarFuncs<T>> m_VarFuncs;
};

MAKEPREPOSTPARVAR(Hexes, hexes, HEXES)
MAKEPREPOSTPARVAR(Nblur, nBlur, NBLUR)
MAKEPREPOSTPARVAR(Octapol, octapol, OCTAPOL)
MAKEPREPOSTPARVAR(Crob, crob, CROB)
MAKEPREPOSTPARVAR(BubbleT3D, bubbleT3D, BUBBLET3D)
MAKEPREPOSTPARVAR(Synth, synth, SYNTH)
MAKEPREPOSTPARVAR(Crackle, crackle, CRACKLE)
}
