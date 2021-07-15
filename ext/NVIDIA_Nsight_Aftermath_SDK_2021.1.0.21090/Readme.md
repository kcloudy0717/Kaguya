# Nsight Aftermath SDK

Aftermath is a compact, easy to use C/C++ library aimed at developers of D3D12
or Vulkan based applications on Microsoft Windows or Linux, enabling post-mortem
GPU crash analysis on NVIDIA GeForce based GPUs.

## Key Features

The Nsight Aftermath SDK is an easy to use C/C++ API (shared libraries + header
files), which provides support for three major use cases:

* GPU crash dump creation
* GPU crash dump analysis
* Application instrumentation with light-weight GPU event markers

Nsight Aftermath's GPU crash dump collection performance footprint is low
enough to include in a shipping application - allowing developers to mine
details on why the GPU crashed in the wild, and gather GPU crash dumps for
further analysis (should the required cloud infrastructure already exist).

## Application Instrumentation with Event Markers

In its basic form, this works by allowing programmers to insert markers into
the GPU pipeline, which can be read post-TDR, in order to determine what work
item the GPU was processing at the point of failure. Nsight Aftermath also
includes a facility to query the current device state, much like the
conventional graphics APIs, but reporting a finer grained reason.

One of the key principles of Aftermath is for the marker insertion to be as
unobtrusive as possible. Avoiding situations common with other similar
debuggers, where their associated performance cost changes timing enough to
make a bug repro vanish: a "Heisenbug". Aftermath avoids this problem by
design, while simultaneously not compromising on functionality: a catch-all
solution for post-mortem GPU crash analysis.

For Vulkan the Aftermath event marker functionality is exposed as a Vulkan
extension: `NV_device_diagnostic_checkpoints`.

## GPU Crash Dump Creation

In addition to event marker-based GPU work tracking and device state queries,
Nsight Aftermath also supports the creation of GPU crash dumps for in-depth
post-mortem GPU crash analysis.

Integrating GPU crash dump creation into a graphics application allows
collecting and processing the GPU crash dumps by already established crash
handling workflows and infrastructure.

## GPU Crash Dump Inspection and Analysis

There are two possibilities for a developer to inspect and analyze GPU crash
dumps collected from an application enabled with Nsight Aftermath's GPU crash
dump creation feature:

* Load the GPU crash dump into Nsight Graphics and use the graphical GPU crash
  dump inspector. This option allows for quick and easy access to the data
  stored in the GPU crash dump for visual inspection.
* Use the Nsight Aftermath GPU crash dump decoding functions to
  programmatically access the data stored in the GPU crash dump. This allows
  for automatic processing and binning of GPU crash dumps in a user-defined
  fashion.

## Distribution

The following portions of the SDK are distributable: all .dll / .so files in the
`lib` directory.

NOTE: Redistribution of the files is subject to the terms and conditions listed
in the LICENSE file, including the terms and conditions inherited from any
third-party component used within this product.

## Support

* Microsoft Windows 10 (Version 1809, Version 1903, Version 1909)
* Linux (kernel 4.15.0 or newer)
* D3D12, DXR, D3D11 (basic support), Vulkan
* NVIDIA Turing GPU
* NVIDIA Display Driver R440 or newer (for D3D)
* NVIDIA Display Driver R445 or newer (for Vulkan on Windows)
* NVIDIA Display Driver R455 or newer (for Vulkan on Linux)

# Usage Examples

In this section some code snippets can be found that show how to use the Nsight
Aftermath API for collecting and decoding crash dumps for a D3D12 or Vulkan
application.

The code samples cover the following commonly required tasks:

1. Enable GPU crash dump collection in an application
2. Configure what data to include in GPU crash dumps
3. Instrument an application with Aftermath event markers
4. Handle GPU crash dump callback events
5. Disable GPU crash dump collection
6. Use the GPU crash dump decoding API

## Enabling GPU Crash Dumps

An application enables GPU crash dump creation by calling
`GFSDK_Aftermath_EnableGpuCrashDumps()`. To use the Nsight Aftermath API
functions related to GPU crash dump collection include the
`GFSDK_Aftermath_GpuCrashDump.h` header file.

