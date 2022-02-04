# Kaguya

This is a hobby project using DirectX 12 and DirectX RayTracing (DXR). This project is inspired by Peter Shirley and his ray tracing book series: [In One Weekend, The Next Week, The Rest of Your Life](https://github.com/RayTracing/raytracing.github.io), Alan Wolfe's blog post series on [causual shadertoy path tracing](https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/) and lastly [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys. For those of you who haven't read any of these and would like to explore ray tracing, I highly recommend them!

# Showcase

![0](/Gallery/hyperion_viewport.png?raw=true "hyperion_viewport")
![1](/Gallery/bedroom_viewport.png?raw=true "bedroom_viewport")
![2](/Gallery/classroom_viewport.png?raw=true "classroom_viewport")
![3](/Gallery/livingroom_viewport.png?raw=true "livingroom_viewport")
![4](/Gallery/glass-of-water_viewport.png?raw=true "glass-of-water_viewport")

# Features

## Graphics

- Render graph system
- Progressive stochastic path tracing
- Importance sampling of BSDFs and multiple importance sampling of lights
- Lambertian, Mirror, Glass, and Disney BSDFs
- Point and quad lights
- Real-time path tracing using both DXR 1.0 and DXR 1.1

## D3D12

- Bindless resource
- Asynchronous PSO compilation using coroutines and threadpools
- Utilization of graphics and asynchronous compute queues
- Acceleration structure compaction using modified [RTXMU](https://github.com/NVIDIAGameWorks/RTXMU)

## Misc

- Custom scene serialization using json
- Custom binary format for loading meshes and meshlets
- Asynchronous asset importing using threads
- Asset management using lightweight handles

## World

- Ecs with the following implemented components:
  - Core: Main component that all actor in the scene have, it contains tag that identifies the actor and a transform which controls position, scale, and orientation
  - Camera: controls camera parameters for scene viewing
  - Light: used to illuminate the scene, controls light parameters
    - Supported types:
      - Point
      - Quad
  - Static Mesh: references a mesh in the asset window to be rendered and controls material parameters and submits the referenced mesh to RaytracingAccelerationStructure for path tracing
    - Supported BSDFs:
      - Lambertian
      - Mirror
      - Glass
      - Disney
  - NativeScript: C++ scripting

# Build

- Visual Studio 2019. I am using VS 2022, but 2019 should still work.
- GPU/Driver that supports DXR.
- Windows SDK Version: 10.0.19041.0
- Windows 10 Minimum Version: 20H2. I am on Windows 11 insider build, older version should still work.
- [CMake](https://cmake.org/) Version 3.16 or up. Make sure the option "Add CMake to the system PATH for all users" during installation.
- C++ 20

1. Initialize submodules after you have cloned, obtain assets used by the project from here: [Assets](https://drive.google.com/drive/folders/1ySITDyVAiylAj6iqpWpnyJIxHXsbt4Zj?usp=sharing). use `Setup.bat` to generate visual studio solution (Make sure you have CMake installed and is a environmental variable).

2. When the project is build, all the assets and required dlls will be copied to the directory of the executable. There is a folder containing all the scenes in the asset directory for the showcase, those can be loaded from the context menu of the World window of the application's UI.

Let me know if you have any trouble setting up the project and getting it up and running!

# Future stuff to work on

- GPU driven architecture
- Deferred renderer
- Hybrid renderer (Raster & Raytracing)
- Explore spectral rendering and all kinds of integrators mentioned in PBRT
- Add physx back into World system
- Mesh shading using meshlets
- Vulkan support

# Acknowledgements

- [assimp](https://github.com/assimp/assimp)
- [DirectX Mesh](https://github.com/microsoft/DirectXMesh)
- [DirectX Tex](https://github.com/microsoft/DirectXTex)
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler)
- [entt](https://github.com/skypjack/entt)
- [cityhash](https://github.com/google/cityhash)
- [imgui](https://github.com/ocornut/imgui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [json](https://github.com/nlohmann/json.git)
- [D3D12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)
- [spdlog](https://github.com/gabime/spdlog)
- [wil](https://github.com/microsoft/wil)
- [WinPixEventRuntime](https://devblogs.microsoft.com/pix/winpixeventruntime)

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

- ## Render Graph
  - https://media.contentapi.ea.com/content/dam/ea/seed/presentations/wihlidal-halcyonarchitecture-notes.pdf
  - https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in
  - https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3

- ## BSDF
  - Disney
    - https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
    - http://simon-kallweit.me/rendercompo2015/report/#disneybrdf
- ## Light Sampling
  - Solid angle quadrilateral light sampling: https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf
