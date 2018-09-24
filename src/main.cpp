// CPU flags modification
#include <xmmintrin.h>
#include <pmmintrin.h>

#include <embree3/rtcore.h>

#include <iostream>

#include "io.hpp"

int main(int argc, char* argv[]) {
	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	// The #ifndef is needed to make VSCode Intellisense ignore these lines. 
	// It believes that _MM_SET_DENORMALS_ZERO_MODE and _MM_DENORMALS_ZERO_ON are defined nowhere.
	#ifndef __INTELLISENSE__
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	#endif // __INTELLISENSE__

	const auto embree_device	= rtcNewDevice("verbose=3");
	const auto embree_scene		= rtcNewScene(embree_device);

	/** Start building scene **/
	const auto model_low = io::load_obj("in/sphere_low.obj");
	const auto model_hi = io::load_obj("in/sphere_hi.obj");

	const auto embree_geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, model_hi.vtxs.data(), 0, sizeof(Vec3f), model_hi.vtxs.size());
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, model_hi.vtris.data(), 0, sizeof(Vec3i), model_hi.vtris.size());
	rtcCommitGeometry(embree_geom);
	const auto geom_id = rtcAttachGeometry(embree_scene, embree_geom);
	rtcReleaseGeometry(embree_geom);

	rtcCommitScene(embree_scene);
	/** End building scene **/

	/** Start scene intersection **/

	const auto img_w = 512;
	const auto img_h = 512;
	std::vector<Vec4h> img(img_w*img_h, {0.f,0.f,1.f,1.f});

	for (auto t = 0; t < model_low.uvtris.size(); ++t) {
		const auto vtri = model_low.vtris[t];
		const auto uvtri = model_low.uvtris[t];

		for(auto a = 0.0f; a < 1.0f; a += .03) {
			for(auto b = 0.0f; b < 1.0f; b += .03) {
				const auto c = 1 - a - b;
				if(c < 0 || c > 1) continue;

				const auto p = a*model_low.vtxs[vtri[0]] + b*model_low.vtxs[vtri[1]] + c*model_low.vtxs[vtri[2]];
				const auto uv = a*model_low.uvs[uvtri[0]] + b*model_low.uvs[uvtri[1]] + c*model_low.uvs[uvtri[2]];

				const int i = (.8 * uv[0] + .1) * img_w;
				const int j = (.8 * uv[1] + .1) * img_h;

				auto& pix = img[i + j*img_w];
				pix[2] = 0.0f;
				if(a == 0) pix[0] = 1.0f;
				if(b == 0) pix[1] = 1.0f;
			}	
		}
	}
	io::save_exr("./test.exr", img_w, img_h, img);
	/** End scene intersection **/

	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}