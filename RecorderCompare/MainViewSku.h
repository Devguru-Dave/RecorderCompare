#pragma once
#include "MainViewSku.g.h"

namespace winrt::RecorderCompare::implementation
{
    struct MainViewSku : MainViewSkuT<MainViewSku>
    {
        MainViewSku();

        winrt::hstring LeftLatency();
        winrt::hstring LeftFPS();
        winrt::hstring RightLatency();
        winrt::hstring RightFPS();
        void LeftLatency(winrt::hstring const& value);
        void LeftFPS(winrt::hstring const& value);
        void RightLatency(winrt::hstring const& value);
        void RightFPS(winrt::hstring const& value);
        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(winrt::event_token const& token);

    private:
        winrt::hstring m_LeftLatency;
        winrt::hstring m_LeftFPS;
        winrt::hstring m_RightLatency;
        winrt::hstring m_RightFPS;
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::RecorderCompare::factory_implementation
{
    struct MainViewSku : MainViewSkuT<MainViewSku, implementation::MainViewSku>
    {
    };
}