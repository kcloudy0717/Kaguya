# Kaguya

This is a hobby project using DirectX 12 and DirectX RayTracing (DXR). This project is inspired by Peter Shirley and his ray tracing book series: [In One Weekend, The Next Week, The Rest of Your Life](https://github.com/RayTracing/raytracing.github.io), Alan Wolfe's blog post series on [causual shadertoy path tracing](https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/) and lastly [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys. For those of you who haven't read any of these and would like to explore ray tracing, I highly recommend them!

# Features

- Progressive stochastic path tracing
- Importance sampling of BSDFs and multiple importance sampling of lights
- Lambertian, Mirror, Glass, and Disney BSDFs
- Point and quad lights
- ECS with the following implemented components:
  - Tag: used to identify a game object
  - Transform: controls position, scale, orientation of GameObject
  - Camera: controls camera parameters for scene viewing
  - Light: used to illuminate the scene, controls light parameters
    - Supported types:
      - Point
      - Quad
  - Mesh Filter: references a mesh in the asset window to be rendered
  - Mesh Renderer: controls material parameters and submits the referenced mesh to RaytracingAccelerationStructure for path tracing
    - Supported BSDFs:
      - Lambertian
      - Mirror
      - Glass
      - Disney
  - NativeScript: C++ scripting
  - Collider: Adds collider to the entity to detect collisions with other rigidbodies with a collider
    - Supported Collider:
      - Box Collider
      - Capsule Collider
      - Mesh Collider (WIP)
  - Rigid Body: Adds rigidbody physics to the entity and can be applied forces and velocities
    - Static Rigid Body
    - Dynamic Rigid Body
- Custom scene serialization using json
- Asynchronous resource loading
- Bindless resource
- Utilization of graphics and asynchronous compute queues
- Acceleration structure compaction
- Custom and efficient binary format for streaming meshes and meshlets

# Goals

- Implement wavefront path tracer
- Implement volumetric scattering and sub-surface scattering
- Implement temporal anti-aliasing
- Implement denoising
- Implement DXR 1.1 (inline raytracing) and compare against 1.0

# Work in progress

- Render graph
- GPU driven architecture
- Mesh shading using meshlets

# Build

- Visual Studio 2019
- GPU that supports DXR
- Windows SDK Version 10.0.19041.0
- Windows 10 Version: 20H2
- CMake Version 3.15
- C++ 20

1. Initialize submodules after you have cloned, use CMake GUI to configure and generate visual studio solution. (I ran [bfg-repo cleaner](https://rtyley.github.io/bfg-repo-cleaner/) on this repo because I was committing all my assets, the repo is way smaller now with the downside of unable to see old commit file changes)

2. Download assets from [ProjectAssets](https://github.com/KaiH0717/ProjectAssets/tree/Kaguya) repo and extract the contents into a directory named assets in the root directory of the repo. (There's a build event that'll copy contents from asset to the executable directory)

3. When the project is build, all the assets and required dlls will be copied to the directory of the executable. There is a folder containing all the scenes in the asset directory for the showcase, those can be loaded from the context menu of the Hierarchy window of the application's UI.

Let me know if you have any trouble setting up the project and getting it up and running!

# Acknowledgements

- [FidelityFX Super Resolution 1.0 (FSR)](https://github.com/GPUOpen-Effects/FidelityFX-FSR)
- [D3D12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler)
- [DirectXTex](https://github.com/microsoft/DirectXTex)
- [WinPixEventRuntime](https://devblogs.microsoft.com/pix/winpixeventruntime)
- [EnTT](https://github.com/skypjack/entt)
- [imgui](https://github.com/ocornut/imgui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [spdlog](https://github.com/gabime/spdlog)
- [wil](https://github.com/microsoft/wil)
- [json](https://github.com/nlohmann/json.git)
- [assimp](https://github.com/assimp/assimp)
- [NVIDIA PhysX](https://github.com/NVIDIAGameWorks/PhysX)

Thanks to Benedikt Bitterli for [rendering resources](https://benedikt-bitterli.me/resources/)!

# References

- 3D Game Programming with DirectX 12 Book by Frank D Luna
- [Direct3D 12 programming guide from MSDN](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [DirectX Specs](https://microsoft.github.io/DirectX-Specs/)
- [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys.
- [Ray Tracing Gems: High-Quality and Real-Time Rendering with DXR and Other APIs](http://www.realtimerendering.com/raytracinggems/)
- [Ray Tracing book series (In One Weekend, The Next Week, The Rest of Your Life)](https://github.com/RayTracing/raytracing.github.io) by Peter Shirley
- Real-Time Rendering, Fourth Edition by Eric Haines, Naty Hoffman, and Tomas Möller
- [Casual Shadertoy Path Tracing series](https://blog.demofox.org/) by Alan Wolfe

## Implementation details

- ## BSDF
  - Disney
    - https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
    - http://simon-kallweit.me/rendercompo2015/report/#disneybrdf
- Light Sampling
  - Solid angle quadrilateral light sampling: https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf

# Showcase

![0](/Gallery/hyperion_viewport.png?raw=true "hyperion_viewport")
![1](/Gallery/bedroom_viewport.png?raw=true "bedroom_viewport")
![2](/Gallery/classroom_viewport.png?raw=true "classroom_viewport")
![3](/Gallery/livingroom_viewport.png?raw=true "livingroom_viewport")
![4](/Gallery/glass-of-water_viewport.png?raw=true "glass-of-water_viewport")
