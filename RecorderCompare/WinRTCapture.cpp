#include "pch.h"
#include "WinRTCapture.h"
#include "MainViewModel.h"
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
    using namespace RecorderCompare;
}

WinRTCapture::WinRTCapture(
    winrt::IDirect3DDevice const& device,
    winrt::GraphicsCaptureItem const& item
)
{
    m_item = item;
    m_lastSize = m_item.Size();
    m_device = device;
    m_pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;

    auto d3dDevice = Util::GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());

	m_swapChain = Util::CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
		static_cast<DXGI_FORMAT>(m_pixelFormat), 2);

	// Creating our frame pool with 'Create' instead of 'CreateFreeThreaded'
	// means that the frame pool's FrameArrived event is called on the thread
	// the frame pool was created on. This also means that the creating thread
	// must have a DispatcherQueue. If you use this method, it's best not to do
	// it on the UI thread. 
    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, m_pixelFormat, 2, m_lastSize);
	m_session = m_framePool.CreateCaptureSession(m_item);
	m_framePool.FrameArrived({ this, &WinRTCapture::OnFrameArrived });
}

void WinRTCapture::StartCapture()
{
    CheckClosed();
	m_session.StartCapture();

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
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

        auto frame = sender.TryGetNextFrame();

        if (m_IsDraw)
        {
            swapChainResizedToFrame = TryResizeSwapChain(frame);

            winrt::com_ptr<ID3D11Texture2D> backBuffer;
            winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
            auto surfaceTexture = Util::GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
            // copy surfaceTexture to backBuffer
            m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());

            DXGI_PRESENT_PARAMETERS presentParameters{};
            m_swapChain->Present1(1, 0, &presentParameters);
        }

        frameCount++;
        LARGE_INTEGER endTime;
        QueryPerformanceCounter(&endTime);
        double elapsedTime = static_cast<double>(endTime.QuadPart - startTime.QuadPart) / static_cast<double>(frequency.QuadPart);
        if (elapsedTime >= 1)
        {
            m_fps.store(frameCount / elapsedTime);
            m_latency.store(1.0 / m_fps);

            frameCount = 0;

            QueryPerformanceCounter(&startTime);
        }

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
