Starlight
=========

Starlight is a simple, opinionated light baking library meant for integration into an asset pipeline.

Instead of baking lightmaps, Starlight bakes 'light masks', which contain just the light from a single source, and which assume that the light is fully bright and constant white. This is so that once the light masks have been baked, they can be combined at runtime in whatever context they are being used. Combining the light masks in real-time allows light brightness and color to be freely varied at runtime, rather than fixed at bake time.

Starlight uses DirectX 12 and DXR to bake light masks for point lights, directional lights, and cubemaps ('sky lights').

Starlight is meant to be consumed from both C++ as a static library, and from other languages as a dynamic-link library with unmangled exports (`cstarlight`).

StarlightDemo is a command-line application that aims to demonstrate how to bake light masks with Starlight, while being as simple and clean as possible.


Light Masks
-----------

Starlight light masks are irradiance maps that contain the total light (irradiance) reaching each texel.

To integrate these light masks with a PBR rendering equation, replace the integral of `Li(p, wi)n.wi dwi` with the value in the light mask.

For lights whose specular components vary with the incoming light direction (e.g. sky lights), you will want to only sample the light mask for the diffuse part of the equation, to show bounce lighting - the specular part should still be computed analytically at runtime.
