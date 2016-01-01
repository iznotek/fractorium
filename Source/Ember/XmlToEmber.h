#pragma once

#include "Utils.h"
#include "PaletteList.h"
#include "VariationList.h"

#ifdef __APPLE__
	#include <libgen.h>
#endif

/// <summary>
/// XmlToEmber and Locale classes.
/// </summary>

namespace EmberNs
{
/// <summary>
/// Convenience class for setting and resetting the locale.
/// It's set up in the constructor and restored in the destructor.
/// This relieves the caller of having to manually do it everywhere.
/// </summary>
class EMBER_API Locale
{
public:
	/// <summary>
	/// Constructor which saves the state of the current locale and
	/// sets the new one based on the parameters passed in.
	/// </summary>
	/// <param name="category">The locale category. Default: LC_NUMERIC.</param>
	/// <param name="loc">The locale. Default: "C".</param>
	Locale(int category = LC_NUMERIC, const char* loc = "C")
	{
		m_Category = category;
		m_NewLocale = string(loc);
		m_OriginalLocale = setlocale(category, nullptr);//Query.

		if (m_OriginalLocale.empty())
			cout << "Couldn't get original locale." << endl;

		if (setlocale(category, loc) == nullptr)//Set.
			cout << "Couldn't set new locale " << category << ", " << loc << "." << endl;
	}

