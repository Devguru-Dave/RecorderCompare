#pragma once
#include "MainViewModel.h"

namespace Capture
{
	class DXGIOutputDuplication
	{
	public:
		DXGIOutputDuplication(
			winrt::com_ptr<IDXGIOutputDuplication>& pOutputDuplication,
			DXGI_OUTPUT_DESC& outputDesc
		) : OutputDuplication(pOutputDuplication)
		{
			memcpy(&this->outputDesc, &outputDesc, sizeof(outputDesc));
		};

		winrt::com_ptr<IDXGIOutputDuplication> OutputDuplication;
		DXGI_OUTPUT_DESC outputDesc;
	};

	class DXGICapture
	{
	public:
		DXGICapture() :
			m_pixelFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized),
			m_Init(false) {};
		~DXGICapture() { Close(); };

		HRESULT Init(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device);
		bool SetTarget(std::shared_ptr<Util::MonitorInfo>& target);
		void Close();
		winrt::Windows::UI::Composition::ICompositionSurface CreateSurface(winrt::Windows::UI::Composition::Compositor const& compositor);
		void StartCapture();
		void IsDraw(bool value) { m_IsDraw = value; };
		void GetFrameInfo(double& latency, unsigned long long& fps);
		static std::vector<winrt::hstring> GetAdpaterInfo(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device);

	private:
		HRESULT GetTexture2D(winrt::com_ptr<ID3D11Texture2D>& pTexture2D);

		bool m_Init;
		winrt::com_ptr<IDXGIFactory1> m_spDXGIFactory1;
		std::vector<winrt::com_ptr<IDXGIAdapter1>> m_vAdapters;
		std::vector<std::shared_ptr<DXGIOutputDuplication>> m_vOutputs;
		std::shared_ptr<DXGIOutputDuplication> m_OutputDuplication;
		winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
		winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;
		winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };
		winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
		std::thread CaptureThread;

		bool m_IsDraw{ true };

		std::atomic<unsigned long long> frameCount{ 0 };
		LARGE_INTEGER frequency;
		LARGE_INTEGER startTime;
		std::atomic<double> m_latency{ 0 };
	};
}