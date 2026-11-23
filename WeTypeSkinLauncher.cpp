#define UNICODE
#define _UNICODE
#define IDI_WETYPE 101

#include <windows.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <taskschd.h>
#include <winver.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

static const wchar_t* kTaskName = L"WeTypeColorPatch";
static const wchar_t* kRendererName = L"wetype_renderer.exe";
static const wchar_t* kWatchMutex = L"Local\\WeTypeSkinNativeWatch";
static const wchar_t* kGuiMutex = L"Local\\WeTypeSkinNativeGui";
static const wchar_t* kConfigEvent = L"Local\\WeTypeSkinConfigChanged";
static const wchar_t* kWindowClass = L"WeTypeModernLauncherWindow";
static const wchar_t* kPaletteClass = L"WeTypePaletteWindow";

namespace ModernTheme {
    constexpr COLORREF Bg = RGB(247, 248, 250);
    constexpr COLORREF CardBg = RGB(255, 255, 255);
    constexpr COLORREF CardBorder = RGB(231, 233, 237);
    constexpr COLORREF TextPrimary = RGB(17, 24, 39);
    constexpr COLORREF TextSecondary = RGB(107, 114, 128);
    constexpr COLORREF TextTertiary = RGB(156, 163, 175);
    constexpr COLORREF Accent = RGB(37, 99, 235);
    constexpr COLORREF AccentHover = RGB(29, 78, 216);
    constexpr COLORREF StatusGreen = RGB(34, 197, 94);
    constexpr COLORREF StatusGray = RGB(191, 195, 202);
    constexpr COLORREF InputBorder = RGB(209, 213, 219);
    constexpr COLORREF SegmentBg = RGB(244, 246, 248);
    constexpr COLORREF Invalid = RGB(220, 38, 38);
}

struct ColorPreset {
    const wchar_t* name;
    COLORREF color;
};

static const std::vector<ColorPreset> kDarkPresets = {
    {L"浅云灰", RGB(0xD4, 0xD4, 0xD4)},
    {L"柔和白", RGB(0xEF, 0xEF, 0xEF)},
    {L"银雾灰", RGB(0xC3, 0xC7, 0xCE)},
    {L"冷浅蓝", RGB(0xCB, 0xD5, 0xE1)}
};

static const std::vector<ColorPreset> kLightPresets = {
    {L"候选粉", RGB(0xFF, 0xD0, 0xE8)},
    {L"深空灰", RGB(0x3A, 0x3A, 0x3A)},
    {L"经典蓝", RGB(0x25, 0x63, 0xEB)},
    {L"微信绿", RGB(0x00, 0xA8, 0x6F)},
    {L"典雅紫", RGB(0x7C, 0x3A, 0xED)}
};

static const COLORREF kPaletteColors[] = {
    RGB(0x11, 0x18, 0x27), RGB(0x1F, 0x1F, 0x1F), RGB(0x2B, 0x2D, 0x30), RGB(0x3A, 0x3A, 0x3A), RGB(0x4B, 0x55, 0x63), RGB(0x66, 0x66, 0x66),
    RGB(0x88, 0x88, 0x88), RGB(0xA3, 0xA3, 0xA3), RGB(0xD4, 0xD4, 0xD4), RGB(0xEF, 0xEF, 0xEF), RGB(0xFA, 0xFA, 0xFA), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xE1, 0x1D, 0x48), RGB(0xF9, 0x73, 0x16), RGB(0xE9, 0xAB, 0x08), RGB(0x65, 0xA3, 0x0D), RGB(0x05, 0x96, 0x69), RGB(0x08, 0x91, 0xB2),
    RGB(0x02, 0x84, 0xC7), RGB(0x25, 0x63, 0xEB), RGB(0x4F, 0x46, 0xE5), RGB(0x7C, 0x3A, 0xED), RGB(0xC0, 0x26, 0xD3), RGB(0xDB, 0x27, 0x77),
    RGB(0xFB, 0x71, 0x85), RGB(0xFB, 0x92, 0x3C), RGB(0xFA, 0xCC, 0x15), RGB(0x84, 0xCC, 0x16), RGB(0x10, 0xB9, 0x81), RGB(0x06, 0xB6, 0xD4)
};


static std::wstring g_baseDir;
static std::wstring g_iniPath;
static HWND g_darkEdit = nullptr;
static HWND g_lightEdit = nullptr;
static HWND g_toggleButton = nullptr;
static HWND g_resetButton = nullptr;
static HWND g_applyButton = nullptr;
static HWND g_darkPaletteButton = nullptr;
static HWND g_lightPaletteButton = nullptr;
static HWND g_mainWindow = nullptr;
static HWND g_paletteWindow = nullptr;
static HWND g_paletteApplyButton = nullptr;
static HWND g_paletteCancelButton = nullptr;
static constexpr COLORREF kDefaultDarkColor = RGB(0xD4, 0xD4, 0xD4);
static constexpr COLORREF kDefaultLightColor = RGB(0xFF, 0xD0, 0xE8);
static COLORREF g_darkColor = kDefaultDarkColor;
static COLORREF g_lightColor = kDefaultLightColor;
static HFONT g_fontTitle = nullptr;
static HFONT g_fontSub = nullptr;
static HFONT g_fontBody = nullptr;
static HFONT g_fontBold = nullptr;
static HFONT g_fontMono = nullptr;
static HBRUSH g_editBrush = nullptr;
static double g_dpiScale = 1.0;
static double g_userZoom = 1.0;
static int g_scrollX = 0;
static int g_scrollY = 0;
static bool g_updatingScrollBars = false;
static bool g_darkModeSelected = false;
static bool g_darkTextValid = true;
static bool g_lightTextValid = true;
static TASK_STATE g_cachedTaskState = TASK_STATE_UNKNOWN;
static COLORREF g_paletteColor = kDefaultDarkColor;
static bool g_paletteForDark = true;
static bool g_paletteTextValid = true;
static ULONG_PTR g_gdiplusToken = 0;

enum ControlId {
    ID_BTN_TOGGLE_TASK = 105,
    ID_BTN_APPLY = 107,
    ID_BTN_RESET = 108,
    ID_DARK_EDIT = 109,
    ID_LIGHT_EDIT = 110,
    ID_TIMER_REFRESH = 111,
    ID_DARK_PALETTE = 112,
    ID_LIGHT_PALETTE = 113,
    ID_PALETTE_EDIT = 201,
    ID_PALETTE_APPLY = 202,
    ID_PALETTE_CANCEL = 203
};

struct HitPreset {
    RECT rect;
    COLORREF color;
    bool isDark;
};
static std::vector<HitPreset> g_hitPresets;
static std::vector<std::pair<RECT, COLORREF>> g_paletteHits;
static RECT g_darkSegmentHit{};
static RECT g_lightSegmentHit{};

enum class LucideIcon { Moon, Sun, Palette, RotateCcw, Power, Check };

struct Config {
    uint64_t darkTarget;
    uint64_t lightTarget;
    uint64_t foregroundTarget;
    std::vector<uint64_t> darkSources;
    std::vector<uint64_t> lightSources;
    std::vector<uint64_t> foregroundSources;
    DWORD maxMatches;
    DWORD pollMs;
};

struct PatchState {
    DWORD pid = 0;
    uint64_t created = 0;
    std::vector<uintptr_t> darkAddresses;
    std::vector<uintptr_t> lightAddresses;
    std::vector<uintptr_t> foregroundAddresses;
};

static std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring result(path, length);
    size_t slash = result.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : result.substr(0, slash);
}

static std::wstring ReadIni(const wchar_t* section, const wchar_t* key, const wchar_t* fallback) {
    wchar_t buffer[4096];
    GetPrivateProfileStringW(section, key, fallback, buffer, 4096, g_iniPath.c_str());
    return buffer;
}

static void WriteIni(const wchar_t* section, const wchar_t* key, const std::wstring& value) {
    WritePrivateProfileStringW(section, key, value.c_str(), g_iniPath.c_str());
}

