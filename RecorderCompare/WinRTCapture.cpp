#include "pch.h"
#include "WinRTCapture.h"
#include "Util.h"

using namespace Capture;

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

WinRTCapture::WinRTCapture(
    winrt::IDirect3DDevice const& device,
    winrt::com_ptr<ID3D11Device>& d3dDevice,
    winrt::com_ptr<ID3D11DeviceContext>& d3dContext,
    winrt::GraphicsCaptureItem const& item
)
{
    m_item = item;
    m_device = device;
    m_d3dContext = d3dContext;
    m_pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    
    m_swapChain = Util::CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_item.Size().Width), static_cast<uint32_t>(m_item.Size().Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 2);

    // Creating our frame pool with 'Create' instead of 'CreateFreeThreaded'
    // means that the frame pool's FrameArrived event is called on the thread
    // the frame pool was created on. This also means that the creating thread
    // must have a DispatcherQueue. If you use this method, it's best not to do
    // it on the UI thread. 
    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, m_pixelFormat, 2, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &WinRTCapture::OnFrameArrived });
}

void WinRTCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}

void WinRTCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        m_swapChain = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

winrt::ICompositionSurface WinRTCapture::CreateSurface(winrt::Compositor const& compositor)
{
    CheckClosed();
    return Util::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

void WinRTCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
    auto swapChainResizedToFrame = false;

    {
        auto frame = sender.TryGetNextFrame();
        swapChainResizedToFrame = TryResizeSwapChain(frame);

        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
        auto surfaceTexture = Util::GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        // copy surfaceTexture to backBuffer
        m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());
    }

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    }
}

bool WinRTCapture::TryResizeSwapChain(winrt::Direct3D11CaptureFrame const& frame)
{
    auto const contentSize = frame.ContentSize();
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        // The thing we have been capturing has changed size, resize the swap chain to match.
        m_lastSize = contentSize;
        ResizeSwapChain();
        return true;
    }
    return false;
}

void WinRTCapture::ResizeSwapChain()
{
    winrt::check_hresult(m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 0));
}
