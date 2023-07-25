// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "WinRTCapture.h"

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
        InitCompositor(m_compositor, m_root, m_content, m_brush, m_shadow, m_target, m_hWnd);
        Initd3dDevice(m_device);

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
        winrt::ContainerVisual& root,
        winrt::SpriteVisual& content,
        winrt::CompositionSurfaceBrush& brush,
        winrt::DropShadow& shadow,
        winrt::DesktopWindowTarget& target,
        HWND hWnd
    )
    {
        compositor = winrt::Compositor();

        root = compositor.CreateContainerVisual();

        root.RelativeSizeAdjustment({ 1.0f, 1.0f });
        root.Size({ -220.0f, 0.0f });
        root.Offset({ 220.0f, 0.0f, 0.0f });

        content = compositor.CreateSpriteVisual();
        brush = compositor.CreateSurfaceBrush();

        content.AnchorPoint({ 0.5f, 0.5f });
        content.RelativeOffsetAdjustment({ 0.5f, 0.5f, 0 });
        content.RelativeSizeAdjustment({ 1, 1 });
        content.Size({ -80, -80 });
        content.Brush(brush);
        brush.HorizontalAlignmentRatio(0.5f);
        brush.VerticalAlignmentRatio(0.5f);
        brush.Stretch(winrt::CompositionStretch::Uniform);

        shadow = compositor.CreateDropShadow();
        shadow.Mask(brush);
        content.Shadow(shadow);
        root.Children().InsertAtTop(content);

        auto interop = compositor.as<abi::ICompositorDesktopInterop>();
        check_hresult(interop->CreateDesktopWindowTarget(hWnd, true, reinterpret_cast<abi::IDesktopWindowTarget**>(put_abi(target))));
        target.Root(root);
    }

    void MainWindow::Initd3dDevice(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device)
    {
        auto d3dDevice = Util::CreateD3DDevice();
        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        device = Util::CreateDirect3DDevice(dxgiDevice.get());
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::GetCaptureItemAsync()
    {
        auto item = co_await m_graphicsPicker.PickSingleItemAsync();

        if (item)
        {
            co_await m_dispatcherController.DispatcherQueue().GetForCurrentThread();

            m_winrtCapture = std::make_shared<Capture::WinRTCapture>(m_device, item);
            auto surface = m_winrtCapture->CreateSurface(m_compositor);
            m_brush.Surface(surface);

            IsCursorEnabled(IsMouseCapture().IsChecked().GetBoolean());
            IsBorderRequired(IsBorder().IsChecked().GetBoolean());
            winrt::check_bool(SetWindowDisplayAffinity(m_hWnd, IsAffinity().IsChecked().GetBoolean() ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE));

            m_winrtCapture->StartCapture();

            auto isBorderRequiredPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
            IsBorder().IsEnabled(isBorderRequiredPresent);
            IsMouseCapture().IsEnabled(true);
            IsAffinity().IsEnabled(true);
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
        m_brush.Surface(nullptr);

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