static std::wstring Trim(std::wstring value) {
    const wchar_t* whitespace = L" \t\r\n";
    size_t start = value.find_first_not_of(whitespace);
    if (start == std::wstring::npos) return L"";
    size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

static uint64_t ParseRgb(const std::wstring& input) {
    std::wstring value = Trim(input);
    if (!value.empty() && value[0] == L'#') value.erase(0, 1);
    if (value.size() == 8 && (value.substr(0, 2) == L"FF" || value.substr(0, 2) == L"ff")) value.erase(0, 2);
    if (value.size() != 6) throw std::runtime_error("invalid color");
    wchar_t* end = nullptr;
    unsigned long rgb = wcstoul(value.c_str(), &end, 16);
    if (!end || *end != L'\0') throw std::runtime_error("invalid color");
    return static_cast<uint64_t>(0xFF000000UL | rgb);
}

static std::wstring RgbText(uint64_t argb) {
    std::wstringstream out;
    out << std::uppercase << std::hex << std::setfill(L'0') << std::setw(6) << (argb & 0xFFFFFFULL);
    return out.str();
}

static std::vector<uint64_t> ParseColorList(const std::wstring& text) {
    std::vector<uint64_t> values;
    std::wstringstream stream(text);
    std::wstring item;
    while (std::getline(stream, item, L',')) {
        item = Trim(item);
        if (item.empty()) continue;
        try { values.push_back(ParseRgb(item)); } catch (...) {}
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

static Config LoadConfig() {
    Config config;
    config.darkTarget = ParseRgb(ReadIni(L"targets", L"dark", L"D4D4D4"));
    config.lightTarget = ParseRgb(ReadIni(L"targets", L"light", L"FFD0E8"));
    config.foregroundTarget = ParseRgb(ReadIni(L"targets", L"foreground", L"FFFFFF"));
    config.darkSources = ParseColorList(ReadIni(L"signatures", L"dark", L"00A86F"));
    config.lightSources = ParseColorList(ReadIni(L"signatures", L"light", L"00B176"));
    config.foregroundSources = ParseColorList(ReadIni(L"signatures", L"foreground", L"383838,191919,F7F7F7"));
    config.maxMatches = GetPrivateProfileIntW(L"signatures", L"max_matches", 8, g_iniPath.c_str());
    config.pollMs = GetPrivateProfileIntW(L"engine", L"poll_ms", 1500, g_iniPath.c_str());
    config.maxMatches = std::max<DWORD>(1, std::min<DWORD>(config.maxMatches, 32));
    config.pollMs = std::max<DWORD>(500, std::min<DWORD>(config.pollMs, 10000));
    return config;
}

static uint64_t FileTimeValue(const FILETIME& time) {
    return (static_cast<uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
}

static bool GetProcessCreation(HANDLE process, uint64_t& created) {
    FILETIME creation, exitTime, kernel, user;
    if (!GetProcessTimes(process, &creation, &exitTime, &kernel, &user)) return false;
    created = FileTimeValue(creation);
    return true;
}

static DWORD FindRendererPid() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, kRendererName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

static std::wstring ProcessVersion(HANDLE process) {
    wchar_t path[MAX_PATH];
    DWORD length = MAX_PATH;
    if (!QueryFullProcessImageNameW(process, 0, path, &length)) return L"unknown";
    DWORD ignored = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &ignored);
    if (!size) return L"unknown";
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path, 0, size, data.data())) return L"unknown";
    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &infoSize) || !info) return L"unknown";
    std::wstringstream out;
    out << HIWORD(info->dwFileVersionMS) << L'.' << LOWORD(info->dwFileVersionMS) << L'.'
        << HIWORD(info->dwFileVersionLS) << L'.' << LOWORD(info->dwFileVersionLS);
    return out.str();
}

static std::wstring JoinAddresses(const std::vector<uintptr_t>& addresses) {
    std::wstringstream out;
    for (size_t i = 0; i < addresses.size(); ++i) {
        if (i) out << L',';
        out << std::uppercase << std::hex << addresses[i];
    }
    return out.str();
}

static std::vector<uintptr_t> ParseAddresses(const std::wstring& text) {
    std::vector<uintptr_t> result;
    std::wstringstream stream(text);
    std::wstring item;
    while (std::getline(stream, item, L',')) {
        wchar_t* end = nullptr;
        unsigned long long value = wcstoull(Trim(item).c_str(), &end, 16);
        if (end && *end == L'\0' && value) result.push_back(static_cast<uintptr_t>(value));
    }
    return result;
}

static PatchState LoadState() {
    PatchState state;
    state.pid = GetPrivateProfileIntW(L"state", L"pid", 0, g_iniPath.c_str());
    state.created = wcstoull(ReadIni(L"state", L"created", L"0").c_str(), nullptr, 16);
    state.darkAddresses = ParseAddresses(ReadIni(L"state", L"dark_addresses", L""));
    state.lightAddresses = ParseAddresses(ReadIni(L"state", L"light_addresses", L""));
    state.foregroundAddresses = ParseAddresses(ReadIni(L"state", L"foreground_addresses", L""));
    return state;
}

static void SaveState(const PatchState& state) {
    WriteIni(L"state", L"pid", std::to_wstring(state.pid));
    std::wstringstream created;
    created << std::uppercase << std::hex << state.created;
    WriteIni(L"state", L"created", created.str());
    WriteIni(L"state", L"dark_addresses", JoinAddresses(state.darkAddresses));
    WriteIni(L"state", L"light_addresses", JoinAddresses(state.lightAddresses));
    WriteIni(L"state", L"foreground_addresses", JoinAddresses(state.foregroundAddresses));
}

static bool IsReadableWritable(const MEMORY_BASIC_INFORMATION& info) {
    return info.State == MEM_COMMIT && info.Type == MEM_PRIVATE && info.Protect == PAGE_READWRITE
        && info.RegionSize > 0 && info.RegionSize <= 16ULL * 1024ULL * 1024ULL;
}

static void CollectMatches(
    const BYTE* data,
    SIZE_T size,
    uintptr_t base,
    const std::vector<uint64_t>& sources,
    std::vector<uintptr_t>& output
) {
    if (size < sizeof(uint64_t)) return;
    for (SIZE_T offset = 0; offset + sizeof(uint64_t) <= size; ++offset) {
        uint64_t value;
        memcpy(&value, data + offset, sizeof(value));
        if (std::find(sources.begin(), sources.end(), value) != sources.end()) output.push_back(base + offset);
    }
}

static bool WriteValue(HANDLE process, uintptr_t address, uint64_t value) {
    SIZE_T written = 0;
    return WriteProcessMemory(process, reinterpret_cast<void*>(address), &value, sizeof(value), &written)
        && written == sizeof(value);
}

static bool PatchKnownAddresses(
    HANDLE process,
    const Config& config,
    PatchState& state
) {
    if (state.darkAddresses.empty() || state.lightAddresses.empty() || state.foregroundAddresses.empty()) return false;
    for (uintptr_t address : state.darkAddresses) if (!WriteValue(process, address, config.darkTarget)) return false;
    for (uintptr_t address : state.lightAddresses) if (!WriteValue(process, address, config.lightTarget)) return false;
    for (uintptr_t address : state.foregroundAddresses) if (!WriteValue(process, address, config.foregroundTarget)) return false;
    return true;
}

static bool ScanAndPatch(HANDLE process, const Config& config, PatchState& state) {
    std::vector<uintptr_t> dark;
    std::vector<uintptr_t> light;
    std::vector<uintptr_t> foreground;
    std::vector<uintptr_t> darkTargets;
    std::vector<uintptr_t> lightTargets;
    const std::vector<uint64_t> darkTargetPattern{config.darkTarget};
    const std::vector<uint64_t> lightTargetPattern{config.lightTarget};
    uintptr_t address = 0;
    MEMORY_BASIC_INFORMATION info{};
    while (VirtualQueryEx(process, reinterpret_cast<void*>(address), &info, sizeof(info)) == sizeof(info)) {
        uintptr_t base = reinterpret_cast<uintptr_t>(info.BaseAddress);
        SIZE_T size = info.RegionSize;
        if (IsReadableWritable(info)) {
            std::vector<BYTE> buffer(size);
            SIZE_T read = 0;
            if (ReadProcessMemory(process, info.BaseAddress, buffer.data(), size, &read)) {
                CollectMatches(buffer.data(), read, base, config.darkSources, dark);
                CollectMatches(buffer.data(), read, base, config.lightSources, light);
                CollectMatches(buffer.data(), read, base, config.foregroundSources, foreground);
                CollectMatches(buffer.data(), read, base, darkTargetPattern, darkTargets);
                CollectMatches(buffer.data(), read, base, lightTargetPattern, lightTargets);
            }
        }
        uintptr_t next = base + size;
        if (next <= address || next >= 0x00007FFFFFFFFFFFULL) break;
        address = next;
    }
    std::sort(dark.begin(), dark.end());
    dark.erase(std::unique(dark.begin(), dark.end()), dark.end());
    std::sort(light.begin(), light.end());
    light.erase(std::unique(light.begin(), light.end()), light.end());
    std::sort(foreground.begin(), foreground.end());
    foreground.erase(std::unique(foreground.begin(), foreground.end()), foreground.end());

    bool usedTargetFallback = dark.empty() || light.empty();
    if (usedTargetFallback) {
        const std::vector<uintptr_t>& darkCandidates = dark.empty() ? darkTargets : dark;
        const std::vector<uintptr_t>& lightCandidates = light.empty() ? lightTargets : light;
        std::vector<uintptr_t> pairedDark;
        std::vector<uintptr_t> pairedLight;
        for (uintptr_t darkAddress : darkCandidates) {
            for (uintptr_t lightAddress : lightCandidates) {
                uintptr_t distance = darkAddress > lightAddress ? darkAddress - lightAddress : lightAddress - darkAddress;
                if (distance <= 0x400) {
                    pairedDark.push_back(darkAddress);
                    pairedLight.push_back(lightAddress);
                }
            }
        }
        std::sort(pairedDark.begin(), pairedDark.end());
        pairedDark.erase(std::unique(pairedDark.begin(), pairedDark.end()), pairedDark.end());
        std::sort(pairedLight.begin(), pairedLight.end());
        pairedLight.erase(std::unique(pairedLight.begin(), pairedLight.end()), pairedLight.end());
        dark.swap(pairedDark);
        light.swap(pairedLight);
    }

    WriteIni(L"status", L"dark_matches", std::to_wstring(dark.size()));
    WriteIni(L"status", L"light_matches", std::to_wstring(light.size()));
    WriteIni(L"status", L"foreground_matches", std::to_wstring(foreground.size()));
    if (dark.empty() || light.empty() || foreground.empty()) {
        WriteIni(L"status", L"result", L"no_compatible_signature");
        return false;
    }
    if (dark.size() > config.maxMatches || light.size() > config.maxMatches || foreground.size() > config.maxMatches) {
        WriteIni(L"status", L"result", L"ambiguous_signature");
        return false;
    }
    for (uintptr_t item : dark) if (!WriteValue(process, item, config.darkTarget)) return false;
    for (uintptr_t item : light) if (!WriteValue(process, item, config.lightTarget)) return false;
    for (uintptr_t item : foreground) if (!WriteValue(process, item, config.foregroundTarget)) return false;
    state.darkAddresses = dark;
    state.lightAddresses = light;
    state.foregroundAddresses = foreground;
    WriteIni(L"status", L"result", usedTargetFallback ? L"already_patched" : L"patched");
    return true;
}

