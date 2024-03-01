#pragma once

#include <functional>
#include <sstream>
#include <string>

namespace dancenet
{
	std::string readTextFile(const std::string& path);
	void writeTextFile(const std::string& path, const std::string& text);

	template<class C, class T>
	std::string rangeToString(const C& container, const std::function<void(std::stringstream&, const T& )>& f)
	{
		std::stringstream ss;
		for (const auto& v : container)
			f(ss, v);
		return ss.str();
	}

	std::vector<std::string> string_split(const std::string& str, char separator, int maxSplits= INT_MAX);

	std::string copyfiletotemp(std::string infile);
}