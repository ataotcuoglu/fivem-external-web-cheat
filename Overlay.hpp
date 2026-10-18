static const float CLEAR_R = 0.0f;
static const float CLEAR_G = 0.0f;
static const float CLEAR_B = 2.0f / 255.0f;
static const float CLEAR_A = 1.0f;


static char s_overlayClassName[20] = {};
static const char* GetOverlayClassName() {
    if (!s_overlayClassName[0]) {
        s_overlayClassName[0]='P'; s_overlayClassName[1]='r'; s_overlayClassName[2]='e';
        s_overlayClassName[3]='s'; s_overlayClassName[4]='e'; s_overlayClassName[5]='n';
        s_overlayClassName[6]='t'; s_overlayClassName[7]='e'; s_overlayClassName[8]='r';
        s_overlayClassName[9]='C'; s_overlayClassName[10]='o'; s_overlayClassName[11]='r';
        s_overlayClassName[12]='e'; s_overlayClassName[13]='H'; s_overlayClassName[14]='o';
        s_overlayClassName[15]='s'; s_overlayClassName[16]='t'; s_overlayClassName[17]=0;
    }
    return s_overlayClassName;
}

static struct OverlayStruct {
    HWND                    hWnd                  = nullptr;
    WNDCLASSEX              wndClassEx            = {};
    LPCSTR                  lpClassName           = nullptr;   
    LPCSTR                  lpWindowName          = "";
    ID3D11Device*           pDevice               = nullptr;
    ID3D11DeviceContext*    pDeviceContext         = nullptr;
    IDXGISwapChain*         pSwapChain             = nullptr;
    UINT                    resizeWidth            = 0;
    UINT                    resizeHeight           = 0;
    ID3D11RenderTargetView* pMainRenderTargetView  = nullptr;
    POINT                   WindowSize             = {};
    POINT                   ScreenSize             = {};
} Overlay;

bool CreateOverlayWindow();
bool CreateDeviceD3D();
void CreateRenderTarget();
void UpdateOverlay();
LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
void CleanupRenderTarget();
void CleanupOverlayWindow();
void CleanupDeviceD3D();
void CleanupImGui();


static void ApplyOverlayAttributes() {
    if (!Overlay.hWnd) return;
  
    LONG ex = GetWindowLong(Overlay.hWnd, GWL_EXSTYLE);
    ex |= WS_EX_TRANSPARENT;
    SetWindowLong(Overlay.hWnd, GWL_EXSTYLE, ex);
    SetWindowPos(Overlay.hWnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    /
    SetLayeredWindowAttributes(Overlay.hWnd, RGB(0, 0, 2), 0, LWA_COLORKEY);
}

void InitImGui() {
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
        MonitorFromPoint(POINT{0,0}, MONITOR_DEFAULTTOPRIMARY));
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiPlugins(Overlay.pDevice);
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui_ImplWin32_Init(Overlay.hWnd);
    ImGui_ImplDX11_Init(Overlay.pDevice, Overlay.pDeviceContext);
}

int OverlayMain(int& errorId) {
    if (!CreateOverlayWindow()) { CleanupOverlayWindow(); errorId = 1; return 1; }
    if (!CreateDeviceD3D())     { CleanupDeviceD3D(); CleanupOverlayWindow(); errorId = 2; return 1; }

    std::thread([]() { UpdateOverlay(); }).detach();

    InitImGui();
    ShowWindow(Overlay.hWnd, SW_SHOWNOACTIVATE);
    ApplyOverlayAttributes();

    while (keepRunning) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) keepRunning = false;
        }
        if (!keepRunning) break;

        if (screenSizeChanged || sizeChanged) {
            CleanupImGui(); CleanupDeviceD3D(); CleanupOverlayWindow();
            if (!CreateOverlayWindow()) { errorId = 1; break; }
            Sleep(500);
            if (!CreateDeviceD3D())     { errorId = 2; break; }
            InitImGui();
            ApplyOverlayAttributes();
            screenSizeChanged = sizeChanged = false;
            continue;
        }

        if (Overlay.resizeWidth && Overlay.resizeHeight) {
            CleanupRenderTarget();
            Overlay.pSwapChain->ResizeBuffers(0, Overlay.resizeWidth, Overlay.resizeHeight,
                                              DXGI_FORMAT_UNKNOWN, 0);
            Overlay.resizeWidth = Overlay.resizeHeight = 0;
            CreateRenderTarget();
        }

        if (GetAsyncKeyState(VK_END) & 0x8000)
            keepRunning = false;

        if (Cheats::Settings::PanicKey &&
            (GetAsyncKeyState(Cheats::Settings::PanicKey) & 1)) {
            extern void RestoreSilent();
            try { RestoreSilent(); } catch(...) {}
            if (Game.hProcess) { CloseHandle(Game.hProcess); Game.hProcess = nullptr; }
            HWND hCon = GetConsoleWindow();
            if (hCon) ShowWindow(hCon, SW_HIDE);
            TerminateProcess(GetCurrentProcess(), 0);
        }

        
        RenderImGui(Overlay.hWnd, false);

        const float cc[4] = { CLEAR_R, CLEAR_G, CLEAR_B, CLEAR_A };  
        Overlay.pDeviceContext->OMSetRenderTargets(1, &Overlay.pMainRenderTargetView, NULL);
        Overlay.pDeviceContext->ClearRenderTargetView(Overlay.pMainRenderTargetView, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = Overlay.pSwapChain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
        if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED)
            sizeChanged = true;   
        mainReady = true;

        static DWORD tLast = GetTickCount();
        DWORD tNow = GetTickCount();
        if (tNow - tLast < 8) Sleep(8 - (tNow - tLast));
        tLast = GetTickCount();
    }

    CleanupImGui();
    CleanupDeviceD3D();
    CleanupOverlayWindow();
    return 0;
}

