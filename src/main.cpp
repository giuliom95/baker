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
	const auto model = io::load_obj("in/cube.obj");

	const auto embree_geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, model.vtxs.data(), 0, sizeof(Vec3f), model.vtxs.size());
	rtcSetSharedGeometryBuffer(embree_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, model.vtris.data(), 0, sizeof(Vec3i), model.vtris.size());
	rtcCommitGeometry(embree_geom);
	const auto geom_id = rtcAttachGeometry(embree_scene, embree_geom);
	rtcReleaseGeometry(embree_geom);

	rtcCommitScene(embree_scene);
	/** End building scene **/

	/** Start scene intersection **/

	const auto img_w = 300;
	const auto img_h = 300;
	std::vector<Vec4h> img(img_w*img_h, Vec4h());

	for (auto j = 0; j < img_h; ++j) {
		for (auto i = 0; i < img_w; ++i) {
			RTCIntersectContext ray_context;
			rtcInitIntersectContext(&ray_context);

			RTCRayHit ray_hit;
			ray_hit.ray.org_x = 2.f*i/img_w - .5f; ray_hit.ray.org_y = 2.f*j/img_h - .5f; ray_hit.ray.org_z = 3.0f;
			ray_hit.ray.dir_x = 0.0f; ray_hit.ray.dir_y = 0.0f; ray_hit.ray.dir_z = -1.0f;
			ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
			ray_hit.ray.flags = 0;
			ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
			ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			// std::cout << ray_hit.ray.org_x << ", " << ray_hit.ray.org_y << ": ";
			rtcIntersect1(embree_scene, &ray_context, &ray_hit);
			// std::cout << (signed) ray_hit.hit.geomID << " -- ";
			// std::cout << (signed) ray_hit.ray.tfar << std::endl;
			if(ray_hit.hit.geomID == geom_id) {
				const auto tri_id = ray_hit.hit.primID;
				// std::cout << "## " << tri_id << ": ";
				// std::cout << ray_hit.hit.u << " ";
				// std::cout << ray_hit.hit.v << std::endl;

				const auto uv_tri = model.uvtris[tri_id];
				const auto gamma = 1 - ray_hit.hit.u - ray_hit.hit.v;
				const auto texuv = gamma * model.uvs[uv_tri[0]] + ray_hit.hit.u * model.uvs[uv_tri[1]] + ray_hit.hit.v * model.uvs[uv_tri[2]];
				// std::cout << texuv << std::endl;
				img[i + img_w*j][0] = texuv[0];
				img[i + img_w*j][1] = texuv[1];
				img[i + img_w*j][3] = 1.0;
			}
		}
	}

	io::save_exr("./test.exr", img_w, img_h, img);
	/** End scene intersection **/

	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}