static void UpdateStatus(DWORD pid, const std::wstring& version, const wchar_t* result) {
    WriteIni(L"status", L"pid", std::to_wstring(pid));
    WriteIni(L"status", L"version", version);
    if (result) WriteIni(L"status", L"result", result);
}

static bool PatchRenderer(DWORD pid, Config& config, PatchState& state, bool preferState) {
    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
        FALSE,
        pid
    );
    if (!process) {
        UpdateStatus(pid, L"unknown", L"access_denied");
        return false;
    }
    uint64_t created = 0;
    GetProcessCreation(process, created);
    std::wstring version = ProcessVersion(process);
    bool stateValid = preferState && state.pid == pid && state.created == created;
    bool success = false;
    if (stateValid) {
        success = PatchKnownAddresses(process, config, state);
        if (success) WriteIni(L"status", L"result", L"address_update");
    }
    if (!success) {
        state.pid = pid;
        state.created = created;
        state.darkAddresses.clear();
        state.lightAddresses.clear();
        state.foregroundAddresses.clear();
        success = ScanAndPatch(process, config, state);
    }
    if (success) {
        SaveState(state);
        UpdateStatus(pid, version, nullptr);
    } else {
        UpdateStatus(pid, version, nullptr);
    }
    CloseHandle(process);
    return success;
}

static int WatchMode() {
    HANDLE mutex = CreateMutexW(nullptr, FALSE, kWatchMutex);
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        return 0;
    }
    HANDLE changeEvent = CreateEventW(nullptr, FALSE, FALSE, kConfigEvent);
    Config config = LoadConfig();
    PatchState state = LoadState();
    DWORD currentPid = 0;
    HANDLE processWait = nullptr;
    bool waitingStatusWritten = false;

    while (true) {
        if (!currentPid) {
            DWORD pid = FindRendererPid();
            if (pid) {
                PatchRenderer(pid, config, state, true);
                currentPid = pid;
                processWait = OpenProcess(SYNCHRONIZE, FALSE, pid);
                waitingStatusWritten = false;
            } else if (!waitingStatusWritten) {
                UpdateStatus(0, L"unknown", L"waiting_for_renderer");
                waitingStatusWritten = true;
            }
        }

        DWORD wait;
        if (processWait) {
            HANDLE handles[] = {changeEvent, processWait};
            wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) {
                CloseHandle(processWait);
                processWait = nullptr;
                currentPid = 0;
                continue;
            }
        } else {
            wait = WaitForSingleObject(changeEvent, config.pollMs);
        }

        if (wait == WAIT_OBJECT_0) {
            config = LoadConfig();
            DWORD pid = FindRendererPid();
            if (pid) {
                if (pid != currentPid) {
                    if (processWait) CloseHandle(processWait);
                    processWait = OpenProcess(SYNCHRONIZE, FALSE, pid);
                }
                PatchRenderer(pid, config, state, true);
                currentPid = pid;
            }
        }
    }
}

static HRESULT GetRegisteredTask(IRegisteredTask** output) {
    *output = nullptr;
    ITaskService* service = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&service));
    if (FAILED(hr)) return hr;
    VARIANT empty;
    VariantInit(&empty);
    hr = service->Connect(empty, empty, empty, empty);
    if (FAILED(hr)) { service->Release(); return hr; }
    ITaskFolder* folder = nullptr;
    BSTR root = SysAllocString(L"\\");
    hr = service->GetFolder(root, &folder);
    SysFreeString(root);
    if (FAILED(hr)) { service->Release(); return hr; }
    BSTR name = SysAllocString(kTaskName);
    hr = folder->GetTask(name, output);
    SysFreeString(name);
    folder->Release();
    service->Release();
    return hr;
}

static bool StartTask() {
    IRegisteredTask* task = nullptr;
    HRESULT hr = GetRegisteredTask(&task);
    if (FAILED(hr)) return false;
    task->put_Enabled(VARIANT_TRUE);
    VARIANT empty;
    VariantInit(&empty);
    IRunningTask* running = nullptr;
    hr = task->Run(empty, &running);
    if (running) running->Release();
    task->Release();
    return SUCCEEDED(hr);
}

static bool StopTask() {
    IRegisteredTask* task = nullptr;
    HRESULT hr = GetRegisteredTask(&task);
    if (FAILED(hr)) return false;
    task->Stop(0);
    hr = task->put_Enabled(VARIANT_FALSE);
    task->Release();
    return SUCCEEDED(hr);
}

static TASK_STATE QueryTaskState(bool& enabled) {
    enabled = false;
    IRegisteredTask* task = nullptr;
    HRESULT hr = GetRegisteredTask(&task);
    if (FAILED(hr)) return TASK_STATE_UNKNOWN;
    TASK_STATE state = TASK_STATE_UNKNOWN;
    VARIANT_BOOL value = VARIANT_FALSE;
    task->get_State(&state);
    task->get_Enabled(&value);
    enabled = value == VARIANT_TRUE;
    task->Release();
    return state;
}

