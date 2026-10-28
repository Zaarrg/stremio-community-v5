#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>
#include <thread>
#include <atomic>
#include <set>
#include <vector>
#include <chrono>
#include <filesystem>
#include <optional>
#include <wil/com.h>
#include "nlohmann/json.hpp"
#include <unordered_set>


#include "mpv/client.h"
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>

// For our JSON convenience
using json = nlohmann::json;

// -----------------------------------------------------------------------------
// App info
// -----------------------------------------------------------------------------
#define APP_TITLE "Stremio - Freedom to Stream"
#define APP_NAME  "Stremio"
#define APP_CLASS L"Stremio"
#define APP_VERSION "5.0.22"

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
extern TCHAR  szWindowClass[];
extern TCHAR  szTitle[];

extern HINSTANCE g_hInst;
extern HWND      g_hWnd;
extern HBRUSH    g_darkBrush;
extern HANDLE    g_hMutex;
extern HHOOK     g_hMouseHook;

extern std::vector<std::wstring> g_webuiUrls;
extern std::vector<std::wstring> g_domainWhitelist;
extern std::string  g_updateUrl;
extern std::wstring  g_extensionsDetailsUrl;
extern std::wstring  g_webuiUrl;

// Args
extern bool g_streamingServer;
extern bool g_autoupdaterForceFull;

// mpv
extern mpv_handle* g_mpv;
extern std::set<std::string> g_observedProps;
extern bool g_initialSet;
extern std::string g_initialVO;
extern int g_currentVolume;
extern const std::vector<std::wstring> g_subtitleExtensions;
// Security: default-deny allow-lists (lowercased, utf8)
extern std::unordered_set<std::string> g_mpvCommandAllowlist;
extern std::unordered_set<std::string> g_mpvSetPropAllowlist;

// custom messages
#define WM_MPV_WAKEUP (WM_APP + 2)
#define WM_TRAYICON   (WM_APP + 1)

// Node server
extern std::atomic_bool g_nodeRunning;
extern std::thread      g_nodeThread;
extern HANDLE           g_nodeProcess;
extern HANDLE           g_nodeOutPipe;
extern HANDLE           g_nodeInPipe;

// WebView2
extern wil::com_ptr<ICoreWebView2Controller4> g_webviewController;
extern wil::com_ptr<ICoreWebView2Profile8>    g_webviewProfile;
extern wil::com_ptr<ICoreWebView2_21>         g_webview;

// Tray IDs
#define ID_TRAY_SHOWWINDOW        1001
#define ID_TRAY_ALWAYSONTOP       1002
#define ID_TRAY_CLOSE_ON_EXIT     1003
#define ID_TRAY_USE_DARK_THEME    1004
#define ID_TRAY_PAUSE_MINIMIZED   1005
#define ID_TRAY_PAUSE_FOCUS_LOST  1006
#define ID_TRAY_PICTURE_IN_PICTURE 1007
#define ID_TRAY_QUIT             1008

struct MenuItem
{
    UINT id;
    bool checked;
    bool separator;
    std::wstring text;
};

extern std::vector<MenuItem> g_menuItems;
extern NOTIFYICONDATA  g_nid;
extern bool            g_showWindow;
extern bool            g_alwaysOnTop;
extern bool            g_isFullscreen;
extern bool            g_closeOnExit;
extern bool            g_useDarkTheme;
extern bool            g_isPipMode;
extern int             g_thumbFastHeight;
extern int             g_hoverIndex;
extern HFONT           g_hMenuFont;
extern HANDLE          g_serverJob;
extern HWND            g_trayHwnd;

// Ini Settings
extern bool g_pauseOnMinimize;
extern bool g_pauseOnLostFocus;
extern bool g_allowZoom;
extern bool g_isRpcOn;

// Tray sizes
extern int g_tray_itemH;
extern int g_tray_sepH;
extern int g_tray_w;
extern int g_font_height;

// Splash
extern HWND       g_hSplash;
extern HBITMAP    g_hSplashImage;
extern float      g_splashOpacity;
extern int        g_pulseDirection;
extern ULONG_PTR  g_gdiplusToken;

// App Ready and Event Queue
#define WM_NOTIFY_FLUSH (WM_USER + 101)
#define WM_REACHABILITY_DONE (WM_USER + 200)
extern std::vector<nlohmann::json> g_outboundMessages;
extern std::wstring g_launchProtocol;
extern std::atomic<bool> g_isAppReady;
extern std::atomic<bool> g_waitStarted;

// Extensions
extern std::map<std::wstring, std::wstring> g_extensionMap;
extern std::vector<std::wstring> g_scriptQueue;
extern std::atomic<int>          g_pendingExtensions;
extern std::optional<std::wstring> g_pendingUrl;
constexpr UINT_PTR TIMER_DEFERRED_NAV      = 0xB0B1;
constexpr UINT     DEFERRED_NAV_TIMEOUT_MS = 20000;

// Updater
extern std::atomic_bool       g_updaterRunning;
extern std::filesystem::path  g_installerPath;
extern std::thread            g_updaterThread;

extern const char* public_key_pem;

// Thumb Fast
extern std::atomic<bool> g_ignoreHover;
extern std::chrono::steady_clock::time_point g_ignoreUntil;
constexpr std::chrono::milliseconds IGNORE_DURATION(200);

// -----------------------------------------------------------------------------
// Functions declared here if you need them globally
// -----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#endif // GLOBALS_H
