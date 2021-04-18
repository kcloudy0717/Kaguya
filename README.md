# Kaguya

This is a hobby project using DirectX 12 and DirectX RayTracing (DXR). This project is inspired by Peter Shirley and his ray tracing book series: [In One Weekend, The Next Week, The Rest of Your Life](https://github.com/RayTracing/raytracing.github.io), Alan Wolfe's blog post series on [causual shadertoy path tracing](https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/) and lastly [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys. For those of you who haven't read any of these and would like to explore ray tracing, I highly recommend them!

# Features

- Progressive stochastic path tracing
- Bindless resource
- Multi-threaded rendering
- Utilization of graphics and asynchronous compute queues
- Lambertian, Mirror, Glass, and Disney BSDFs
- Point and quad lights
- Entity component system with the following implemented components:
  - Tag: used to identify a game object
  - Transform: controls position, scale, orientation of GameObject
  - Mesh Filter: references a mesh in the asset window to be rendered
  - Mesh Renderer: controls material parameters and submits the referenced mesh to RaytracingAccelerationStructure for path tracing
    - Supported BSDFs:
      - Lambertian
      - Mirror
      - Glass
      - Disney
  - Light: used to illuminate the scene, controls light parameters
    - Supported types:
      - Point
      - Quad
- Custom scene parser using yaml
- Asynchronous resource loading
- Importance sampling of BSDFs and multiple importance sampling of lights

# Goals

- Implement spectral path tracing
- Implement volumetric scattering and sub-surface scattering
- Implement anti-aliasing techniques
- Implement more materials
- Implement more light types (spot, directional, spherical)
- Implement denoising
- Implement compaction to acceleration structures
- Upgrade bindless resource using SM6.6 (Reduces root parameter, potentially minor performance increase)
- Add environment lights ([Portal-masked environment map sampling](https://cs.dartmouth.edu/wjarosz/publications/bitterli15portal.html)) and geometry area lights ([Spherical triangle sampling](https://www.graphics.cornell.edu/pubs/1995/Arv95c.pdf))
- Upgrade from DXR 1.0 to DXR 1.1 (inline raytracing)

# Build

- Visual Studio 2019
- GPU that supports DXR
- Windows SDK Version 10.0.19041.0
- Windows 10 Version: 20H2
- CMake Version 3.15
- C++ 20

1. Initialize submodules after you have cloned, use CMake GUI to configure and generate visual studio solution. (I ran [bfg-repo cleaner](https://rtyley.github.io/bfg-repo-cleaner/) on this repo because I was comitting all my assets, the repo is way smaller now with the downside of unable to see old commit file changes)

2. Download assets from [Releases](https://github.com/KaiH0717/Kaguya/releases/tag/v1.0) and extract the contents into a directory named assets in the root directory of the repo. (There's a build event that'll copy contents from asset to the executable directory)

3. When the project is build, all the assets and required dlls will be copied to the directory of the executable. There is a scene folder in the root directory of the project containing all the scenes for the showcase, those can be loaded from the context menu of the Hierarchy window of the application's UI.

# Acknowledgements

- [D3D12MemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator)
- [EnTT](https://github.com/skypjack/entt)
- [imgui](https://github.com/ocornut/imgui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [spdlog](https://github.com/gabime/spdlog)
- [wil](https://github.com/microsoft/wil)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [assimp](https://github.com/assimp/assimp)
- [DirectXTex](https://github.com/microsoft/DirectXTex)
- [DirectXTK12](https://github.com/microsoft/DirectXTK12)
- [dxc](https://github.com/microsoft/DirectXShaderCompiler)
- [nativefiledialog](https://github.com/mlabbe/nativefiledialog)
- [WinPixEventRuntime](https://devblogs.microsoft.com/pix/winpixeventruntime)

Thanks to Benedikt Bitterli for [rendering resources](https://benedikt-bitterli.me/resources/)!

# References

- 3D Game Programming with DirectX 12 Book by Frank D Luna
- [Direct3D 12 programming guide from MSDN](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys.
- [Ray Tracing Gems: High-Quality and Real-Time Rendering with DXR and Other APIs](http://www.realtimerendering.com/raytracinggems/)
- [Ray Tracing book series (In One Weekend, The Next Week, The Rest of Your Life)](https://github.com/RayTracing/raytracing.github.io) by Peter Shirley
- Real-Time Rendering, Fourth Edition by Eric Haines, Naty Hoffman, and Tomas Möller
- [Casual Shadertoy Path Tracing series](https://blog.demofox.org/) by Alan Wolfe

# Showcase

![0](/Gallery/hyperion_viewport.png?raw=true "hyperion_viewport")
![1](/Gallery/bedroom_viewport.png?raw=true "bedroom_viewport")
![2](/Gallery/classroom_viewport.png?raw=true "classroom_viewport")
![3](/Gallery/livingroom_viewport.png?raw=true "livingroom_viewport")
![4](/Gallery/glass-of-water_viewport.png?raw=true "glass-of-water_viewport")
