#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>

#include <embree3/rtcore.h>

#include "math.hpp"

namespace io {
	const void save_exr (	const std::string& path,
							const int w, const int h, 
							const std::vector<Vec4h>& image);
	
	const int load_model(	const std::string& path,
							const RTCDevice& embree_device,
							const RTCScene& embree_scene,
							bool load_uvmap = false);
}

#endif