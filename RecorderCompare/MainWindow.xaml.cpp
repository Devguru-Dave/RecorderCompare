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
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Microsoft::UI::Xaml::Controls::Primitives;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::RecorderCompare::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    void MainWindow::ButtonClickHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
        winrt::Button button = sender.try_as<winrt::Button>();
        if (button == nullptr) return;

        auto name = button.Name();
        if (name == L"PickerButton")
        {
            button.Content(winrt::box_value(L"Picker"));
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
}
