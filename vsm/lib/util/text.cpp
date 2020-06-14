#include "util/text.hpp"
#include <algorithm>


bool startswith(std::string const &text, std::string const &beginning) {
	// Reference used:
	//  https://stackoverflow.com/a/874160/1692112
	if (text.length() >= beginning.length() &&
			text.compare(0, beginning.length(), beginning) == 0)
		return true;
	return false;
}

bool endswith(std::string const &text, std::string const &ending) {
	// Reference used:
	//  https://stackoverflow.com/a/874160/1692112
	if (text.length() >= ending.length() &&
			text.compare(text.length() - ending.length(), ending.length(), ending) == 0)
		return true;
	return false;
}

std::string tolower(std::string const &text) {
	// Reference used:
	//  https://stackoverflow.com/a/313990/1692112
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c); });
	return result;
}

std::string toupper(std::string const &text) {
	// Reference used:
	//  https://stackoverflow.com/a/313990/1692112
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::toupper(c); });
	return result;
}