GPU crash dump collection should be enabled before the application creates any
D3D12, D3D11, or Vulkan device. No GPU crash dumps will be generated for GPU
crashes or hangs related to devices that were created before the
`GFSDK_Aftermath_EnableGpuCrashDumps()` call.

Besides enabling the GPU crash dump feature, this call allows the application
to register a callback function that will be invoked once a GPU crash is
detected. In addition, the application can also provide two optional callback
functions that will be invoked if debug information for shaders is available or
the application intends to provide additional description or context for the
exception, such as the current state of the application at the time of the
crash, to be included with the GPU crash dump.

The following code snippet shows an example of how to enable GPU crash dumps
and how to setup the callbacks for crash dump notifications, for shader debug
information notifications, and for providing additional crash dump description
data. Only the crash dump callback is mandatory. The other two callbacks are
optional and can be omitted by passing a NULL pointer if the corresponding
functionality is not needed. In this example, GPU cash dumps are only enabled
for D3D12 and D3D11 devices. For watching Vulkan devices the
`GFSDK_Aftermath_EnableGpuCrashDumps()` functions must be called with
`GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan`. It is also possible to
combine both flags, if an application uses both the D3D and the Vulkan API.

```C++
    void MyApp::InitDevice()
    {
        [...]

        // Enable GPU crash dumps and register callbacks.
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,   // Default behavior.
            GpuCrashDumpCallback,                               // Register callback for GPU crash dumps.
            ShaderDebugInfoCallback,                            // Register callback for shader debug information.
            CrashDumpDescriptionCallback,                       // Register callback for GPU crash dump description.
            &m_gpuCrashDumpTracker));                           // Set the GpuCrashTracker object as user data passed back by the above callbacks.

        [...]
    }

    // Static wrapper for the GPU crash dump handler. See the 'Handling GPU crash dump Callbacks' section for details.
    void MyApp::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
    }

    // Static wrapper for the shader debug information handler. See the 'Handling Shader Debug Information callbacks' section for details.
    void MyApp::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
    }

    // Static wrapper for the GPU crash dump description handler. See the 'Handling GPU Crash Dump Description Callbacks' section for details.
    void MyApp::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnDescription(addDescription);
    }
```

Enabling GPU crash dumps in an application with
`GFSDK_Aftermath_EnableGpuCrashDumps()` will override any settings from an
already active Nsight Aftermath GPU crash dump monitor for this application.
That means the GPU crash dump monitor will not be notified of any GPU crash
related to this process nor will it create any GPU crash dumps or shader debug
information files for D3D or Vulkan devices that are created after the function
was called. Also, all configuration settings made in the GPU crash dump monitor
will be ignored.

## Configuring GPU Crash Dumps

Which data will be included in GPU crash dumps is configured by the Aftermath
per-device feature flags. How to configure Aftermath feature flags differs
between D3D and Vulkan.

For D3D, the application must call the appropriate
`GFSDK_Aftermath_DX*_Initialize()` functions to initialize the desired Aftermath
feature flags.

For Vulkan, Aftermath feature flags are configured at logical device creation time
via the `VK_NV_device_diagnostics_config` extension.

The following sample code shows how to use `GFSDK_Aftermath_DX12_Initialize()`
to enable the following features for a D3D12 device:

* _Aftermath event markers_ - this will include information about the Aftermath
  event marker nearest to the crash. Using event markers should be considered
  carefully. Injecting markers in high-frequency code paths can introduce hight
  CPU overhead.
* _Resource tracking_ - this will include additional information about the
  resource related to a GPU virtual address seen in the case of a crash due to
  a GPU page fault. This includes, for example, information about the size of
  the resource, its format, and the current deletion status of the resource
  object.
* _Call stack capturing_ - this will include call stack and module information
  for the draw call, compute dispatch, or resource copy nearest to the crash.
  Using this option should be considered carefully. Enabling call stack
  capturing can cause considerable CPU overhead.
* _Generating shader debug information_ - this instructs the shader compiler to
  generate debug information (line tables) for all shaders. Using this option
  should be considered carefully. It may cause considerable shader compilation
  overhead and additional overhead for handling the corresponding shader debug
  information callbacks.

