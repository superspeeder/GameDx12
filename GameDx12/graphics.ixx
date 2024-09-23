module;

#include "core.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include <wrl/module.h>
#include <comdef.h>

#include <directxmath.h>
#include <locale>

#include <d3dx12.h>

#include <d3dcompiler.h>

export module game.graphics;

export import game.intfc;
export import game.core;

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Details::ComPtrRef;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;


namespace game {
    export constexpr size_t FRAME_COUNT = 2;

    export inline void ThrowIfFailed(HRESULT hr) {
        if (FAILED(hr))
        {
            _com_error err(hr);

            std::wstring err_msg = err.ErrorMessage();
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::runtime_error(std::string(err_msg.begin(), err_msg.end()));
        }
    }


    export class RenderSystem {
        explicit RenderSystem(const std::shared_ptr<IWindow>& window);

    public:
        void OnInit();
        void OnRender();
        void OnUpdate();
        void OnDestroy();

        static std::shared_ptr<RenderSystem> Create(const std::shared_ptr<IWindow>& window) {
            return std::shared_ptr<RenderSystem>(new RenderSystem(window));
        }

    private:
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_Scissor;
        ComPtr<IDXGISwapChain4> m_Swapchain;
        ComPtr<ID3D12Device10> m_Device;
        ComPtr<ID3D12Resource> m_RenderTargets[FRAME_COUNT];
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        ComPtr<ID3D12CommandQueue> m_CommandQueue;
        ComPtr<ID3D12RootSignature> m_RootSignature;
        ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        ComPtr<ID3D12PipelineState> m_PipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        uint32_t m_RtvDescriptorSize;


        struct Vertex {
            XMFLOAT3 position;
            XMFLOAT4 color;
        };

        ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

        uint32_t m_FrameIndex;
        HANDLE m_FenceEvent;
        ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValue;

        std::shared_ptr<IWindow> m_TargetWindow;


        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;
        using time_point = std::chrono::time_point<clock, duration>;

        static inline time_point now() {
            return std::chrono::time_point_cast<duration>(clock::now());
        }

        time_point m_ThisFrame = now();
        time_point m_LastFrame = now() - duration(0.0166667);
        duration m_DeltaTime = m_ThisFrame - m_LastFrame;


