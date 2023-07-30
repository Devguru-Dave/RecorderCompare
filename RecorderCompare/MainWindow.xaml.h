// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"
#include "WinRTCapture.h"
#include "DXGICapture.h"
#include "MainViewModel.h"

namespace winrt::RecorderCompare::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        //////////////////
        // XAML 핸들러
        void ButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ToggleButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        //////////////////

        //////////////////
        // 초기화
        void GetHWND(HWND& hWnd);
        void InitCompositor(
            winrt::Windows::UI::Composition::Compositor& compositor,
            winrt::Windows::UI::Composition::CompositionSurfaceBrush& brush,
            winrt::Windows::UI::Composition::CompositionSurfaceBrush& brush2,
            winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget& target,
            HWND hWnd
        );
        void Initd3dDevice(
            winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& device,
            winrt::com_ptr<ID3D11Device>& d3dDevice,
            winrt::com_ptr<ID3D11DeviceContext>& d3dContext
        );
        void IInspectableInitialize(IInspectable& item, HWND hWnd);
        //////////////////

        winrt::Windows::Foundation::IAsyncAction GetCaptureItemAsync();
        winrt::fire_and_forget WinRTCaptureStart(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item);
        winrt::fire_and_forget DXGICaptureStart(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item);

        void IsCursorEnabled(bool value);
        winrt::fire_and_forget IsBorderRequired(bool value);

        winrt::RecorderCompare::MainViewModel mainViewModel();

        void StopCapture();

        winrt::Windows::System::DispatcherQueueController m_dispatcherController{ nullptr };
        HWND m_hWnd{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCapturePicker m_graphicsPicker{ nullptr };
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
        winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };
        winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };

        std::shared_ptr<Capture::WinRTCapture> m_winrtCapture{ nullptr };
        std::shared_ptr<Capture::DXGICapture> m_dxgiCapture{ nullptr };

        ///////
        // Compositor
        winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
        winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget m_target{ nullptr };
        winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush{ nullptr };
        winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush2{ nullptr };
        //////

        winrt::RecorderCompare::MainViewModel m_mainViewModel{ nullptr };
    };
}

namespace winrt::RecorderCompare::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
