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

		for(auto a = -0.01f; a <= 1.01f; a += .003) {
			for(auto b = -0.01f; b <= 1.01f; b += .003) {
				const auto c = 1 - a - b;
				if(c < 0 || c > 1) continue;

				const auto p0 = model_low.vtxs[vtri[0]];
				const auto p1 = model_low.vtxs[vtri[1]];
				const auto p2 = model_low.vtxs[vtri[2]];
				const auto p = a*p0 + b*p1 + c*p2;
				const auto n = normalize(a*model_low.norms[ntri[0]] + b*model_low.norms[ntri[1]] + c*model_low.norms[ntri[2]]);
				const auto o = p - 10*n;

				RTCIntersectContext ray_context;
				rtcInitIntersectContext(&ray_context);

				RTCRayHit ray_hit;
				ray_hit.ray.org_x = o[0]; ray_hit.ray.org_y = o[1]; ray_hit.ray.org_z = o[2];
				ray_hit.ray.dir_x = n[0]; ray_hit.ray.dir_y = n[1]; ray_hit.ray.dir_z = n[2];
				ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
				ray_hit.ray.flags = 0;
				ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
				ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
				rtcIntersect1(embree_scene, &ray_context, &ray_hit);

				if(ray_hit.hit.geomID == geom_id) {
					
					// UV coords on tri vertices
					const auto uv0 = model_low.uvs[uvtri[0]];
					const auto uv1 = model_low.uvs[uvtri[1]];
					const auto uv2 = model_low.uvs[uvtri[2]];

					const auto uv = a*uv0 + b*uv1 + c*uv2;
					
					const auto uv_tu = uv + Vec2f({0.01f, 0.0f});
					const auto t_area = triarea(uv0, uv1, uv2);

					const auto new_a = triarea(uv_tu,   uv1,   uv2) / t_area;
					const auto new_b = triarea(  uv0, uv_tu,   uv2) / t_area;
					const auto new_c = triarea(  uv0,   uv1, uv_tu) / t_area;
					const auto p_tu = new_a*p0 + new_b*p1 + new_c*p2;
					const auto tang_tu = p_tu - p;

					const auto bitang = normalize(cross(n, tang_tu));
					const auto tang = cross(bitang, n);

					const auto t_hi = ray_hit.hit.primID;
					const auto ntri_hi = model_hi.ntris[t_hi];
					const auto n_hi = normalize(a*model_hi.norms[ntri_hi[0]] + b*model_hi.norms[ntri_hi[1]] + c*model_hi.norms[ntri_hi[2]]);

					const Mat4 mat{tang, bitang, n, Vec3f()};

					const auto n_loc = transformVector(transpose(mat), n_hi);
					
					const int i = uv[0] * img_w;
					const int j = (1 - uv[1]) * img_h;

					auto& pix = img[i + j*img_w];
					pix[0] += 0.5f * n_loc[0] + 0.5f;
					pix[1] += 0.5f * n_loc[1] + 0.5f;
					pix[2] += 0.5f * n_loc[2] + 0.5f;
					pix[3] += 1;
				}
			}	
		}
	}

	for(auto pix_idx = 0; pix_idx < img_w*img_h; ++pix_idx) {
		auto& pix = img[pix_idx];
		if(pix[3] > 0) {
			pix[0] /= pix[3];
			pix[1] /= pix[3];
			pix[2] /= pix[3];
		} else {
			pix[0] = 0.5f;
			pix[1] = 0.5f;
			pix[2] = 1.0f;
		}
		pix[3] = 1.0f;
	}

	io::save_exr("./test.exr", img_w, img_h, img);
	/** End scene intersection **/

	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}