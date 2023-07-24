#pragma once
#include <Unknwn.h>
#include <DispatcherQueue.h>
#include <winrt/Windows.System.h>

namespace Util
{
    //현재 스레드의 DispatherController 생성
    inline winrt::Windows::System::DispatcherQueueController CreateDispatcherController()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_NONE
        };

        winrt::Windows::System::DispatcherQueueController controller{ nullptr };
        winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller))));

        return controller;
    }

    struct __declspec(uuid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"))
        IInitializeWithWindow : ::IUnknown
    {
        virtual HRESULT __stdcall Initialize(HWND hwnd) = 0;
    };
}