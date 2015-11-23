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
