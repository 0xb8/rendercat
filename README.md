
A simple OpenGL 4.5 renderer used to teach myself various realtime graphics techniques.

## Sponza scene screenshot
[![Screenshot](https://user-images.githubusercontent.com/12952180/103081831-96424c80-45fa-11eb-8f9d-15d5aaa574af.jpg)](https://user-images.githubusercontent.com/12952180/103081850-a35f3b80-45fa-11eb-907b-b414295f7e97.jpg)
(click for hi-res)

## Features
* Basic forward rendering: no depth prepass (yet), no alpha blending (yet), no animations, etc.
* Anti-aliased alpha masking using *Alpha to Coverage* technique
* Directional fog
* sRGB textures
* GLTF model loading 
* HDR with Uncharted 2 tonemapping
* Physically-based rendering with image-based lighting
* Normal mapping
* Directional light shadows
* Spot light shadows
* Omnidirectional point light shadows
* Bloom effect
* Using modern OpenGL 4.5 API
* Simple FPS controls and GUI using ImGui library

## Features (todo)
* Skeletal animation
* Mesh LOD
* Precomputed GI lighting
* Procedural sky/sun
* DOF effect
* Animated textures
* Water reflection/refraction
