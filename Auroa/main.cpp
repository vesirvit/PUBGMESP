#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <thread>

#include "Tools.hpp"

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static int ScrX = 0, ScrY = 0;

//CheatDatas
float matrix[16];
UINT64 UnrealEngine = NULL;

struct sESP {
    bool Line;
    bool Bone;
    bool Box;
    bool Distance;
    bool Name;
    bool Health;
    bool IgnoreBots;
};

struct sAim {
    bool Enable;
    float FovSize;
    int AimPos;
};

struct sMemory {
    bool 聚点;
    bool 全自动;
};

struct sConifig {
    sESP X;
    sAim Aim;
    sMemory MEM;
} Config{};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline ImVec2 InWindow(int x, int y) {
    return ImVec2{(float)(ScrX + x), (float)(ScrY + y)};
}

void ESPView(ImDrawList* draw) {
    draw->AddText(InWindow(40, 40), ImColor(255, 0, 0), "@beijigua");
    UINT64 libbase = UnrealEngine + 0x9FF2000;
    UINT64 UWorld = GetPtr(GetPtr(GetPtr(libbase + 0x410E20) + 0x20) + 0x340);
    if (!UWorld) {
        char temp[32];
        sprintf_s(temp, "大厅");
        ImVec2 TextSize = ImGui::CalcTextSize(temp);
        draw->AddRectFilled(InWindow(px - TextSize.x * 0.9, 90 - TextSize.y * 0.6), InWindow(px + TextSize.x * 0.9, 90 + TextSize.y * 0.6), ImColor(10, 255, 10, 110), 12);
        draw->AddRect(InWindow(px - TextSize.x * 0.9, 90 - TextSize.y * 0.6), InWindow(px + TextSize.x * 0.9, 90 + TextSize.y * 0.6), ImColor(0, 255, 0, 190), 12, 0, 2.5f);
        draw->AddText(InWindow(px - TextSize.x * 0.5, 80), ImColor(255, 255, 255), temp);
        return;
    } 
    
    UINT64 Matrix = GetPtr(GetPtr(libbase + 0x410E20) + 0x7C) + 0x510;
    ReadVM(Matrix, &matrix, 64);

    UINT64 MySelf = UWorld;

    UINT64 ArraryPtr = GetPtr(MySelf + 0x18);
    UINT64 ArraryList = GetPtr(ArraryPtr + 0x70);
    DWORD ArraryCount = GetDWORD(ArraryPtr + 0x74);

    DWORD MyTeamID = GetDWORD(MySelf + 0x6D8);
    DWORD ActorCount = 0;
    DWORD PlayerCount = 0;
    DWORD BotCount = 0;

    ImColor LineColor = {};

    FVector MyPos = {};
    ReadVM(GetPtr(MySelf + 0x158) + 0x120, &MyPos, 12);

    FVector TargetPos = {};
    uintptr_t Target = NULL;
    float OldW = 99999.0f;

    for (int i = 0; i < ArraryCount; i++) {
        UINT64 ObjAddr = GetPtr(ArraryList + (i * 4));
        if (GetFloat(ObjAddr + 0x20A4) != 479.5f)
            continue;
        if (GetDWORD(ObjAddr + 0x6d8) == MyTeamID)
            continue;
        if (GetBool(ObjAddr + 0xA60))
            continue;
        if (GetBool(ObjAddr + 0x60))
            continue;

        bool bEnsure = GetBool(ObjAddr + 0x771);
        float Health = GetFloat(ObjAddr + 0xA48);
        float HealthMax = GetFloat(ObjAddr + 0xA4C);

        if (Config.X.IgnoreBots) {
            if (bEnsure) {
                continue;
            }
        }

        FVector ObjPos;
        ReadVM(GetPtr(ObjAddr + 0x158) + 0x120, &ObjPos, 12);

        float camera = matrix[3] * ObjPos.X + matrix[7] * ObjPos.Y + matrix[11] * ObjPos.Z + matrix[15];

        float r_x = px + (matrix[0] * ObjPos.X + matrix[4] * ObjPos.Y + matrix[8] * ObjPos.Z + matrix[12]) / camera * px;
        float r_y = py - (matrix[1] * ObjPos.X + matrix[5] * ObjPos.Y + matrix[9] * (ObjPos.Z - 5) + matrix[13]) / camera * py;
        float r_w = py - (matrix[1] * ObjPos.X + matrix[5] * ObjPos.Y + matrix[9] * (ObjPos.Z + 105) + matrix[13]) / camera * py;

        float X = r_x - (r_y - r_w) / 4;
        float Y = r_y;
        float W = (r_y - r_w) / 2;

        float MIDDLE = X + W / 2;
        float BOTTOM = Y + W * 1.5f;
        float TOP = Y - W * 1.75f;

        float left = (X + W / 2) - W;
        float right = (X + W / 2) + W;

        float Distance = sqrtf(powf(ObjPos.X - MyPos.X, 2) + powf(ObjPos.Y - MyPos.Y, 2) + powf(ObjPos.Z - MyPos.Z, 2)) * 0.01f;
        float ScrDistance = sqrtf(powf(r_x - px, 2) + powf(r_y - py, 2));


        UINT64 Mesh = GetPtr(ObjAddr + 0x380);
        UINT64 human = Mesh + 0x1A0;
        UINT64 Bone = GetPtr(Mesh + 0x6f4) + 0x30;

        //头部
        FVector relLocation = getBoneXYZ(human, Bone, 5);
        ImVec2 头 = WorldToScreen(relLocation, matrix, camera);

        // 胸部
        FVector relLocation1 = getBoneXYZ(human, Bone, 4);
        ImVec2 胸 = WorldToScreen(relLocation1, matrix, camera);
        // 盆骨
        FVector relLocation2 = getBoneXYZ(human, Bone, 1);
        ImVec2 盆骨 = WorldToScreen(relLocation2, matrix, camera);
        // 左肩膀
        FVector relLocation3 = getBoneXYZ(human, Bone, 11);
        ImVec2 左肩 = WorldToScreen(relLocation3, matrix, camera);
        // 右肩膀
        FVector relLocation4 = getBoneXYZ(human, Bone, 32);
        ImVec2 右肩 = WorldToScreen(relLocation4, matrix, camera);
        // 左手肘
        FVector relLocation5 = getBoneXYZ(human, Bone, 12);
        ImVec2 左手肘 = WorldToScreen(relLocation5, matrix, camera);
        // 右手肘
        FVector relLocation6 = getBoneXYZ(human, Bone, 33);
        ImVec2 右手肘 = WorldToScreen(relLocation6, matrix, camera);
        // 左手腕
        FVector relLocation7 = getBoneXYZ(human, Bone, 63);
        ImVec2 左手腕 = WorldToScreen(relLocation7, matrix, camera);
        // 右手腕
        FVector relLocation8 = getBoneXYZ(human, Bone, 62);
        ImVec2 右手腕 = WorldToScreen(relLocation8, matrix, camera);
        // 左大腿
        FVector relLocation9 = getBoneXYZ(human, Bone, 52);
        ImVec2 左大腿 = WorldToScreen(relLocation9, matrix, camera);
        // 右大腿
        FVector relLocation10 = getBoneXYZ(human, Bone, 56);;
        ImVec2 右大腿 = WorldToScreen(relLocation10, matrix, camera);
        // 左膝盖
        FVector relLocation11 = getBoneXYZ(human, Bone, 53);
        ImVec2 左膝盖 = WorldToScreen(relLocation11, matrix, camera);
        // 右膝盖
        FVector relLocation12 = getBoneXYZ(human, Bone, 57);
        ImVec2 右膝盖 = WorldToScreen(relLocation12, matrix, camera);
        // 左脚腕
        FVector relLocation13 = getBoneXYZ(human, Bone, 54);
        ImVec2 左脚踝 = WorldToScreen(relLocation13, matrix, camera);
        // 右脚腕
        FVector relLocation14 = getBoneXYZ(human, Bone, 58);
        ImVec2 右脚踝 = WorldToScreen(relLocation14, matrix, camera);

        if (Distance < 450.0f) {
            if (bEnsure) {
                LineColor = ImColor(255, 255, 255);
                BotCount++;
            } else {
                LineColor = ImColor(255, 0, 0);
                PlayerCount++;
            } ActorCount++;
        }

        if (W > 0) {
            if (Distance < 450.0f) {
                if (Config.X.Line) {
                    draw->AddLine(InWindow(px, py * 2), InWindow((left + right) / 2, BOTTOM), LineColor, 1.0f);
                } if (Config.X.Bone) {
                    if (!bEnsure) {
                        draw->AddLine(InWindow(头.x, 头.y), InWindow(胸.x, 胸.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(胸.x, 胸.y), InWindow(左肩.x, 左肩.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(左肩.x, 左肩.y), InWindow(左手肘.x, 左手肘.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(左手肘.x, 左手肘.y), InWindow(左手腕.x, 左手腕.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(胸.x, 胸.y), InWindow(右肩.x, 右肩.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(右肩.x, 右肩.y), InWindow(右手肘.x, 右手肘.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(右手肘.x, 右手肘.y), InWindow(右手腕.x, 右手腕.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(胸.x, 胸.y), InWindow(盆骨.x, 盆骨.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(盆骨.x, 盆骨.y), InWindow(左大腿.x, 左大腿.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(左大腿.x, 左大腿.y), InWindow(左膝盖.x, 左膝盖.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(左膝盖.x, 左膝盖.y), InWindow(左脚踝.x, 左脚踝.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(盆骨.x, 盆骨.y), InWindow(右大腿.x, 右大腿.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(右大腿.x, 右大腿.y), InWindow(右膝盖.x, 右膝盖.y), LineColor, 1.0f);
                        draw->AddLine(InWindow(右膝盖.x, 右膝盖.y), InWindow(右脚踝.x, 右脚踝.y), LineColor, 1.0f);
                    }
                } if (Config.X.Box) {
                    float 长度 = (right - left) / 3;
                    draw->AddLine(InWindow(left, TOP), InWindow(left + 长度, TOP), LineColor, 1.0f);
                    draw->AddLine(InWindow(left, TOP), InWindow(left, TOP + 长度), LineColor, 1.0f);
                    draw->AddLine(InWindow(right, TOP), InWindow(right - 长度, TOP), LineColor, 1.0f);
                    draw->AddLine(InWindow(right, TOP), InWindow(right, TOP + 长度), LineColor, 1.0f);
                    draw->AddLine(InWindow(left, BOTTOM), InWindow(left + 长度, BOTTOM), LineColor, 1.0f);
                    draw->AddLine(InWindow(left, BOTTOM), InWindow(left, BOTTOM - 长度), LineColor, 1.0f);
                    draw->AddLine(InWindow(right, BOTTOM), InWindow(right - 长度, BOTTOM), LineColor, 1.0f);
                    draw->AddLine(InWindow(right, BOTTOM), InWindow(right, BOTTOM - 长度), LineColor, 1.0f);
                } if (Config.X.Health) {
                    draw->AddRect(InWindow(((left + right) / 2) - 50.0f, TOP - 30.0f), InWindow(((left + right) / 2) + 50.0f, TOP - 10.0f), ImColor(0, 255, 0, 255), 8.0f, 0, 1.0f);
                    draw->AddRectFilled(InWindow(((left + right) / 2) - 50.0f, TOP - 30.0f), InWindow((((left + right) / 2) - 50.0f) + ((Health / HealthMax) * 100.0f), TOP - 10.0f), ImColor(0, 255, 0, 120), 8.0f, 0);
                } if (Config.X.Name) {
                    char PlayerName[28] = {};
                    if (bEnsure) {
                        sprintf_s(PlayerName, "机器人");
                    }
                    else {
                        GetFString(PlayerName, GetPtr(ObjAddr + 0x6b0));
                    } ImVec2 TextSize = ImGui::CalcTextSize(PlayerName);
                    draw->AddText(InWindow(((left + right) / 2) - (TextSize.x / 2), TOP - 30.0f), ImColor(255, 255, 255), PlayerName);
                } if (Config.X.Distance) {
                    char temp[32];
                    sprintf_s(temp, "%d M", (int)Distance);
                    ImVec2 TextSize = ImGui::CalcTextSize(temp);
                    draw->AddText(InWindow((right + left) / 2 - TextSize.x * 0.5, BOTTOM), ImColor(255, 255, 255), temp);
                }
            } if (Config.Aim.Enable) {
                if (ScrDistance < Config.Aim.FovSize) {
                    if (ScrDistance < OldW) {
                        OldW = ScrDistance;
                        Target = ObjAddr;
                        if (Config.Aim.AimPos == 0) {
                            TargetPos = relLocation1;
                        } else if (Config.Aim.AimPos == 1) {
                            TargetPos = relLocation2;
                        } else if (Config.Aim.AimPos == 2) {
                            TargetPos = relLocation3;
                        }
                    }
                }
            }
        }
    } if (PlayerCount || BotCount) {
        char temp[64] = {};
        sprintf_s(temp, "玩家:%d  机器人:%d", PlayerCount, BotCount);
        ImVec2 TextSize = ImGui::CalcTextSize(temp);
        draw->AddRectFilled(InWindow(px - TextSize.x * 0.7, 90 - TextSize.y * 0.5), InWindow(px + TextSize.x * 0.7, 90 + TextSize.y * 1.3), ImColor(255, 255, 10, 110), 12);
        draw->AddRect(InWindow(px - TextSize.x * 0.7, 90 - TextSize.y * 0.5), InWindow(px + TextSize.x * 0.7, 90 + TextSize.y * 1.2), ImColor(255, 255, 0, 190), 12, 0, 2.5f);
        draw->AddText(InWindow(px - TextSize.x * 0.5, 90), ImColor(255, 255, 255), temp);
    } if (Config.Aim.Enable) {
        if (Target) {
            UINT64 CameraAddr = GetPtr(GetPtr(GetPtr(libbase + 0x410E20) + 0x20) + 0x3A0) + 0x3C0;
            FVector Start = {};
            driver->ReadVM(GameLoop, (PVOID)CameraAddr, &Start, 12);

            FVector Movement = {};
            UINT64 CurrentVehicle = GetPtr(Target + 0xA80);
            if (CurrentVehicle) {
                driver->ReadVM(GameLoop, (PVOID)(CurrentVehicle + 0x80), &Movement, 12);
            } else {
                driver->ReadVM(GameLoop, (PVOID)(GetPtr(Target + 0x158) + 0x200), &Movement, 12);
            }

            float BulletFireSpeed = GetFloat(GetPtr(GetPtr(GetPtr(GetPtr(MySelf + 0x1C40) + 0x45C) + 0xBF0) + 0x200) + 0x3F4);
            float Distance = sqrtf(powf(TargetPos.X - Start.X, 2) + powf(TargetPos.Y - Start.Y, 2) + powf(TargetPos.Z - Start.Z, 2));

            float TimeToTraval = Distance / BulletFireSpeed;

            TargetPos.X = TargetPos.X + (TimeToTraval * Movement.X);
            TargetPos.Y = TargetPos.Y + (TimeToTraval * Movement.Y);
            TargetPos.Z = TargetPos.Z + (TimeToTraval * Movement.Z);

            FRotator rot = ToRotator(Start, TargetPos);
            UINT64 ControlRotation = GetPtr(GetPtr(libbase + 0x410E20) + 0x20) + 0x358;
            if (GetBool(MySelf + 0x1104)) {
                driver->WriteVM(GameLoop, (PVOID)ControlRotation, &rot, 12);
            }
        }
    }
}

void MemoryHacK() {
    //UINT64 libbase = UnrealEngine + 0x9FF2000;
    if (Config.MEM.聚点) {
        int Value = -509607936;
        driver->WriteVM(GameLoop, (PVOID)(UnrealEngine + 0x344B048), &Value, 4);
    }if (Config.MEM.全自动) {
        int Value = -476053501;
        driver->WriteVM(GameLoop, (PVOID)(UnrealEngine + 0x3446104), &Value, 4);
    }
}

void BYPASS() {
    int NOP = -509607936;
    int RET = -509546482;
    short int Bypass = 0xE032;
    driver->WriteVM(GameLoop, (PVOID)(GetAnogsHeader() + 0xC9DD6), &Bypass, 2);
}

// Main code
int main(int argc, const char** argv) {
    if (!driver->Connection(L"\\??\\AuroraProtect")) {
        MessageBoxW(NULL, L"驱动链接失败", L"Kernel Driver", MB_OK);
        return -1;
    } printf("Waiting for game loading.\n");

    //获取PID
    GameLoop = (HANDLE)GetAOWHANDLE();
    UnrealEngine = GetUEHeader();
    while (!UnrealEngine) {
        GameLoop = (HANDLE)GetAOWHANDLE();
        UnrealEngine = GetUEHeader();
        Sleep(100);
    } BYPASS();
    
    DeleteFileW(L"imgui.ini");
    ShowWindow(FindWindowW(L"ConsoleWindowClass", NULL), SW_HIDE);
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Arctic Fox Hax", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"AFoxEngine", WS_POPUP, 0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);//   ر   
    
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, 10);
    ::UpdateWindow(hwnd);

    ImGui_ImplWin32_EnableAlphaCompositing(hwnd);

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    //io.ConfigViewportsNoAutoMerge = true;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    HWND EmuWindow = ::GetDlgItem(FindWindowW(L"TXGuiFoundation", L"Gameloop"), NULL);
    RECT EmulatorWindowRect;

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    
    style.FrameRounding = 12;
    ImGui::StyleColorsLight();

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        } if (done)
            break;
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        } 
        
        static ImVec4  clear_color = ImVec4{ 0, 0, 0, 0 };

        {
            GetWindowRect(EmuWindow, &EmulatorWindowRect);
            ScrX = EmulatorWindowRect.left;
            ScrY = EmulatorWindowRect.top;
            px = (EmulatorWindowRect.right - EmulatorWindowRect.left) / 2;
            py = (EmulatorWindowRect.bottom - EmulatorWindowRect.top) / 2;
            MoveWindow(hwnd, ScrX, ScrY, (EmulatorWindowRect.right - EmulatorWindowRect.left), (EmulatorWindowRect.bottom - EmulatorWindowRect.top), TRUE);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        {
            //Draw ESP
            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            ESPView(draw);

            //Memory Hack
            MemoryHacK();
        }
        
        { //Draw Menu
            ImGuiWindowClass noAutoMerge;
            noAutoMerge.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
            ImGui::SetNextWindowClass(&noAutoMerge);
            ImGui::Begin("AnyHax");
            if (ImGui::CollapsingHeader("人物绘制")) {
                ImGui::Text("PLAYER ESP");
                    ImGui::Checkbox("Line", &Config.X.Line);
                    ImGui::Checkbox("Bone", &Config.X.Bone);
                    ImGui::Checkbox("Box", &Config.X.Box);
                    ImGui::Checkbox("Name", &Config.X.Name);
                    ImGui::Checkbox("Distance", &Config.X.Distance);
                    ImGui::Checkbox("Health", &Config.X.Health);
                    ImGui::Checkbox("Ignore Bots", &Config.X.IgnoreBots);
            } if (ImGui::CollapsingHeader("自瞄")) {
                ImGui::Text("自瞄");
                ImGui::Checkbox("启用", &Config.Aim.Enable);
                ImGui::SliderFloat("自瞄范围", &Config.Aim.FovSize, 0.0f, 300.0f, "%.f");
                ImGui::Combo("自瞄部位", &Config.Aim.AimPos, "头\0胸\0腰\0");
            } if (ImGui::CollapsingHeader("内存")) {
                ImGui::Text("MEMORY HACK");
                ImGui::Checkbox("聚点", &Config.MEM.聚点);
                ImGui::Checkbox("全自动", &Config.MEM.全自动);
            } if (ImGui::Button("EXIT")) {
                done = true;
            } ImGui::End();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        } g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    } return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
