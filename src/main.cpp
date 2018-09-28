// CPU flags modification
#include <xmmintrin.h>
#include <pmmintrin.h>

#include <embree3/rtcore.h>

#include <iostream>

#include "io.hpp"

// Given a model and an Embree ray hit compute the normal on the given model on the hit point
const Vec3f inters_normal(const Model& model, const RTCRayHit& rh) {
	const auto t_hi = rh.hit.primID;
	const auto ntri_hi = model.ntris[t_hi];
	const auto tria_hi = rh.hit.u;
	const auto trib_hi = rh.hit.v;
	const auto tric_hi = 1 - tria_hi - trib_hi;
	return normalize(	tric_hi*model.norms[ntri_hi[0]] + 
						tria_hi*model.norms[ntri_hi[1]] + 
						trib_hi*model.norms[ntri_hi[2]]);
}


int main(int argc, char* argv[]) {
	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	// The #ifndef is needed to make VSCode Intellisense ignore these lines. 
	// It believes that _MM_SET_DENORMALS_ZERO_MODE and _MM_DENORMALS_ZERO_ON are defined nowhere.
	#ifndef __INTELLISENSE__
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	#endif // __INTELLISENSE__

	io::CmdArgs cmd_args;
	try {
		cmd_args = io::parse_cmd_args(argc, argv);
	} catch (std::invalid_argument e) {
		return -1;
	}

	const auto embree_device	= rtcNewDevice("verbose=3");
	const auto embree_scene		= rtcNewScene(embree_device);

	const auto model_low = io::load_obj(cmd_args.model_low_path);
	
	// Start building Embree scene
	const auto model_hi = io::load_obj(cmd_args.model_hi_path, false);
	const auto embree_geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, model_hi.vtxs.data(), 0, sizeof(Vec3f), model_hi.vtxs.size());
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, model_hi.vtris.data(), 0, sizeof(Vec3i), model_hi.vtris.size());
	rtcCommitGeometry(embree_geom);
	const auto geom_id = rtcAttachGeometry(embree_scene, embree_geom);
	rtcReleaseGeometry(embree_geom);
	rtcCommitScene(embree_scene);

	// Create texture buffer
	const auto img_w = 2048;
	const auto img_h = 2048;
	std::vector<Vec4h> img(img_w*img_h, {0.f,0.f,0.f,0.f});

	// This data structere is needed by embree. It is never read in this code.
	RTCIntersectContext ray_context;
	rtcInitIntersectContext(&ray_context);

	for (auto t = 0; t < model_low.uvtris.size(); ++t) {
		const auto vtri = model_low.vtris[t];
		const auto ntri = model_low.ntris[t];
		const auto uvtri = model_low.uvtris[t];

		for(auto a = -0.01f; a <= 1.01f; a += .01) {
			for(auto b = -0.01f; b <= 1.01f; b += .01) {
				const auto c = 1 - a - b;

				// Discard samples which are out of triangle convex combination
				if(c < 0 || c > 1) continue;

				// If the ray direction has been already flipped
				auto flipped = false;

				const auto p0 = model_low.vtxs[vtri[0]];
				const auto p1 = model_low.vtxs[vtri[1]];
				const auto p2 = model_low.vtxs[vtri[2]];
				const auto p = a*p0 + b*p1 + c*p2;
				const auto n = normalize(a*model_low.norms[ntri[0]] + b*model_low.norms[ntri[1]] + c*model_low.norms[ntri[2]]);

				// These params won't change later
				RTCRayHit ray_hit;
				ray_hit.ray.org_x = p[0]; ray_hit.ray.org_y = p[1]; ray_hit.ray.org_z = p[2];
				ray_hit.ray.flags = 0;

				ray_hit.ray.dir_x = n[0]; ray_hit.ray.dir_y = n[1]; ray_hit.ray.dir_z = n[2];
				ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
				ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
				ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
				rtcIntersect1(embree_scene, &ray_context, &ray_hit);

				// If no intersection shooting outward the low mesh try inwards
				if(ray_hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
					ray_hit.ray.dir_x = -n[0]; ray_hit.ray.dir_y = -n[1]; ray_hit.ray.dir_z = -n[2];
					ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
					ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
					ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
					rtcIntersect1(embree_scene, &ray_context, &ray_hit);
					flipped = true;
				}

				// Get hi model normal
				auto n_hi = inters_normal(model_hi, ray_hit);

				if(dot(n_hi, n) < 0 && !flipped) {
					ray_hit.ray.dir_x = -n[0]; ray_hit.ray.dir_y = -n[1]; ray_hit.ray.dir_z = -n[2];
					ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
					ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
					ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
					rtcIntersect1(embree_scene, &ray_context, &ray_hit);

					// Compute normal again after new hit
					n_hi = inters_normal(model_hi, ray_hit);
				}

				// UV coords on tri vertices
				const auto uv0 = model_low.uvs[uvtri[0]];
				const auto uv1 = model_low.uvs[uvtri[1]];
				const auto uv2 = model_low.uvs[uvtri[2]];

				const auto uv = a*uv0 + b*uv1 + c*uv2;
				
				// Move uv coords slightly on the right
				const auto uv_tu = uv + Vec2f({0.01f, 0.0f});
				// Compute barycentric coords for new uv_tu
				const auto t_area = triarea(uv0, uv1, uv2);
				const auto new_a = triarea(uv_tu,   uv1,   uv2) / t_area;
				const auto new_b = triarea(  uv0, uv_tu,   uv2) / t_area;
				const auto new_c = triarea(  uv0,   uv1, uv_tu) / t_area;
				// Get world space point mapped to uv_tu 
				const auto p_tu = new_a*p0 + new_b*p1 + new_c*p2;
				const auto tang_tu = p_tu - p;
				// Build tangent space to world space reference frame
				const auto bitang = normalize(cross(n, tang_tu));
				const auto tang = cross(bitang, n);
				const Mat4 mat{tang, bitang, n, Vec3f()};

				// Hi poly normal into tangent space of low poly model
				const auto n_loc = transformVector(transpose(mat), n_hi);
				
				// Texel coords
				const int i = uv[0] * img_w;
				const int j = (1 - uv[1]) * img_h;

				// Accomulate normal on the texel
				auto& pix = img[i + j*img_w];
				pix[0] += n_loc[0];
				pix[1] += n_loc[1];
				pix[2] += n_loc[2];
				pix[3] += 1;
			}	
		}
	}

	for(auto pix_idx = 0; pix_idx < img_w*img_h; ++pix_idx) {
		auto& pix = img[pix_idx];
		if(pix[3] > 0) {
			// Average the accomulated computed normals
			const auto n = normalize((1/pix[3]) * Vec3f({pix[0], pix[1], pix[2]}));
			// Scale and translate color space to map -1->0, 0->.5, +1->1
			pix[0] = 0.5f * n[0] + 0.5f;
			pix[1] = 0.5f * n[1] + 0.5f;
			pix[2] = 0.5f * n[2] + 0.5f;
		} else {
			pix[0] = 0.5f;
			pix[1] = 0.5f;
			pix[2] = 1.0f;
		}
		pix[3] = 1.0f;
	}

	io::save_exr(cmd_args.texture_path, img_w, img_h, img);
	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}