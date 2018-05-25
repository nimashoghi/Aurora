#pragma once

#include <windows.h>
#include <string_view>

#pragma comment(lib, "advapi32.dll")

using namespace std;

#define _MAKE_LOGGING_TYPE(name, event_type) \
    auto name(string_view msg, DWORD event_id = 0, WORD event_category = 0) const \
    { \
        if (!ReportEvent(_handle, event_type, event_category, event_id, nullptr, 1, 0, msg.data(), nullptr)) \
        { \
            throw runtime_error{"Failed to report event. Error code: " + GetLastError()}; \
        } \
    }

namespace logging
{
    class logger
    {
        HANDLE _handle;

    public:
        _MAKE_LOGGING_TYPE(success, EVENTLOG_SUCCESS)
        _MAKE_LOGGING_TYPE(audit_failure, EVENTLOG_AUDIT_FAILURE)
        _MAKE_LOGGING_TYPE(audit_success, EVENTLOG_AUDIT_SUCCESS)
        _MAKE_LOGGING_TYPE(error, EVENTLOG_ERROR_TYPE)
        _MAKE_LOGGING_TYPE(info, EVENTLOG_INFORMATION_TYPE)
        _MAKE_LOGGING_TYPE(warning, EVENTLOG_WARNING_TYPE)

    public:
        logger(string_view name): _handle{RegisterEventSource(nullptr, name.data())}
        {
            if (!_handle)
            {
                throw runtime_error{"Failed to register event source. Error code: " + GetLastError()};
            }
        }
    };
}
