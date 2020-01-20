
# Phantasm Renderer Backend

PRB is an abstraction over Vulkan and D3D12, aiming to be succinct without restricting access to the underlying APIs within reason.

Goals:

- Small API surface
- High performance
- Free threadedness
- Explicit lifetimes
- Sane defaults
- Low boilerplate

## Objects

The core of PRB is the backend. This is the only type in the library with methods. Everything happening with regards to the real graphics API is instructed through the backend. The two backends  (`BackendD3D12` and `BackendVulkan`) share a virtual interface and can be used interchangably.

PRB has four main objects, accessed using lightweight, POD handles.

1. `handle::resource`
    A texture, render target, or buffer of any form

2. `handle::pipeline_state`
    Shaders, their input shape, and various pipeline configuration options form a pipeline_state. Graphics, compute, or raytracing.

3. `handle::shader_view`
    Shader views are a combination of resource views, for access in a shader.

4. `handle::command_list`
    Represents a recorded command list ready for GPU submission. Only valid between `recordCommandList` and the first of either `submit` or `discard`.

### Commands

Commands do the heavy lifting in communicating with a PRB backend. They are simple structs, which are written contiguously into a piece of memory ("software command buffer"). This memory is then passed to the backend,

```C++
/// create a command list handle from a software command buffer
handle::command_list recordCommandList(std::byte* buffer, size_t size);
```

creating a `handle::command_list`. Command list recording is CPU-only, and entirely free-threaded. The received handle is eventually submitted to the GPU using `submit`.

Commands live in the `cmd` namespace, found in `commands.hh`. For easy command buffer writing, `command_stream_writer` is provided in the same header.

Commands are almost entirely stateless. The only state is the currently active render pass, marked by `cmd::begin_render_pass` and `cmd::end_render_pass` respectively. Other commands like `cmd::draw`, or `cmd::dispatch` (compute) contain all of the state they require, including their `handle::pipeline_state`.

### Shader Arguments

PRB is opinionated with regards to shader inputs. There are four input types, following D3D12 conventions:

1. CBVs - Constant Buffer Views
    A read-only buffer of limited size. HLSL `b` register.

    ```HLSL
    struct camera_constants
    {
        float4x4 view_proj;
        float3 cam_pos;
    };

    ConstantBuffer<camera_constants> g_frame_data       : register(b0, space0);
    ```

2. SRVs - Shader Resource Views
    A read-only texture, or a large, strided buffer. HLSL `t` register.

    ```HLSL
    StructuredBuffer<float4x4> g_model_matrices         : register(t0, space0);
    Texture2D g_albedo                                  : register(t1, space0);
    Texture2D g_normal                                  : register(t2, space0);
    ```

3. UAVs - Unordered Access Views
    A read-write texture or buffer. HLSL `u` register.

    ```HLSL
    RWTexture2DArray<float4> g_output                   : register(u0, space0);
    ```

4. Samplers
    State regarding the way to sample textures. HLSL `s` register.

    ```HLSL
    SamplerState g_sampler                              : register(s0, space0);
    ```

Shaders in PRB can be fed with 0 to 4 "shader arguments". A shader argument consists of a `handle::shader_view`, a CBV, and an offset into the CBV memory. Shader arguments correspond to HLSL spaces. Argument 0 is space0, 1 is space1, and so forth.

Inputs are not strictly typed, for example, a `Texture2D` can be supplied by simply "viewing" a single array slice of a texture array. Details regarding this process can be inferred from the creation of `handle::shader_view`, and the `shader_view_element` type.

Shader view "shapes", as in the amount of arguments, and the amount of inputs per argument, are supplied when creating a `handle::pipeline_state`. The actual argument values are supplied in commands, like `cmd::draw`.

## Threading

With one exception, the PRB backend is entirely free-threaded and internally synchronized. The internal synchronization is minimal, and parallel recording of command lists is encouraged. The exception is the `backend::submit` method, which must only be called on one thread at a time.

## Resource States

PRB exposes a simplified version of resource states, especially when compared with Vulkan. Additionally, resource transitions only ever specify the after-state, the before-state is internally tracked (including thread- and record-order safety).

When transitioning towards `shader_resource` or `unordered_access`, the shader stage(s) which will use the resource next must also be specified.

## Main Loop

In the steady state, there is very little interaction with the backend apart from `command_list` recording and submission. A prototypical PRB main loop looks like this:

```C++
while (window_open)
{
    if (window_resized)
    {
        backend.onResize({w, h});
    }

    if (backend.clearPendingResize())
    {
        // the backend backbuffer has resized,
        // re-create window size dependant resources

        // think of this event as entirely independent from
        // a window resize, do all your resize logic here
    }

    // ... write and record command lists ...

    auto const backbuffer = backend.acquireBackbuffer();

    if (!backbuffer.is_valid())
    {
        // backbuffer acquire has failed, discard this frame
        backend.discard(cmdlists);
        continue;
    }

    // ... write and record final present command list ...
    // ... transition backbuffer to resource_state::present ...

    backend.submit(cmdlists);
    backend.present();
}
```

## Vulkan and D3D12 Specifics

### Shader Compilation

All shaders passed to PRB must be in binary format: DXIL for D3D12, SPIR-V for Vulkan. Shaders are to be compiled from HLSL, using [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler/releases) (find Linux binaries [here](https://github.com/project-arcana/arcana-samples/tree/develop/res/pr/liveness_sample/shader/dxc_bin/linux) or use [docker](https://hub.docker.com/r/gwihlidal/dxc/)).

When compiling HLSL to SPIR-V for Vulkan, use these flags: `-spirv -fspv-target-env=vulkan1.1 -fvk-b-shift 0 all -fvk-t-shift 1000 all -fvk-u-shift 2000 all -fvk-s-shift 3000 all`. If it is a vertex, geometry or domain shader, the `-fvk-invert-y` must also be added.

For Vulkan, the shader `main` function must be named as such: Vertex - `main_vs`, hull - `main_hs`, domain - `main_ds`, geometry - `main_gs`, pixel - `main_ps`, compute - `main_cs`.

### D3D12 MIP alignment

As PRB is a relatively thin layer over the native APIs, memory access to mapped buffers is unchanged from usual behavior. In D3D12, texture MIP pixel rows are aligned by 256 bytes, which must be respected. See arcana-samples for texture upload examples.

### D3D12 Relaxed API

Some parts of the API require less care when using D3D12:

1. Resource state transitions can omit shader depdencies.
2. `Backend::flushMappedMemory` does nothing and can be ignored.
3. `Backend::acquireBackbuffer` will never fail.

Some other fields of `cmd` structs might also be optional, which will be noted in comments.
