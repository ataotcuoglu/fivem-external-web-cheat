#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <cstdio>

#include <mjLib/mjLib.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_internal.h>

#include "GuiGlobal.hpp"

bool keepRunning       = true;
bool isMenuVisible     = false;  
bool screenSizeChanged = false;
bool sizeChanged       = false;
bool mainReady         = false;


static HANDLE g_overlayThread = nullptr;

#include "Fonts.hpp"
#include "GuiItems.hpp"
#include "GameSDK.hpp"
#include "Config.hpp"
#include "Cheat.hpp"
#include "Gui.hpp"
#include "Overlay.hpp"
#include "WebServer.hpp"   


bool g_gameConnected  = false;   
static bool g_threadsStarted = false;

static void CheatLog(const char* msg) {
    FILE* f = nullptr;
    fopen_s(&f, "C:\\temp\\cheat_log.txt", "a");
    if (f) { fprintf(f, "[%lu] %s\n", GetCurrentProcessId(), msg); fclose(f); }
}

static bool IsGameAlive() {
    if (!Game.hProcess) return false;
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(Game.hProcess, &exitCode)) return false;
    return exitCode == STILL_ACTIVE;
}

static void DisconnectGame() {
    g_gameConnected = false;
    keepRunning = false;         
    if (g_overlayThread) {
        WaitForSingleObject(g_overlayThread, 4000);
        CloseHandle(g_overlayThread);
        g_overlayThread = nullptr;
    }

    if (Game.hProcess) {
        CloseHandle(Game.hProcess);
        Game.hProcess = nullptr;
    }
    Game.pID         = 0;
    Game.hWnd        = nullptr;
    Offsets.GameBase = 0;

    Sleep(200);       
    keepRunning = true; 
}


static void StartWorkerThreads() {
    if (g_threadsStarted) return;
    g_threadsStarted = true;

    std::thread([]() { NameCacheThread();  }).detach();
    std::thread([]() { UpdatePeds();       }).detach();
    std::thread([]() { UpdateVehicles();   }).detach();
    std::thread([]() { AimAssist();        }).detach();
    std::thread([]() { Exploits();         }).detach();
}


static void OverlayThreadBody() {
    __try {
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        ImGui_ImplWin32_EnableDpiAwareness();
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    int errorId = 0;
    OverlayMain(errorId);
}


static DWORD WINAPI OverlayThread(LPVOID) {
    __try {
        OverlayThreadBody();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
     
    }
    return 0;
}


static void HideDDNetBringFiveM() {
    Sleep(1500);
    HWND hFiveM = FindWindowA(BuildClassName(), nullptr);
    if (hFiveM) {
        HWND hFG = GetForegroundWindow();
        DWORD fgTid = hFG ? GetWindowThreadProcessId(hFG, nullptr) : 0;
        DWORD myTid = GetCurrentThreadId();
        if (fgTid && fgTid != myTid) AttachThreadInput(fgTid, myTid, TRUE);
        SetForegroundWindow(hFiveM);
        BringWindowToTop(hFiveM);
        if (fgTid && fgTid != myTid) AttachThreadInput(fgTid, myTid, FALSE);
        CheatLog("FiveM brought to foreground");
    }
}


static void CheatThreadBody() {
    std::thread([]() { WebServerThread(); }).detach();

    std::thread([]() { HideDDNetBringFiveM(); }).detach();

    Sleep(3000);

    while (true) {
        CheatLog("CheatThread: loop start");

        if (GetAsyncKeyState(VK_END) & 0x8000) {
            keepRunning = false;
            if (g_gameConnected) {
                RestoreSilent();
                DisconnectGame();
            }
            break;
        }

        bool grcOpen = (FindWindowA(BuildClassName(), NULL) != NULL);
        CheatLog(grcOpen ? "Game window found" : "Game window NOT found");

        if (g_gameConnected) {
            if (!grcOpen || !IsGameAlive())
                DisconnectGame();
        } else {
            if (grcOpen) {
                CheatLog("Game window open, attempting attach...");
                Sleep(2000);
                if (AttachToGameProcess() && FetchOffsetsAndVersion()) {
                    CheatLog("Attached to game successfully!");
                    g_gameConnected = true;
                    keepRunning     = true;

                    
                    if (Game.hWnd) {
                        RECT r = {};
                        if (GetClientRect(Game.hWnd, &r) && r.right > 0)
                            Game.lpRect = r;
                        if (!Game.lpRect.right) {
                            Game.lpRect.right  = GetSystemMetrics(SM_CXSCREEN);
                            Game.lpRect.bottom = GetSystemMetrics(SM_CYSCREEN);
                        }
                    }

                   
                    LoadConfig(CfgPath());
                    CheatLog("Config loaded.");

                    StartWorkerThreads();

                 
                    if (!g_overlayThread) {
                        CheatLog("Starting overlay thread...");
                        g_overlayThread = CreateThread(NULL, 0, OverlayThread, NULL, 0, NULL);
                    }

                    std::thread([]() {
                        Sleep(3000);
                        for (int i = 0; i < 60 && keepRunning; i++) {
                            uintptr_t gw = ReadMemory<uintptr_t>(Offsets.GameBase + Offsets.GameWorld);
                            if (gw) {
                                LocalPlayer.Pointer = ReadMemory<uintptr_t>(gw + Offsets.LocalPlayer);
                                if (LocalPlayer.Pointer) {
                                    int id = ReadMemory<int>(LocalPlayer.Pointer + Offsets.Id);
                                    if (id > 0 && id < 2048) { ScanCitizenNames(id); return; }
                                }
                            }
                            Sleep(1000);
                        }
                    }).detach();
                }
            }
        }

        Sleep(1000);
    }
}

static DWORD WINAPI CheatThread(LPVOID) {
    __try {
        CheatThreadBody();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
   
    }
    return 0;
}


static bool IsSafeEnvironment() {

    return true;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

      
        if (!IsSafeEnvironment()) return FALSE;


        {
            
            wchar_t mutPfx[] = { 'L','o','c','a','l','\\','R','P','C','S','S','_',0 };
            wchar_t mutName[64] = {};
            swprintf_s(mutName, 64, L"%s%lu", mutPfx, GetCurrentProcessId());

            HANDLE hExisting = OpenMutexW(SYNCHRONIZE, FALSE, mutName);
            if (hExisting) {
                CloseHandle(hExisting);
                return TRUE;  
            }

            CreateMutexW(nullptr, FALSE, mutName);
     
        }

        HANDLE h = CreateThread(NULL, 0, CheatThread, NULL, 0, NULL);
        if (h) CloseHandle(h);
    }
    return TRUE;
}
