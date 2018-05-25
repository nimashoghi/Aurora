#include "logger.hpp"
#include "service.hpp"
#include "undocumented.hpp"
#include "util.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <string_view>

#pragma comment(lib, "advapi32.lib")

using namespace std;
using namespace logging;
using namespace util;

static inline unique_ptr<logger> logger = nullptr;

auto initiate_logging()
{
    logger = make_unique<logger>("Aurora");
}


static inline array<u8, 0x100> backup;
static inline size_t backup_size = 0;


auto set_night_light_setting(const u8 *setting, const size_t size)
{
    constexpr auto registry_path =
        R"(Software\Microsoft\Windows\CurrentVersion\CloudStore\Store\Cache\DefaultAccount\$$windows.data.bluelightreduction.settings\Current)";

    HKEY key_handle;
    if (!SUCCEEDED(RegOpenKeyEx(HKEY_CURRENT_USER, registry_path, 0, KEY_READ | KEY_WRITE, &key_handle)))
    {
        logger->error("Failed to open night light key.");
        return;
    }
    if (!SUCCEEDED(RegSetValueEx(key_handle, "Data", 0, REG_BINARY, reinterpret_cast<const BYTE *>(setting), size)))
    {
        logger->error("Failed to set night light registry value.");
        return;
    }
}

inline auto make_backup()
{
        constexpr auto registry_path =
        R"(Software\Microsoft\Windows\CurrentVersion\CloudStore\Store\Cache\DefaultAccount\$$windows.data.bluelightreduction.settings\Current)";

    HKEY key_handle;
    if (!SUCCEEDED(RegOpenKeyEx(HKEY_CURRENT_USER, registry_path, 0, KEY_READ | KEY_WRITE, &key_handle)))
    {
        logger->error("Failed to open night light key.");
        return;
    }
    if (!SUCCEEDED(RegQueryValueEx(key_handle, "Data", 0, REG_BINARY, reinterpret_cast<BYTE *>(backup), &backup_size)))
    {
        logger->error("Failed to read night light registry value.");
        return;
    }
}

inline auto enable_night_light()
{
    static const auto enabled_setting = make_array_of<u8>(0x02, 0x00, 0x00, 0x00, 0x46, 0x1b, 0x1b, 0xe9, 0x11, 0xef, 0xd3, 0x01, 0x00, 0x00, 0x00, 0x00, 0x43, 0x42, 0x01, 0x00, 0x02, 0x01, 0xca, 0x14, 0x0e, 0x15, 0x00, 0xca, 0x1e, 0x0e, 0x07, 0x00, 0xcf, 0x28, 0xbc, 0x3e, 0xca, 0x32, 0x0e, 0x14, 0x2e, 0x00, 0x00, 0xca, 0x3c, 0x0e, 0x06, 0x2e, 0x22, 0x00, 0x00);
    if (!backup_size)
    {
        set_night_light_setting(enabled_setting.data(), enabled_setting.size());
        return;
    }
    set_night_light_setting(backup, backup_size);
}


inline auto disable_night_light()
{
    make_backup();

    static const auto disabled_setting = make_array_of<u8>(0x02, 0x00, 0x00, 0x00, 0xbb, 0x51, 0xcc, 0x45, 0x12, 0xef, 0xd3, 0x01, 0x00, 0x00, 0x00, 0x00, 0x43, 0x42, 0x01, 0x00, 0xca, 0x14, 0x0e, 0x15, 0x00, 0xca, 0x1e, 0x0e, 0x07, 0x00, 0xcf, 0x28, 0xbc, 0x3e, 0xca, 0x32, 0x0e, 0x14, 0x2e, 0x00, 0x00, 0xca, 0x3c, 0x0e, 0x06, 0x2e, 0x22, 0x00, 0x00);
    set_night_light_setting(disabled_setting.data(), disabled_setting.size());
}

inline auto toggle_night_light(const bool option)
{
    if (option)
    {
        enable_night_light();
    }
    else
    {
        disable_night_light();
    }
}

/*
 * https://i.imgur.com/6HQQmUP.png
 */

inline auto get_nt_query_wnf_state_data()
{
    HINSTANCE ntdll = GetModuleHandle("ntdll.dll");
    if (!ntdll)
    {
        logging->error("Failed to find ntdll.dll module.");
        return nullptr;
    }
    FARPROC fptr = GetProcAddress(ntdll, "NtQueryWnfStateData");
    if (!fptr)
    {
        logging->error("Failed to find NtQueryWnfStateData function.");
        return nullptr;
    }
    return reinterpret_cast<decltype(&NtQueryWnfStateData)>(fptr);
}

inline auto is_game_mode_on()
{
    static const auto nt_query_wnf_state_data = get_nt_query_wnf_state_data();

    WNF_CHANGE_STAMP change_stamp;
    DWORD process_id;
    ULONG buffer_size = sizeof(DWORD);
    if (!SUCCEEDED(nt_query_wnf_state_data(&WNF_RM_GAME_MODE_ACTIVE, nullptr, nullptr, &change_stamp, &process_id, &buffer_size)))
    {
        logging->warning("Call to NtQueryWnfStateData failed.");
        return 0;
    }
    return !!process_id;
}

auto worker()
{
    using namespace chrono;

    initiate_logging();

    auto last_game_mode_value = is_game_mode_on();
    while (true)
    {
        const auto current_game_mode_value = is_game_mode_on();
        if (current_game_mode_value != last_game_mode_value)
        {
            toggle_night_light(!current_game_mode_value);
            last_game_mode_value = current_game_mode_value;
        }
        this_thread::sleep_for(100ms);
    }
}

inline auto already_running()
{
    CreateMutex(nullptr, TRUE, "Aurora_Instance_Mutex");
    return GetLastError() == ERROR_ALREADY_EXISTS;
}


auto main(int /* argc */, char ** /* argv */) -> int
{
    if (already_running())
    {
        return 0;
    }

    worker();
    return 0;
}

auto __stdcall WinMain(HINSTANCE /* instance */, HINSTANCE /* prev_instance */, LPSTR /* cmd_line */, int /* cmd_show */) -> int
{
    if (already_running())
    {
        return 0;
    }

    worker();
    return 0;
}

/*
SERVICE_INIT(Aurora,
    {
        using namespace chrono;

        auto last_game_mode_value = is_game_mode_on();
        while (active())
        {
            const auto current_game_mode_value = is_game_mode_on();
            if (current_game_mode_value != last_game_mode_value)
            {
                last_game_mode_value = current_game_mode_value;
            }
            this_thread::sleep_for(100ms);
        }

        return 0;
    })
 */
