module;

#include "core.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include <wrl/module.h>
#include <comdef.h>

#include <directxmath.h>
#include <locale>

#include <directx/d3dx12.h>

#include <d3dcompiler.h>

export module game.graphics;

export import game.intfc;

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Details::ComPtrRef;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;


namespace game
{
    export constexpr size_t FRAME_COUNT = 2;

    export inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            _com_error err(hr);

            std::wstring err_msg = err.ErrorMessage();
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::runtime_error(std::string(err_msg.begin(), err_msg.end()));
        }
    }


    export class RenderSystem
    {
    public:
        void OnInit();
        void OnRender();
        void OnUpdate();
        void OnDestroy();

    private:
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_Scissor;
        ComPtr<IDXGISwapChain4> m_Swapchain;
        ComPtr<ID3D12Device10> m_Device;
        ComPtr<ID3D12Resource> m_RenderTarget[FRAME_COUNT];
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        ComPtr<ID3D12CommandQueue> m_CommandQueue;
        ComPtr<ID3D12RootSignature> m_RootSignature;
        ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        ComPtr<ID3D12PipelineState> m_PipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        uint32_t m_RtvDescriptorSize;


        struct Vertex
        {
            XMFLOAT3 position;
            XMFLOAT4 color;
        };

        ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

        uint32_t m_FrameIndex;
        HANDLE m_FenceEvent;
        ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValue;

        std::shared_ptr<I2DSurface> m_TargetWindow;


        void LoadPipeline()
        {
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
            ThrowIfFailed(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&hardwareAdapter)));
            ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_Device)));

            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

            ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

            DXGI_SWAP_CHAIN_DESC swapchainDesc{};
            swapchainDesc.BufferCount = FRAME_COUNT;
            swapchainDesc.BufferDesc.Width = m_TargetWindow->GetWidth();
            swapchainDesc.BufferDesc.Height = m_TargetWindow->GetHeight();
            swapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
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

                for (UINT n = 0 ; n < FRAME_COUNT ; n++)
                {
                    ThrowIfFailed(m_Swapchain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTarget[n])));
                    m_Device->CreateRenderTargetView(m_RenderTarget[n].Get(), nullptr, rtvHandle);
                    rtvHandle.Offset(1, m_RtvDescriptorSize);
                }
            }

            ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
        }

        void LoadAssets()
        {
            {
                CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
                rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

                ComPtr<ID3DBlob> signature;
                ComPtr<ID3DBlob> error;
                ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
                ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
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

                ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
                ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));


            }
        }

        void PopulateCommandList()
        {
        }

        void WaitForPreviousFrame()
        {
        }
    };

    void RenderSystem::OnInit()
    {
    }

    void RenderSystem::OnRender()
    {
    }

    void RenderSystem::OnUpdate()
    {
    }

    void RenderSystem::OnDestroy()
    {
    }
}