``` C++
    void MyApp::InitDevice()
    {
        [...]

        D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

        // Initialize Nsight Aftermath for this device.
        const uint32_t aftermathFlags =
            GFSDK_Aftermath_FeatureFlags_EnableMarkers |           // Enable event marker tracking.
            GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |  // Enable tracking of resources.
            GFSDK_Aftermath_FeatureFlags_CallStackCapturing |      // Capture call stacks for all draw calls, compute dispatches, and resource copies.
            GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;  // Generate debug information for shaders.

        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_Initialize(
            GFSDK_Aftermath_Version_API,
            aftermathFlags,
            m_device.Get()));

        [...]
    }
```

The same kind of Aftermath feature selection for a Vulkan device could look like
this:

``` C++
    void MyApp::InitDevice()
    {
        std::vector<char const*> extensionNames;

        [...]

        // Enable NV_device_diagnostic_checkpoints extension to be able to
        // use Aftermath event markers.
        extensionNames.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);

        // Enable NV_device_diagnostics_config extension to configure Aftermath
        // features.
        extensionNames.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

        // Set up device creation info for Aftermath feature flag configuration.
        VkDeviceDiagnosticsConfigFlagsNV aftermathFlags =
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |      // Enable tracking of resources.
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |  // Capture call stacks for all draw calls, compute dispatches, and resource copies.
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV;       // Generate debug information for shaders.
        VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
        aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        aftermathInfo.flags = aftermathFlags;

        // Set up device creation info.
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceInfo.pNext = &aftermath_info;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = extensionNames.size();
        deviceInfo.ppEnabledExtensionNames = extensionNames.data();

        // Create the logical device.
        VkDevice device;
        vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);

        [...]
    }
```

## Inserting Event Markers

The Aftermath API provides a simple and light-weight solution for inserting
event markers on the GPU timeline.

Here is some D3D12 example code that shows how to create the necessary command
context handle with `GFSDK_Aftermath_DX12_CreateContextHandle()` and how to
call `GFSDK_Aftermath_SetEventMarker()` to set a simple event marker with a
character string as payload.

```C++
    void MyApp::PopulateCommandList()
    {
        // Create the command list.
        m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

        // Create an Nsight Aftermath context handle for setting Aftermath event markers in this command list.
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_CreateContextHandle(m_commandList.Get(), &m_hAftermathCommandListContext));

        [...]

        // Add an Aftermath event marker with a 0-terminated string as payload.
        std::string eventMarker = "Draw Triangle";
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(m_hAftermathCommandListContext, (void*)eventMarker.c_str(), (unsigned int)eventMarker.size() + 1));
        m_commandList->DrawInstanced(3, 1, 0, 0);

        [...]
    }
```

For reduced CPU overhead, use `GFSDK_Aftermath_SetEventMarker()` with
`dataSize=0`. This instructs Aftermath not to allocate and copy off memory
internally, relying on the application to manage marker pointers itself.

For Vulkan, similar functionality is provided via the
`NV_device_diagnostic_checkpoints` extension. When this extension is enabled for
a Vulkan device event markers can be inserted in a command buffer with the
`vkCmdSetCheckpointNV()` function.

```C++
    void MyApp::RecordCommandBuffer()
    {
        [...]

        // Add an Aftermath event marker pointing to a null-terminated string.
        vkCmdSetCheckpointNV(commandBuffer, "Draw Cube");

        [...]
    }
```

## Handling GPU Crash Dump Callbacks

When Nsight Aftermath GPU crash dumps are enabled, and a GPU crash or hang is
detected, the necessary data is gathered and a GPU crash dump is created from
it. Then the GPU crash dump callback function that was registered with the
`GFSDK_Aftermath_EnableGpuCrashDumps()` will be invoked to notify the
application. In the callback the application can either decode the GPU crash
dump data using the GPU crash dump decoding API or forward it to the crash
handling infrastructure.

In the simple example implementation of a GPU crash dump callback handler is
shown below the GPU crash dump data is simply stored to a file. This file could
then be opened for analysis with Nsight Graphics.