static COLORREF ArgbToColor(uint64_t value) {
    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

static std::wstring ColorText(COLORREF value) {
    std::wstringstream out;
    out << L'#' << std::uppercase << std::hex << std::setfill(L'0')
        << std::setw(2) << static_cast<int>(GetRValue(value))
        << std::setw(2) << static_cast<int>(GetGValue(value))
        << std::setw(2) << static_cast<int>(GetBValue(value));
    return out.str();
}

static int Ui(int value) {
    return static_cast<int>(std::lround(value * g_dpiScale * g_userZoom));
}

static RECT UiRect(int left, int top, int right, int bottom) {
    return {Ui(left), Ui(top), Ui(right), Ui(bottom)};
}

static void DeleteGuiFonts() {
    if (g_fontTitle) DeleteObject(g_fontTitle);
    if (g_fontBold) DeleteObject(g_fontBold);
    if (g_fontBody) DeleteObject(g_fontBody);
    if (g_fontSub) DeleteObject(g_fontSub);
    if (g_fontMono) DeleteObject(g_fontMono);
    g_fontTitle = g_fontBold = g_fontBody = g_fontSub = g_fontMono = nullptr;
}

static void CreateGuiFonts() {
    DeleteGuiFonts();
    g_fontTitle = CreateFontW(Ui(32), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Variable Display");
    g_fontBold = CreateFontW(Ui(19), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    g_fontBody = CreateFontW(Ui(17), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    g_fontSub = CreateFontW(Ui(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    g_fontMono = CreateFontW(Ui(17), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
}

static int CanvasWidth() {
    RECT client{};
    if (g_mainWindow) GetClientRect(g_mainWindow, &client);
    return std::max(Ui(860), static_cast<int>(client.right));
}

static void UpdateScrollBars(HWND window) {
    if (g_updatingScrollBars) return;
    g_updatingScrollBars = true;
    RECT client;
    GetClientRect(window, &client);
    int contentWidth = Ui(860);
    int contentHeight = Ui(620);
    int maxX = std::max(0, contentWidth - static_cast<int>(client.right));
    int maxY = std::max(0, contentHeight - static_cast<int>(client.bottom));
    g_scrollX = std::max(0, std::min(g_scrollX, maxX));
    g_scrollY = std::max(0, std::min(g_scrollY, maxY));

    SCROLLINFO horizontal{sizeof(horizontal), SIF_RANGE | SIF_PAGE | SIF_POS};
    horizontal.nMin = 0;
    horizontal.nMax = std::max(0, contentWidth - 1);
    horizontal.nPage = client.right;
    horizontal.nPos = g_scrollX;
    SetScrollInfo(window, SB_HORZ, &horizontal, TRUE);

    SCROLLINFO vertical{sizeof(vertical), SIF_RANGE | SIF_PAGE | SIF_POS};
    vertical.nMin = 0;
    vertical.nMax = std::max(0, contentHeight - 1);
    vertical.nPage = client.bottom;
    vertical.nPos = g_scrollY;
    SetScrollInfo(window, SB_VERT, &vertical, TRUE);
    g_updatingScrollBars = false;
}

static void LayoutMainControls() {
    if (!g_mainWindow) return;
    int canvasRight = CanvasWidth();
    int split = canvasRight / 2;
    int inputX = split + Ui(34);
    int paletteX = split + Ui(208);
    int paletteWidth = std::max(Ui(142), canvasRight - Ui(64) - paletteX);
    HWND activeEdit = g_darkModeSelected ? g_darkEdit : g_lightEdit;
    HWND activePalette = g_darkModeSelected ? g_darkPaletteButton : g_lightPaletteButton;
    for (HWND edit : {g_darkEdit, g_lightEdit}) ShowWindow(edit, edit == activeEdit ? SW_SHOW : SW_HIDE);
    for (HWND button : {g_darkPaletteButton, g_lightPaletteButton}) ShowWindow(button, button == activePalette ? SW_SHOW : SW_HIDE);
    SetWindowPos(activeEdit, nullptr, inputX - g_scrollX, Ui(412) - g_scrollY, Ui(158), Ui(34), SWP_NOZORDER);
    SetWindowPos(activePalette, nullptr, paletteX - g_scrollX, Ui(402) - g_scrollY, paletteWidth, Ui(54), SWP_NOZORDER);
    SetWindowPos(g_resetButton, nullptr, Ui(64) - g_scrollX, Ui(515) - g_scrollY, Ui(146), Ui(46), SWP_NOZORDER);
    SetWindowPos(g_toggleButton, nullptr, Ui(222) - g_scrollX, Ui(515) - g_scrollY, Ui(146), Ui(46), SWP_NOZORDER);
    SetWindowPos(g_applyButton, nullptr, canvasRight - Ui(234) - g_scrollX, Ui(515) - g_scrollY, Ui(170), Ui(46), SWP_NOZORDER);
    SendMessageW(g_darkEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);
    SendMessageW(g_lightEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);
    for (HWND button : {g_darkPaletteButton, g_lightPaletteButton, g_resetButton, g_toggleButton})
        SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBody), TRUE);
    SendMessageW(g_applyButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBold), TRUE);
}

static void ApplyZoom(HWND window, double nextZoom) {
    nextZoom = std::max(0.75, std::min(1.60, nextZoom));
    if (std::abs(nextZoom - g_userZoom) < 0.001) return;
    g_userZoom = nextZoom;
    CreateGuiFonts();
    UpdateScrollBars(window);
    LayoutMainControls();
    InvalidateRect(window, nullptr, TRUE);
}

static void DrawModernCard(HDC hdc, const RECT& rect) {
    HBRUSH brush = CreateSolidBrush(ModernTheme::CardBg);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, Ui(1)), ModernTheme::CardBorder);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, Ui(16), Ui(16));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static Gdiplus::Color GdiPlusColor(COLORREF color, BYTE alpha = 255) {
    return Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color));
}

static void ConfigureLucidePen(Gdiplus::Pen& pen) {
    // Lucide uses round caps / joins. GDI's Polyline/CreatePen rasterizes these
    // poorly at 16-20 px, so use GDI+ anti-aliased vector strokes instead.
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    pen.SetMiterLimit(2.0f);
}

static void DrawLucideIcon(HDC hdc, LucideIcon icon, const RECT& box, COLORREF color) {
    using namespace Gdiplus;

    const REAL width = static_cast<REAL>(box.right - box.left);
    const REAL height = static_cast<REAL>(box.bottom - box.top);
    const REAL size = std::min(width, height);
    if (size <= 0.0f) return;

    // Lucide's native viewBox is 24 x 24. Use one uniform transform so every icon
    // retains the same optical weight and is pixel-perfect at different DPI/zoom.
    const REAL scale = size / 24.0f;
    const REAL x = static_cast<REAL>(box.left) + (width - size) * 0.5f;
    const REAL y = static_cast<REAL>(box.top) + (height - size) * 0.5f;

    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetCompositingQuality(CompositingQualityHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    GraphicsState saved = graphics.Save();
    graphics.TranslateTransform(x, y);
    graphics.ScaleTransform(scale, scale);

    Pen pen(GdiPlusColor(color), 2.0f);
    ConfigureLucidePen(pen);
    SolidBrush dotBrush(GdiPlusColor(color));

    auto line = [&](REAL x1, REAL y1, REAL x2, REAL y2) {
        graphics.DrawLine(&pen, x1, y1, x2, y2);
    };

    switch (icon) {
        case LucideIcon::Sun: {
            graphics.DrawEllipse(&pen, 8.0f, 8.0f, 8.0f, 8.0f);
            line(12, 2, 12, 4); line(12, 20, 12, 22);
            line(4.93f, 4.93f, 6.34f, 6.34f);
            line(17.66f, 17.66f, 19.07f, 19.07f);
            line(2, 12, 4, 12); line(20, 12, 22, 12);
            line(6.34f, 17.66f, 4.93f, 19.07f);
            line(19.07f, 4.93f, 17.66f, 6.34f);
            break;
        }

        case LucideIcon::Moon: {
            // Smooth Lucide-style crescent. Cubic curves avoid the angular
            // Polyline silhouette from the original implementation.
            GraphicsPath path;
            path.StartFigure();
            path.AddBezier(20.7f, 13.0f, 18.1f, 13.8f, 15.5f, 13.1f, 13.7f, 11.3f);
            path.AddBezier(11.9f, 9.5f, 11.2f, 6.9f, 12.1f, 4.4f, 12.9f, 2.6f);
            path.AddBezier(9.9f, 2.5f, 7.1f, 3.6f, 5.2f, 5.7f, 3.4f, 7.8f);
            path.AddBezier(3.3f, 11.0f, 4.5f, 14.2f, 6.8f, 16.5f, 9.1f, 18.8f);
            path.AddBezier(12.4f, 20.2f, 16.2f, 19.5f, 18.7f, 17.0f, 20.7f, 13.0f);
            graphics.DrawPath(&pen, &path);
            break;
        }

        case LucideIcon::Palette: {
            // Based on Lucide's palette silhouette, rendered with smooth Bezier
            // curves and filled color wells for a cleaner 18 px appearance.
            GraphicsPath path;
            path.StartFigure();
            path.AddBezier(12.0f, 3.0f, 6.5f, 3.0f, 2.0f, 6.9f, 2.0f, 11.8f);
            path.AddBezier(2.0f, 15.9f, 5.2f, 19.2f, 9.1f, 19.2f, 9.8f, 19.2f);
            path.AddBezier(10.6f, 19.2f, 11.2f, 19.8f, 11.2f, 20.5f, 11.2f, 21.3f);
            path.AddBezier(11.2f, 22.0f, 11.8f, 22.6f, 12.6f, 22.6f, 17.8f, 22.6f);
            path.AddBezier(22.0f, 18.7f, 22.0f, 13.7f, 22.0f, 12.6f, 22.0f, 7.3f);
            path.AddBezier(17.8f, 3.0f, 12.0f, 3.0f, 12.0f, 3.0f, 12.0f, 3.0f);
            graphics.DrawPath(&pen, &path);

            // Lucide palette wells are intentionally tiny. Filled dots remain
            // crisp at 125/150% DPI unlike nested GDI ellipses.
            graphics.FillEllipse(&dotBrush, 7.1f, 7.0f, 2.0f, 2.0f);
            graphics.FillEllipse(&dotBrush, 11.3f, 5.2f, 2.0f, 2.0f);
            graphics.FillEllipse(&dotBrush, 15.6f, 7.0f, 2.0f, 2.0f);
            graphics.FillEllipse(&dotBrush, 5.2f, 11.3f, 2.0f, 2.0f);
            break;
        }

        case LucideIcon::RotateCcw: {
            // Clean circular arrow with a deliberate top-left opening.
            GraphicsPath arc;
            arc.AddArc(3.5f, 3.5f, 17.0f, 17.0f, -120.0f, 300.0f);
            graphics.DrawPath(&pen, &arc);
            line(3.0f, 3.0f, 3.0f, 8.0f);
            line(3.0f, 8.0f, 8.0f, 8.0f);
            break;
        }

        case LucideIcon::Power: {
            line(12, 2, 12, 12);
            GraphicsPath arc;
            // A 275 degree arc approximates Lucide's power ring while preserving
            // a clean opening around the power stem.
            arc.AddArc(3.5f, 4.0f, 17.0f, 17.0f, -49.0f, 278.0f);
            graphics.DrawPath(&pen, &arc);
            break;
        }

        case LucideIcon::Check: {
            GraphicsPath path;
            path.StartFigure();
            path.AddLine(20.0f, 6.0f, 9.0f, 17.0f);
            path.AddLine(9.0f, 17.0f, 4.0f, 12.0f);
            graphics.DrawPath(&pen, &path);
            break;
        }
    }

    graphics.Restore(saved);
}

static void DrawColorChip(HDC hdc, const RECT& rect, COLORREF color, bool selected) {
    RECT chip = rect;
    InflateRect(&chip, -Ui(4), -Ui(4));
    if (selected) {
        HPEN ringPen = CreatePen(PS_SOLID, std::max(1, Ui(2)), ModernTheme::Accent);
        HGDIOBJ oldRingPen = SelectObject(hdc, ringPen);
        HGDIOBJ oldRingBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, Ui(12), Ui(12));
        SelectObject(hdc, oldRingBrush);
        SelectObject(hdc, oldRingPen);
        DeleteObject(ringPen);
    }
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, Ui(1)), RGB(238, 239, 242));
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, chip.left, chip.top, chip.right, chip.bottom, Ui(10), Ui(10));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

}

