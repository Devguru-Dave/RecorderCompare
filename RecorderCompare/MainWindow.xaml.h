// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

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
            winrt::Windows::UI::Composition::ContainerVisual& root,
            winrt::Windows::UI::Composition::SpriteVisual& content,
            winrt::Windows::UI::Composition::CompositionSurfaceBrush& brush,
            winrt::Windows::UI::Composition::DropShadow& shadow,
            winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget& target,
            HWND hWnd
        );
        void IInspectableInitialize(IInspectable& item, HWND hWnd);
        //////////////////

        winrt::Windows::Foundation::IAsyncAction GetCaptureItemAsync();

        winrt::Windows::System::DispatcherQueueController m_dispatcherController{ nullptr };
        HWND m_hWnd{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCapturePicker m_graphicsPicker{ nullptr };

        ///////
        // Compositor
        winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
        winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget m_target{ nullptr };
        winrt::Windows::UI::Composition::ContainerVisual m_root{ nullptr };
        winrt::Windows::UI::Composition::SpriteVisual m_content{ nullptr };
        winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush{ nullptr };
        winrt::Windows::UI::Composition::DropShadow m_shadow{ nullptr };
        //////
    };
}

namespace winrt::RecorderCompare::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
