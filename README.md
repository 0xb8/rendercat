
A simple OpenGL 4.5 renderer used to teach myself various realtime graphics techniques.

## Sponza scene screenshot
[![Screenshot](https://user-images.githubusercontent.com/12952180/31048847-c29714f4-a63f-11e7-83cd-f7da8709ade0.jpg)](https://user-images.githubusercontent.com/12952180/31048845-a65bc758-a63f-11e7-9679-0c319d453dfd.jpg)

## Features
* Basic forward rendering: no depth prepass (yet), no alpha blending (yet), no animations, etc.
* SRGB textures
* HDR (currently using Uncharted 2 tonemapping)
* Reliable tangent space generation using mikktspace library
* Normal and specular mapping
* Handles 10's of dynamic lights on my 8 years old mid-end GPU with MSAA. Could be increased with forward+
* Using modern-style OpenGL 4.5 API
* Simple FPS controls and GUI using ImGui library

## Features that might be implemented
* Skeletal animation
* Mesh LOD
* Precomputed GI lighting
* Procedural sky/sun
* Bloom effect
* Water reflection/refraction

## Features that will NOT be implemented
* Deferred rendering, because my GPU has only 1 GB of memory, and I'd rather not waste time trying to make everything fit
* FXAA/TXAA/etc, because why bother when you have MSAA
