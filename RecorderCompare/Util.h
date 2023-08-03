#pragma once
#include <Unknwn.h>
#include <DispatcherQueue.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.capture.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

namespace Util
{
    //현재 스레드의 DispatherController 생성
    inline winrt::Windows::System::DispatcherQueueController CreateDispatcherController()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        winrt::Windows::System::DispatcherQueueController controller{ nullptr };
        winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller))));

        return controller;
    }

    struct __declspec(uuid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"))
        IInitializeWithWindow : ::IUnknown
    {
        virtual HRESULT __stdcall Initialize(HWND hwnd) = 0;
    };

    extern "C"
    {
        HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(::IDXGIDevice* dxgiDevice,
            ::IInspectable** graphicsDevice);

        HRESULT __stdcall CreateDirect3D11SurfaceFromDXGISurface(::IDXGISurface* dgxiSurface,
            ::IInspectable** graphicsSurface);
    }

    struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
        IDirect3DDxgiInterfaceAccess : ::IUnknown
    {
        virtual HRESULT __stdcall GetInterface(GUID const& id, void** object) = 0;
    };

    inline auto CreateDirect3DDevice(IDXGIDevice* dxgi_device)
    {
        winrt::com_ptr<::IInspectable> d3d_device;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
        return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    }

    inline auto CreateDirect3DSurface(IDXGISurface* dxgi_surface)
    {
        winrt::com_ptr<::IInspectable> d3d_surface;
        winrt::check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgi_surface, d3d_surface.put()));
        return d3d_surface.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface>();
    }

    template <typename T>
    auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
    {
        auto access = object.as<IDirect3DDxgiInterfaceAccess>();
        winrt::com_ptr<T> result;
        winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
        return result;
    }

    inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device, const DXGI_SWAP_CHAIN_DESC1* desc)
    {
        auto dxgiDevice = device.as<IDXGIDevice2>();
        winrt::com_ptr<IDXGIAdapter> adapter;
        winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
        winrt::com_ptr<IDXGIFactory2> factory;
        winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

        winrt::com_ptr<IDXGISwapChain1> swapchain;
        winrt::check_hresult(factory->CreateSwapChainForComposition(device.get(), desc, nullptr, swapchain.put()));
        return swapchain;
    }

    inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device,
        uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferCount = bufferCount;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

        return CreateDXGISwapChain(device, &desc);
    }

    inline auto CreateCompositionSurfaceForSwapChain(winrt::Windows::UI::Composition::Compositor const& compositor, ::IUnknown* swapChain)
    {
        winrt::Windows::UI::Composition::ICompositionSurface surface{ nullptr };
        auto compositorInterop = compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
        winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> surfaceInterop;
        winrt::check_hresult(compositorInterop->CreateCompositionSurfaceForSwapChain(swapChain, surfaceInterop.put()));
        winrt::check_hresult(surfaceInterop->QueryInterface(winrt::guid_of<winrt::Windows::UI::Composition::ICompositionSurface>(), winrt::put_abi(surface)));
        return surface;
    }

    inline auto CreateD3DDevice(D3D_DRIVER_TYPE const type, UINT flags, winrt::com_ptr<ID3D11Device>& device)
    {
        WINRT_ASSERT(!device);

        return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.put(),
            nullptr, nullptr);
    }

    inline auto CreateD3DDevice(UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT)
    {
        winrt::com_ptr<ID3D11Device> device;
        HRESULT hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, flags, device);
        if (DXGI_ERROR_UNSUPPORTED == hr)
        {
            hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, flags, device);
        }

        winrt::check_hresult(hr);
        return device;
    }

    struct MonitorInfo
    {
        MonitorInfo(HMONITOR monitorHandle)
        {
            MonitorHandle = monitorHandle;
            MONITORINFOEX monitorInfo = { sizeof(monitorInfo) };
            winrt::check_bool(GetMonitorInfo(MonitorHandle, &monitorInfo));
            std::wstring displayName(monitorInfo.szDevice);
            DisplayName = displayName;
        }
        MonitorInfo(HMONITOR monitorHandle, std::wstring const& displayName)
        {
            MonitorHandle = monitorHandle;
            DisplayName = displayName;
        }

        HMONITOR MonitorHandle;
        std::wstring DisplayName;

        bool operator==(const MonitorInfo& monitor) { return MonitorHandle == monitor.MonitorHandle; }
        bool operator!=(const MonitorInfo& monitor) { return !(*this == monitor); }
    };

    inline std::vector<MonitorInfo> EnumerateAllMonitors()
    {
        std::vector<MonitorInfo> monitors;
        EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
            {
                auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(lparam);
                monitors.push_back(MonitorInfo(hmon));

                return TRUE;
            }, reinterpret_cast<LPARAM>(&monitors));
        if (monitors.size() > 1)
        {
            monitors.push_back(MonitorInfo(nullptr, L"All Displays"));
        }
        return monitors;
    }

    inline auto CreateCaptureItemForWindow(HWND hwnd)
    {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }

    inline auto CreateCaptureItemForMonitor(HMONITOR hmon)
    {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForMonitor(hmon, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }
}