#include "pch.h"
#include "DXGICapture.h"

using namespace Capture;

HRESULT DXGICapture::Init(
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device)
{
	HRESULT hr = S_OK;

	hr = CreateDXGIFactory1(winrt::guid_of<IDXGIFactory1>(), m_spDXGIFactory1.put_void());
	if (FAILED(hr))
	{
		auto err = GetLastError();
		return hr;
	}

	//Getting all adapters
	winrt::com_ptr<IDXGIAdapter1> spAdapter;
	for (int i = 0; m_spDXGIFactory1->EnumAdapters1(i, spAdapter.put()) != DXGI_ERROR_NOT_FOUND; i++)
	{
		m_vAdapters.push_back(spAdapter);
	}

	//Iterating over all adapters to get all outputs
	for (std::vector<winrt::com_ptr<IDXGIAdapter1>>::iterator AdapterIter = m_vAdapters.begin();
		AdapterIter != m_vAdapters.end();
		AdapterIter++)
	{
		std::vector<winrt::com_ptr<IDXGIOutput>> vOutputs;

		winrt::com_ptr<IDXGIOutput> spDXGIOutput;
		//hr = m_vAdapters[0]->EnumOutputs(0, spDXGIOutput.put());
		//hr = AdapterIter->get()->EnumOutputs(0, spDXGIOutput.put());
		for (int i = 0; (*AdapterIter)->EnumOutputs(i, spDXGIOutput.put()) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			spDXGIOutput->GetDesc(&outputDesc);

			if (outputDesc.AttachedToDesktop)
			{
				vOutputs.push_back(spDXGIOutput);
			}
		}

		if (vOutputs.size() == 0)
			continue;

		for (std::vector<winrt::com_ptr<IDXGIOutput>>::iterator OutputIter = vOutputs.begin();
			OutputIter != vOutputs.end();
			OutputIter++)
		{
			winrt::com_ptr<IDXGIOutput1> spDXGIOutput1;
			hr = (*OutputIter)->QueryInterface(winrt::guid_of<IDXGIOutput1>(), spDXGIOutput1.put_void());
			if (!spDXGIOutput1)
			{
				continue;
			}

			winrt::com_ptr<IDXGIDevice1> spDXGIDevice;
			hr = m_d3dDevice->QueryInterface(winrt::guid_of<IDXGIDevice1>(), spDXGIDevice.put_void());
			if (!spDXGIDevice)
			{
				continue;
			}

			winrt::com_ptr<IDXGIOutputDuplication> spDXGIOutputDuplication;
			hr = spDXGIOutput1->DuplicateOutput(spDXGIDevice.get(), spDXGIOutputDuplication.put());
			if (FAILED(hr))
			{
				continue;
			}

			DXGI_OUTPUT_DESC outputDesc;
			(*OutputIter)->GetDesc(&outputDesc);

			std::shared_ptr<DXGIOutputDuplication> pOutputDuplication = std::make_shared<DXGIOutputDuplication>(spDXGIOutputDuplication, outputDesc);

			m_vOutputs.push_back(
				pOutputDuplication
			);
		}
	}

	m_Init = true;

	return hr;
}

bool Capture::DXGICapture::SetTarget(std::shared_ptr<Util::MonitorInfo>& target)
{
	bool result = false;
	m_OutputDuplication = m_vOutputs[0];

	for (std::vector<std::shared_ptr<DXGIOutputDuplication>>::iterator iter = m_vOutputs.begin();
		iter != m_vOutputs.end();
		iter++)
	{
		std::shared_ptr<DXGIOutputDuplication> output = (*iter);

		if (output->outputDesc.Monitor == target->MonitorHandle)
		{
			m_OutputDuplication = output;
			result = true;
			break;
		}
	}

	auto desc = m_OutputDuplication->outputDesc;
	auto width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
	auto height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

	m_swapChain = Util::CreateDXGISwapChain(m_d3dDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height),
		static_cast<DXGI_FORMAT>(m_pixelFormat), 2);

	return result;
}

void DXGICapture::Close()
{
	m_Init = false;

	if (CaptureThread.joinable()) CaptureThread.join();
}

winrt::Windows::UI::Composition::ICompositionSurface DXGICapture::CreateSurface(winrt::Windows::UI::Composition::Compositor const& compositor)
{
	return Util::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

HRESULT DXGICapture::GetTexture2D(winrt::com_ptr<ID3D11Texture2D>& pTexture2D)
{
	HRESULT hr;


	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	winrt::com_ptr<IDXGIResource> iDXGIResource;
	winrt::com_ptr<ID3D11Texture2D> pTextureResource;
	hr = m_OutputDuplication->OutputDuplication->AcquireNextFrame(1000, &frameInfo, iDXGIResource.put());
	if (FAILED(hr))
	{
		auto err = GetLastError();
		return hr;
	}
	hr = iDXGIResource->QueryInterface(winrt::guid_of<ID3D11Texture2D>(), pTextureResource.put_void());
	if (FAILED(hr))
	{
		auto err = GetLastError();
		return hr;
	}
	
	D3D11_TEXTURE2D_DESC desc;
	pTextureResource->GetDesc(&desc);

	D3D11_TEXTURE2D_DESC texDesc;
	memset(&texDesc, 0x0, sizeof(texDesc));
	texDesc.Width = desc.Width;
	texDesc.Height = desc.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = desc.Format;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	hr = m_d3dDevice->CreateTexture2D(&texDesc, NULL, pTexture2D.put());
	if (FAILED(hr))
		return hr;

	m_d3dContext->CopyResource(pTexture2D.get(), pTextureResource.get());

	m_OutputDuplication->OutputDuplication->ReleaseFrame();

	return hr;
}

void DXGICapture::StartCapture()
{
	if (m_Init == false)
		return;

	CaptureThread = std::thread{ [&] {
		winrt::com_ptr<ID3D11Texture2D> backBuffer;
		winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));

		while (m_Init)
		{
			winrt::com_ptr<ID3D11Texture2D> pDXGISurface;
			auto hr = GetTexture2D(pDXGISurface);
			if (FAILED(hr) || !pDXGISurface)
			{
				Sleep(16);
				continue;
			}
			m_d3dContext->CopyResource(backBuffer.get(), pDXGISurface.get());

			DXGI_PRESENT_PARAMETERS presentParameters{};
			m_swapChain->Present1(1, 0, &presentParameters);

			//Sleep(16);
		}
	}};
}