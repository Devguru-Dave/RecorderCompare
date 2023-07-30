#include "pch.h"
#include "MainViewSku.h"
#include "MainViewSku.g.cpp"

namespace winrt::RecorderCompare::implementation
{
    MainViewSku::MainViewSku(winrt::hstring const& title) : m_title{ title }
    {
    }

    winrt::hstring MainViewSku::Title()
    {
        return m_title;
    }

    void MainViewSku::Title(winrt::hstring const& value)
    {
        if (m_title != value)
        {
            m_title = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Title" });
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