```C++
    // Handler for GPU crash dump callbacks (called by GpuCrashDumpCallback).
    void GpuCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
    {
        // Make sure only one thread at a time...
        std::lock_guard<std::mutex> lock(m_mutex);

        // Write to file for later in-depth analysis.
        WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
    }
```

Note that all callback functions are free-threaded, and that the application is
responsible for providing thread-safe callback handlers.

It is also important to note that `DXGI_ERROR` error notification is asynchronous
to the NVIDIA display driver's GPU crash handling. That means applications
should check the return value of IDXGISwapChain::Present() and give the Nsight
Aftermath GPU crash dump processing thread some time to do its work before
releasing the D3D device or terminating the process.

## Handling GPU Crash Dump Description Callbacks

An application can register an optional callback function that allows it to
provide supplemental information about a crash. This callback is called after
the GPU crash happened, but before the actual GPU crash dump callback. This
presents the opportunity for the application to provide information such as
application name, application version, or user defined data, for example,
current engine state. The data provided will be stored in the GPU crash dump.

Here is an example of a basic GPU crash dump description handler. Data is added
to the crash dump by calling the `addDescription()` function provided by the
callback.

```C++
    // Handler for GPU crash dump description callbacks (called by CrashDumpDescriptionCallback).
    void GpuCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
    {
        // Add some basic description about the crash.
        addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "Hello Nsight Aftermath");
        addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
        addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "This is a GPU crash dump example");
        addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 1, "Engine State: Rendering");
    }
```

Note that all callback functions are free-threaded; the application is
responsible for providing thread-safe callback handlers.

## Handling Shader Debug Information Callbacks

If the device was configured with the `GenerateShaderDebugInfo` feature flag,
the generated shader debug information will be communicated to the application
through the (optional) shader debug information callback function that was
registered when the `GFSDK_Aftermath_EnableGpuCrashDumps()` was called. This debug
information will be required to map from shader instruction addresses to
intermediate assembly language (IL) instructions or high-level source lines when
analyzing a crash dump in Nsight Graphics or when using the GPU crash dump to
JSON decoding functions of the Nsight Aftermath API. If this functionality is
not required, an application can omit the `GenerateShaderDebugInfo` flag when
configuring the device and pass `nullptr` for the shader debug information
callback. This might be desirable, because generating shader debug information
incurs overhead in shader compilation and for handling the callback.

Here is a simple example implementation of a callback handler that writes
the data to disk using the unique shader debug info identifier queried from the
opaque shader debug information blob.

```C++
// Handler for shader debug information callbacks (called by ShaderDebugInfoCallback)
void GpuCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
    // Make sure only one thread at a time...
    std::lock_guard<std::mutex> lock(m_mutex);

    // Get shader debug information identifier.
    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, pShaderDebugInfo, shaderDebugInfoSize, &identifier));

    // Write to file for later in-depth analysis of crash dumps with Nsight Graphics.
    WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}
```

By default, the shader debug information callback will be invoked for every
shader that is compiled by the NVIDIA display driver. It is the responsibility
of the implementation to handle those callbacks and store the data in case a GPU
crash occurs. To simplify the process the Nsight Aftermath library can handle
the caching of the debug information and only invoke the callback in case of a
GPU crash and only for the shaders referenced in the corresponding GPU crash
dump. This behavior is enabled by passing the
`GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks` flag to
`GFSDK_Aftermath_EnableGpuCrashDumps()` when enabling GPU crash dumps.

Note that all callback functions are free-threaded; the application is
responsible for providing thread-safe callback handlers.

## Disable GPU Crash Dumps

To disable GPU crash dumps simply call `GFSDK_Aftermath_DisableGpuCrashDumps()`.

```C++
    MyApp::~MyApp()
    {
        [...]

        // Disable GPU crash dump creation.
        GFSDK_Aftermath_DisableGpuCrashDumps();

        [...]
    }
```

Disabling GPU crash dumps in an application with
`GFSDK_Aftermath_DisableGpuCrashDumps()` will re-enable any Nsight Aftermath GPU
crash dump monitor settings for this application, if it is running on the
system. After this, the GPU crash dump monitor will be notified of any GPU crash
related to this process and will create GPU crash dumps and shader debug
information files for all active devices.

