
#pragma once
#include <vector>
#include <string>

namespace
{
	unsigned int AligmentedValue(unsigned int Size, unsigned int AlignmentSize)
	{
		return (Size + AlignmentSize - (Size%AlignmentSize));
	}

	//WideString‚É•ÏŠ·‚µ‚Ü‚·
	std::wstring GetWideString(const std::string & str)
	{
		std::wstring ReturnWStr;
		auto StirngLengh = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), nullptr, 0);
		ReturnWStr.resize(StirngLengh);

		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &ReturnWStr[0], StirngLengh);

		return ReturnWStr;
	}

	std::string GetExtension(const std::string & Path)
	{
		int Index = Path.rfind('.');
		if (Index == -1)
		{
			return "";
		}
		return Path.substr(Index + 1, Path.length() - Index - 1);
	}

	std::vector <std::string> SplitFilePath(const std::string & Path, const char Splitter)
	{
		std::vector<std::string> ReturnVector;
		size_t Index = 0;
		int Offset = 0;

		if (std::count(Path.begin(), Path.end(), '*') > 0)
		{
			do
			{
				Index = Path.find(Splitter, Offset);

				if (Index != -1)
				{
					ReturnVector.emplace_back(Path.substr(Offset, Index - Offset));
					Offset = Index + 1;
				}


			} while (Index != -1);
		}
		else
		{
			ReturnVector.emplace_back(Path);
		}

		return ReturnVector;
	}
}