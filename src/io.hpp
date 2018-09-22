#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <vector>
#include <string>

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>

#include "math.hpp"

namespace io {
	const void save_exr (	const std::string& path,
							const int w, const int h, 
							const std::vector<Vec4h>& image);
}

#endif