## Reading GPU Crash Dumps

The Nsight Aftermath library provides several functions for decoding GPU crash
dumps and for querying data from the crash dumps. To use these functions
include the `GFSD_Aftermath_GpuCrashDumpDecoding.h` header file.

The first step in decoding a GPU crash dump is to create a decoder object for
it by calling `GFSDK_Aftermath_GpuCrashDump_CreateDecoder()`:

```C++
    // Create a GPU crash dump decoder object for the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
        GFSDK_Aftermath_Version_API,
        pGpuCrashDump,
        gpuCrashDumpSize,
        &decoder));
```
Then one or more decoder functions can be used to read data from a GPU crash
dump. The data in the GPU crash dumps varies depending on the type of GPU crash
and the feature flags used when initializing Aftermath. For example, if the
application does not use Aftermath event markers, querying Aftermath event
marker information will fail. The decoder will return
`GFSDK_Aftermath_Result_NotAvailable` if the requested data is not available.
An implementation should be aware of that and handle the situation accordingly.

Here is an example of how to query GPU page fault information from a GPU crash
dump using a previously created decoder object:

```C++
    // Query GPU page fault information.
    GFSDK_Aftermath_GpuCrashDump_PageFaultInfo pageFaultInfo = {};
    GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo(decoder, &pageFaultInfo);

    if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
    {
        // Print information about the GPU page fault.
        Utility::Printf("GPU page fault at 0x%016llx", pageFaultInfo.faultingGpuVA);
        if (pageFaultInfo.bHasResourceInfo)
        {
            Utility::Printf("Fault in resource starting at 0x%016llx", pageFaultInfo.resourceInfo.gpuVa);
            Utility::Printf("Size of resource: (w x h x d x ml) = {%u, %u, %u, %u} = %llu bytes",
                pageFaultInfo.resourceInfo.width,
                pageFaultInfo.resourceInfo.height,
                pageFaultInfo.resourceInfo.depth,
                pageFaultInfo.resourceInfo.mipLevels,
                pageFaultInfo.resourceInfo.size);
            Utility::Printf("Format of resource: %u", pageFaultInfo.resourceInfo.format);
            Utility::Printf("Resource was destroyed: %d", pageFaultInfo.resourceInfo.bWasDestroyed);
        }
    }
```

Some decoding functions require the caller to provide an appropriately sized
buffer for the data they return. Those come with an additional function to
query the size of the buffer. For example, querying all the active shaders at
the time of the GPU crash or hang could look like this:

```C++
    // First query active shaders count.
    uint32_t shaderCount = 0;
    GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount(decoder, &shaderCount);

    if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
    {
        // Allocate buffer for results.
        std::vector<GFSDK_Aftermath_GpuCrashDump_ShaderInfo> shaderInfos(shaderCount);

        // Query active shaders information.
        result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo(decoder, shaderCount, shaderInfos.data());

        if (GFSDK_Aftermath_SUCCEED(result))
        {
            // Print information for each active shader
            for (const GFSDK_Aftermath_GpuCrashDump_ShaderInfo& shaderInfo : shaderInfos)
            {
                Utility::Printf("Active shader: ShaderHash = 0x%016llx ShaderInstance = 0x%016llx Shadertype = %u",
                    shaderInfo.shaderHash,
                    shaderInfo.shaderInstance,
                    shaderInfo.shaderType);
            }
        }
    }
```

Finally, the GPU crash dump decoder also provides functions for converting a GPU
crash dump into JSON format. Here is a code example for creating JSON from a GPU
crash dump including, information about all the shaders active at the time of
the GPU crash or hang. The infomration also includes the corresponding active
shader warps, including their mapping back to intermediate assembly language
(IL) instructions or source, if available. The later requires the caller to also
provide a couple of callback functions the decoder will invoke to query shader
debug information and shader binaries (dxc shader object outputs or SPIR-V
shader files). These are optional and implementations not interested in mapping
shader instruction addresses to IL or source lines can simply pass
`nullptr`. However, if shader instruction mapping is desired, the implementation
needs to ensure that it can provide the necessary information to the decoder.

