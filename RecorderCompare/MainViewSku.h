#pragma once
#include "MainViewSku.g.h"

namespace winrt::RecorderCompare::implementation
{
    struct MainViewSku : MainViewSkuT<MainViewSku>
    {
        MainViewSku() = delete;
        MainViewSku(winrt::hstring const& title);

        winrt::hstring Title();
        void Title(winrt::hstring const& value);
        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(winrt::event_token const& token);

    private:
        winrt::hstring m_title;
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::RecorderCompare::factory_implementation
{
    struct MainViewSku : MainViewSkuT<MainViewSku, implementation::MainViewSku>
    {
    };
}