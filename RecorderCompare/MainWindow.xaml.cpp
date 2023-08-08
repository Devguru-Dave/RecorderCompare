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

using namespace std::chrono_literals;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::RecorderCompare::implementation
{
    MainWindow::MainWindow()
    {
        // App.xaml.g.hpp는 빌드시 자동으로 생성되는 코드로
        // winrt::init_apartment(winrt::apartment_type::single_threaded) 를 수행한다
        // 다만 single_threaded로 apartment를 초기화하여 winrt object들을 스레드끼리 접근이 불가능하다
        // 초기화를 해제하고 multi_threaded로 다시 초기화하면 문제가 없다
        winrt::uninit_apartment();
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        m_dispatcherController = Util::CreateDispatcherController();
        m_dispatcherQueue = m_dispatcherController.DispatcherQueue().GetForCurrentThread();
        GetHWND(m_hWnd);
        InitCompositor(m_compositor, m_brush, m_brush2, m_target, m_hWnd);
        Initd3dDevice(m_device);

        m_graphicsPicker = winrt::GraphicsCapturePicker();
        IInspectableInitialize(m_graphicsPicker, m_hWnd);
        
        m_mainViewModel = winrt::make<MainViewModel>();

        // FrameTime을 출력하는 Dedicate DispatcherQueue 생성
        dedicateDispatcher = Util::CreateDispatcherController(DISPATCHERQUEUE_THREAD_TYPE::DQTYPE_THREAD_DEDICATED);
        dedicateQueue = dedicateDispatcher.DispatcherQueue().GetForCurrentThread();
        dispatcherTimer = dedicateQueue.CreateTimer();
        dispatcherTimer.Interval(1s);
        dispatcherTimer.Tick({ this, &MainWindow::frameInfoWorker });
        dispatcherTimer.Start();

        InitializeComponent();
    }

    void MainWindow::ButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
        winrt::Button button = sender.try_as<winrt::Button>();
        if (button == nullptr) return;

        auto name = button.Name();
        if (name == L"AdapterInfoButton")
        {
            auto adapters = Capture::DXGICapture::GetAdpaterInfo(m_device);
            wchar_t buffer[1024]{ '\0' };
            for (auto const& adapter : adapters)
            {
                lstrcatW(buffer, adapter.c_str());
                lstrcatW(buffer, L"\n");
            }
            winrt::hstring Title = L"Adapter Info";
            winrt::hstring Content = winrt::hstring(buffer);
            
            winrt::Windows::UI::Popups::MessageDialog dialog = winrt::Windows::UI::Popups::MessageDialog(Content, Title);
            IInspectableInitialize(dialog, m_hWnd);
            dialog.ShowAsync();
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
        else if (name == L"DrawToggle")
        {
            if(m_winrtCapture)
                m_winrtCapture->IsDraw(isChecked);
            if(m_dxgiCapture)
                m_dxgiCapture->IsDraw(isChecked);
        }
        else if (name == L"ToggleWinRTCapture")
        {
            if (isChecked)
            {
                GetCaptureItemAsync(0);
                ToggleWinRTCapture().Content(winrt::box_value(L"Stop WinRT Capture"));
            }
            else
            {
                StopCapture(0);
                ToggleWinRTCapture().Content(winrt::box_value(L"Start WinRT Capture"));
            }
        }
        else if (name == L"ToggleDXGICapture")
        {
            if (isChecked)
            {
                GetCaptureItemAsync(1);
                ToggleDXGICapture().Content(winrt::box_value(L"Stop DXGI Capture"));
            }
            else
            {
                StopCapture(1);
                ToggleDXGICapture().Content(winrt::box_value(L"Start DXGI Capture"));
            }
        }
    }

    winrt::RecorderCompare::MainViewModel MainWindow::mainViewModel()
    {
        return m_mainViewModel;
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

        float leftBarSize = 200.0f;
        auto root = compositor.CreateContainerVisual();

        root.RelativeSizeAdjustment({ 1.0f, 1.0f });
        root.Size({ leftBarSize * -1, 0.0f });
        root.Offset({ leftBarSize, 0.0f, 1.0f });

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

    void MainWindow::Initd3dDevice(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device)
    {
        auto d3dDevice = Util::CreateD3DDevice();
        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        device = Util::CreateDirect3DDevice(dxgiDevice.get());
    }

    void RecorderCompare::implementation::MainWindow::Close()
    {
        dispatcherTimer.Stop();
        dedicateDispatcher.ShutdownQueueAsync();
    }

    winrt::IAsyncAction MainWindow::GetCaptureItemAsync(int CaptureType)
    {
        auto item = co_await m_graphicsPicker.PickSingleItemAsync();
        
        if (item)
        {
            co_await m_dispatcherQueue;

            StopCapture(CaptureType);

            DrawToggle().IsEnabled(true);

            switch (CaptureType)
            {
            case 0:
                WinRTCaptureStart(item);
                break;
            case 1:
                DXGICaptureStart(item);
                break;
            default:
                break;
            }
		}
    }

    winrt::fire_and_forget MainWindow::WinRTCaptureStart(winrt::GraphicsCaptureItem const& item)
    {
        m_winrtCapture = std::make_shared<Capture::WinRTCapture>(m_device, item);
        auto surface = m_winrtCapture->CreateSurface(m_compositor);

        IsCursorEnabled(IsMouseCapture().IsChecked().GetBoolean());
        IsBorderRequired(IsBorder().IsChecked().GetBoolean());

        auto isBorderRequiredPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
        IsBorder().IsEnabled(isBorderRequiredPresent);
        IsMouseCapture().IsEnabled(true);
        IsAffinity().IsEnabled(true);

        m_brush.Surface(surface);
        m_winrtCapture->StartCapture();

        co_return;
    }

    winrt::fire_and_forget MainWindow::DXGICaptureStart(winrt::GraphicsCaptureItem const& item)
    {
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

            m_dxgiCapture = std::make_shared<Capture::DXGICapture>();
            m_dxgiCapture->Init(m_device);
            if (m_dxgiCapture->SetTarget(target))
            {
                auto surface2 = m_dxgiCapture->CreateSurface(m_compositor);

                m_brush2.Surface(surface2);

                m_dxgiCapture->StartCapture();
            }
            else
            {
                m_dxgiCapture.reset();
            }
        }

        co_return;
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

    void RecorderCompare::implementation::MainWindow::StopCapture(int type)
    {
        switch (type)
        {
        case 0:
            m_winrtCapture.reset();
            m_brush.Surface(nullptr);
            IsBorder().IsEnabled(false);
            IsMouseCapture().IsEnabled(false);
            break;
        case 1:
            m_dxgiCapture.reset();
            m_brush2.Surface(nullptr);
            break;
        }


    }

    void MainWindow::IInspectableInitialize(IInspectable& item, HWND hWnd)
    {
        auto initializer = item.as<Util::IInitializeWithWindow>();
        check_hresult(initializer->Initialize(hWnd));
    }

    void MainWindow::frameInfoWorker(winrt::DispatcherQueueTimer const& sender, winrt::IInspectable const& args)
    {
        double winrtLatency = 0;
        unsigned long long winrtFPS = 0;
        double dxgiLatency = 0;
        unsigned long long dxgiFPS = 0;
        auto millisec = std::chrono::duration_cast<std::chrono::milliseconds>(sender.Interval()).count() / 1000.0;

        if (m_winrtCapture)
            m_winrtCapture->GetFrameInfo(winrtLatency, winrtFPS);
        if (m_dxgiCapture)
            m_dxgiCapture->GetFrameInfo(dxgiLatency, dxgiFPS);

        wchar_t buffer[10];
        swprintf_s(buffer, L"%.5f", winrtLatency);
        m_mainViewModel.MainViewSku().LeftLatency(winrt::hstring(buffer));
        swprintf_s(buffer, L"%.2f", winrtFPS / millisec);
        m_mainViewModel.MainViewSku().LeftFPS(winrt::hstring(buffer));
        swprintf_s(buffer, L"%.5f", dxgiLatency);
        m_mainViewModel.MainViewSku().RightLatency(winrt::hstring(buffer));
        swprintf_s(buffer, L"%.2f", dxgiFPS / millisec);
        m_mainViewModel.MainViewSku().RightFPS(winrt::hstring(buffer));
    }
}