	/// <summary>
	/// Reset the locale to the value stored during construction.
	/// </summary>
	~Locale()
	{
		if (!m_OriginalLocale.empty())
			if (setlocale(m_Category, m_OriginalLocale.c_str()) == nullptr)//Restore.
				cout << "Couldn't restore original locale " << m_Category << ", " << m_OriginalLocale << "." << endl;
	}

private:
	int m_Category;
	string m_NewLocale;
	string m_OriginalLocale;
};

/// <summary>
/// Class for reading Xml files into ember objects.
/// This class derives from EmberReport, so the caller is able
/// to retrieve a text dump of error information if any errors occur.
/// Since this class contains a VariationList object, it's important to declare one
/// instance and reuse it for the duration of the program instead of creating and deleting
/// them as local variables.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API XmlToEmber : public EmberReport
{
public:
	/// <summary>
	/// Constructor that initializes the random context.
	/// </summary>
	XmlToEmber()
	{
		Timing t;

		if (!m_Init)
		{
			m_BadParamNames.reserve(100);
			m_BadParamNames["swtin_distort"] =   "stwin_distort";//stwin.
			m_BadParamNames["pow_numerator"] =   "pow_block_numerator";//pow_block.
			m_BadParamNames["pow_denominator"] = "pow_block_denominator";
			m_BadParamNames["pow_root"] =        "pow_block_root";
			m_BadParamNames["pow_correctn"] =    "pow_block_correctn";
			m_BadParamNames["pow_correctd"] =    "pow_block_correctd";
			m_BadParamNames["pow_power"] =       "pow_block_power";
			m_BadParamNames["lt"] = "linearT_powX";//linearT.
			m_BadParamNames["lt"] = "linearT_powY";
			m_BadParamNames["re_a"] = "Mobius_Re_A";//Mobius.
			m_BadParamNames["im_a"] = "Mobius_Im_A";
			m_BadParamNames["re_b"] = "Mobius_Re_B";
			m_BadParamNames["im_b"] = "Mobius_Im_B";
			m_BadParamNames["re_c"] = "Mobius_Re_C";
			m_BadParamNames["im_c"] = "Mobius_Im_C";
			m_BadParamNames["re_d"] = "Mobius_Re_D";
			m_BadParamNames["im_d"] = "Mobius_Im_D";
			m_BadParamNames["rx_sin"] = "rotate_x_sin";//rotate_x.
			m_BadParamNames["rx_cos"] = "rotate_x_cos";
			m_BadParamNames["ry_sin"] = "rotate_y_sin";//rotate_y.
			m_BadParamNames["ry_cos"] = "rotate_y_cos";
			m_BadParamNames["intrfr2_a1"] = "interference2_a1";//interference2.
			m_BadParamNames["intrfr2_b1"] = "interference2_b1";
			m_BadParamNames["intrfr2_c1"] = "interference2_c1";
			m_BadParamNames["intrfr2_p1"] = "interference2_p1";
			m_BadParamNames["intrfr2_t1"] = "interference2_t1";
			m_BadParamNames["intrfr2_a2"] = "interference2_a2";
			m_BadParamNames["intrfr2_b2"] = "interference2_b2";
			m_BadParamNames["intrfr2_c2"] = "interference2_c2";
			m_BadParamNames["intrfr2_p2"] = "interference2_p2";
			m_BadParamNames["intrfr2_t2"] = "interference2_t2";
			m_BadParamNames["octa_x"] = "octagon_x";//octagon.
			m_BadParamNames["octa_y"] = "octagon_y";
			m_BadParamNames["octa_z"] = "octagon_z";
			m_BadParamNames["bubble_x"] = "bubble2_x";//bubble2.
			m_BadParamNames["bubble_y"] = "bubble2_y";
			m_BadParamNames["bubble_z"] = "bubble2_z";
			m_BadParamNames["cubic3d_xpand"] = "cubicLattice_3D_xpand";//cubicLattice_3D.
			m_BadParamNames["cubic3d_style"] = "cubicLattice_3D_style";
			m_BadParamNames["splitb_x"] =  "SplitBrdr_x";//SplitBrdr.
			m_BadParamNames["splitb_y"] =  "SplitBrdr_y";
			m_BadParamNames["splitb_px"] = "SplitBrdr_px";
			m_BadParamNames["splitb_py"] = "SplitBrdr_py";
			m_BadParamNames["dc_cyl_offset"] = "dc_cylinder_offset";//dc_cylinder.
			m_BadParamNames["dc_cyl_angle"] =  "dc_cylinder_angle";
			m_BadParamNames["dc_cyl_scale"] =  "dc_cylinder_scale";
			m_BadParamNames["cyl_x"] =         "dc_cylinder_x";
			m_BadParamNames["cyl_y"] =         "dc_cylinder_y";
			m_BadParamNames["cyl_blur"] =      "dc_cylinder_blur";
			m_BadParamNames["mobius_radius"] =   "mobius_strip_radius";//mobius_strip.
			m_BadParamNames["mobius_width"] =    "mobius_strip_width";
			m_BadParamNames["mobius_rect_x"] =   "mobius_strip_rect_x";
			m_BadParamNames["mobius_rect_y"] =   "mobius_strip_rect_y";
			m_BadParamNames["mobius_rotate_x"] = "mobius_strip_rotate_x";
			m_BadParamNames["mobius_rotate_y"] = "mobius_strip_rotate_y";
			m_BadParamNames["bwraps2_cellsize"] =    "bwraps_cellsize";//bwraps2.
			m_BadParamNames["bwraps2_space"] =       "bwraps_space";
			m_BadParamNames["bwraps2_gain"] =        "bwraps_gain";
			m_BadParamNames["bwraps2_inner_twist"] = "bwraps_inner_twist";
			m_BadParamNames["bwraps2_outer_twist"] = "bwraps_outer_twist";
			m_BadParamNames["bwraps7_cellsize"] =    "bwraps_cellsize";//bwraps7.
			m_BadParamNames["bwraps7_space"] =       "bwraps_space";
			m_BadParamNames["bwraps7_gain"] =        "bwraps_gain";
			m_BadParamNames["bwraps7_inner_twist"] = "bwraps_inner_twist";
			m_BadParamNames["bwraps7_outer_twist"] = "bwraps_outer_twist";
			m_BadParamNames["pre_bwraps2_cellsize"] =    "pre_bwraps_cellsize";
			m_BadParamNames["pre_bwraps2_space"] =       "pre_bwraps_space";
			m_BadParamNames["pre_bwraps2_gain"] =        "pre_bwraps_gain";
			m_BadParamNames["pre_bwraps2_inner_twist"] = "pre_bwraps_inner_twist";
			m_BadParamNames["pre_bwraps2_outer_twist"] = "pre_bwraps_outer_twist";
			m_BadParamNames["post_bwraps2_cellsize"] =    "post_bwraps_cellsize";
			m_BadParamNames["post_bwraps2_space"] =       "post_bwraps_space";
			m_BadParamNames["post_bwraps2_gain"] =        "post_bwraps_gain";
			m_BadParamNames["post_bwraps2_inner_twist"] = "post_bwraps_inner_twist";
			m_BadParamNames["post_bwraps2_outer_twist"] = "post_bwraps_outer_twist";
			m_BadParamNames["hexa3d_majp"] =  "hexaplay3D_majp";
			m_BadParamNames["hexa3d_scale"] = "hexaplay3D_scale";
			m_BadParamNames["hexa3d_zlift"] = "hexaplay3D_zlift";
			m_BadParamNames["nb_numedges"] =		"nBlur_numEdges";
			m_BadParamNames["nb_numstripes"] =		"nBlur_numStripes";
			m_BadParamNames["nb_ratiostripes"] =	"nBlur_ratioStripes";
			m_BadParamNames["nb_ratiohole"] =		"nBlur_ratioHole";
			m_BadParamNames["nb_circumcircle"] =	"nBlur_circumCircle";
			m_BadParamNames["nb_adjusttolinear"] = "nBlur_adjustToLinear";
			m_BadParamNames["nb_equalblur"] =		"nBlur_equalBlur";
			m_BadParamNames["nb_exactcalc"] =		"nBlur_exactCalc";
			m_BadParamNames["nb_highlightedges"] = "nBlur_highlightEdges";
			m_BadParamNames["octapol_r"] = "octapol_radius";
			m_BadParamNames["number_of_stripes"] = "bubbleT3D_number_of_stripes";
			m_BadParamNames["ratio_of_stripes"]  = "bubbleT3D_ratio_of_stripes";
			m_BadParamNames["angle_of_hole"]	 = "bubbleT3D_angle_of_hole";
			m_BadParamNames["exponentZ"]		 = "bubbleT3D_exponentZ";
			m_BadParamNames["_symmetryZ"]		 = "bubbleT3D_symmetryZ";
			m_BadParamNames["_modusBlur"]		 = "bubbleT3D_modusBlur";
			m_FlattenNames.reserve(24);
			m_FlattenNames.push_back("pre_crop");
			m_FlattenNames.push_back("pre_falloff2");
			m_FlattenNames.push_back("pre_rotate_x");
			m_FlattenNames.push_back("pre_rotate_y");
			m_FlattenNames.push_back("pre_ztranslate");
			m_FlattenNames.push_back("blur3D");
			m_FlattenNames.push_back("bubble");
			m_FlattenNames.push_back("bwraps");
			m_FlattenNames.push_back("bwraps2");
			m_FlattenNames.push_back("crop");
			m_FlattenNames.push_back("cylinder");
			m_FlattenNames.push_back("falloff2");
			m_FlattenNames.push_back("hemisphere");
			m_FlattenNames.push_back("julia3D");
			m_FlattenNames.push_back("julia3Dz");
			m_FlattenNames.push_back("linear3D");
			m_FlattenNames.push_back("zblur");
			m_FlattenNames.push_back("zcone");
			m_FlattenNames.push_back("ztranslate");
			m_FlattenNames.push_back("post_crop");
			m_FlattenNames.push_back("post_falloff2");
			m_FlattenNames.push_back("post_rotate_x");
			m_FlattenNames.push_back("post_rotate_y");
			m_FlattenNames.push_back("curl3D_cz");
			//This is a vector of the param names as they are in the legacy, badly named flam3/Apophysis code.
			vector<string> badParams;
			badParams.reserve(6);
			badParams.push_back("bwraps7_cellsize");
			badParams.push_back("bwraps7_space");
			badParams.push_back("bwraps7_gain");
			badParams.push_back("bwraps7_inner_twist");
			badParams.push_back("bwraps7_outer_twist");
			m_BadVariationNames.push_back(make_pair(make_pair(string("bwraps7"), string("bwraps")), badParams));//bwraps7 is the same as bwraps.
			badParams.clear();
			badParams.push_back("bwraps2_cellsize");
			badParams.push_back("bwraps2_space");
			badParams.push_back("bwraps2_gain");
			badParams.push_back("bwraps2_inner_twist");
			badParams.push_back("bwraps2_outer_twist");
			m_BadVariationNames.push_back(make_pair(make_pair(string("bwraps2"), string("bwraps")), badParams));//bwraps2 is the same as bwraps.
			badParams.clear();
			badParams.push_back("pre_bwraps2_cellsize");
			badParams.push_back("pre_bwraps2_space");
			badParams.push_back("pre_bwraps2_gain");
			badParams.push_back("pre_bwraps2_inner_twist");
			badParams.push_back("pre_bwraps2_outer_twist");
			m_BadVariationNames.push_back(make_pair(make_pair(string("pre_bwraps2"), string("pre_bwraps")), badParams));
			badParams.clear();
			badParams.push_back("post_bwraps2_cellsize");
			badParams.push_back("post_bwraps2_space");
			badParams.push_back("post_bwraps2_gain");
			badParams.push_back("post_bwraps2_inner_twist");
			badParams.push_back("post_bwraps2_outer_twist");
			m_BadVariationNames.push_back(make_pair(make_pair(string("post_bwraps2"), string("post_bwraps")), badParams));
			badParams.clear();
			badParams.push_back("mobius_radius");
			badParams.push_back("mobius_width");
			badParams.push_back("mobius_rect_x");
			badParams.push_back("mobius_rect_y");
			badParams.push_back("mobius_rotate_x");
			badParams.push_back("mobius_rotate_y");
			m_BadVariationNames.push_back(make_pair(make_pair(string("mobius"),	string("mobius_strip")), badParams));//mobius_strip clashes with Mobius.
			badParams.clear();
			badParams.push_back("post_dcztransl_x0");
			badParams.push_back("post_dcztransl_x1");
			badParams.push_back("post_dcztransl_factor");
			badParams.push_back("post_dcztransl_overwrite");
			badParams.push_back("post_dcztransl_clamp");
			m_BadVariationNames.push_back(make_pair(make_pair(string("post_dcztransl"), string("post_dc_ztransl")), badParams));
			badParams.clear();
			m_BadVariationNames.push_back(make_pair(make_pair(string("pre_blur"),    string("pre_gaussian_blur")), badParams));//No other special params for these.
			m_BadVariationNames.push_back(make_pair(make_pair(string("pre_spin_z"),  string("pre_rotate_z")),      badParams));
			m_BadVariationNames.push_back(make_pair(make_pair(string("post_spin_z"), string("post_rotate_z")),     badParams));
			m_Init = true;
		}
	}

	/// <summary>
	/// Parse the specified buffer and place the results in the vector of embers passed in.
	/// </summary>
	/// <param name="buf">The buffer to parse</param>
	/// <param name="filename">Full path and filename, optionally empty</param>
	/// <param name="embers">The newly constructed embers based on what was parsed</param>
	/// <param name="useDefaults">True to use defaults if they are not present in the file, else false to use invalid values as placeholders to indicate the values were not present. Default: true.</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool Parse(byte* buf, const char* filename, vector<Ember<T>>& embers, bool useDefaults = true)
	{
		char* bn;
		const char* xmlPtr;
		const char* loc = __FUNCTION__;
		size_t emberSize;
		size_t bufSize;
		xmlDocPtr doc;//Parsed XML document tree.
		xmlNodePtr rootnode;
		Locale locale;//Sets and restores on exit.
		//Timing t;
		ClearErrorReport();
		//Parse XML string into internal document.
		xmlPtr = CX(&buf[0]);
		bufSize = strlen(xmlPtr);
		embers.reserve(bufSize / 2500);//The Xml text for an ember is around 2500 bytes, but can be much more. Pre-allocate to aovid unnecessary resizing.
		doc = xmlReadMemory(xmlPtr, int(bufSize), filename, "ISO-8859-1", XML_PARSE_NONET);//Forbid network access during read.
		//t.Toc("xmlReadMemory");

		if (doc == nullptr)
		{
			AddToReport(string(loc) + " : Error parsing xml file " + string(filename));
			return false;
		}

		//What is the root node of the document?
		rootnode = xmlDocGetRootElement(doc);
		//Scan for <flame> nodes, starting with this node.
		//t.Tic();
		bn = basename(const_cast<char*>(filename));
		ScanForEmberNodes(rootnode, bn, embers, useDefaults);
		xmlFreeDoc(doc);
		emberSize = embers.size();
		//t.Toc("ScanForEmberNodes");

		//Check to see if the first control point or the second-to-last
		//control point has interpolation="smooth".  This is invalid
		//and should be reset to linear (with a warning).
		if (emberSize > 0)
		{
			if (embers[0].m_Interp == eInterp::EMBER_INTERP_SMOOTH)
			{
				cout << "Warning: smooth interpolation cannot be used for first segment.\n         switching to linear.\n" << endl;
				embers[0].m_Interp = eInterp::EMBER_INTERP_LINEAR;
			}

			if (emberSize >= 2 && embers[emberSize - 2].m_Interp == eInterp::EMBER_INTERP_SMOOTH)
			{
				cout << "Warning: smooth interpolation cannot be used for last segment.\n         switching to linear.\n" << endl;
				embers[emberSize - 2].m_Interp = eInterp::EMBER_INTERP_LINEAR;
			}
		}

		//Finally, ensure that consecutive 'rotate' parameters never exceed
		//a difference of more than 180 degrees (+/-) for interpolation.
		//An adjustment of +/- 360 degrees is made until this is true.
		if (emberSize > 1)
		{
			for (size_t i = 1; i < emberSize; i++)
			{
				//Only do this adjustment if not in compat mode.
				if (embers[i - 1].m_AffineInterp != eAffineInterp::AFFINE_INTERP_COMPAT && embers[i - 1].m_AffineInterp != eAffineInterp::AFFINE_INTERP_OLDER)
				{
					while (embers[i].m_Rotate < embers[i - 1].m_Rotate - 180)
						embers[i].m_Rotate += 360;

					while (embers[i].m_Rotate > embers[i - 1].m_Rotate + 180)
						embers[i].m_Rotate -= 360;
				}
			}
		}

		return true;
	}

	/// <summary>
	/// Parse the specified file and place the results in the vector of embers passed in.
	/// This will strip out ampersands because the Xml parser can't handle them.
	/// </summary>
	/// <param name="filename">Full path and filename</param>
	/// <param name="embers">The newly constructed embers based on what was parsed</param>
	/// <param name="useDefaults">True to use defaults if they are not present in the file, else false to use invalid values as placeholders to indicate the values were not present. Default: true.</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool Parse(const char* filename, vector<Ember<T>>& embers, bool useDefaults = true)
	{
		const char* loc = __FUNCTION__;
		string buf;

		//Ensure palette list is setup first.
		if (!m_PaletteList.Size())
		{
			AddToReport(string(loc) + " : Palette list must be initialized before parsing embers.");
			return false;
		}

		if (ReadFile(filename, buf))
		{
			std::replace(buf.begin(), buf.end(), '&', '+');
			return Parse(reinterpret_cast<byte*>(const_cast<char*>(buf.data())), filename, embers, useDefaults);
		}
		else
			return false;
	}

	/// <summary>
	/// Thin wrapper around converting the string to a numeric value and return a bool indicating success.
	/// See error report for errors.
	/// </summary>
	/// <param name="str">The string to convert</param>
	/// <param name="val">The converted value</param>
	/// <returns>True if success, else false.</returns>
	template <typename valT>
	bool Aton(const char* str, valT& val)
	{
		bool b = true;
		const char* loc = __FUNCTION__;
		std::istringstream istr(str);
		istr >> val;

		if (istr.bad() || istr.fail())
		{
			AddToReport(string(loc) + " : Error converting " + string(str));
			b = false;
		}

		return b;
	}


	/// <summary>
	/// Convert an integer to a string.
	/// Just a wrapper around _itoa_s() which wraps the result in a std::string.
	/// </summary>
	/// <param name="i">The integer to convert</param>
	/// <param name="radix">The radix of the integer. Default: 10.</param>
	/// <returns>The converted string</returns>
	static string Itos(int i, int radix = 10)
	{
		char ch[16];
#ifdef WIN32
		_itoa_s(i, ch, 16, radix);
#else
		sprintf(ch, "%d", i);
#endif
		return string(ch);
	}

	/// <summary>
	/// Convert an unsigned 64-bit integer to a string.
	/// Just a wrapper around _ui64toa_s() which wraps the result in a std::string.
	/// </summary>
	/// <param name="i">The unsigned 64-bit integer to convert</param>
	/// <param name="radix">The radix of the integer. Default: 10.</param>
	/// <returns>The converted string</returns>
	static string Itos64(size_t i, int radix = 10)
	{
		char ch[64];
#ifdef WIN32
		_ui64toa_s(i, ch, 64, radix);
#else
		sprintf(ch, "%lu", i);
#endif
		return string(ch);
	}

	static vector<string> m_FlattenNames;

private:
	/// <summary>
	/// Scan the file for ember nodes, and parse them out into the vector of embers.
	/// </summary>
	/// <param name="curNode">The current node to parse</param>
	/// <param name="parentFile">The full path and filename</param>
	/// <param name="embers">The newly constructed embers based on what was parsed</param>
	/// <param name="useDefaults">True to use defaults if they are not present in the file, else false to use invalid values as placeholders to indicate the values were not present.</param>
	void ScanForEmberNodes(xmlNode* curNode, char* parentFile, vector<Ember<T>>& embers, bool useDefaults)
	{
		bool parseEmberSuccess;
		xmlNodePtr thisNode = nullptr;
		const char* loc = __FUNCTION__;
		string parentFileString = string(parentFile);

		//Original memset to 0, but the constructors should handle that.
		//Loop over this level of elements.
		for (thisNode = curNode; thisNode; thisNode = thisNode->next)
		{
			//Check to see if this element is a <ember> element.
			if (thisNode->type == XML_ELEMENT_NODE && !Compare(thisNode->name, "flame"))
			{
				Ember<T> currentEmber;//Place this inside here so its constructor is called each time.

				//Useful for parsing templates when not every member should be set.
				if (!useDefaults)
					currentEmber.Clear(false);

				parseEmberSuccess = ParseEmberElement(thisNode, currentEmber);

				if (!parseEmberSuccess)
				{
					//Original leaked memory here, ours doesn't.
					AddToReport(string(loc) + " : Error parsing ember element");
					return;
				}

				if (currentEmber.PaletteIndex() != -1)
				{
					if (auto pal = m_PaletteList.GetPalette(PaletteList<T>::m_DefaultFilename, currentEmber.PaletteIndex()))
						currentEmber.m_Palette = *pal;
					else
						AddToReport(string(loc) + " : Error assigning palette with index " + Itos(currentEmber.PaletteIndex()));
				}

				//if (!Interpolater<T>::InterpMissingColors(currentEmber.m_Palette.m_Entries))
				//	AddToReport(string(loc) + " : Error interpolating missing palette colors");
				currentEmber.CacheXforms();
				currentEmber.m_Index = embers.size();
				currentEmber.m_ParentFilename = parentFileString;
				embers.push_back(currentEmber);
			}
			else
			{
				//Check all of the children of this element.
				ScanForEmberNodes(thisNode->children, parentFile, embers, useDefaults);
			}
		}
	}

	/// <summary>
	/// Parse an ember element.
	/// </summary>
	/// <param name="emberNode">The current node to parse</param>
	/// <param name="currentEmber">The newly constructed ember based on what was parsed</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool ParseEmberElement(xmlNode* emberNode, Ember<T>& currentEmber)
	{
		bool ret = true;
		bool fromEmber = false;
		size_t newLinear = 0;
		char* attStr;
		const char* loc = __FUNCTION__;
		int soloXform = -1;
		size_t i, count, index = 0;
		double vals[16];
		xmlAttrPtr att, curAtt;
		xmlNodePtr editNode, childNode, motionNode;
		currentEmber.m_Palette.Clear();//Wipe out the current palette.
		att = emberNode->properties;//The top level element is a ember element, read the attributes of it and store them.

		if (att == nullptr)
		{
			AddToReport(string(loc) + " : <flame> element has no attributes");
			return false;
		}

		for (curAtt = att; curAtt; curAtt = curAtt->next)
		{
			attStr = reinterpret_cast<char*>(xmlGetProp(emberNode, curAtt->name));

			//First parse out simple float reads.
			if		(ParseAndAssign(curAtt->name, attStr, "time",                  currentEmber.m_Time,                ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "scale",				   currentEmber.m_PixelsPerUnit,	   ret)) { currentEmber.m_OrigPixPerUnit = currentEmber.m_PixelsPerUnit; }
			else if (ParseAndAssign(curAtt->name, attStr, "rotate",                currentEmber.m_Rotate,              ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "zoom",				   currentEmber.m_Zoom,				   ret)) { ClampGteRef<T>(currentEmber.m_Zoom, 0); }
			else if (ParseAndAssign(curAtt->name, attStr, "filter",                currentEmber.m_SpatialFilterRadius, ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "temporal_filter_width", currentEmber.m_TemporalFilterWidth, ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "temporal_filter_exp",   currentEmber.m_TemporalFilterExp,   ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "quality",               currentEmber.m_Quality,             ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "brightness",            currentEmber.m_Brightness,          ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "gamma",                 currentEmber.m_Gamma,               ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "highlight_power",       currentEmber.m_HighlightPower,      ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "vibrancy",              currentEmber.m_Vibrancy,            ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "estimator_radius",      currentEmber.m_MaxRadDE,            ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "estimator_minimum",     currentEmber.m_MinRadDE,            ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "estimator_curve",       currentEmber.m_CurveDE,             ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "gamma_threshold",       currentEmber.m_GammaThresh,         ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "cam_zpos",			   currentEmber.m_CamZPos,			   ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "cam_persp",			   currentEmber.m_CamPerspective,      ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "cam_perspective",	   currentEmber.m_CamPerspective,      ret)) { }//Apo bug.
			else if (ParseAndAssign(curAtt->name, attStr, "cam_yaw",			   currentEmber.m_CamYaw,			   ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "cam_pitch",			   currentEmber.m_CamPitch,			   ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "cam_dof",			   currentEmber.m_CamDepthBlur,        ret)) { }
			//Parse simple int reads.
			else if (ParseAndAssign(curAtt->name, attStr, "palette",          currentEmber.m_Palette.m_Index, ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "oversample",       currentEmber.m_Supersample    , ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "supersample",      currentEmber.m_Supersample    , ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "temporal_samples", currentEmber.m_TemporalSamples, ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "sub_batch_size",	  currentEmber.m_SubBatchSize   , ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "fuse",			  currentEmber.m_FuseCount	    , ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "soloxform",		  soloXform                     , ret)) { }
			else if (ParseAndAssign(curAtt->name, attStr, "new_linear",		  newLinear					    , ret)) { }
			//Parse more complicated reads that have multiple possible values.
			else if (!Compare(curAtt->name, "interpolation"))
			{
				if (!_stricmp("linear", attStr))
					currentEmber.m_Interp = eInterp::EMBER_INTERP_LINEAR;
				else if (!_stricmp("smooth", attStr))
					currentEmber.m_Interp = eInterp::EMBER_INTERP_SMOOTH;
				else
					AddToReport(string(loc) + " : Unrecognized interpolation type " + string(attStr));
			}
			else if (!Compare(curAtt->name, "palette_interpolation"))
			{
				if (!_stricmp("hsv", attStr))
					currentEmber.m_PaletteInterp = ePaletteInterp::INTERP_HSV;
				else if (!_stricmp("sweep", attStr))
					currentEmber.m_PaletteInterp = ePaletteInterp::INTERP_SWEEP;
				else
					AddToReport(string(loc) + " : Unrecognized palette interpolation type " + string(attStr));
			}
			else if (!Compare(curAtt->name, "interpolation_space") || !Compare(curAtt->name, "interpolation_type"))
			{
				if (!_stricmp("linear", attStr))
					currentEmber.m_AffineInterp = eAffineInterp::AFFINE_INTERP_LINEAR;
				else if (!_stricmp("log", attStr))
					currentEmber.m_AffineInterp = eAffineInterp::AFFINE_INTERP_LOG;
				else if (!_stricmp("old", attStr))
					currentEmber.m_AffineInterp = eAffineInterp::AFFINE_INTERP_COMPAT;
				else if (!_stricmp("older", attStr))
					currentEmber.m_AffineInterp = eAffineInterp::AFFINE_INTERP_OLDER;
				else
					AddToReport(string(loc) + " : Unrecognized interpolation type " + string(attStr));
			}
			else if (!Compare(curAtt->name, "name"))
			{
				currentEmber.m_Name = string(attStr);
				std::replace(currentEmber.m_Name.begin(), currentEmber.m_Name.end(), ' ', '_');
			}
			else if (!Compare(curAtt->name, "version"))
			{
				if (ToLower(string(attStr)).find("ember") != string::npos)
					fromEmber = true;
			}
			else if (!Compare(curAtt->name, "size"))
			{
				if (sscanf_s(attStr, "%lu %lu", &currentEmber.m_FinalRasW, &currentEmber.m_FinalRasH) != 2)
				{
					AddToReport(string(loc) + " : Invalid size attribute " + string(attStr));
					xmlFree(attStr);
					//These return statements are bad. One because they are inconsistent with others that just assign defaults.
					//Two, because assigning easily guessable defaults is easy and less drastic.
					return false;
				}

				currentEmber.m_OrigFinalRasW = currentEmber.m_FinalRasW;
				currentEmber.m_OrigFinalRasH = currentEmber.m_FinalRasH;
			}
			else if (!Compare(curAtt->name, "center"))
			{
				if (sscanf_s(attStr, "%lf %lf", &vals[0], &vals[1]) != 2)
				{
					AddToReport(string(loc) + " : Invalid center attribute " + string(attStr));
					xmlFree(attStr);
					return false;
				}

				currentEmber.m_CenterX = T(vals[0]);
				currentEmber.m_CenterY = currentEmber.m_RotCenterY = T(vals[1]);
			}
			else if (!Compare(curAtt->name, "filter_shape"))
			{
				currentEmber.m_SpatialFilterType = SpatialFilterCreator<T>::FromString(string(attStr));
			}
			else if (!Compare(curAtt->name, "temporal_filter_type"))
			{
				currentEmber.m_TemporalFilterType = TemporalFilterCreator<T>::FromString(string(attStr));
			}
			else if (!Compare(curAtt->name, "palette_mode"))
			{
				if (!_stricmp("step", attStr))
					currentEmber.m_PaletteMode = ePaletteMode::PALETTE_STEP;
				else if (!_stricmp("linear", attStr))
					currentEmber.m_PaletteMode = ePaletteMode::PALETTE_LINEAR;
				else
				{
					currentEmber.m_PaletteMode = ePaletteMode::PALETTE_STEP;
					AddToReport(string(loc) + " : Unrecognized palette mode " + string(attStr) + ", using step");
				}
			}
			else if (!Compare(curAtt->name, "background"))
			{
				if (sscanf_s(attStr, "%lf %lf %lf", &vals[0], &vals[1], &vals[2]) != 3)
				{
					AddToReport(string(loc) + " : Invalid background attribute " + string(attStr));
					xmlFree(attStr);
					return false;
				}

				currentEmber.m_Background[0] = T(vals[0]);//[0..1]
				currentEmber.m_Background[1] = T(vals[1]);
				currentEmber.m_Background[2] = T(vals[2]);
			}
			else if (!Compare(curAtt->name, "curves"))
			{
				stringstream ss(attStr);

				for (i = 0; i < 4; i++)
				{
					for (glm::length_t j = 0; j < 4; j++)
					{
						ss >> currentEmber.m_Curves.m_Points[i][j].x;
						ss >> currentEmber.m_Curves.m_Points[i][j].y;
						ss >> currentEmber.m_Curves.m_Weights[i][j];
					}
				}
			}

			xmlFree(attStr);
		}

		//Finished with ember attributes. Now look at the children of the ember element.
		for (childNode = emberNode->children; childNode; childNode = childNode->next)
		{
			if (!Compare(childNode->name, "color"))
			{
				index = -1;
				double r = 0, g = 0, b = 0, a = 0;
				//Loop through the attributes of the color element.
				att = childNode->properties;

				if (att == nullptr)
				{
					AddToReport(string(loc) + " : No attributes for color element");
					continue;
				}

				for (curAtt = att; curAtt; curAtt = curAtt->next)
				{
					attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));
					a = 255;
					//This signifies that a palette is not being retrieved from the palette file, rather it's being parsed directly out of the ember xml.
					//This also means the palette has already been hue adjusted and it doesn't need to be done again, which would be necessary if it were
					//coming from the palette file.
					currentEmber.m_Palette.m_Index = -1;

					if (!Compare(curAtt->name, "index"))
					{
						Aton(attStr, index);
					}
					else if (!Compare(curAtt->name, "rgb"))
					{
						if (sscanf_s(attStr, "%lf %lf %lf", &r, &g, &b) != 3)
							AddToReport(string(loc) + " : Invalid rgb attribute " + string(attStr));
					}
					else if (!Compare(curAtt->name, "rgba"))
					{
						if (sscanf_s(attStr, "%lf %lf %lf %lf", &r, &g, &b, &a) != 4)
							AddToReport(string(loc) + " : Invalid rgba attribute " + string(attStr));
					}
					else if (!Compare(curAtt->name, "a"))
					{
						if (sscanf_s(attStr, "%lf", &a) != 1)
							AddToReport(string(loc) + " : Invalid a attribute " + string(attStr));
					}
					else
					{
						AddToReport(string(loc) + " : Unknown color attribute " + string(CCX(curAtt->name)));
					}

					xmlFree(attStr);
				}

				//Palette colors are [0..255], convert to [0..1].
				if (index >= 0 && index <= 255)
				{
					T alphaPercent = T(a) / T(255);//Aplha percentage in the range of 0 to 1.
					//Premultiply the palette.
					currentEmber.m_Palette.m_Entries[index].r = alphaPercent * (T(r) / T(255));
					currentEmber.m_Palette.m_Entries[index].g = alphaPercent * (T(g) / T(255));
					currentEmber.m_Palette.m_Entries[index].b = alphaPercent * (T(b) / T(255));
					currentEmber.m_Palette.m_Entries[index].a = T(a) / 255;//Will be one for RGB, and other than one if RGBA with A != 255.
				}
				else
				{
					stringstream ss;
					ss << "ParseEmberElement() : Color element with bad/missing index attribute " << index;
					AddToReport(ss.str());
				}
			}
			else if (!Compare(childNode->name, "colors"))
			{
				//Loop through the attributes of the color element.
				att = childNode->properties;

				if (att == nullptr)
				{
					AddToReport(string(loc) + " : No attributes for colors element");
					continue;
				}

				for (curAtt = att; curAtt; curAtt = curAtt->next)
				{
					attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

					if (!Compare(curAtt->name, "count"))
					{
						Aton(attStr, count);
					}
					else if (!Compare(curAtt->name, "data"))
					{
						if (!ParseHexColors(attStr, currentEmber, count, -4))
						{
							AddToReport(string(loc) + " : Error parsing hexformatted colors, some may be set to zero");
						}
					}
					else
					{
						AddToReport(string(loc) + " : Unknown color attribute " + string(CCX(curAtt->name)));
					}

					xmlFree(attStr);
				}
			}
			else if (!Compare(childNode->name, "palette"))
			{
				//This could be either the old form of palette or the new form.
				//Make sure BOTH are not specified, otherwise either are ok.
				int numColors = 0;
				int numBytes = 0;
				//Loop through the attributes of the palette element.
				att = childNode->properties;

				if (att == nullptr)
				{
					AddToReport(string(loc) + " : No attributes for palette element");
					continue;
				}

				for (curAtt = att; curAtt; curAtt = curAtt->next)
				{
					attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

					if (!Compare(curAtt->name, "count"))
					{
						Aton(attStr, numColors);
					}
					else if (!Compare(curAtt->name, "format"))
					{
						if (!_stricmp(attStr, "RGB"))
							numBytes = 3;
						else if (!_stricmp(attStr, "RGBA"))
							numBytes = 4;
						else
						{
							AddToReport(string(loc) + " : Unrecognized palette format string " + string(attStr) + ", defaulting to RGB");
							numBytes = 3;
						}
					}
					else
					{
						AddToReport(string(loc) + " : Unknown palette attribute " + string(CCX(curAtt->name)));
					}

					xmlFree(attStr);
				}

				//Removing support for whatever "old format" was in flam3.
				//Read formatted string from contents of tag.
				char* palStr = CX(xmlNodeGetContent(childNode));

				if (!ParseHexColors(palStr, currentEmber, numColors, numBytes))
				{
					AddToReport(string(loc) + " : Problem reading hexadecimal color data in palette");
				}

				xmlFree(palStr);
			}
			else if (!Compare(childNode->name, "symmetry"))
			{
				int symKind = INT_MAX;
				//Loop through the attributes of the palette element.
				att = childNode->properties;

				if (att == nullptr)
				{
					AddToReport(string(loc) + " : No attributes for palette element");
					continue;
				}

				for (curAtt = att; curAtt; curAtt = curAtt->next)
				{
					attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

					if (!Compare(curAtt->name, "kind"))
					{
						Aton(attStr, symKind);
					}
					else
					{
						AddToReport(string(loc) + " : Unknown symmetry attribute " + string(attStr));
						continue;
					}

					xmlFree(attStr);
				}

				//if (symKind != INT_MAX)//What to do about this? Should sym not be saved? Or perhaps better intelligence when adding?//TODO//BUG.
				//{
				//	currentEmber.AddSymmetry(symKind, *(GlobalRand.get()));//Determine what to do here.
				//}
			}
			else if (!Compare(childNode->name, "xform") || !Compare(childNode->name, "finalxform"))
			{
				Xform<T>* theXform = nullptr;

				if (!Compare(childNode->name, "finalxform"))
				{
					Xform<T> finalXform;

					if (!ParseXform(childNode, finalXform, false, fromEmber))
					{
						AddToReport(string(loc) + " : Error parsing final xform");
					}
					else
					{
						if (finalXform.m_Weight != 0)
						{
							finalXform.m_Weight = 0;
							AddToReport(string(loc) + " : Final xforms should not have weight specified, setting to zero");
						}

						currentEmber.SetFinalXform(finalXform);
						theXform = currentEmber.NonConstFinalXform();
					}
				}
				else
				{
					Xform<T> xform;

					if (!ParseXform(childNode, xform, false, fromEmber))
					{
						AddToReport(string(loc) + " : Error parsing xform");
					}
					else
					{
						currentEmber.AddXform(xform);
						theXform = currentEmber.GetXform(currentEmber.XformCount() - 1);
					}
				}

				if (theXform)
				{
					//Check for non-zero motion params.
					if (fabs(theXform->m_MotionFreq) > 0.0)//Original checked for motion func being non-zero, but it was set to MOTION_SIN (1) in Xform::Init(), so don't check for 0 here.
					{
						AddToReport(string(loc) + " : Motion parameters should not be specified in regular, non-motion xforms");
					}

					//Motion Language:  Check the xform element for children - should be named 'motion'.
					for (motionNode = childNode->children; motionNode; motionNode = motionNode->next)
					{
						if (!Compare(motionNode->name, "motion"))
						{
							Xform<T> xform(false);//Will only have valid values in fields parsed for motion, all others will be EMPTYFIELD.

							if (!ParseXform(motionNode, xform, true, fromEmber))
								AddToReport(string(loc) + " : Error parsing motion xform");
							else
								theXform->m_Motion.push_back(xform);
						}
					}
				}
			}
			else if (!Compare(childNode->name, "edit"))
			{
				//Create a new XML document with this edit node as the root node.
				currentEmber.m_Edits = xmlNewDoc(XC("1.0"));
				editNode = xmlCopyNode(childNode, 1);
				xmlDocSetRootElement(currentEmber.m_Edits, editNode);
			}
			else if (!Compare(childNode->name, "flame_motion"))
			{
				EmberMotion<T> motion;
				att = childNode->properties;

				if (att == nullptr)
				{
					AddToReport(string(loc) + " : <flame_motion> element has no attributes");
					return false;
				}

				for (curAtt = att; curAtt; curAtt = curAtt->next)
				{
					attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

					if		(ParseAndAssign(curAtt->name, attStr, "motion_frequency", motion.m_MotionFreq,   ret)) { }
					else if	(ParseAndAssign(curAtt->name, attStr, "motion_offset",	  motion.m_MotionOffset, ret)) { }
					else if (!Compare(curAtt->name, "motion_function"))
					{
						string func(attStr);

						if (func == "sin")
							motion.m_MotionFunc = eMotion::MOTION_SIN;
						else if (func == "triangle")
							motion.m_MotionFunc = eMotion::MOTION_TRIANGLE;
						else if (func == "hill")
							motion.m_MotionFunc = eMotion::MOTION_HILL;
						else if (func == "saw")
							motion.m_MotionFunc = eMotion::MOTION_SAW;
						else
						{
							AddToReport(string(loc) + " : invalid flame motion function " + func);
							return false;
						}
					}
					else if (!Compare(curAtt->name, "zoom"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_ZOOM, motion);
					else if (!Compare(curAtt->name, "cam_zpos"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_ZPOS, motion);
					else if (!Compare(curAtt->name, "cam_persp"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_PERSPECTIVE, motion);
					else if (!Compare(curAtt->name, "cam_yaw"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_YAW, motion);
					else if (!Compare(curAtt->name, "cam_pitch"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_PITCH, motion);
					else if (!Compare(curAtt->name, "cam_dof"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_DEPTH_BLUR, motion);
					else if (!Compare(curAtt->name, "rotate"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_ROTATE, motion);
					else if (!Compare(curAtt->name, "brightness"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_BRIGHTNESS, motion);
					else if (!Compare(curAtt->name, "gamma"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_GAMMA, motion);
					else if (!Compare(curAtt->name, "gamma_threshold"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_GAMMA_THRESH, motion);
					else if (!Compare(curAtt->name, "highlight_power"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_HIGHLIGHT_POWER, motion);
					else if (!Compare(curAtt->name, "vibrancy"))
						ret = ret && AttToEmberMotionFloat(att, attStr, eEmberMotionParam::FLAME_MOTION_VIBRANCY, motion);
					else if (!Compare(curAtt->name, "background"))
					{
						double r, g, b;

						if (sscanf_s(attStr, "%lf %lf %lf", &r, &g, &b) != 3)
						{
							AddToReport(string(loc) + " : Invalid flame motion background attribute " + string(attStr));
							xmlFree(attStr);
							return false;
						}

						if (r != 0)
							motion.m_MotionParams.push_back(MotionParam<T>(eEmberMotionParam::FLAME_MOTION_BACKGROUND_R, T(r)));

						if (g != 0)
							motion.m_MotionParams.push_back(MotionParam<T>(eEmberMotionParam::FLAME_MOTION_BACKGROUND_G, T(g)));

						if (b != 0)
							motion.m_MotionParams.push_back(MotionParam<T>(eEmberMotionParam::FLAME_MOTION_BACKGROUND_B, T(b)));
					}
					else if (!Compare(curAtt->name, "center"))
					{
						double cx, cy;

						if (sscanf_s(attStr, "%lf %lf", &cx, &cy) != 2)
						{
							AddToReport(string(loc) + " : Invalid flame motion center attribute " + string(attStr));
							xmlFree(attStr);
							return false;
						}

						if (cx != 0)
							motion.m_MotionParams.push_back(MotionParam<T>(eEmberMotionParam::FLAME_MOTION_CENTER_X, T(cx)));

						if (cy != 0)
							motion.m_MotionParams.push_back(MotionParam<T>(eEmberMotionParam::FLAME_MOTION_CENTER_Y, T(cy)));
					}
					else
					{
						AddToReport(string(loc) + " : Unknown flame motion attribute " + string(CCX(curAtt->name)));
						xmlFree(attStr);
						return false;
					}

					xmlFree(attStr);
				}

				currentEmber.m_EmberMotionElements.push_back(motion);
			}
		}

		if (!fromEmber && !newLinear)
			currentEmber.Flatten(m_FlattenNames);

		for (i = 0; i < currentEmber.XformCount(); i++)
			if (soloXform >= 0 && i != soloXform)
				currentEmber.GetXform(i)->m_Opacity = 0;//Will calc the cached adjusted viz value later.

		return ErrorReport().empty();
	}

	/// <summary>
	/// Parse a floating point value from an xml attribute and add the value to a EmberMotion object
	/// </summary>
	/// <param name="att">The current attribute</param>
	/// <param name="attStr">The attribute value to parse</param>
	/// <param name="param">The flame motion parameter type</param>
	/// <param name="motion">The flame motion element to add the parameter to</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool AttToEmberMotionFloat(xmlAttrPtr att, const char* attStr, eEmberMotionParam param, EmberMotion<T>& motion)
	{
		const char* loc = __FUNCTION__;
		bool r = false;
		T val = 0.0;

		if (Aton(attStr, val))
		{
			motion.m_MotionParams.push_back(MotionParam<T>(param, val));
			r = true;
		}
		else
		{
			AddToReport(string(loc) + " : Failed to parse float value for flame motion attribute \"" + string(CCX(att->name)) + "\" : " + string(attStr));
		}

		return r;
	}

	/// <summary>
	/// Parse an xform element.
	/// </summary>
	/// <param name="childNode">The current node to parse</param>
	/// <param name="xform">The newly constructed xform based on what was parsed</param>
	/// <param name="motion">True if this xform is a motion within a parent xform, else false</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool ParseXform(xmlNode* childNode, Xform<T>& xform, bool motion, bool fromEmber)
	{
		bool success = true;
		char* attStr;
		const char* loc = __FUNCTION__;
		size_t j;
		T temp;
		double a, b, c, d, e, f;
		double vals[10];
		xmlAttrPtr attPtr, curAtt;
		//Loop through the attributes of the xform element.
		attPtr = childNode->properties;

		if (attPtr == nullptr)
		{
			AddToReport(string(loc) + " : Error: No attributes for element");
			return false;
		}

		for (curAtt = attPtr; curAtt; curAtt = curAtt->next)
		{
			attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

			//First parse out simple float reads.
			if		(ParseAndAssign(curAtt->name, attStr, "weight",			  xform.m_Weight, success))		  { }
			else if (ParseAndAssign(curAtt->name, attStr, "color_speed",	  xform.m_ColorSpeed, success))   { }
			else if (ParseAndAssign(curAtt->name, attStr, "animate",		  xform.m_Animate, success))      { }
			else if (ParseAndAssign(curAtt->name, attStr, "opacity",		  xform.m_Opacity, success))      { }
			else if (ParseAndAssign(curAtt->name, attStr, "var_color",		  xform.m_DirectColor, success))  { }
			else if (ParseAndAssign(curAtt->name, attStr, "motion_frequency", xform.m_MotionFreq, success))   { }
			else if (ParseAndAssign(curAtt->name, attStr, "motion_offset",	  xform.m_MotionOffset, success)) { }
			//Parse more complicated reads that have multiple possible values.
			else if (!Compare(curAtt->name, "name"))
			{
				xform.m_Name = string(attStr);
				std::replace(xform.m_Name.begin(), xform.m_Name.end(), ' ', '_');
			}
			else if (!Compare(curAtt->name, "symmetry"))//Legacy support.
			{
				//Deprecated, set both color_speed and animate to this value.
				//Huh? Either set it or not?
				Aton(attStr, temp);
				xform.m_ColorSpeed = (1 - temp) / 2;
				xform.m_Animate = T(temp > 0 ? 0 : 1);
			}
			else if (!Compare(curAtt->name, "motion_function"))
			{
				if (!_stricmp("sin", attStr))
					xform.m_MotionFunc = eMotion::MOTION_SIN;
				else if (!_stricmp("triangle", attStr))
					xform.m_MotionFunc = eMotion::MOTION_TRIANGLE;
				else if (!_stricmp("hill", attStr))
					xform.m_MotionFunc = eMotion::MOTION_HILL;
				else if (!_stricmp("saw", attStr))
					xform.m_MotionFunc = eMotion::MOTION_SAW;
				else
				{
					xform.m_MotionFunc = eMotion::MOTION_SIN;
					AddToReport(string(loc) + " : Unknown motion function " + string(attStr) + ", using sin");
				}
			}
			else if (!Compare(curAtt->name, "color"))
			{
				xform.m_ColorX = xform.m_ColorY = 0;

				//Try two coords first .
				if (sscanf_s(attStr, "%lf %lf", &vals[0], &vals[1]) == 2)
				{
					xform.m_ColorX = T(vals[0]);
					xform.m_ColorY = T(vals[1]);
				}
				else if (sscanf_s(attStr, "%lf", &vals[0]) == 1)//Try one color.
				{
					xform.m_ColorX = T(vals[0]);
				}
				else
				{
					xform.m_ColorX = xform.m_ColorY = T(0.5);
					AddToReport(string(loc) + " : Malformed xform color attribute " + string(attStr) + ", using 0.5, 0.5");
				}
			}
			else if (!Compare(curAtt->name, "chaos"))
			{
				stringstream ss(attStr);
				j = 0;

				while (ss >> temp)
				{
					xform.SetXaos(j, temp);
					j++;
				}
			}
			else if (!Compare(curAtt->name, "plotmode"))
			{
				if (motion == 1)
				{
					AddToReport(string(loc) + " : Motion element cannot have a plotmode attribute");
				}
				else if (!_stricmp("off", attStr))
					xform.m_Opacity = 0;
			}
			else if (!Compare(curAtt->name, "coefs"))
			{
				if (sscanf_s(attStr, "%lf %lf %lf %lf %lf %lf", &a, &d, &b, &e, &c, &f) != 6)//Original did a complicated parsing scheme. This is easier.//ORIG
				{
					a = d = b = e = c = f = 0;
					AddToReport(string(loc) + " : Bad coeffs attribute " + string(attStr));
				}

				xform.m_Affine.A(T(a));
				xform.m_Affine.B(T(b));
				xform.m_Affine.C(T(c));
				xform.m_Affine.D(T(d));
				xform.m_Affine.E(T(e));
				xform.m_Affine.F(T(f));
			}
			else if (!Compare(curAtt->name, "post"))
			{
				if (sscanf_s(attStr, "%lf %lf %lf %lf %lf %lf", &a, &d, &b, &e, &c, &f) != 6)//Original did a complicated parsing scheme. This is easier.//ORIG
				{
					a = d = b = e = c = f = 0;
					AddToReport(string(loc) + " : Bad post coeffs attribute " + string(attStr));
				}

				xform.m_Post.A(T(a));
				xform.m_Post.B(T(b));
				xform.m_Post.C(T(c));
				xform.m_Post.D(T(d));
				xform.m_Post.E(T(e));
				xform.m_Post.F(T(f));
			}
			else
			{
				//Only correct names if it came from an outside source. Names originating from this library are always considered correct.
				string s = fromEmber ? string(CCX(curAtt->name)) : GetCorrectedVariationName(m_BadVariationNames, curAtt);

				if (auto var = m_VariationList.GetVariation(s))
				{
					auto varCopy = var->Copy();
					Aton(attStr, varCopy->m_Weight);
					xform.AddVariation(varCopy);
				}

				//else
				//{
				//	AddToReport("Unsupported variation: " + string((const char*)curAtt->name));
				//}
			}

			xmlFree(attStr);
		}

		//Handle var1.
		for (curAtt = attPtr; curAtt; curAtt = curAtt->next)//Legacy fields, most likely not used.
		{
			bool var1 = false;

			if (!Compare(curAtt->name, "var1"))
			{
				attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

				for (j = 0; j < xform.TotalVariationCount(); j++)
					xform.GetVariation(j)->m_Weight = 0;

				if (Aton(attStr, temp))
				{
					uint iTemp = static_cast<uint>(temp);

					if (iTemp < xform.TotalVariationCount())
					{
						xform.GetVariation(iTemp)->m_Weight = 1;
						var1 = true;
					}
				}

				if (!var1)
					AddToReport(string(loc) + " : Bad value for var1 " + string(attStr));

				xmlFree(attStr);
				break;
			}
		}

		//Handle var.
		for (curAtt = attPtr; curAtt; curAtt = curAtt->next)//Legacy fields, most likely not used.
		{
			bool var = false;

			if (!Compare(curAtt->name, "var"))
			{
				attStr = reinterpret_cast<char*>(xmlGetProp(childNode, curAtt->name));

				if (Aton(attStr, temp))
				{
					for (j = 0; j < xform.TotalVariationCount(); j++)
						xform.GetVariation(j)->m_Weight = temp;

					var = true;
				}

				if (!var)
					AddToReport(string(loc) + " : Bad value for var " + string(attStr));

				xmlFree(attStr);
				break;
			}
		}

		//Now that all xforms have been parsed, go through and try to find params for the parametric variations.
		for (size_t i = 0; i < xform.TotalVariationCount(); i++)
		{
			if (ParametricVariation<T>* parVar = dynamic_cast<ParametricVariation<T>*>(xform.GetVariation(i)))
			{
				for (curAtt = attPtr; curAtt; curAtt = curAtt->next)
				{
					//Only correct names if it came from an outside source. Names originating from this library are always considered correct.
					string s = fromEmber ? string(CCX(curAtt->name)) : GetCorrectedParamName(m_BadParamNames, CCX(curAtt->name));
					const char* name = s.c_str();

					if (parVar->ContainsParam(name))
					{
						T val = 0;
						attStr = CX(xmlGetProp(childNode, curAtt->name));

						if (Aton(attStr, val))
						{
							parVar->SetParamVal(name, val);
						}
						else
						{
							AddToReport(string(loc) + " : Failed to parse parametric variation parameter " + s + " - " + string(attStr));
						}

						xmlFree(attStr);
					}
				}
			}
		}

		return true;
	}

	/// <summary>
	/// Some Apophysis plugins use an inconsistent naming scheme for the parametric variation variables.
	/// This function identifies and converts them to Ember's consistent naming convention.
	/// </summary>
	/// <param name="names">The map of corrected names to search</param>
	/// <param name="name">The current Xml node to check</param>
	/// <returns>The corrected name if one was found, else the passed in name.</returns>
	static string GetCorrectedParamName(const unordered_map<string, string>& names, const char* name)
	{
		const auto& newName = names.find(ToLower(name));

		if (newName != names.end())
			return newName->second;
		else
			return name;
	}

	/// <summary>
	/// Some Apophysis plugins use an inconsistent naming scheme for variation names.
	/// This function identifies and converts them to Ember's consistent naming convention.
	/// It uses some additional intelligence to ensure the variation is the expected one,
	/// by examining the rest of the xform for the existence of parameter names.
	/// </summary>
	/// <param name="vec">The vector of corrected names to search</param>
	/// <param name="att">The current Xml node to check</param>
	/// <returns>The corrected name if one was found, else the passed in name.</returns>
	static string GetCorrectedVariationName(vector<pair<pair<string, string>, vector<string>>>& vec, xmlAttrPtr att)
	{
		for (auto& v : vec)
		{
			if (!_stricmp(v.first.first.c_str(), CCX(att->name)))//Do case insensitive here.
			{
				if (!v.second.empty())
				{
					for (size_t j = 0; j < v.second.size(); j++)
					{
						if (XmlContainsTag(att, v.second[j].c_str()))
							return v.first.second;
					}
				}
				else
				{
					return v.first.second;
				}
			}
		}

		return string(CCX(att->name));
	}

	/// <summary>
	/// Determine if an Xml node contains a given tag.
	/// </summary>
	/// <param name="att">The Xml node to search</param>
	/// <param name="name">The node name to search for</param>
	/// <returns>True if name was found, else false.</returns>
	static bool XmlContainsTag(xmlAttrPtr att, const char* name)
	{
		xmlAttrPtr temp = att;

		do
		{
			if (!_stricmp(name, CCX(temp->name)))
				return true;
		}
		while ((temp = temp->next));

		return false;
	}

	/// <summary>
	/// Parse hexadecimal colors.
	/// This can read RGB and RGBA, however only RGB will be stored.
	/// </summary>
	/// <param name="colstr">The string of hex colors to parse</param>
	/// <param name="ember">The ember whose palette will be assigned the colors</param>
	/// <param name="numColors">The number of colors present</param>
	/// <param name="chan">The number of channels in each color</param>
	/// <returns>True if there were no errors, else false.</returns>
	bool ParseHexColors(char* colstr, Ember<T>& ember, size_t numColors, intmax_t chan)
	{
		size_t colorIndex = 0;
		size_t colorCount = 0;
		uint r, g, b, a;
		int ret;
		char tmps[2];
		size_t skip = std::abs(chan);
		bool ok = true;
		const char* loc = __FUNCTION__;

		//Strip whitespace prior to first color.
		while (isspace(static_cast<int>(colstr[colorIndex])))
			colorIndex++;

		do
		{
			//Parse an RGB triplet at a time.
			if (chan == 3)
				ret = sscanf_s(&(colstr[colorIndex]), "%2x%2x%2x", &r, &g, &b);
			else if (chan == -4)
				ret = sscanf_s(&(colstr[colorIndex]), "00%2x%2x%2x", &r, &g, &b);
			else // chan==4
				ret = sscanf_s(&(colstr[colorIndex]), "%2x%2x%2x%2x", &r, &g, &b, &a);

			a = 1;//Original allows for alpha, even though it will most likely never happen. Ember omits support for it.

			if ((chan != 4 && ret != 3) || (chan == 4 && ret != 4))
			{
				ok = false;
				r = g = b = 0;
				AddToReport(string(loc) + " : Problem reading hexadecimal color data, assigning to 0");
				break;
			}

			colorIndex += 2 * skip;

			while (isspace(static_cast<int>(colstr[colorIndex])))
				colorIndex++;

			ember.m_Palette.m_Entries[colorCount].r = T(r) / T(255);//Hex palette is [0..255], convert to [0..1].
			ember.m_Palette.m_Entries[colorCount].g = T(g) / T(255);
			ember.m_Palette.m_Entries[colorCount].b = T(b) / T(255);
			ember.m_Palette.m_Entries[colorCount].a = T(a);
			colorCount++;
		}
		while (colorCount < numColors && colorCount < ember.m_Palette.m_Entries.size());

#ifdef WIN32

		if (sscanf_s(&(colstr[colorIndex]), "%1s", tmps, sizeof(tmps)) > 0) //Really need to migrate all of this parsing to C++.//TODO
#else
		if (sscanf_s(&(colstr[colorIndex]), "%1s", tmps) > 0)
#endif
		{
			AddToReport(string(loc) + " : Extra data at end of hex color data " + string(&(colstr[colorIndex])));
			ok = false;
		}

		return ok;
	}

	/// <summary>
	/// Wrapper to parse a numeric Xml string value and convert it.
	/// </summary>
	/// <param name="name">The xml tag to parse</param>
	/// <param name="attStr">The name of the Xml attribute</param>
	/// <param name="str">The name of the Xml tag</param>
	/// <param name="val">The parsed value</param>
	/// <param name="b">Bitwise ANDed with true if name matched str and the conversion succeeded, else false. Used for keeping a running value between successive calls.</param>
	/// <returns>True if the tag was matched and the conversion succeeded, else false</returns>
	template <typename valT>
	bool ParseAndAssign(const xmlChar* name, const char* attStr, const char* str, valT& val, bool& b)
	{
		bool ret = false;

		if (!Compare(name, str))
		{
			istringstream istr(attStr);
			istr >> val;
			ret = !istr.bad() && !istr.fail();//Means the Compare() was right, and the conversion succeeded.
		}

		return ret;
	}

	static bool m_Init;
	static unordered_map<string, string> m_BadParamNames;
	static vector<pair<pair<string, string>, vector<string>>> m_BadVariationNames;
	VariationList<T> m_VariationList;//The variation list used to make copies of variations to populate the embers with.
	PaletteList<T> m_PaletteList;
};
}
