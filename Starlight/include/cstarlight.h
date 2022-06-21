//
// Exports C functions for the Starlight API (useful for interop with other languages)
//

#pragma once

#ifdef STARLIGHT_DLL
#define STARLIGHT_API __declspec(dllexport)
#else
#define STARLIGHT_API __declspec(dllimport)
#endif

typedef struct MeshFormat starlight_mesh_format_t;
typedef struct MeshInfo starlight_mesh_info_t;
typedef struct MaterialInfo starlight_material_info_t;
typedef struct PointLight starlight_point_light_t;
typedef struct DirectionalLight starlight_directional_light_t;
typedef struct SkyLight starlight_sky_light_t;
typedef struct OutputBuffer starlight_output_buffer_t;

#ifdef __cplusplus
extern "C"
{
#endif

typedef void starlight_t;

STARLIGHT_API starlight_t* starlight_create(void* device, starlight_mesh_format_t meshFormat);
STARLIGHT_API void starlight_destroy(starlight_t* starlight);

STARLIGHT_API void starlight_add_mesh(starlight_t* starlight, const starlight_mesh_info_t* mesh_info, const starlight_material_info_t* material_info);
STARLIGHT_API void starlight_bake_point_light(starlight_t* starlight, const starlight_point_light_t* point_light, const starlight_output_buffer_t* output_buffer);
STARLIGHT_API void starlight_bake_directional_light(starlight_t* starlight, const starlight_directional_light_t* directional_light, const starlight_output_buffer_t* output_buffer);
STARLIGHT_API void starlight_bake_sky_light(starlight_t* starlight, const starlight_sky_light_t* sky_light, const starlight_output_buffer_t* output_buffer);

#ifdef __cplusplus
}
#endif