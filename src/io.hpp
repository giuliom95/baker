#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>

#include "math.hpp"
#include "model.hpp"

namespace io {
	class CmdArgs {
	public:
		std::string model_low_path;
		std::string model_hi_path;
		std::string texture_path;
	};

	const void	save_exr(	const std::string& path,
							const int w, const int h, 
							const std::vector<Vec4h>& image);
	
	const Model	load_obj(	const std::string& path,
							const bool load_uv = true);

	const CmdArgs parse_cmd_args(int argc, char* argv[]);
}

#endif