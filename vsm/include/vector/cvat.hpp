#pragma once

#include "util/geometry.hpp"

#include <iostream>
#include <vector>


class CVATXML {
public:
	CVATXML();
	~CVATXML();

	std::string task_name;

	std::string owner_username;
	std::string owner_email;

	std::string filename;

	int w, h;

	std::string cvat_header(const std::vector<std::string> &classes);
	std::string cvat_polygon(int class_index, const std::vector<FVertex> &coordinates);
	std::string cvat_footer();

	bool validate();

	std::vector<std::string> classes;
};

