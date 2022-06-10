Starlight
=========

Starlight is a simple, opinionated light baking library meant for integration into an asset pipeline.

Instead of baking lightmaps, Starlight bakes 'light masks', which contain just the light from a single source, and which assume that the light is fully bright and constant white. This is so that once the light masks have been baked, they can be combined at runtime in whatever context they are being used. Combining the light masks in real-time allows light brightness and color to be freely varied at runtime, rather than fixed at bake time.

Starlight uses DirectX 12 and DXR to bake light masks for point lights, directional lights, and cubemaps ('sky lights').

Starlight is meant to be consumed from both C++ as a static library, and from other languages as a dynamic-link library with unmangled exports (`cstarlight`).