```C++
    // Flags controlling what to include in the JSON data
    const uint32_t jsonDecoderFlags =
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_INFO |          // Include information about active shaders.
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_WARP_STATE_INFO |      // Include information about active shader warps.
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO;   // Try to map shader instruction addresses to shader lines.

    // Query the size of the required results buffer
    uint32_t jsonSize = 0;
    GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
        decoder,
        jsonDecoderFlags,                                            // The flags controlling what information to include in the JSON.
        GFSDK_Aftermath_GpuCrashDumpFormatterFlags_CONDENSED_OUTPUT, // Generate condensed out, i.e. omit all unnecessary whitespace.
        ShaderDebugInfoLookupCallback,                               // Callback function invoked to find shader debug information data.
        ShaderLookupCallback,                                        // Callback function invoked to find shader binary data by shader hash.
        ShaderInstructionsLookupCallback,                            // Callback function invoked to find shader binary shader data by instructions hash.
        ShaderSourceDebugDataLookupCallback,                         // Callback function invoked to find shader source debug data by shader DebugName.
        &m_gpuCrashDumpTracker,                                      // User data that will be provided to the above callback functions.
        &jsonSize);                                                  // Result of the call: size in bytes of the generated JSON data.

    if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
    {
        // Allocate buffer for results.
        std::vector<char> json(jsonSize);

        // Query the generated JSON data taht si cached inside the decoder object.
        result = GFSDK_Aftermath_GpuCrashDump_GetJSON(
            decoder,
            json.size(),
            json.data());
        if (GFSDK_Aftermath_SUCCEED(result))
        {
            Utility::Printf("JSON: %s", json.data());
        }
    }
```

Possible implementations for the shader debug information and shader binary
lookup callbacks:

```C++

    // Static callback wrapper for OnShaderDebugInfoLookup
    void MyApp::ShaderDebugInfoLookupCallback(
        const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
        PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
        void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnShaderDebugInfoLookup(*pIdentifier, setShaderDebugInfo);
    }

    // Static callback wrapper for OnShaderLookup
    void MyApp::ShaderLookupCallback(
        const GFSDK_Aftermath_ShaderHash* pShaderHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary,
        void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnShaderLookup(*pShaderHash, setShaderBinary);
    }

    // Static callback wrapper for OnShaderInstructionsLookup
    void MyApp::ShaderInstructionsLookupCallback(
        const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary,
        void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnShaderInstructionsLookup(*pShaderInstructionsHash, setShaderBinary);
    }

    // Static callback wrapper for OnShaderSourceDebugInfoLookup
    void MyApp::ShaderSourceDebugInfoLookupCallback(
        const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
        PFN_GFSDK_Aftermath_SetData setShaderBinary,
        void* pUserData)
    {
        GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
        pGpuCrashTracker->OnShaderSourceDebugInfoLookup(*pShaderDebugName, setShaderBinary);
    }

    // Handler for shader debug information lookup callbacks.
    // This is used by the JSON decoder for mapping shader instruction
    // addresses to IL lines or source lines.
    void GpuCrashTracker::OnShaderDebugInfoLookup(
        const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
        PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const
    {
        // Search the list of shader debug information blobs received earlier.
        auto i_debugInfo = m_shaderDebugInfo.find(identifier);
        if (i_debugInfo == m_shaderDebugInfo.end())
        {
            // Early exit, nothing found. No need to call setShaderDebugInfo.
            return;
        }

        // Let the GPU crash dump decoder know about the shader debug information
        // that was found.
        setShaderDebugInfo(i_debugInfo->second.data(), i_debugInfo->second.size());
    }

    // Handler for shader lookup callbacks.
    // This is used by the JSON decoder for mapping shader instruction
    // addresses to IL lines or source lines.
    // NOTE: If the application loads stripped shader binaries, Aftermath
    // will require access to both the stripped and the non-stripped
    // shader binaries.
    void GpuCrashTracker::OnShaderLookup(
        const GFSDK_Aftermath_ShaderHash& shaderHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary) const
    {
        // Find shader binary data for the shader instruction hash in the shader database.
        std::vector<uint8_t> shaderBinary;
        if (!m_shaderDatabase.FindShaderBinary(shaderInstructionsHash, shaderBinary))
        {
            // Early exit, nothing found. No need to call setShaderBinary.
            return;
        }

        // Let the GPU crash dump decoder know about the shader data
        // that was found.
        setShaderBinary(shaderBinary.data(), shaderBinary.size());
    }

    // Handler for shader instructions lookup callbacks (D3D-only).
    // This is used by the JSON decoder for mapping shader instruction
    // addresses to DXIL lines or HLSL source lines.
    // NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
    // Aftermath will require access to both the stripped and the non-stripped
    // shader binaries.
    void GpuCrashTracker::OnShaderInstructionsLookup(
        const GFSDK_Aftermath_ShaderInstructionsHash& shaderInstructionsHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary) const
    {
        // Find shader binary data for the shader instruction hash in the shader database.
        std::vector<uint8_t> shaderBinary;
        if (!m_shaderDatabase.FindShaderBinary(shaderInstructionsHash, shaderBinary))
        {
            // Early exit, nothing found. No need to call setShaderBinary.
            return;
        }

        // Let the GPU crash dump decoder know about the shader data
        // that was found.
        setShaderBinary(shaderBinary.data(), shaderBinary.size());
    }

    // Handler for shader source debug info lookup callbacks.
    // This is used by the JSON decoder for mapping shader instruction addresses to
    // source lines, if the shaders used by the application were compiled with
    // separate debug info data files.
    void GpuCrashTracker::OnShaderSourceDebugInfoLookup(
        const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
        PFN_GFSDK_Aftermath_SetData setShaderBinary) const
    {
        // Find source debug info for the shader DebugName in the shader database.
        std::vector<uint8_t> sourceDebugInfo;
        if (!m_shaderDatabase.FindSourceShaderDebugData(shaderDebugName, sourceDebugInfo))
        {
            // Early exit, nothing found. No need to call setShaderBinary.
            return;
        }

        // Let the GPU crash dump decoder know about the shader debug data that
        // was found.
        setShaderBinary(sourceDebugInfo.data(), sourceDebugInfo.size());
    }

```

