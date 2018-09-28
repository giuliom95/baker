
#include "io.hpp"

namespace io {
	const void save_exr (	const std::string& path, 
							const int w, const int h, 
							const std::vector<Vec4h>& image) {
		Imf::RgbaOutputFile file(path.c_str(), w, h, Imf::WRITE_RGBA);
		file.setFrameBuffer((Imf::Rgba*)image.data(), 1, w);
		file.writePixels(h);
	}

	const Model load_obj(const std::string& path, const bool load_uv) {
		std::ifstream input{path};
		if(!input) {
			std::cerr << "ERROR: Impossible to open \"" << path << "\"!" << std::endl;
			throw;
		}

		std::vector<Vec3f> vtxs;
		std::vector<Vec3f> norms;
		std::vector<Vec2f> uvs;

		std::vector<Vec3i> vtris;
		std::vector<Vec3i> ntris;
		std::vector<Vec3i> uvtris;

		while(!input.eof()) {
			std::string head;
			input >> head;
			if(head == "v") {
				Vec3f v;
				input >> v[0] >> v[1] >> v[2];
				vtxs.push_back(v);
			} else if(head == "vn") {
				Vec3f n;
				input >> n[0] >> n[1] >> n[2];
				n = normalize(n);
				norms.push_back(n);
			} else if(head == "vt") {
				Vec2f uv;
				input >> uv[0] >> uv[1];
				uvs.push_back(uv);
			} else if(head == "f") {
				// Remember that OBJ start counting from one
				Vec3i vtri;
				Vec3i ntri;
				Vec3i uvtri;
				for(auto i = 0; i < 3; ++i) {
					std::string elem;
					input >> elem;
					const auto sep1 = elem.find('/');
					vtri[i] = std::stoi(elem.substr(0, sep1)) - 1;

					elem = elem.substr(sep1+1, elem.length()-(sep1+1));
					const auto sep2 = elem.find('/');
					if(load_uv) uvtri[i] = std::stoi(elem.substr(0, sep2)) - 1;
					ntri[i] = std::stoi(elem.substr(sep2+1, elem.length()-(sep2+1))) - 1;
				}
				vtris.push_back(vtri);
				ntris.push_back(ntri);
				if(load_uv) uvtris.push_back(uvtri);
			}
		}

		return {vtxs, norms, uvs, vtris, ntris, uvtris};
	}


	const CmdArgs parse_cmd_args(int argc, char* argv[]) {
		CmdArgs args{};
		if(argc != 4) {
			std::cerr << "ERROR: Wrong number of arguments! " << std::endl;
			std::cerr << "  " << io::usage_msg << std::endl;
			throw std::invalid_argument("Wrong number of arguments!");
		}
		args.model_low_path = argv[1];
		args.model_hi_path = argv[2];
		args.texture_path = argv[3];
		return args;
	}
}