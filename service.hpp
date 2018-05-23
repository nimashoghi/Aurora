#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <windows.h>

using namespace std;

#define SERVICE_INIT(service_name, implementation) \
    static inline unique_ptr<service> service_name; \
 \
    static inline auto __stdcall ctrl_handler(DWORD ctrl_code) -> void \
    { \
        switch (ctrl_code) \
        { \
            case SERVICE_ACCEPT_SHUTDOWN: \
            case SERVICE_CONTROL_STOP: \
                if (!service_name->running) \
                { \
                    return; \
                } \
                service_name->notify_stop(); \
                SetEvent(service_name->stop_event); \
        } \
    } \
 \
    static inline auto __stdcall smain(DWORD argc, char **argv) -> void \
    { \
        /* register control handler*/ \
        assert(service_name->status_handle = RegisterServiceCtrlHandler(#service_name, ctrl_handler)); \
 \
        /* tell service controller that we want to start*/ \
        service_name->notify_start_pending(); \
 \
        /* do any more initialization that's left*/ \
        service_name->initiate(); \
 \
        /* register stop event*/ \
        assert(service_name->stop_event = CreateEvent(nullptr, TRUE, FALSE, nullptr)); \
 \
        /* tell service controller that we have started*/ \
        service_name->notify_start(); \
 \
        /* wait for worker to stop */ \
        service_name->join_thread(); \
        /* if we do exit our worker thread, then we need to notify the service controller that we stopped */ \
        service_name->notify_stop(); \
    } \
 \
    auto main(int argc, char **argv) -> int \
    { \
        this_thread::sleep_for(5s); \
        /* init the service class */ \
        service_name = make_unique<service>( \
            #service_name, \
            [](auto active) implementation); \
 \
        /* register windows callbacks */ \
        SERVICE_TABLE_ENTRY service_table[] = \
        { \
            {service_name->get_name().data(), reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(smain)}, \
            {nullptr, nullptr} \
        }; \
 \
        assert(StartServiceCtrlDispatcher(service_table)); \
 \
        return 0; \
    }

struct service
{
    template<typename Worker>
    service(string name, Worker &&worker):
        name{move(name)},
        worker_thread{
            [worker = forward<Worker>(worker), this]
            {
                while (!this->running)
                {
                    this_thread::sleep_for(100ms);
                }
                worker([this] { return WaitForSingleObject(this->stop_event, 0) != WAIT_OBJECT_0; });
            }}
    {
        worker_thread.detach();
    }

    ~service()
    {
        CloseHandle(stop_event);
    }

    auto get_name() const
    {
        return name;
    }

    auto join_thread()
    {
        worker_thread.join();
    }

    auto initiate()
    {
        ZeroMemory(&status, sizeof(status));
        status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        status.dwWin32ExitCode = NO_ERROR;
        status.dwServiceSpecificExitCode = NO_ERROR;
    }

    auto report_status(DWORD current_state, DWORD controls_accepted = static_cast<DWORD>(-1), DWORD wait_hint = 0)
    {
        status.dwCurrentState = current_state;
        if (controls_accepted != static_cast<DWORD>(-1))
        {
            status.dwControlsAccepted = controls_accepted;
        }
        status.dwWaitHint = wait_hint;
        return SetServiceStatus(status_handle, &status) ? true : false;
    }

    auto notify_start()
    {
        assert(report_status(SERVICE_RUNNING, status.dwControlsAccepted | (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN)));
        running = true;
    }

    auto notify_stop()
    {
        assert(report_status(SERVICE_STOP_PENDING, status.dwControlsAccepted & ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN), 0));
        running = false;
    }

    auto notify_start_pending()
    {
        assert(report_status(SERVICE_START_PENDING, 0));
    }

    atomic<bool> running{false};
    SERVICE_STATUS status{};
    SERVICE_STATUS_HANDLE status_handle{nullptr};
    HANDLE stop_event{nullptr};
private:
    string name;
    thread worker_thread;
};