Last, the decoder object should be destroyed, if no longer needed, to free up
all memory allocated for it:

```C++
    // Destroy the GPU crash dump decoder object.
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
```

# Shader Compilation for Source Mapping

## D3D12

The following variants of generating source shader debug information for HLSL
shaders are supported:

1. Compile and use full shader blobs

   Compile the shaders with the debug information. Use the full (i.e. not
   stripped) shader binary when running the application and make it accessible
   through `ShaderLookupCallback` and `ShaderInstructionsLookupCallback`. In
   this case there is no need to provide a `ShaderSourceDebugDataLookupCallback`.

   Compilation example:
   ```
        dxc -Zi [..] -Fo shader.bin shader.hlsl
   ```

2. Compile and strip

   Compile the shaders with debug information and then strip off the debug
   information. Use the stripped shader binary data when running the
   application. Make the stripped shader binary data accessible through
   `ShaderLookupCallback` and `ShaderInstructionsLookupCallback`. In addition,
   make the non-stripped shader binary data accessible through
   `ShaderSourceDebugDataLookupCallback`.

   Compilation example:
   ```
        dxc -Zi [..] -Fo full_shader.bin shader.hlsl
        dxc -dumpbin -Qstrip_debug -Fo shader.bin full_shader.bin
   ```

   The shader's DebugName required for implementing the
   ShaderSourceDebugDataLookupCallback may be extracted from the stripped or
   the non-stripped shader binary data with
   `GFSDK_Aftermath_GetShaderDebugName()`.

3. Compile with separate debug information (and auto-generated debug data file name)

   Compile the shaders with debug information and instruct the compiler to store
   the debug meta data in a separate shader debug information file. The name of
   the file generated by the compiler will match the DebugName of the shader.
   Make the shader binary data accessible through `ShaderLookupCallback` and
   `ShaderInstructionsLookupCallback`. In addition, make the data from the
   compiler generated shader debug data file accessible through
   `ShaderSourceDebugDataLookupCallback`.

   Compilation example:
   ```
        dxc -Zi [..] -Fo shader.bin -Fd debugInfo\ shader.hlsl
   ```

   The debug data file generated by the compiler does not contain any reference
   to the shader's DebugName. It is the responsibility of the user providing
   the `ShaderSourceDebugDataLookupCallback` callback to implement a solution to
   lookup the debug data based on the name of the generated debug data file.