bool CreateOverlayWindow() {
    Overlay.ScreenSize = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

   
    Overlay.lpClassName = GetOverlayClassName();

    ZeroMemory(&Overlay.wndClassEx, sizeof(Overlay.wndClassEx));
    Overlay.wndClassEx.cbSize        = sizeof(WNDCLASSEX);
    Overlay.wndClassEx.lpfnWndProc   = WndProc;
    Overlay.wndClassEx.hInstance     = GetModuleHandle(NULL);
    Overlay.wndClassEx.lpszClassName = Overlay.lpClassName;
    Overlay.wndClassEx.lpszMenuName  = Overlay.lpWindowName;
    if (!RegisterClassEx(&Overlay.wndClassEx)) return false;

    Overlay.WindowSize = Overlay.ScreenSize;


    Overlay.hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        Overlay.lpClassName, Overlay.lpWindowName, WS_POPUP,
        0, 0, Overlay.ScreenSize.x, Overlay.ScreenSize.y,
        NULL, NULL, Overlay.wndClassEx.hInstance, NULL);
    if (!Overlay.hWnd) return false;


    SetLayeredWindowAttributes(Overlay.hWnd, RGB(0, 0, 2), 0, LWA_COLORKEY);
    return true;
}

bool CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;

    sd.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = 0;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = Overlay.hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL fla[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    const UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, deviceFlags, fla, 2, D3D11_SDK_VERSION, &sd,
        &Overlay.pSwapChain, &Overlay.pDevice, &fl, &Overlay.pDeviceContext);
    if (FAILED(res))
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP,
            nullptr, deviceFlags, fla, 2, D3D11_SDK_VERSION, &sd,
            &Overlay.pSwapChain, &Overlay.pDevice, &fl, &Overlay.pDeviceContext);
    if (FAILED(res)) return false;
    CreateRenderTarget();
    return true;
}

void CreateRenderTarget() {
    ID3D11Texture2D* bb = nullptr;
    Overlay.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&bb));
    if (bb) {
        Overlay.pDevice->CreateRenderTargetView(bb, nullptr, &Overlay.pMainRenderTargetView);
        bb->Release();
    }
}

void UpdateOverlay() {
    ShowWindow(Overlay.hWnd, SW_SHOWNOACTIVATE);

    SetWindowPos(Overlay.hWnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    UpdateWindow(Overlay.hWnd);

    while (keepRunning) {
        Sleep(500);
        if (!mjLib::Process::Check(Game.pID) || !FindWindowA(BuildClassName(), NULL)) {
            keepRunning = false; break;
        }
       
        SetWindowPos(Overlay.hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        SetWindowDisplayAffinity(Overlay.hWnd,
            Cheats::Settings::StreamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        if ((wParam & 0xfff0) == SC_CLOSE)   return 0;
        break;
    case WM_DESTROY:
        if (!sizeChanged && !screenSizeChanged) PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CleanupRenderTarget() {
    if (Overlay.pMainRenderTargetView) {
        Overlay.pMainRenderTargetView->Release();
        Overlay.pMainRenderTargetView = nullptr;
    }
}
void CleanupOverlayWindow() {
    if (Overlay.hWnd && IsWindow(Overlay.hWnd)) { DestroyWindow(Overlay.hWnd); Overlay.hWnd = nullptr; }
    if (Overlay.wndClassEx.hInstance) {
        UnregisterClass(Overlay.wndClassEx.lpszClassName, Overlay.wndClassEx.hInstance);
        Overlay.wndClassEx.hInstance = nullptr;
    }
}
void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (Overlay.pSwapChain)    { Overlay.pSwapChain->Release();    Overlay.pSwapChain    = nullptr; }
    if (Overlay.pDeviceContext){ Overlay.pDeviceContext->Release(); Overlay.pDeviceContext = nullptr; }
    if (Overlay.pDevice)       { Overlay.pDevice->Release();       Overlay.pDevice       = nullptr; }
}
void CleanupImGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
