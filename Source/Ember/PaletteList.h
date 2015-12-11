#pragma once

#include "Palette.h"

/// <summary>
/// PaletteList class.
/// </summary>

namespace EmberNs
{
/// <summary>
/// Holds a list of palettes read from an Xml file. Since the default list from flam3-palettes.xml is fairly large at 700 palettes,
/// the list member is kept as a static. This class derives from EmberReport in order to report any errors that occurred while reading the Xml.
/// Note that although the Xml color values are expected to be 0-255, they are converted and stored as normalized colors, with values from 0-1.
/// Template argument expected to be float or double.
/// </summary>
template <typename T>
class EMBER_API PaletteList : public EmberReport
{
public:
	static const char* m_DefaultFilename;

	/// <summary>
	/// Empty constructor which initializes the palette map with the default palette file.
	/// </summary>
	PaletteList()
	{
		Add(string(m_DefaultFilename));
	}

	/// <summary>
	/// Read an Xml palette file into memory.
	/// This must be called before any palette file usage.
	/// </summary>
	/// <param name="filename">The full path to the file to read</param>
	/// <param name="force">If true, override the initialization state and force a read, else observe the initialization state.</param>
	/// <returns>Whether anything was read</returns>
	bool Add(const string& filename, bool force = false)
	{
		bool added = true;
		auto palettes = m_Palettes.insert(make_pair(filename, vector<Palette<T>>()));

		if (force || palettes.second)
		{
			string buf;
			const char* loc = __FUNCTION__;

			if (ReadFile(filename.c_str(), buf))
			{
				xmlDocPtr doc = xmlReadMemory(static_cast<const char*>(buf.data()), int(buf.size()), filename.c_str(), nullptr, XML_PARSE_NONET);

				if (doc)
				{
					auto rootNode = xmlDocGetRootElement(doc);
					auto pfilename = shared_ptr<string>(new string(filename));

					palettes.first->second.clear();
					palettes.first->second.reserve(buf.size() / 2048);//Roughly what it takes per palette.
					ParsePalettes(rootNode, pfilename, palettes.first->second);
					xmlFreeDoc(doc);
				}
				else
				{
					added = false;
					m_Palettes.erase(filename);
					AddToReport(string(loc) + " : Couldn't load xml doc");
				}
			}
			else
			{
				added = false;
				m_Palettes.erase(filename);
				AddToReport(string(loc) + " : Couldn't read palette file " + filename);
			}
		}

		return added;
	}

	/// <summary>
	/// Get the palette at a random index in a random file in the map.
	/// </summary>
	Palette<T>* GetRandomPalette()
	{
		auto p = m_Palettes.begin();
		size_t i = 0, paletteFileIndex = QTIsaac<ISAAC_SIZE, ISAAC_INT>::GlobalRand->Rand() % Size();
		
		//Move p forward i elements.
		while (i < paletteFileIndex && p != m_Palettes.end())
		{
			++i;
			++p;
		}

		if (i < Size())
		{
			size_t paletteIndex = QTIsaac<ISAAC_SIZE, ISAAC_INT>::GlobalRand->Rand() % p->second.size();

			if (paletteIndex < p->second.size())
				return &p->second[paletteIndex];
		}

		return nullptr;
	}

	/// <summary>
	/// Get the palette at a specified index in the specified file in the map.
	/// </summary>
	/// <param name="filename">The filename of the palette to retrieve</param>
	/// <param name="i">The index of the palette to read. A value of -1 indicates a random palette.</param>
	/// <returns>A pointer to the requested palette if the index was in range, else nullptr.</returns>
	Palette<T>* GetPalette(const string& filename, size_t i)
	{
		auto& palettes = m_Palettes[filename];

		if (!palettes.empty() && i < palettes.size())
			return &palettes[i];

		return nullptr;
	}

	/// <summary>
	/// Get a pointer to a palette with a specified name in the specified file in the map.
	/// </summary>
	/// <param name="filename">The filename of the palette to retrieve</param>
	/// <param name="name">The name of the palette to retrieve</param>
	/// <returns>A pointer to the palette if found, else nullptr</returns>
	Palette<T>* GetPaletteByName(const string& filename, const string& name)
	{
		for (auto& palettes : m_Palettes)
			if (palettes.first == filename)
				for (auto& palette : palettes.second)
					if (palette.m_Name == name)
						return &palette;

		return nullptr;
	}

