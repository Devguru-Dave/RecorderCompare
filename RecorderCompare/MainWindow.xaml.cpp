// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt
{
    using namespace winrt;
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Composition::Desktop;
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Microsoft::UI::Xaml::Controls::Primitives;
}

namespace abi
{
    using namespace ABI;
    using namespace Windows::System;
    using namespace Windows::UI::Composition::Desktop;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::RecorderCompare::implementation
{
    MainWindow::MainWindow()
    {
        m_dispatcherController = Util::CreateDispatcherController();
        GetHWND(m_hWnd);
        InitCompositor(m_compositor, m_brush, m_brush2, m_target, m_hWnd);
        Initd3dDevice(m_device, m_d3dDevice, m_d3dContext);

        m_graphicsPicker = winrt::GraphicsCapturePicker();
        IInspectableInitialize(m_graphicsPicker, m_hWnd);

        InitializeComponent();
    }

    void MainWindow::ButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
        winrt::Button button = sender.try_as<winrt::Button>();
        if (button == nullptr) return;

        auto name = button.Name();
        if (name == L"PickerButton")
        {
            GetCaptureItemAsync();
        }
        else if(name == L"StopButton")
        {
            StopCapture();
        }
    }

    void MainWindow::ToggleButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        winrt::ToggleButton toggleButton = sender.try_as<winrt::ToggleButton>();
        if (toggleButton == nullptr) return;

        auto name = toggleButton.Name();
        auto isChecked = toggleButton.IsChecked().GetBoolean();

