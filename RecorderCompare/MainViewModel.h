#pragma once
#include "MainViewModel.g.h"
#include "MainViewSku.h"

namespace winrt::RecorderCompare::implementation
{
    struct MainViewModel : MainViewModelT<MainViewModel>
    {
        MainViewModel();

        RecorderCompare::MainViewSku MainViewSku();

    private:
        RecorderCompare::MainViewSku m_MainViewSku{ nullptr };
    };
}
namespace winrt::RecorderCompare::factory_implementation
{
    struct MainViewModel : MainViewModelT<MainViewModel, implementation::MainViewModel>
    {
    };
}