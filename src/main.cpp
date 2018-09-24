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
	const auto model_low = io::load_obj("in/dino_low.obj");
	const auto model_hi = io::load_obj("in/dino_hi.obj", false);

	const auto embree_geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, model_hi.vtxs.data(), 0, sizeof(Vec3f), model_hi.vtxs.size());
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, model_hi.vtris.data(), 0, sizeof(Vec3i), model_hi.vtris.size());
	rtcCommitGeometry(embree_geom);
	const auto geom_id = rtcAttachGeometry(embree_scene, embree_geom);
	rtcReleaseGeometry(embree_geom);

	rtcCommitScene(embree_scene);
	/** End building scene **/

	/** Start scene intersection **/

	const auto img_w = 2048;
	const auto img_h = 2048;
	std::vector<Vec4h> img(img_w*img_h, {0.f,0.f,0.f,0.f});

	for (auto t = 0; t < model_low.uvtris.size(); ++t) {
		const auto vtri = model_low.vtris[t];
		const auto ntri = model_low.ntris[t];
		const auto uvtri = model_low.uvtris[t];

		for(auto a = 0.0f; a <= 1.0f; a += .03) {
			for(auto b = 0.0f; b <= 1.0f; b += .03) {
				const auto c = 1 - a - b;
				if(c < 0 || c > 1) continue;

				const auto p = a*model_low.vtxs[vtri[0]] + b*model_low.vtxs[vtri[1]] + c*model_low.vtxs[vtri[2]];
				const auto n = normalize(a*model_low.norms[ntri[0]] + b*model_low.norms[ntri[1]] + c*model_low.norms[ntri[2]]);

				RTCIntersectContext ray_context;
				rtcInitIntersectContext(&ray_context);

				RTCRayHit ray_hit;
				ray_hit.ray.org_x = p[0]; ray_hit.ray.org_y = p[1]; ray_hit.ray.org_z = p[2];
				ray_hit.ray.dir_x = n[0]; ray_hit.ray.dir_y = n[1]; ray_hit.ray.dir_z = n[2];
				ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
				ray_hit.ray.flags = 0;
				ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
				ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
				rtcIntersect1(embree_scene, &ray_context, &ray_hit);

				if(ray_hit.hit.geomID == geom_id) {
					
					const auto t_hi = ray_hit.hit.primID;
					const auto ntri_hi = model_hi.ntris[t_hi];
					const auto n_hi = normalize(a*model_hi.norms[ntri_hi[0]] + b*model_hi.norms[ntri_hi[1]] + c*model_hi.norms[ntri_hi[2]]);

					const auto mat = transpose(refFromVec(n));
					const auto n_loc = transformVector(mat, n_hi);

					const auto uv = a*model_low.uvs[uvtri[0]] + b*model_low.uvs[uvtri[1]] + c*model_low.uvs[uvtri[2]];
					
					const int i = uv[0] * img_w;
					const int j = uv[1] * img_h;

					auto& pix = img[i + j*img_w];
					pix[0] += 0.5f * n_loc[2] + 0.5f;
					pix[1] += 0.5f * n_loc[1] + 0.5f;
					pix[2] += 0.5f * n_loc[0] + 0.5f;
					pix[3] += 1;
				}
			}	
		}
	}

	for(auto pix_idx = 0; pix_idx < img_w*img_h; ++pix_idx) {
		auto& pix = img[pix_idx];
		if(pix[3] == 0) continue;
		pix[0] /= pix[3];
		pix[1] /= pix[3];
		pix[2] /= pix[3];
		pix[3] = 1.0f;
	}

	io::save_exr("./test.exr", img_w, img_h, img);
	/** End scene intersection **/

	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}