#include "pch.h"
#include "MainViewModel.h"
#include "MainViewModel.g.cpp"

namespace winrt::RecorderCompare::implementation
{
    MainViewModel::MainViewModel()
    {
        m_MainViewSku = winrt::make<RecorderCompare::implementation::MainViewSku>();
    }

    RecorderCompare::MainViewSku MainViewModel::MainViewSku()
    {
        return m_MainViewSku;
    }
}