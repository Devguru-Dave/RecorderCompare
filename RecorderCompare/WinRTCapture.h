#pragma once
#include "MainViewModel.h"

namespace Capture
{
	class WinRTCapture
	{
	public:
		WinRTCapture(
			winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
			winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item
		);
		~WinRTCapture() { Close(); }

		void StartCapture();
		void Close();
		winrt::Windows::UI::Composition::ICompositionSurface CreateSurface(winrt::Windows::UI::Composition::Compositor const& compositor);

		void IsCursorEnabled(bool value) { CheckClosed(); m_session.IsCursorCaptureEnabled(value); }
		void IsBorderRequired(bool value) { CheckClosed(); m_session.IsBorderRequired(value); }

		void IsDraw(bool value) { CheckClosed(); m_IsDraw = value; }
		void GetFrameInfo(double& latency, unsigned long long& fps);

	private:
		void OnFrameArrived(
			winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
			winrt::Windows::Foundation::IInspectable const& args);

		inline void CheckClosed()
		{
			if (m_closed.load() == true)
			{
				throw winrt::hresult_error(RO_E_CLOSED);
			}
		}

		bool TryResizeSwapChain(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame);
		void ResizeSwapChain();

		winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
		winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
		winrt::Windows::Graphics::SizeInt32 m_lastSize;

		winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
		winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
		winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
		winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;

		std::atomic<bool> m_closed = false;
		std::atomic<bool> m_captureNextImage = false;

		bool m_IsDraw{ true };

		std::atomic<unsigned long long> frameCount{ 0 };
		std::atomic<double> m_latency{ 0 };

		LARGE_INTEGER frequency;
		LARGE_INTEGER startTime;
	};
}