	/// <summary>
	/// Get a copy of the palette at a specified index in the specified file in the map
	/// with its hue adjusted by the specified amount.
	/// </summary>
	/// <param name="filename">The filename of the palette to retrieve</param>
	/// <param name="i">The index of the palette to read.</param>
	/// <param name="hue">The hue adjustment to apply</param>
	/// <param name="palette">The palette to store the output</param>
	/// <returns>True if successful, else false.</returns>
	bool GetHueAdjustedPalette(const string& filename, size_t i, T hue, Palette<T>& palette)
	{
		bool b = false;

		if (Palette<T>* unadjustedPal = GetPalette(filename, i))
		{
			unadjustedPal->MakeHueAdjustedPalette(palette, hue);
			b = true;
		}

		return b;
	}

	/// <summary>
	/// Clear the palette list and reset the initialization state.
	/// </summary>
	void Clear()
	{
		m_Palettes.clear();
	}

	/// <summary>
	/// Get the size of the palettes map.
	/// This will be the number of files read.
	/// </summary>
	/// <returns>The size of the palettes map</returns>
	size_t Size() { return m_Palettes.size(); }

	/// <summary>
	/// Get the size of specified palette vector in the palettes map.
	/// </summary>
	/// <param name="index">The index of the palette in the map to retrieve</param>
	/// <returns>The size of the palette vector at the specified index in the palettes map</returns>
	size_t Size(size_t index)
	{
		size_t i = 0;
		auto p = m_Palettes.begin();

		while (i < index && p != m_Palettes.end())
		{
			++i;
			++p;
		}

		return p->second.size();
	}

	/// <summary>
	/// Get the size of specified palette vector in the palettes map.
	/// </summary>
	/// <param name="s">The filename of the palette in the map to retrieve</param>
	/// <returns>The size of the palette vector at the specified index in the palettes map</returns>
	size_t Size(const string& s)
	{
		return m_Palettes[s].size();
	}

	/// <summary>
	/// Get the name of specified palette in the palettes map.
	/// </summary>
	/// <param name="index">The index of the palette in the map to retrieve</param>
	/// <returns>The name of the palette vector at the specified index in the palettes map</returns>
	const string& Name(size_t index)
	{
		size_t i = 0;
		auto p = m_Palettes.begin();

		while (i < index && p != m_Palettes.end())
		{
			++i;
			++p;
		}

		return p->first;
	}

private:
	/// <summary>
	/// Parses an Xml node for all palettes present and stores them in the passed in palette vector.
	/// Note that although the Xml color values are expected to be 0-255, they are converted and
	/// stored as normalized colors, with values from 0-1.
	/// </summary>
	/// <param name="node">The parent note of all palettes in the Xml file.</param>
	/// <param name="filename">The name of the Xml file.</param>
	/// <param name="palettes">The vector to store the paresed palettes associated with this file in.</param>
	void ParsePalettes(xmlNode* node, const shared_ptr<string>& filename, vector<Palette<T>>& palettes)
	{
		bool hexError = false;
		char* val;
		const char* loc = __FUNCTION__;
		xmlAttrPtr attr;

		while (node)
		{
			if (node->type == XML_ELEMENT_NODE && !Compare(node->name, "palette"))
			{
				attr = node->properties;
				Palette<T> palette;

				while (attr)
				{
					val = reinterpret_cast<char*>(xmlGetProp(node, attr->name));

					if (!Compare(attr->name, "data"))
					{
						int colorIndex = 0;
						uint r, g, b;
						int colorCount = 0;
						hexError = false;

						do
						{
							int ret = sscanf_s(static_cast<char*>(&(val[colorIndex])),"00%2x%2x%2x", &r, &g, &b);

							if (ret != 3)
							{
								AddToReport(string(loc) + " : Problem reading hexadecimal color data " + string(&val[colorIndex]));
								hexError = true;
								break;
							}

							colorIndex += 8;

							while (isspace(int(val[colorIndex])))
								colorIndex++;

							palette[colorCount].r = T(r) / T(255);//Store as normalized colors in the range of 0-1.
							palette[colorCount].g = T(g) / T(255);
							palette[colorCount].b = T(b) / T(255);

							colorCount++;
						} while (colorCount < COLORMAP_LENGTH);
					}
					else if (!Compare(attr->name, "number"))
					{
						palette.m_Index = atoi(val);
					}
					else if (!Compare(attr->name, "name"))
					{
						palette.m_Name = string(val);
					}

					xmlFree(val);
					attr = attr->next;
				}

				if (!hexError)
				{
					palette.m_Filename = filename;
					palettes.push_back(palette);
				}
			}
			else
			{
				ParsePalettes(node->children, filename, palettes);
			}

			node = node->next;
		}
	}

	static map<string, vector<Palette<T>>> m_Palettes;//The map of filenames to vectors that store the palettes.
};
}
