#include "pch.h"
#include "MainViewSku.h"
#include "MainViewSku.g.cpp"

namespace winrt::RecorderCompare::implementation
{
    MainViewSku::MainViewSku() :
        m_LeftLatency{ L"0" },
        m_LeftFPS{ L"0" },
        m_RightLatency{ L"0" },
        m_RightFPS{ L"0" } {}

    winrt::hstring MainViewSku::LeftLatency()
    {
        return m_LeftLatency;
    }
    winrt::hstring MainViewSku::LeftFPS()
    {
        return m_LeftFPS;
    }
    winrt::hstring MainViewSku::RightLatency()
    {
        return m_RightLatency;
    }
    winrt::hstring MainViewSku::RightFPS()
    {
        return m_RightFPS;
    }


    void MainViewSku::LeftLatency(winrt::hstring const& value)
    {
        if (m_LeftLatency != value)
        {
            m_LeftLatency = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"LeftLatency" });
        }
    }
    void MainViewSku::LeftFPS(winrt::hstring const& value)
    {
        if (m_LeftFPS != value)
        {
            m_LeftFPS = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"LeftFPS" });
        }
    }
    void MainViewSku::RightLatency(winrt::hstring const& value)
    {
        if (m_RightLatency != value)
        {
            m_RightLatency = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"RightLatency" });
        }
    }
    void MainViewSku::RightFPS(winrt::hstring const& value)
    {
        if (m_RightFPS != value)
        {
            m_RightFPS = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"RightFPS" });
        }
    }

    winrt::event_token MainViewSku::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void MainViewSku::PropertyChanged(winrt::event_token const& token)
    {
        m_propertyChanged.remove(token);
    }
}