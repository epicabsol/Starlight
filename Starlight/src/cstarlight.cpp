#include "cstarlight.h"

#include "Starlight.hpp"

typedef MeshInfo starlight_mesh_info_t;
typedef MaterialInfo starlight_material_info_t;
typedef PointLight starlight_point_light_t;
typedef DirectionalLight starlight_directional_light_t;
typedef SkyLight starlight_sky_light_t;
typedef OutputBuffer starlight_output_buffer_t;

starlight_t* starlight_create(void* device)
{
	return Starlight::CreateStarlight((ID3D12Device*)device);
}

void starlight_destroy(starlight_t* starlight)
{
	delete (Starlight*)starlight;
}

void starlight_add_mesh(starlight_t* starlight, const starlight_mesh_info_t* mesh_info, const starlight_material_info_t* material_info)
{
	((Starlight*)starlight)->AddMesh(mesh_info, material_info);
}

void starlight_bake_point_light(starlight_t* starlight, const starlight_point_light_t* point_light, const starlight_output_buffer_t* output_buffer)
{
	((Starlight*)starlight)->BakePointLight(point_light, output_buffer);
}

void starlight_bake_directional_light(starlight_t* starlight, const starlight_directional_light_t* directional_light, const starlight_output_buffer_t* output_buffer)
{
	((Starlight*)starlight)->BakeDirectionalLight(directional_light, output_buffer);
}

void starlight_bake_sky_light(starlight_t* starlight, const starlight_sky_light_t* sky_light, const starlight_output_buffer_t* output_buffer)
{
	((Starlight*)starlight)->BakeSkyLight(sky_light, output_buffer);
}