// CPU flags modification
#include <xmmintrin.h>
#include <pmmintrin.h>

#include <embree3/rtcore.h>

#include <iostream>

#include <stdint.h>

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
	const auto embree_tri		= rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	float* embree_vtx_buf	 	= (float*) rtcSetNewGeometryBuffer(	embree_tri, 
																	RTC_BUFFER_TYPE_VERTEX, 
																	0, RTC_FORMAT_FLOAT3,
																	3*sizeof(float), 5);
	embree_vtx_buf[0] = 0.0f;  embree_vtx_buf[1] = 0.0f;  embree_vtx_buf[2] = 0.0f;
	embree_vtx_buf[3] = 1.0f;  embree_vtx_buf[4] = 0.0f;  embree_vtx_buf[5] = 0.0f;
	embree_vtx_buf[6] = 0.0f;  embree_vtx_buf[7] = 1.0f; embree_vtx_buf[8] = 0.0f;
	embree_vtx_buf[9] = 1.0f; embree_vtx_buf[10] = 1.0f; embree_vtx_buf[11] = 0.0f;
	embree_vtx_buf[12] = 0.5f; embree_vtx_buf[13] = 0.5f; embree_vtx_buf[14] = 0.0f;

	unsigned* embree_idx_buf	= (unsigned*) rtcSetNewGeometryBuffer(	embree_tri, 
															RTC_BUFFER_TYPE_INDEX, 
															0, RTC_FORMAT_UINT3,
															3*sizeof(unsigned), 4);
	embree_idx_buf[0] = 0;  embree_idx_buf[1] = 1;  embree_idx_buf[2] = 4;
	embree_idx_buf[3] = 1;  embree_idx_buf[4] = 3;  embree_idx_buf[5] = 4;
	embree_idx_buf[6] = 4;  embree_idx_buf[7] = 3;  embree_idx_buf[8] = 2;
	embree_idx_buf[9] = 0; embree_idx_buf[10] = 4; embree_idx_buf[11] = 2;


	rtcCommitGeometry(embree_tri);
	const auto geom_id = rtcAttachGeometry(embree_scene, embree_tri);
	rtcReleaseGeometry(embree_tri);

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