#include "EmberCLPch.h"
#include "FunctionMapper.h"

namespace EmberCLns
{
std::unordered_map<string, string> FunctionMapper::m_GlobalMap;

FunctionMapper::FunctionMapper()
{
	if (m_GlobalMap.empty())
	{
		m_GlobalMap["LRint"] =
			"inline real_t LRint(real_t x)\n"
			"{\n"
			"    intPrec temp = (x >= 0.0 ? (intPrec)(x + 0.5) : (intPrec)(x - 0.5));\n"
			"    return (real_t)temp;\n"
			"}\n";
		m_GlobalMap["Round"] =
			"inline real_t Round(real_t r)\n"
			"{\n"
			"	return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);\n"
			"}\n";
		m_GlobalMap["Sign"] =
			"inline real_t Sign(real_t v)\n"
			"{\n"
			"	return (v < 0.0) ? -1 : (v > 0.0) ? 1 : 0.0;\n"
			"}\n";
		m_GlobalMap["SignNz"] =
			"inline real_t SignNz(real_t v)\n"
			"{\n"
			"	return (v < 0.0) ? -1.0 : 1.0;\n"
			"}\n";
		m_GlobalMap["Sqr"] =
			"inline real_t Sqr(real_t v)\n"
			"{\n"
			"	return v * v;\n"
			"}\n";
		m_GlobalMap["SafeSqrt"] =
			"inline real_t SafeSqrt(real_t x)\n"
			"{\n"
			"	if (x <= 0.0)\n"
			"		return 0.0;\n"
			"\n"
			"	return sqrt(x);\n"
			"}\n";
		m_GlobalMap["Cube"] =
			"inline real_t Cube(real_t v)\n"
			"{\n"
			"	return v * v * v;\n"
			"}\n";
		m_GlobalMap["Hypot"] =
			"inline real_t Hypot(real_t x, real_t y)\n"
			"{\n"
			"	return sqrt(SQR(x) + SQR(y));\n"
			"}\n";
		m_GlobalMap["Spread"] =
			"inline real_t Spread(real_t x, real_t y)\n"
			"{\n"
			"	return Hypot(x, y) * ((x) > 0.0 ? 1.0 : -1.0);\n"
			"}\n";
		m_GlobalMap["Powq4"] =
			"inline real_t Powq4(real_t x, real_t y)\n"
			"{\n"
			"	return pow(fabs(x), y) * SignNz(x);\n"
			"}\n";
		m_GlobalMap["Powq4c"] =
			"inline real_t Powq4c(real_t x, real_t y)\n"
			"{\n"
			"	return y == 1.0 ? x : Powq4(x, y);\n"
			"}\n";
		m_GlobalMap["Zeps"] =
			"inline real_t Zeps(real_t x)\n"
			"{\n"
			"	return x == 0.0 ? EPS : x;\n"
			"}\n";
		m_GlobalMap["Lerp"] =
			"inline real_t Lerp(real_t a, real_t b, real_t p)\n"
			"{\n"
			"	return a + (b - a) * p;\n"
			"}\n";
		m_GlobalMap["Fabsmod"] =
			"inline real_t Fabsmod(real_t v)\n"
			"{\n"
			"	real_t dummy;\n"
			"\n"
			"	return modf(v, &dummy);\n"
			"}\n";
		m_GlobalMap["Fosc"] =
			"inline real_t Fosc(real_t p, real_t amp, real_t ph)\n"
			"{\n"
			"	return 0.5 - cos(p * amp + ph) * 0.5;\n"
			"}\n";
		m_GlobalMap["Foscn"] =
			"inline real_t Foscn(real_t p, real_t ph)\n"
			"{\n"
			"	return 0.5 - cos(p + ph) * 0.5;\n"
			"}\n";
		m_GlobalMap["LogScale"] =
			"inline real_t LogScale(real_t x)\n"
			"{\n"
			"	return x == 0.0 ? 0.0 : log((fabs(x) + 1) * M_E) * SignNz(x) / M_E;\n"
			"}\n";
		m_GlobalMap["LogMap"] =
			"inline real_t LogMap(real_t x)\n"
			"{\n"
			"	return x == 0.0 ? 0.0 : (M_E + log(x * M_E)) * 0.25 * SignNz(x);\n"
			"}\n";
		m_GlobalMap["ClampGte"] =
			"inline real_t ClampGte(real_t val, real_t gte)\n"
			"{\n"
			"	return (val < gte) ? gte : val;\n"
			"}\n";
		m_GlobalMap["Swap"] =
			"inline void Swap(real_t* val1, real_t* val2)\n"
			"{\n"
			"	real_t tmp = *val1;\n"
			"	*val1 = *val2;\n"
			"	*val2 = tmp;\n"
			"}\n";
		m_GlobalMap["Vratio"] =
			"inline real_t Vratio(real2* p, real2* q, real2* u)\n"
			"{\n"
			"	real_t pmQx, pmQy;\n"
			"\n"
			"	pmQx = (*p).x - (*q).x;\n"
			"	pmQy = (*p).y - (*q).y;\n"
			"\n"
			"	if (pmQx == 0 && pmQy == 0)\n"
			"		return 1.0;\n"
			"\n"
			"	return 2 * (((*u).x - (*q).x) * pmQx + ((*u).y - (*q).y) * pmQy) / (pmQx * pmQx + pmQy * pmQy);\n"
			"}\n";
		m_GlobalMap["Closest"] =
			"inline int Closest(real2* p, int n, real2* u)\n"
			"{\n"
			"	real_t d2;\n"
			"	real_t d2min = TMAX;\n"
			"	int i, j = 0;\n"
			"\n"
			"	for (i = 0; i < n; i++)\n"
			"	{\n"
			"		d2 = Sqr(p[i].x - (*u).x) + Sqr(p[i].y - (*u).y);\n"
			"\n"
			"		if (d2 < d2min)\n"
			"		{\n"
			"			d2min = d2;\n"
			"			j = i;\n"
			"		}\n"
			"	}\n"
			"\n"
			"	return j;\n"
			"}\n";
		m_GlobalMap["Voronoi"] =
			"inline real_t Voronoi(real2* p, int n, int q, real2* u)\n"
			"{\n"
			"	real_t ratio;\n"
			"	real_t ratiomax = TLOW;\n"
			"	int i;\n"
			"\n"
			"	for (i = 0; i < n; i++)\n"
			"	{\n"
			"		if (i != q)\n"
			"		{\n"
			"			ratio = Vratio(&p[i], &p[q], u);\n"
			"\n"
			"			if (ratio > ratiomax)\n"
			"				ratiomax = ratio;\n"
			"		}\n"
			"	}\n"
			"\n"
			"	return ratiomax;\n"
			"}\n";
		m_GlobalMap["SimplexNoise3D"] =
			"inline real_t SimplexNoise3D(real3* v, __global real_t* p, __global real3* grad)\n"
			"{\n"
			"	real3 c[4];\n"
			"	real_t n = 0;\n"
			"	int gi[4];\n"
			"	real_t t;\n"
			"	real_t skewIn = ((*v).x + (*v).y + (*v).z) * 0.3333;\n"
			"	int i = (int)floor((*v).x + skewIn);\n"
			"	int j = (int)floor((*v).y + skewIn);\n"
			"	int k = (int)floor((*v).z + skewIn);\n"
			"	t = (i + j + k) * 0.16666;\n"
			"	real_t x0 = i - t;\n"
			"	real_t y0 = j - t;\n"
			"	real_t z0 = k - t;\n"
			"	c[0].x = (*v).x - x0;\n"
			"	c[0].y = (*v).y - y0;\n"
			"	c[0].z = (*v).z - z0;\n"
			"	int i1, j1, k1;\n"
			"	int i2, j2, k2;\n"
			"\n"
			"	if (c[0].x >= c[0].y)\n"
			"	{\n"
			"		if (c[0].y >= c[0].z)\n"
			"		{\n"
			"			i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;\n"
			"		}\n"
			"		else\n"
			"		{\n"
			"			if (c[0].x >= c[0].z)\n"
			"			{\n"
			"				i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;\n"
			"			}\n"
			"			else\n"
			"			{\n"
			"				i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;\n"
			"			}\n"
			"		}\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		if (c[0].y < c[0].z)\n"
			"		{\n"
			"			i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;\n"
			"		}\n"
			"		else\n"
			"		{\n"
			"			if (c[0].x < c[0].z)\n"
			"			{\n"
			"				i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;\n"
			"			}\n"
			"			else\n"
			"			{\n"
			"				i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;\n"
			"			}\n"
			"		}\n"
			"	}\n"
			"\n"
			"	c[1].x = c[0].x - i1 + 0.16666;\n"
			"	c[1].y = c[0].y - j1 + 0.16666;\n"
			"	c[1].z = c[0].z - k1 + 0.16666;\n"
			"	c[2].x = c[0].x - i2 + 2 * 0.16666;\n"
			"	c[2].y = c[0].y - j2 + 2 * 0.16666;\n"
			"	c[2].z = c[0].z - k2 + 2 * 0.16666;\n"
			"	c[3].x = c[0].x - 1 + 3 * 0.16666;\n"
			"	c[3].y = c[0].y - 1 + 3 * 0.16666;\n"
			"	c[3].z = c[0].z - 1 + 3 * 0.16666;\n"
			"	int ii = i & 0x3ff;\n"
			"	int jj = j & 0x3ff;\n"
			"	int kk = k & 0x3ff;\n"
			"	gi[0] = (int)p[ii + (int)p[jj + (int)p[kk]]];\n"
			"	gi[1] = (int)p[ii + i1 + (int)p[jj + j1 + (int)p[kk + k1]]];\n"
			"	gi[2] = (int)p[ii + i2 + (int)p[jj + j2 + (int)p[kk + k2]]];\n"
			"	gi[3] = (int)p[ii + 1 + (int)p[jj + 1 + (int)p[kk + 1]]];\n"
			"	for (uint corner = 0; corner < 4; corner++)\n"
			"	{\n"
			"		t = 0.6 - c[corner].x * c[corner].x - c[corner].y * c[corner].y - c[corner].z * c[corner].z;\n"
			"\n"
			"		if (t > 0)\n"
			"		{\n"
			"			real3 u = grad[gi[corner]];\n"
			"			t *= t;\n"
			"			n += t * t * (u.x * c[corner].x + u.y * c[corner].y + u.z * c[corner].z);\n"
			"		}\n"
			"	}\n"
			"\n"
			"	return 32 * n;\n"
			"}\n";
		m_GlobalMap["PerlinNoise3D"] =
			"inline real_t PerlinNoise3D(real3* v, __global real_t* p, __global real3* grad, real_t aScale, real_t fScale, int octaves)\n"
			"{\n"
			"	int i;\n"
			"	real_t n = 0, a = 1;\n"
			"	real3 u = *v;\n"
			"\n"
			"	for (i = 0; i < octaves; i++)\n"
			"	{\n"
			"		n += SimplexNoise3D(&u, p, grad) / a;\n"
			"		a *= aScale;\n"
			"		u.x *= fScale;\n"
			"		u.y *= fScale;\n"
			"		u.x *= fScale;\n"
			"	}\n"
			"\n"
			"	return n;\n"
			"}\n";
	}
}

const string* FunctionMapper::GetGlobalFunc(const string& func)
{
	const auto& text = m_GlobalMap.find(func);

	if (text != m_GlobalMap.end())
		return &text->second;
	else
		return nullptr;
}
}
