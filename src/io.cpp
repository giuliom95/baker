
#include "io.hpp"

namespace io {
	const void saveEXR (const std::string& path, 
						const int w, const int h, 
						const std::vector<Vec4h>& image) {

		Imf::RgbaOutputFile file(path.c_str(), w, h, Imf::WRITE_RGBA);
		file.setFrameBuffer((Imf::Rgba*)image.data(), 1, w);
		file.writePixels(h);
	}
}