        if (name == L"IsMouseCapture")
        {
            IsCursorEnabled(isChecked);
        }
        else if (name == L"IsBorder")
        {
            IsBorderRequired(isChecked);
        }
        else if (name == L"IsAffinity")
        {
            winrt::check_bool(SetWindowDisplayAffinity(m_hWnd, isChecked ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE));
        }
    }

    void MainWindow::GetHWND(HWND& hWnd)
    {
        // Retrieve the window handle (HWND) of the current WinUI 3 window.
        auto windowNative{ this->try_as<::IWindowNative>() };
        winrt::check_bool(windowNative);
        windowNative->get_WindowHandle(&hWnd);
    }

    // 시각, 애니메이션을 담당하는 compositor 인스턴스를 초기화
    void MainWindow::InitCompositor(
        winrt::Compositor& compositor,
        winrt::CompositionSurfaceBrush& brush,
        winrt::CompositionSurfaceBrush& brush2,
        winrt::DesktopWindowTarget& target,
        HWND hWnd
    )
    {
        compositor = winrt::Compositor();

        auto root = compositor.CreateContainerVisual();

        root.RelativeSizeAdjustment({ 1.0f, 1.0f });
        root.Size({ 0.0f, 0.0f });
        root.Offset({ 200.0f, 0.0f, 1.0f });

        auto content = compositor.CreateSpriteVisual();
        brush = compositor.CreateSurfaceBrush();

        // 왼쪽 화면
        content.AnchorPoint({ 1.0f, 0.5f });
        content.RelativeOffsetAdjustment({ 0.5f, 0.5f, 0 });
        content.RelativeSizeAdjustment({ 0.5f, 1 });
        content.Size({ 0.0f, 0.0f });
        content.Brush(brush);
        brush.HorizontalAlignmentRatio(0.5f);
        brush.VerticalAlignmentRatio(0.5f);
        brush.Stretch(winrt::CompositionStretch::Uniform);

        auto shadow = compositor.CreateDropShadow();
        shadow.Mask(brush);
        content.Shadow(shadow);
        root.Children().InsertAtTop(content);
        //

        // 오른쪽 화면
        content = compositor.CreateSpriteVisual();
        brush2 = compositor.CreateSurfaceBrush();

        content.AnchorPoint({ 0.0f, 0.5f });
        content.RelativeOffsetAdjustment({ 0.5f, 0.5f, 0 });
        content.RelativeSizeAdjustment({ 0.5f, 1 });
        content.Size({ 0.0f, 0.0f });
        content.Brush(brush2);
        brush2.HorizontalAlignmentRatio(0.5f);
        brush2.VerticalAlignmentRatio(0.5f);
        brush2.Stretch(winrt::CompositionStretch::Uniform);

        shadow = compositor.CreateDropShadow();
        shadow.Mask(brush2);
        content.Shadow(shadow);
        root.Children().InsertAtTop(content);
        //

        auto interop = compositor.as<abi::ICompositorDesktopInterop>();
        check_hresult(interop->CreateDesktopWindowTarget(hWnd, true, reinterpret_cast<abi::IDesktopWindowTarget**>(put_abi(target))));
        target.Root(root);
    }

    void MainWindow::Initd3dDevice(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device,
        winrt::com_ptr<ID3D11Device>& d3dDevice,
        winrt::com_ptr<ID3D11DeviceContext>& d3dContext
        )
    {
        d3dDevice = Util::CreateD3DDevice();
        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        device = Util::CreateDirect3DDevice(dxgiDevice.get());
        d3dDevice->GetImmediateContext(d3dContext.put());
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::GetCaptureItemAsync()
    {
        auto item = co_await m_graphicsPicker.PickSingleItemAsync();

        if (item)
        {
            co_await m_dispatcherController.DispatcherQueue().GetForCurrentThread();

            StopCapture();

            auto monitors = Util::EnumerateAllMonitors();

            if (monitors.size() > 0)
            {
                std::shared_ptr<Util::MonitorInfo> target{ nullptr };

                for (auto monitor = monitors.begin();
                    monitor != monitors.end();
                    monitor++)
                {
                    auto searchItem = Util::CreateCaptureItemForMonitor(monitor->MonitorHandle);
                    if (item.DisplayName() == searchItem.DisplayName())
                    {
                        target = std::make_shared< Util::MonitorInfo>((*monitor));
                        break;
                    }
                }

                if (target == nullptr)
                    target = std::make_shared< Util::MonitorInfo>(monitors[0]);

				m_dxgiCapture = std::make_shared<Capture::DXGICapture>(m_d3dDevice, m_d3dContext);
				m_dxgiCapture->Init(m_device);
				m_dxgiCapture->SetTarget(target);
				auto surface2 = m_dxgiCapture->CreateSurface(m_compositor);

				m_brush2.Surface(surface2);

				m_dxgiCapture->StartCapture();
            }

			m_winrtCapture = std::make_shared<Capture::WinRTCapture>(m_device, m_d3dDevice, m_d3dContext, item);
			auto surface = m_winrtCapture->CreateSurface(m_compositor);

			IsCursorEnabled(IsMouseCapture().IsChecked().GetBoolean());
			IsBorderRequired(IsBorder().IsChecked().GetBoolean());

			auto isBorderRequiredPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
			IsBorder().IsEnabled(isBorderRequiredPresent);
			IsMouseCapture().IsEnabled(true);
			IsAffinity().IsEnabled(true);

			m_brush.Surface(surface);
			m_winrtCapture->StartCapture();
		}
    }

    void MainWindow::IsCursorEnabled(bool value)
    {
        if (m_winrtCapture != nullptr)
        {
            m_winrtCapture->IsCursorEnabled(value);
        }
    }

    winrt::fire_and_forget MainWindow::IsBorderRequired(bool value)
    {
        if (m_winrtCapture != nullptr)
        {
            auto isBorderRequiredPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
            if (isBorderRequiredPresent)
            {
                // Even if the user or system policy denies access, it's
                // still safe to set the IsBorderRequired property. In the
                // event that the policy changes, the property will be honored.
                auto ignored = co_await winrt::GraphicsCaptureAccess::RequestAccessAsync(winrt::GraphicsCaptureAccessKind::Borderless);

                m_winrtCapture->IsBorderRequired(value);
            }
        }
    }

    void RecorderCompare::implementation::MainWindow::StopCapture()
    {
        m_winrtCapture.reset();
        m_dxgiCapture.reset();
        m_brush.Surface(nullptr);
        m_brush2.Surface(nullptr);

        IsBorder().IsEnabled(false);
        IsMouseCapture().IsEnabled(false);
        IsAffinity().IsEnabled(false);
    }

    void MainWindow::IInspectableInitialize(IInspectable& item, HWND hWnd)
    {
        auto initializer = item.as<Util::IInitializeWithWindow>();
        check_hresult(initializer->Initialize(hWnd));
    }
}
