
A simple OpenGL 4.5 renderer used to teach myself various realtime graphics techniques.

## Sponza scene screenshot
[![Screenshot](https://user-images.githubusercontent.com/12952180/34455696-4320462e-eda6-11e7-91d8-05f119064d4b.jpg)](https://user-images.githubusercontent.com/12952180/34455687-1ad4f71e-eda6-11e7-8c7e-5d85cbcbb109.jpg)
(click for hi-res)

## Features
* Basic forward rendering: no depth prepass (yet), no alpha blending (yet), no animations, etc.
* Anti-aliased alpha masking using *Alpha to Coverage* technique
* Directional fog
* SRGB textures
* HDR (currently using Uncharted 2 tonemapping)
* Reliable tangent space generation using mikktspace library
* Normal and specular mapping
* Handles 10's of dynamic lights on my 8 years old mid-end GPU with MSAA. Could be increased with forward+
* Using modern OpenGL 4.5 API
* Simple FPS controls and GUI using ImGui library

## Features (todo)
* Skeletal animation
* Mesh LOD
* Precomputed GI lighting
* Procedural sky/sun
* Bloom/dof effect
* Animated textures
* Water reflection/refraction
