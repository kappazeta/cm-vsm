#pragma once

#include <filesystem>


class ESA_S2_Image_Operator {
public:
	typedef enum data_type_t {
		DT_TCI,
		DT_SCL,
		DT_GML
	};

	virtual bool operator()(const std::filesystem::path &path, data_type_t type) {}
};


class ESA_S2_Image {
	public:
		ESA_S2_Image();
		~ESA_S2_Image();

		int tile_size;

		void set_tile_size(int tile_size);

		bool process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op);

		bool split_tci_jp2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op);
		bool split_scl_jp2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op);
};

