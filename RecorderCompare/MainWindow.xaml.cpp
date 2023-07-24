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
        InitCompositor(m_compositor, m_root, m_content, m_brush, m_shadow, m_target, m_hWnd);

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
            button.Content(winrt::box_value(L"Stop"));
        }
    }

    void MainWindow::ToggleButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        winrt::ToggleButton toggleButton = sender.try_as<winrt::ToggleButton>();
        if (toggleButton == nullptr) return;

        auto name = toggleButton.Name();
        if (name == L"IsMouseCapture")
        {
            toggleButton.Content(winrt::box_value(L"MouseCapture"));
        }
        else if (name == L"IsBorder")
        {
            toggleButton.Content(winrt::box_value(L"Border"));
        }
        else if (name == L"IsAffinity")
        {
            toggleButton.Content(winrt::box_value(L"Affinity"));
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

    winrt::Windows::Foundation::IAsyncAction MainWindow::GetCaptureItemAsync()
    {
        auto item = co_await m_graphicsPicker.PickSingleItemAsync();

        if (item)
        {
            co_await m_dispatcherController.DispatcherQueue().GetForCurrentThread();
        }
    }

    void MainWindow::IInspectableInitialize(IInspectable& item, HWND hWnd)
    {
        auto initializer = item.as<Util::IInitializeWithWindow>();
        check_hresult(initializer->Initialize(hWnd));
    }
}