        void LoadPipeline() {
#if defined(_DEBUG)
            // Enable the D3D12 debug layer.
            {
                ComPtr<ID3D12Debug> debugController;
                if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                {
                    debugController->EnableDebugLayer();
                }
            }
#endif

            ComPtr<IDXGIFactory7> factory;
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

            ComPtr<IDXGIAdapter4> hardwareAdapter;
            ThrowIfFailed(
                factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                    IID_PPV_ARGS(&hardwareAdapter)));
            ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_Device)));

            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

            ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

            DXGI_SWAP_CHAIN_DESC swapchainDesc{};
            swapchainDesc.BufferCount = FRAME_COUNT;
            swapchainDesc.BufferDesc.Width = m_TargetWindow->GetWidth();
            swapchainDesc.BufferDesc.Height = m_TargetWindow->GetHeight();
            swapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchainDesc.OutputWindow = m_TargetWindow->GetHandle();
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.Windowed = true;

            ComPtr<IDXGISwapChain> swapchain;
            ThrowIfFailed(factory->CreateSwapChain(m_CommandQueue.Get(), &swapchainDesc, &swapchain));
            ThrowIfFailed(swapchain.As(&m_Swapchain));

            ThrowIfFailed(factory->MakeWindowAssociation(m_TargetWindow->GetHandle(), DXGI_MWA_NO_ALT_ENTER));

            m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();

            {
                D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
                rtvHeapDesc.NumDescriptors = FRAME_COUNT;
                rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));

                m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            }

            {
                CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

                for (UINT n = 0; n < FRAME_COUNT; n++)
                {
                    ThrowIfFailed(m_Swapchain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
                    m_Device->CreateRenderTargetView(m_RenderTargets[n].Get(), nullptr, rtvHandle);
                    rtvHandle.Offset(1, m_RtvDescriptorSize);
                }
            }

            ThrowIfFailed(
                m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
        }

        void LoadAssets() {
            {
                CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
                rootSignatureDesc.Init(0, nullptr, 0, nullptr,
                                       D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

                ComPtr<ID3DBlob> signature;
                ComPtr<ID3DBlob> error;
                ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature,
                                                          &error));
                ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(),
                                                            signature->GetBufferSize(),
                                                            IID_PPV_ARGS(&m_RootSignature)));
            }

            {
                ComPtr<ID3DBlob> vertexShader;
                ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
                // Enable better shader debugging with the graphics debugging tools.
                UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
                UINT compileFlags = 0;
#endif
                auto p = GetAssetFullPath(L"shaders.hlsl");
                ThrowIfFailed(D3DCompileFromFile(p.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0,
                                                 &vertexShader, nullptr));
                ThrowIfFailed(D3DCompileFromFile(p.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0,
                                                 &pixelShader, nullptr));

                D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
                {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
                };

                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
                psoDesc.pRootSignature = m_RootSignature.Get();
                psoDesc.VS = {
                    reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize()
                };
                psoDesc.PS = {reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize()};
                psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
                psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;
                ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));
            }

            ThrowIfFailed(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(),
                                                      m_PipelineState.Get(), IID_PPV_ARGS(&m_CommandList)));

            ThrowIfFailed(m_CommandList->Close());

            {
                float aspectRatio = static_cast<float>(m_TargetWindow->GetWidth()) / static_cast<float>(m_TargetWindow->
                    GetHeight());

                Vertex triangleVertices[] =
                {
                    {{-0.25f, -0.25f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                    {{-0.25f, 0.25f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                    {{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                    {{0.25f, 0.25f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
                    //
                    // {{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                    // {{-0.25f, 0.25f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                };

                const UINT VertexBufferSize = sizeof(triangleVertices);

                // Note: using upload heaps to transfer static data like vert buffers is not 
                // recommended. Every time the GPU needs it, the upload heap will be marshalled 
                // over. Please read up on Default Heap usage. An upload heap is used here for 
                // code simplicity and because there are very few verts to actually transfer.
                CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
                auto desc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
                ThrowIfFailed(m_Device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &desc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_VertexBuffer)));

                // Copy the triangle data to the vertex buffer.
                UINT8* pVertexDataBegin;
                CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
                ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
                memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
                m_VertexBuffer->Unmap(0, nullptr);

                // Initialize the vertex buffer view.
                m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
                m_VertexBufferView.StrideInBytes = sizeof(Vertex);
                m_VertexBufferView.SizeInBytes = VertexBufferSize;
            }

            {
                ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
                m_FenceValue = 1;

                // Create an event handle to use for frame synchronization.
                m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                if (m_FenceEvent == nullptr)
                {
                    ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
                }

                // Wait for the command list to execute; we are reusing the same command 
                // list in our main loop but for now, we just want to wait for setup to 
                // complete before continuing.
                WaitForPreviousFrame();
            }
        }

        void PopulateCommandList() {
            ThrowIfFailed(m_CommandAllocator->Reset());

            // However, when ExecuteCommandList() is called on a particular command 
            // list, that command list can then be reset at any time and must be before 
            // re-recording.
            ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_PipelineState.Get()));

            // Set necessary state.
            m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
            m_CommandList->RSSetViewports(1, &m_Viewport);
            m_CommandList->RSSetScissorRects(1, &m_Scissor);

            // Indicate that the back buffer will be used as a render target.
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                                D3D12_RESOURCE_STATE_PRESENT,
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_CommandList->ResourceBarrier(1, &barrier);

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex,
                                                    m_RtvDescriptorSize);
            m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            // Record commands.
            const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
            m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
            m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
            m_CommandList->DrawInstanced(4, 1, 0, 0);

            // Indicate that the back buffer will now be used to present.
            barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
            m_CommandList->ResourceBarrier(1, &barrier);

            ThrowIfFailed(m_CommandList->Close());
        }

        void WaitForPreviousFrame() {
            const UINT64 fence = m_FenceValue;
            ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fence));
            m_FenceValue++;

            // Wait until the previous frame is finished.
            if (m_Fence->GetCompletedValue() < fence)
            {
                ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
                WaitForSingleObject(m_FenceEvent, INFINITE);
            }

            m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
        }

    public:

        void Tick() {
            m_LastFrame = m_ThisFrame;
            m_ThisFrame = now();
            m_DeltaTime = m_ThisFrame - m_LastFrame;
        }

        duration GetDeltaTime() const {
            return m_DeltaTime;
        }

        double GetFPS() const {
            return 1.0 / m_DeltaTime.count();
        }
    };

    void RenderSystem::OnInit() {
        m_Viewport = D3D12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_TargetWindow->GetWidth()),
                                    static_cast<float>(m_TargetWindow->GetHeight()));
        m_Scissor = D3D12_RECT(0, 0, static_cast<long>(m_TargetWindow->GetWidth()),
                               static_cast<long>(m_TargetWindow->GetHeight()));
        m_RtvDescriptorSize = 0;
        m_FrameIndex = 0;

        LoadPipeline();
        LoadAssets();
    }

    void RenderSystem::OnRender() {
        PopulateCommandList();

        ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};
        m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

        ThrowIfFailed(m_Swapchain->Present(0, 0));

        WaitForPreviousFrame();

        Tick();
        std::cout << "FPS: " << GetFPS() << std::endl;
    }

    void RenderSystem::OnUpdate() {
    }

    void RenderSystem::OnDestroy() {
    }

    RenderSystem::RenderSystem(const std::shared_ptr<IWindow>& window) : m_TargetWindow(window) {
        OnInit();
    }
}
