#pragma once

#include "EmberCLPch.h"

namespace EmberCLns
{
/// <summary>
/// Functionality to map OpenCL function names to their full function body program strings.
/// This is used to ensure only the functions that are needed by a program are included once
/// in the program string.
/// </summary>
class EMBERCL_API FunctionMapper
{
public:
	FunctionMapper();
	static const string* GetGlobalFunc(const string& func);

private:
	static std::unordered_map<string, string> m_GlobalMap;
};
}