static void RenderModernUI(HWND window, HDC hdc) {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    g_hitPresets.clear();

    HBRUSH background = CreateSolidBrush(ModernTheme::Bg);
    FillRect(hdc, &clientRect, background);
    DeleteObject(background);
    SetViewportOrgEx(hdc, -g_scrollX, -g_scrollY, nullptr);
    int canvasRight = CanvasWidth();
    int split = canvasRight / 2;

    SetBkMode(hdc, TRANSPARENT);
    // Request the exact header size from the ICO group instead of stretching the
    // title-bar (usually 32 px) icon to 48 px. This keeps a multi-resolution ICO crisp.
    HICON appIcon = reinterpret_cast<HICON>(LoadImageW(
        GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_WETYPE), IMAGE_ICON,
        Ui(48), Ui(48), LR_DEFAULTCOLOR));
    if (!appIcon) appIcon = reinterpret_cast<HICON>(SendMessageW(window, WM_GETICON, ICON_BIG, 0));
    if (appIcon) {
        DrawIconEx(hdc, Ui(34), Ui(23), appIcon, Ui(48), Ui(48), 0, nullptr, DI_NORMAL);
        // LoadImage with an application resource returns a shared-ish resource handle
        // only for LR_SHARED. We did not request LR_SHARED, so release this exact-size copy.
        if (appIcon != reinterpret_cast<HICON>(SendMessageW(window, WM_GETICON, ICON_BIG, 0)))
            DestroyIcon(appIcon);
    }

    SetTextColor(hdc, ModernTheme::TextPrimary);
    SelectObject(hdc, g_fontTitle);
    TextOutW(hdc, Ui(98), Ui(18), L"WeType", 6);

    SetTextColor(hdc, ModernTheme::TextTertiary);
    SelectObject(hdc, g_fontSub);
    TextOutW(hdc, Ui(99), Ui(57), L"微信输入法外观控制", 9);

    bool isRunning = g_cachedTaskState == TASK_STATE_RUNNING;
    HBRUSH glowBrush = CreateSolidBrush(isRunning ? RGB(231, 249, 237) : RGB(242, 243, 245));
    HPEN nullPen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ oldBrush = SelectObject(hdc, glowBrush);
    HGDIOBJ oldPen = SelectObject(hdc, nullPen);
    Ellipse(hdc, canvasRight - Ui(62), Ui(35), canvasRight - Ui(40), Ui(57));
    HBRUSH dotBrush = CreateSolidBrush(isRunning ? ModernTheme::StatusGreen : ModernTheme::StatusGray);
    SelectObject(hdc, dotBrush);
    Ellipse(hdc, canvasRight - Ui(58), Ui(39), canvasRight - Ui(44), Ui(53));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(glowBrush);
    DeleteObject(dotBrush);
    DeleteObject(nullPen);

    RECT mainShadow = {Ui(34), Ui(112), canvasRight - Ui(30), Ui(594)};
    HBRUSH shadowBrush = CreateSolidBrush(RGB(240, 242, 245));
    oldBrush = SelectObject(hdc, shadowBrush);
    oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    RoundRect(hdc, mainShadow.left, mainShadow.top, mainShadow.right, mainShadow.bottom, Ui(18), Ui(18));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(shadowBrush);
    RECT mainCard = {Ui(32), Ui(108), canvasRight - Ui(32), Ui(590)};
    DrawModernCard(hdc, mainCard);

    RECT segment = {Ui(64), Ui(136), Ui(388), Ui(184)};
    HBRUSH segmentBrush = CreateSolidBrush(ModernTheme::SegmentBg);
    HPEN segmentPen = CreatePen(PS_SOLID, std::max(1, Ui(1)), ModernTheme::CardBorder);
    oldBrush = SelectObject(hdc, segmentBrush); oldPen = SelectObject(hdc, segmentPen);
    RoundRect(hdc, segment.left, segment.top, segment.right, segment.bottom, Ui(12), Ui(12));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(segmentBrush); DeleteObject(segmentPen);
    RECT active = !g_darkModeSelected ? RECT{Ui(68), Ui(140), Ui(226), Ui(180)} : RECT{Ui(226), Ui(140), Ui(384), Ui(180)};
    HBRUSH activeBrush = CreateSolidBrush(ModernTheme::CardBg);
    HPEN activePen = CreatePen(PS_SOLID, std::max(1, Ui(1)), RGB(236, 238, 242));
    oldBrush = SelectObject(hdc, activeBrush); oldPen = SelectObject(hdc, activePen);
    RoundRect(hdc, active.left, active.top, active.right, active.bottom, Ui(10), Ui(10));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(activeBrush); DeleteObject(activePen);
    g_lightSegmentHit = {Ui(64), Ui(136), Ui(226), Ui(184)};
    g_darkSegmentHit = {Ui(226), Ui(136), Ui(388), Ui(184)};
    DrawLucideIcon(hdc, LucideIcon::Sun, UiRect(91, 150, 111, 170), !g_darkModeSelected ? ModernTheme::Accent : ModernTheme::TextSecondary);
    DrawLucideIcon(hdc, LucideIcon::Moon, UiRect(249, 150, 269, 170), g_darkModeSelected ? ModernTheme::Accent : ModernTheme::TextSecondary);
    SelectObject(hdc, g_fontBody);
    SetTextColor(hdc, !g_darkModeSelected ? ModernTheme::Accent : ModernTheme::TextPrimary);
    RECT lightText = UiRect(116, 140, 215, 180);
    DrawTextW(hdc, L"浅色模式", -1, &lightText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(hdc, g_darkModeSelected ? ModernTheme::Accent : ModernTheme::TextPrimary);
    RECT darkText = UiRect(274, 140, 373, 180);
    DrawTextW(hdc, L"深色模式", -1, &darkText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, ModernTheme::TextPrimary);
    SelectObject(hdc, g_fontBold);
    TextOutW(hdc, Ui(64), Ui(222), L"预览", 2);
    TextOutW(hdc, split + Ui(34), Ui(222), L"配色", 2);

    RECT previewPanel = {Ui(64), Ui(258), split - Ui(30), Ui(470)};
    HBRUSH previewBrush = CreateSolidBrush(RGB(250, 251, 252));
    HPEN previewPen = CreatePen(PS_SOLID, std::max(1, Ui(1)), ModernTheme::CardBorder);
    oldBrush = SelectObject(hdc, previewBrush); oldPen = SelectObject(hdc, previewPen);
    RoundRect(hdc, previewPanel.left, previewPanel.top, previewPanel.right, previewPanel.bottom, Ui(14), Ui(14));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(previewBrush); DeleteObject(previewPen);

    COLORREF selectedColor = g_darkModeSelected ? g_darkColor : g_lightColor;
    COLORREF panelColor = g_darkModeSelected ? RGB(0x38, 0x38, 0x38) : RGB(0xF7, 0xF7, 0xF7);
    bool panelIsDark = (GetRValue(panelColor) * 299 + GetGValue(panelColor) * 587 + GetBValue(panelColor) * 114) < 128000;
    RECT candidateShadow = {previewPanel.left + Ui(28), Ui(316), previewPanel.right - Ui(24), Ui(414)};
    HBRUSH candidateShadowBrush = CreateSolidBrush(RGB(226, 229, 233));
    oldBrush = SelectObject(hdc, candidateShadowBrush); oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    RoundRect(hdc, candidateShadow.left, candidateShadow.top + Ui(6), candidateShadow.right, candidateShadow.bottom + Ui(8), Ui(13), Ui(13));
    HBRUSH candidateBrush = CreateSolidBrush(panelColor);
    SelectObject(hdc, candidateBrush);
    RoundRect(hdc, candidateShadow.left, candidateShadow.top, candidateShadow.right, candidateShadow.bottom, Ui(12), Ui(12));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(candidateShadowBrush); DeleteObject(candidateBrush);
    SetTextColor(hdc, panelIsDark ? RGB(240, 242, 245) : RGB(55, 65, 81)); SelectObject(hdc, g_fontSub);
    RECT pinyinText = {candidateShadow.left + Ui(18), candidateShadow.top + Ui(8), candidateShadow.right - Ui(18), candidateShadow.top + Ui(43)};
    DrawTextW(hdc, L"weishu'ru'fa", -1, &pinyinText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    HPEN rowPen = CreatePen(PS_SOLID, std::max(1, Ui(1)), panelIsDark ? RGB(92, 99, 108) : RGB(222, 225, 230));
    oldPen = SelectObject(hdc, rowPen); MoveToEx(hdc, candidateShadow.left + Ui(16), candidateShadow.top + Ui(47), nullptr); LineTo(hdc, candidateShadow.right - Ui(16), candidateShadow.top + Ui(47)); SelectObject(hdc, oldPen); DeleteObject(rowPen);
    RECT selectedItem = {candidateShadow.left + Ui(10), candidateShadow.top + Ui(53), candidateShadow.left + Ui(142), candidateShadow.bottom - Ui(8)};
    HBRUSH selectedBrush = CreateSolidBrush(selectedColor);
    oldBrush = SelectObject(hdc, selectedBrush); oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    RoundRect(hdc, selectedItem.left, selectedItem.top, selectedItem.right, selectedItem.bottom, Ui(8), Ui(8));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(selectedBrush);
    bool selectedIsDark = (GetRValue(selectedColor) * 299 + GetGValue(selectedColor) * 587 + GetBValue(selectedColor) * 114) < 128000;
    RECT candidateText = selectedItem; candidateText.left += Ui(8);
    SelectObject(hdc, g_fontSub); SetTextColor(hdc, selectedIsDark ? RGB(255, 255, 255) : RGB(17, 24, 39));
    DrawTextW(hdc, L"1. 微信输入法", -1, &candidateText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    candidateText = {selectedItem.right + Ui(12), candidateShadow.top + Ui(50), candidateShadow.right - Ui(18), candidateShadow.bottom - Ui(8)};
    SetTextColor(hdc, panelIsDark ? RGB(248, 250, 252) : RGB(31, 41, 55));
    DrawTextW(hdc, L"2. 输入法   3. 输入   4. 书", -1, &candidateText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdc, g_fontSub);
    SetTextColor(hdc, ModernTheme::TextSecondary);
    TextOutW(hdc, split + Ui(34), Ui(258), L"候选框选中背景", 7);

    const auto& presets = g_darkModeSelected ? kDarkPresets : kLightPresets;
    COLORREF editingColor = selectedColor;
    int chipX = split + Ui(34);
    int chipSize = Ui(54);
    int chipStep = (canvasRight - Ui(64) - chipX - chipSize) / 3;
    for (size_t index = 0; index < presets.size(); ++index) {
        const auto& preset = presets[index];
        int currentX = chipX + static_cast<int>(index) * chipStep;
        RECT chip = {currentX, Ui(304), currentX + chipSize, Ui(304) + chipSize};
        bool selected = editingColor == preset.color;
        DrawColorChip(hdc, chip, preset.color, selected);
        RECT hit = chip; InflateRect(&hit, Ui(7), Ui(7));
        g_hitPresets.push_back({hit, preset.color, g_darkModeSelected});
    }

    SetTextColor(hdc, ModernTheme::TextPrimary); SelectObject(hdc, g_fontBody);
    TextOutW(hdc, split + Ui(34), Ui(372), L"自定义颜色", 5);
    bool inputValid = g_darkModeSelected ? g_darkTextValid : g_lightTextValid;
    RECT inputFrame = {split + Ui(32), Ui(402), split + Ui(200), Ui(456)};
    HBRUSH inputBrush = CreateSolidBrush(ModernTheme::CardBg);
    HPEN inputPen = CreatePen(PS_SOLID, std::max(1, Ui(inputValid ? 1 : 2)), inputValid ? ModernTheme::CardBorder : ModernTheme::Invalid);
    oldBrush = SelectObject(hdc, inputBrush); oldPen = SelectObject(hdc, inputPen);
    RoundRect(hdc, inputFrame.left, inputFrame.top, inputFrame.right, inputFrame.bottom, Ui(10), Ui(10));
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(inputBrush); DeleteObject(inputPen);
}

static void LoadColorsToGui() {
    Config config = LoadConfig();
    g_darkColor = ArgbToColor(config.darkTarget);
    g_lightColor = ArgbToColor(config.lightTarget);
    SetWindowTextW(g_darkEdit, ColorText(g_darkColor).c_str());
    SetWindowTextW(g_lightEdit, ColorText(g_lightColor).c_str());
}

static bool ReadAndApplyColors(HWND window) {
    wchar_t dark[32], light[32];
    GetWindowTextW(g_darkEdit, dark, 32);
    GetWindowTextW(g_lightEdit, light, 32);
    try {
        g_darkColor = ArgbToColor(ParseRgb(dark));
        g_lightColor = ArgbToColor(ParseRgb(light));
    } catch (...) {
        MessageBoxW(window, L"请输入合法的十六进制颜色代码（如 #3A3A3A）。", L"颜色格式错误", MB_OK | MB_ICONWARNING);
        return false;
    }
    WriteIni(L"targets", L"dark", ColorText(g_darkColor).substr(1));
    WriteIni(L"targets", L"light", ColorText(g_lightColor).substr(1));
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, kConfigEvent);
    if (event) { SetEvent(event); CloseHandle(event); }
    StartTask();
    InvalidateRect(window, nullptr, FALSE);
    return true;
}

static void DrawActionButton(const DRAWITEMSTRUCT& item) {
    bool primary = item.CtlID == ID_BTN_APPLY || item.CtlID == ID_PALETTE_APPLY;
    bool pressed = (item.itemState & ODS_SELECTED) != 0;
    bool hot = (item.itemState & ODS_HOTLIGHT) != 0;
    COLORREF fill = primary ? (pressed || hot ? ModernTheme::AccentHover : ModernTheme::Accent) : (pressed || hot ? RGB(248, 250, 252) : ModernTheme::CardBg);
    COLORREF border = primary ? fill : ModernTheme::CardBorder;
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, Ui(1)), border);
    HGDIOBJ oldBrush = SelectObject(item.hDC, brush);
    HGDIOBJ oldPen = SelectObject(item.hDC, pen);
    RoundRect(item.hDC, item.rcItem.left, item.rcItem.top, item.rcItem.right, item.rcItem.bottom, Ui(10), Ui(10));
    SelectObject(item.hDC, oldBrush);
    SelectObject(item.hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    wchar_t label[32];
    GetWindowTextW(item.hwndItem, label, 32);
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, primary ? RGB(255, 255, 255) : ModernTheme::TextPrimary);
    SelectObject(item.hDC, primary ? g_fontBold : g_fontBody);
    RECT textRect = item.rcItem;
    if (pressed) OffsetRect(&textRect, 0, Ui(1));
    bool hasIcon = item.CtlID == ID_BTN_APPLY || item.CtlID == ID_BTN_RESET || item.CtlID == ID_BTN_TOGGLE_TASK
        || item.CtlID == ID_DARK_PALETTE || item.CtlID == ID_LIGHT_PALETTE
        || item.CtlID == ID_PALETTE_APPLY;
    if (hasIcon) {
        SIZE textSize{};
        GetTextExtentPoint32W(item.hDC, label, static_cast<int>(wcslen(label)), &textSize);
        int iconSize = Ui(18);
        int gap = Ui(9);
        int total = iconSize + gap + textSize.cx;
        int iconLeft = item.rcItem.left + ((item.rcItem.right - item.rcItem.left) - total) / 2;
        int iconTop = item.rcItem.top + ((item.rcItem.bottom - item.rcItem.top) - iconSize) / 2 + (pressed ? Ui(1) : 0);
        LucideIcon icon = LucideIcon::Check;
        if (item.CtlID == ID_BTN_RESET) icon = LucideIcon::RotateCcw;
        else if (item.CtlID == ID_BTN_TOGGLE_TASK) icon = LucideIcon::Power;
        else if (item.CtlID == ID_DARK_PALETTE || item.CtlID == ID_LIGHT_PALETTE) icon = LucideIcon::Palette;
        DrawLucideIcon(item.hDC, icon, {iconLeft, iconTop, iconLeft + iconSize, iconTop + iconSize}, primary ? RGB(255, 255, 255) : ModernTheme::TextPrimary);
        textRect.left = iconLeft + iconSize + gap;
        textRect.right = textRect.left + textSize.cx;
        DrawTextW(item.hDC, label, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawTextW(item.hDC, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static bool RefreshToggleButton(bool force = false) {
    bool enabled = false;
    TASK_STATE state = QueryTaskState(enabled);
    bool changed = state != g_cachedTaskState;
    g_cachedTaskState = state;
    const wchar_t* label = state == TASK_STATE_RUNNING ? L"关闭后台" : L"开启后台";
    wchar_t current[32]{};
    GetWindowTextW(g_toggleButton, current, 32);
    if (force || wcscmp(current, label) != 0) SetWindowTextW(g_toggleButton, label);
    return changed;
}

static void InvalidateStatusDot(HWND window) {
    int canvasRight = CanvasWidth();
    RECT area = {
        canvasRight - Ui(72) - g_scrollX,
        Ui(25) - g_scrollY,
        canvasRight - Ui(30) - g_scrollX,
        Ui(67) - g_scrollY
    };
    InvalidateRect(window, &area, FALSE);
}

static void RenderPalette(HDC hdc, HWND window) {
    RECT client;
    GetClientRect(window, &client);
    HBRUSH background = CreateSolidBrush(ModernTheme::Bg);
    FillRect(hdc, &client, background);
    DeleteObject(background);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ModernTheme::TextPrimary);
    SelectObject(hdc, g_fontTitle);
    TextOutW(hdc, Ui(28), Ui(20), L"选择颜色", 4);
    SetTextColor(hdc, ModernTheme::TextSecondary);
    SelectObject(hdc, g_fontBody);
    TextOutW(hdc, Ui(29), Ui(59), L"从色板选择，或直接输入十六进制色值", 17);

    g_paletteHits.clear();
    for (size_t index = 0; index < sizeof(kPaletteColors) / sizeof(kPaletteColors[0]); ++index) {
        int column = static_cast<int>(index % 6);
        int row = static_cast<int>(index / 6);
        RECT chip = UiRect(28 + column * 58, 96 + row * 58, 74 + column * 58, 142 + row * 58);
        HBRUSH brush = CreateSolidBrush(kPaletteColors[index]);
        bool selected = g_paletteColor == kPaletteColors[index];
        HPEN pen = CreatePen(PS_SOLID, std::max(1, Ui(selected ? 3 : 1)), selected ? ModernTheme::Accent : ModernTheme::InputBorder);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        RoundRect(hdc, chip.left, chip.top, chip.right, chip.bottom, Ui(12), Ui(12));
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
        g_paletteHits.push_back({chip, kPaletteColors[index]});
    }

    SetTextColor(hdc, ModernTheme::TextSecondary);
    SelectObject(hdc, g_fontBody);
    TextOutW(hdc, Ui(402), Ui(96), L"当前颜色", 4);
    RECT preview = UiRect(402, 128, 530, 222);
    HBRUSH previewBrush = CreateSolidBrush(g_paletteColor);
    HPEN previewPen = CreatePen(PS_SOLID, std::max(1, Ui(1)), ModernTheme::CardBorder);
    HGDIOBJ oldBrush = SelectObject(hdc, previewBrush);
    HGDIOBJ oldPen = SelectObject(hdc, previewPen);
    RoundRect(hdc, preview.left, preview.top, preview.right, preview.bottom, Ui(18), Ui(18));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(previewBrush);
    DeleteObject(previewPen);

    SetTextColor(hdc, ModernTheme::TextSecondary);
    SelectObject(hdc, g_fontSub);
    TextOutW(hdc, Ui(402), Ui(250), L"十六进制色值", 6);
    DrawModernCard(hdc, UiRect(398, 277, 534, 325));
}

static LRESULT CALLBACK PaletteWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HWND paletteEdit = CreateWindowExW(0, L"EDIT", ColorText(g_paletteColor).c_str(), WS_CHILD | WS_VISIBLE | ES_CENTER | ES_UPPERCASE,
                Ui(408), Ui(288), Ui(116), Ui(28), window, reinterpret_cast<HMENU>(ID_PALETTE_EDIT), GetModuleHandleW(nullptr), nullptr);
            g_paletteCancelButton = CreateWindowExW(0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                Ui(268), Ui(430), Ui(126), Ui(46), window, reinterpret_cast<HMENU>(ID_PALETTE_CANCEL), GetModuleHandleW(nullptr), nullptr);
            g_paletteApplyButton = CreateWindowExW(0, L"BUTTON", L"应用此颜色", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
                Ui(406), Ui(430), Ui(128), Ui(46), window, reinterpret_cast<HMENU>(ID_PALETTE_APPLY), GetModuleHandleW(nullptr), nullptr);
            SendMessageW(paletteEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);
            SendMessageW(g_paletteCancelButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBody), TRUE);
            SendMessageW(g_paletteApplyButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBold), TRUE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            POINT point = {static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            for (const auto& hit : g_paletteHits) {
                if (PtInRect(&hit.first, point)) {
                    g_paletteColor = hit.second;
                    SetWindowTextW(GetDlgItem(window, ID_PALETTE_EDIT), ColorText(g_paletteColor).c_str());
                    InvalidateRect(window, nullptr, FALSE);
                    break;
                }
            }
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_PALETTE_EDIT && HIWORD(wParam) == EN_CHANGE) {
                wchar_t value[32];
                GetWindowTextW(reinterpret_cast<HWND>(lParam), value, 32);
                try {
                    g_paletteColor = ArgbToColor(ParseRgb(value));
                    g_paletteTextValid = true;
                    InvalidateRect(window, nullptr, FALSE);
                } catch (...) {
                    g_paletteTextValid = false;
                }
            } else if (id == ID_PALETTE_CANCEL) {
                DestroyWindow(window);
            } else if (id == ID_PALETTE_APPLY) {
                if (!g_paletteTextValid) {
                    MessageBoxW(window, L"请输入合法的十六进制颜色代码（如 #3A3A3A）。", L"颜色格式错误", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                if (g_paletteForDark) {
                    g_darkColor = g_paletteColor;
                    SetWindowTextW(g_darkEdit, ColorText(g_darkColor).c_str());
                } else {
                    g_lightColor = g_paletteColor;
                    SetWindowTextW(g_lightEdit, ColorText(g_lightColor).c_str());
                }
                ReadAndApplyColors(g_mainWindow);
                DestroyWindow(window);
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) DestroyWindow(window);
            return 0;
        case WM_DRAWITEM:
            DrawActionButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(window, &paint);
            RenderPalette(hdc, window);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_CTLCOLOREDIT:
            SetTextColor(reinterpret_cast<HDC>(wParam), ModernTheme::TextPrimary);
            SetBkColor(reinterpret_cast<HDC>(wParam), ModernTheme::CardBg);
            return reinterpret_cast<LRESULT>(g_editBrush);
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            g_paletteWindow = nullptr;
            EnableWindow(g_mainWindow, TRUE);
            SetForegroundWindow(g_mainWindow);
            return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static void OpenPalette(HWND owner, bool forDark) {
    if (g_paletteWindow) {
        SetForegroundWindow(g_paletteWindow);
        return;
    }
    g_paletteForDark = forDark;
    g_paletteColor = forDark ? g_darkColor : g_lightColor;
    g_paletteTextValid = true;
    RECT ownerRect;
    GetWindowRect(owner, &ownerRect);
    int width = Ui(560);
    int height = Ui(520);
    int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;
    EnableWindow(owner, FALSE);
    std::wstring title = forDark ? L"深色模式" : L"浅色模式";
    title += L"候选框选中背景";
    g_paletteWindow = CreateWindowExW(WS_EX_DLGMODALFRAME, kPaletteClass, title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, x, y, width, height, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!g_paletteWindow) {
        EnableWindow(owner, TRUE);
        return;
    }
    ShowWindow(g_paletteWindow, SW_SHOW);
    UpdateWindow(g_paletteWindow);
}

static void SetScrollPosition(HWND window, bool vertical, int position) {
    int bar = vertical ? SB_VERT : SB_HORZ;
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(window, bar, &info);
    int maximum = std::max(info.nMin, info.nMax - static_cast<int>(info.nPage) + 1);
    position = std::max(info.nMin, std::min(position, maximum));
    int previous = vertical ? g_scrollY : g_scrollX;
    if (position == previous) return;
    if (vertical) g_scrollY = position; else g_scrollX = position;
    info.fMask = SIF_POS;
    info.nPos = position;
    SetScrollInfo(window, bar, &info, TRUE);
    int dx = vertical ? 0 : previous - position;
    int dy = vertical ? previous - position : 0;
    ScrollWindowEx(window, dx, dy, nullptr, nullptr, nullptr, nullptr, SW_INVALIDATE | SW_SCROLLCHILDREN);
}

static void HandleScroll(HWND window, bool vertical, WPARAM wParam) {
    int bar = vertical ? SB_VERT : SB_HORZ;
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(window, bar, &info);
    int position = vertical ? g_scrollY : g_scrollX;
    switch (LOWORD(wParam)) {
        case SB_LINEUP: position -= Ui(32); break;
        case SB_LINEDOWN: position += Ui(32); break;
        case SB_PAGEUP: position -= static_cast<int>(info.nPage); break;
        case SB_PAGEDOWN: position += static_cast<int>(info.nPage); break;
        case SB_THUMBTRACK: position = info.nTrackPos; break;
        case SB_TOP: position = info.nMin; break;
        case SB_BOTTOM: position = info.nMax; break;
        default: return;
    }
    SetScrollPosition(window, vertical, position);
}

static LRESULT CALLBACK ModernWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            g_mainWindow = window;
            CreateGuiFonts();
            g_editBrush = CreateSolidBrush(ModernTheme::CardBg);

            g_darkEdit = CreateWindowExW(0, L"EDIT", L"#D4D4D4", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_UPPERCASE, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_DARK_EDIT), GetModuleHandleW(nullptr), nullptr);
            g_lightEdit = CreateWindowExW(0, L"EDIT", L"#FFD0E8", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_UPPERCASE, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_LIGHT_EDIT), GetModuleHandleW(nullptr), nullptr);
            g_darkPaletteButton = CreateWindowExW(0, L"BUTTON", L"打开色板", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_DARK_PALETTE), GetModuleHandleW(nullptr), nullptr);
            g_lightPaletteButton = CreateWindowExW(0, L"BUTTON", L"打开色板", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_LIGHT_PALETTE), GetModuleHandleW(nullptr), nullptr);
            g_resetButton = CreateWindowExW(0, L"BUTTON", L"恢复默认", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_BTN_RESET), GetModuleHandleW(nullptr), nullptr);
            g_toggleButton = CreateWindowExW(0, L"BUTTON", L"后台启停", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_BTN_TOGGLE_TASK), GetModuleHandleW(nullptr), nullptr);
            g_applyButton = CreateWindowExW(0, L"BUTTON", L"应用配色", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_DEFPUSHBUTTON, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_BTN_APPLY), GetModuleHandleW(nullptr), nullptr);
            LayoutMainControls();

            LoadColorsToGui();
            RefreshToggleButton(true);
            SetTimer(window, ID_TIMER_REFRESH, 2000, nullptr);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            POINT point = {static_cast<short>(LOWORD(lParam)) + g_scrollX, static_cast<short>(HIWORD(lParam)) + g_scrollY};
            if (PtInRect(&g_darkSegmentHit, point) || PtInRect(&g_lightSegmentHit, point)) {
                g_darkModeSelected = PtInRect(&g_darkSegmentHit, point) != FALSE;
                LayoutMainControls();
                InvalidateRect(window, nullptr, TRUE);
                return 0;
            }
            for (const auto& preset : g_hitPresets) {
                if (PtInRect(&preset.rect, point)) {
                    if (preset.isDark) {
                        g_darkColor = preset.color;
                        SetWindowTextW(g_darkEdit, ColorText(g_darkColor).c_str());
                    } else {
                        g_lightColor = preset.color;
                        SetWindowTextW(g_lightEdit, ColorText(g_lightColor).c_str());
                    }
                    ReadAndApplyColors(window);
                    break;
                }
            }
            return 0;
        }
        case WM_SIZE:
            UpdateScrollBars(window);
            LayoutMainControls();
            InvalidateRect(window, nullptr, TRUE);
            return 0;
        case WM_HSCROLL:
            HandleScroll(window, false, wParam);
            return 0;
        case WM_VSCROLL:
            HandleScroll(window, true, wParam);
            return 0;
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if ((id == ID_DARK_EDIT || id == ID_LIGHT_EDIT) && HIWORD(wParam) == EN_CHANGE) {
                wchar_t value[32];
                GetWindowTextW(reinterpret_cast<HWND>(lParam), value, 32);
                bool valid = true;
                try { ParseRgb(value); } catch (...) { valid = false; }
                if (id == ID_DARK_EDIT) g_darkTextValid = valid;
                else g_lightTextValid = valid;
                InvalidateRect(window, nullptr, FALSE);
            } else if (id == ID_BTN_APPLY) {
                ReadAndApplyColors(window);
            } else if (id == ID_DARK_PALETTE) {
                OpenPalette(window, true);
            } else if (id == ID_LIGHT_PALETTE) {
                OpenPalette(window, false);
            } else if (id == ID_BTN_RESET) {
                g_darkColor = kDefaultDarkColor;
                g_lightColor = kDefaultLightColor;
                SetWindowTextW(g_darkEdit, ColorText(g_darkColor).c_str());
                SetWindowTextW(g_lightEdit, ColorText(g_lightColor).c_str());
                ReadAndApplyColors(window);
            } else if (id == ID_BTN_TOGGLE_TASK) {
                bool enabled = false;
                TASK_STATE state = QueryTaskState(enabled);
                if (state == TASK_STATE_RUNNING) StopTask(); else StartTask();
                RefreshToggleButton(true);
                InvalidateStatusDot(window);
            }
            return 0;
        }
        case WM_TIMER:
            if (wParam == ID_TIMER_REFRESH) {
                if (RefreshToggleButton()) InvalidateStatusDot(window);
            }
            return 0;
        case WM_DRAWITEM:
            DrawActionButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(window, &paint);
            RECT clientRect;
            GetClientRect(window, &clientRect);
            int width = clientRect.right;
            int height = clientRect.bottom;
            HDC memory = CreateCompatibleDC(hdc);
            HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ oldBitmap = SelectObject(memory, bitmap);
            RenderModernUI(window, memory);
            BitBlt(hdc, 0, 0, width, height, memory, 0, 0, SRCCOPY);
            SelectObject(memory, oldBitmap);
            DeleteObject(bitmap);
            DeleteDC(memory);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_CTLCOLOREDIT:
            SetTextColor(reinterpret_cast<HDC>(wParam), ModernTheme::TextPrimary);
            SetBkColor(reinterpret_cast<HDC>(wParam), ModernTheme::CardBg);
            return reinterpret_cast<LRESULT>(g_editBrush);
        case WM_DESTROY:
            KillTimer(window, ID_TIMER_REFRESH);
            if (g_paletteWindow) DestroyWindow(g_paletteWindow);
            DeleteGuiFonts();
            DeleteObject(g_editBrush);
            g_mainWindow = nullptr;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static int GuiMode(HINSTANCE instance, int show) {
    HANDLE mutex = CreateMutexW(nullptr, FALSE, kGuiMutex);
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(kWindowClass, nullptr);
        if (existing) { ShowWindow(existing, SW_RESTORE); SetForegroundWindow(existing); }
        CloseHandle(mutex);
        return 0;
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok) {
        CoUninitialize();
        if (mutex) CloseHandle(mutex);
        MessageBoxW(nullptr, L"无法初始化 GDI+ 图形组件。", L"WeType", MB_OK | MB_ICONERROR);
        return 1;
    }

    HICON appIcon = reinterpret_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WETYPE), IMAGE_ICON, Ui(32), Ui(32), LR_DEFAULTCOLOR));
    if (!appIcon) appIcon = LoadIconW(nullptr, IDI_APPLICATION);
    HICON appIconSmall = reinterpret_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WETYPE), IMAGE_ICON, Ui(16), Ui(16), LR_DEFAULTCOLOR));
    if (!appIconSmall) appIconSmall = appIcon;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ModernWindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = appIcon;
    wc.hIconSm = appIconSmall;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);

    WNDCLASSEXW paletteClass{};
    paletteClass.cbSize = sizeof(paletteClass);
    paletteClass.lpfnWndProc = PaletteWindowProc;
    paletteClass.hInstance = instance;
    paletteClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    paletteClass.hIcon = appIcon;
    paletteClass.hIconSm = appIconSmall;
    paletteClass.hbrBackground = nullptr;
    paletteClass.lpszClassName = kPaletteClass;
    RegisterClassExW(&paletteClass);

    HWND window = CreateWindowExW(0, kWindowClass, L"WeType 外观控制", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, Ui(920), Ui(670), nullptr, nullptr, instance, nullptr);
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (control && message.message == WM_MOUSEWHEEL && !g_paletteWindow) {
            short delta = static_cast<short>(HIWORD(message.wParam));
            ApplyZoom(window, g_userZoom + (delta > 0 ? 0.10 : -0.10));
            continue;
        }
        if (!control && message.message == WM_MOUSEWHEEL && !g_paletteWindow) {
            short delta = static_cast<short>(HIWORD(message.wParam));
            SetScrollPosition(window, true, g_scrollY + (delta > 0 ? -Ui(96) : Ui(96)));
            continue;
        }
        if (control && message.message == WM_KEYDOWN && !g_paletteWindow) {
            if (message.wParam == VK_ADD || message.wParam == VK_OEM_PLUS) {
                ApplyZoom(window, g_userZoom + 0.10);
                continue;
            }
            if (message.wParam == VK_SUBTRACT || message.wParam == VK_OEM_MINUS) {
                ApplyZoom(window, g_userZoom - 0.10);
                continue;
            }
            if (message.wParam == L'0') {
                ApplyZoom(window, 1.0);
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    if (appIconSmall && appIconSmall != appIcon) DestroyIcon(appIconSmall);
    if (appIcon && appIcon != LoadIconW(nullptr, IDI_APPLICATION)) DestroyIcon(appIcon);
    CoUninitialize();
    if (mutex) CloseHandle(mutex);
    return static_cast<int>(message.wParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int show) {
    SetProcessDPIAware();
    g_baseDir = GetExeDirectory();
    g_iniPath = g_baseDir + L"\\WeTypeSkin.ini";
    if (wcsstr(commandLine, L"--watch")) return WatchMode();
    if (wcsstr(commandLine, L"--patch-once")) {
        Config config = LoadConfig();
        PatchState state = LoadState();
        DWORD pid = FindRendererPid();
        return pid && PatchRenderer(pid, config, state, true) ? 0 : 1;
    }
    if (wcsstr(commandLine, L"--task-start") || wcsstr(commandLine, L"--task-stop")) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool success = wcsstr(commandLine, L"--task-start") ? StartTask() : StopTask();
        CoUninitialize();
        return success ? 0 : 1;
    }
    HDC screen = GetDC(nullptr);
    if (screen) {
        g_dpiScale = std::max(1.0, GetDeviceCaps(screen, LOGPIXELSX) / 96.0);
        ReleaseDC(nullptr, screen);
    }
    return GuiMode(instance, show);
}
