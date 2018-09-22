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
	const auto geom_id = io::load_model("in/plane.obj", embree_device, embree_scene, true);

	rtcCommitScene(embree_scene);
	/** End building scene **/

	/** Start scene intersection **/

	for (auto j = -1; j <= 5; j += 2) {
		for (auto i = -1; i <= 5; i += 2) {
			RTCIntersectContext ray_context;
			rtcInitIntersectContext(&ray_context);

			RTCRayHit ray_hit;
			ray_hit.ray.org_x = i * 0.25f + .1f; ray_hit.ray.org_y = j * 0.25f + .1f; ray_hit.ray.org_z = 3.0f;
			ray_hit.ray.dir_x = 0.0f; ray_hit.ray.dir_y = 0.0f; ray_hit.ray.dir_z = -1.0f;
			ray_hit.ray.tnear = 0.0f; ray_hit.ray.tfar = 100.0f; 
			ray_hit.ray.flags = 0;
			ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
			ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			std::cout << ray_hit.ray.org_x << ", " << ray_hit.ray.org_y << ": ";
			rtcIntersect1(embree_scene, &ray_context, &ray_hit);
			std::cout << (signed) ray_hit.hit.geomID << " -- ";
			std::cout << (signed) ray_hit.ray.tfar << std::endl;
			if(ray_hit.hit.geomID == geom_id) {
				std::cout << "## " << ray_hit.hit.primID << ": ";
				std::cout << ray_hit.hit.u << " ";
				std::cout << ray_hit.hit.v << std::endl;
			}
		}
	}
	/** End scene intersection **/

	
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);
}