4. Compile with separate debug information (and user-defined debug data file name)

   Compile the shaders with debug information and instruct the compiler to
   store the debug meta data in a separate shader debug information file. The
   name of the file is freely choosen by the user. Make the shader binary data
   accessible through `ShaderLookupCallback` and
   `ShaderInstructionsLookupCallback`. In addition, make the data from the
   compiler generated shader debug data file accessible through
   `ShaderSourceDebugDataLookupCallback`.

   Compilation example:
   ```
        dxc -Zi [..] -Fo shader.bin -Fd debugInfo\shader.dbg shader.hlsl
   ```

   The debug data file generated by the compiler does not contain any reference
   to the shader's DebugName. It is the responsibility of the user providing
   the `ShaderSourceDebugDataLookupCallback` callback to implement a solution
   that performs the lookup of the debug data based on a mapping between the
   shader's DebugName the debug data file's name that was chosen for the
   compilation. The shader's DebugName may be extracted from the shader binary
   data with `GFSDK_Aftermath_GetShaderDebugName()`.

## Vulkan (SPIR-V)

For SPIR-V shaders, the Aftermath SDK provides support for the following variants
of generating source shader debug information:

1) Compile and use a full shader blob
   Compile the shaders with the debug information. Use the full (i.e. not
   stripped) shader binary when running the application and make it accessible
   through `ShaderLookupCallback`. In this case there is no need to provide
   ShaderInstructionsLookupCallback` or ShaderSourceDebugInfoLookupCallback`.

   Compilation example using the Vulkan SDK tool-chain:
   ```
        glslangValidator -V -g -o ./full/shader.spv shader.vert
   ```

2) Compile and strip
   Compile the shaders with debug information and then strip off the debug
   information. Use the stripped shader binary data when running the application.
   Make the stripped shader binary data accessible through shaderLookupCb.
   In addition, make the non-stripped shader binary data accessible through
   `ShaderSourceDebugInfoLookupCallback`.

   Compilation example using the Vulkan SDK tool-chain:
   ```
        glslangValidator -V -g -o ./full/shader.spv shader.vert
        spirv-remap --map all --strip-all --input full/shader.spv --output ./stripped/
   ````

   The (decoder) application then needs to pass the contents of the
   `full/shader.spv` and `stripped/shader.spv` pair to
   `GFSDK_Aftermath_GetDebugNameSpirv()` to generate the shader DebugName to
   use with `ShaderSourceDebugInfoLookupCallback`.

# Limitations and Known Issues

* Nsight Aftermath covers only GPU crashes. CPU crashes in the NVIDIA display
  driver, the D3D runtime, the Vulkan loader, or the application cannot be
  captured.
* Nsight Aftermath is only fully supported on Turing or later GPUs.

## D3D

* Nsight Aftermath is only fully supported for D3D12 devices. Only basic support
  with a reduced feature set (no resource tracking and no shader address
  mapping) is available for D3D11 devcies.
* Nsight Aftermath is fully supported on Windows 10, with limited support on
  Windows 7.
* Nsight Aftermath event markers and resource tracking is incompatible with the
  D3D debug layer and tools using D3D API interception, such as Microsoft PIX
  or Nsight Graphics.
* Shader line mappings for active warps are only supported for DXIL shaders,
  i.e. Shader Model 6 or above.
* Due to a known driver bug the captured GPU state may be incomplete for
  drivers < R440. This can affect the capturing of shader instruction addresses
  for active warps.

## Vulkan

* On Linux, due to a driver limitation the device status reported by
  `GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo` is always
  `GFSDK_Aftermath_Device_Status_Unknown`. This will be fixed in an upcoming
  Linux display driver release.

# Copyright and Licenses

See LICENSE file.
