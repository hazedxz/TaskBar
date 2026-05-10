#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// --- Estructuras para el efecto cristal ---
struct ACCENTPOLICY { int nAccentState; int nFlags; int nColor; int nAnimationId; };
struct WINCOMPATTRDATA { int nAttribute; PVOID pData; ULONG ulDataSize; };

void EnableLiquidGlass(HWND hwnd) {
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetAttr)(HWND, WINCOMPATTRDATA*);
        pSetAttr SetAttr = (pSetAttr)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (SetAttr) {
            // Tinte oscuro translúcido perfecto para Win11 Dark Mode
            ACCENTPOLICY policy = { 3, 0, 0x50151515, 0 }; 
            WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
            SetAttr(hwnd, &data);
        }
    }
}

// --- Variables Globales ---
bool isDockActive = false;
HWND hwndDock = NULL;
HWND hwndStartMenu = NULL;
HWND hwndControlPanel = NULL;
bool isMenuVisible = false;
std::vector<HWND> openApps;
ULONG_PTR gdiplusToken;

// --- Escáner de Aplicaciones ---
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd) && hwnd != hwndDock && hwnd != hwndStartMenu && hwnd != hwndControlPanel) {
        HWND owner = GetWindow(hwnd, GW_OWNER);
        int exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (owner == NULL && (exStyle & WS_EX_TOOLWINDOW) == 0) {
            char title[256]; GetWindowTextA(hwnd, title, sizeof(title));
            if (strlen(title) > 0 && strcmp(title, "Program Manager") != 0 && strcmp(title, "Settings") != 0) {
                openApps.push_back(hwnd);
            }
        }
    }
    return TRUE;
}

// --- Menú de Inicio (Win11 Replica) ---
LRESULT CALLBACK StartMenuProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            
            HBRUSH bgBrush = CreateSolidBrush(RGB(32, 32, 36)); 
            FillRect(hdc, &ps.rcPaint, bgBrush); DeleteObject(bgBrush);
            SetBkMode(hdc, TRANSPARENT);
            
            HBRUSH searchBrush = CreateSolidBrush(RGB(45, 45, 50));
            SelectObject(hdc, searchBrush); SelectObject(hdc, GetStockObject(NULL_PEN));
            RoundRect(hdc, 30, 30, 570, 70, 10, 10);
            HFONT fontSearch = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, fontSearch); SetTextColor(hdc, RGB(180, 180, 180));
            TextOutA(hdc, 50, 42, "Escribe aqui para buscar", 24);
            DeleteObject(searchBrush); DeleteObject(fontSearch);

            HFONT titleFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, titleFont); SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, 40, 100, "Anclado", 7);
            
            HICON hApp = LoadIcon(NULL, IDI_APPLICATION);
            int startX = 60; int startY = 140;
            for(int i = 0; i < 3; i++) {
                DrawIconEx(hdc, startX + (i * 120), startY, hApp, 32, 32, 0, NULL, DI_NORMAL);
                DrawIconEx(hdc, startX + (i * 120), startY + 100, hApp, 32, 32, 0, NULL, DI_NORMAL);
            }

            HPEN linePen = CreatePen(PS_SOLID, 1, RGB(60, 60, 65)); SelectObject(hdc, linePen);
            MoveToEx(hdc, 0, 580, NULL); LineTo(hdc, 600, 580); DeleteObject(linePen);

            HFONT fontUser = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, fontUser); TextOutA(hdc, 80, 605, "Usuario", 7);
            
            HBRUSH pwrBrush = CreateSolidBrush(RGB(220, 50, 50)); SelectObject(hdc, pwrBrush);
            RoundRect(hdc, 460, 600, 500, 630, 5, 5); DeleteObject(pwrBrush);
            SetTextColor(hdc, RGB(255, 255, 255)); TextOutA(hdc, 470, 607, "OFF", 3);

            DeleteObject(titleFont); DeleteObject(fontUser); EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam); int y = HIWORD(lParam);
            if (x >= 460 && x <= 500 && y >= 600 && y <= 630) system("shutdown /s /t 0");
            return 0;
        }
        case WM_KILLFOCUS:
            ShowWindow(hwnd, SW_HIDE); isMenuVisible = false; return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

void ToggleStartMenu() {
    if (!hwndStartMenu) {
        WNDCLASSA wc = {0}; wc.lpfnWndProc = StartMenuProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = "Win11MenuClass";
        RegisterClassA(&wc);
        int sw = GetSystemMetrics(SM_CXSCREEN); int sh = GetSystemMetrics(SM_CYSCREEN);
        int menuWidth = 600; int menuHeight = 650;
        hwndStartMenu = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            "Win11MenuClass", "Menu", WS_POPUP, (sw - menuWidth) / 2, sh - menuHeight - 60, menuWidth, menuHeight, NULL, NULL, GetModuleHandle(NULL), NULL);
        SetLayeredWindowAttributes(hwndStartMenu, 0, 240, LWA_ALPHA); EnableLiquidGlass(hwndStartMenu);
        HRGN hRgn = CreateRoundRectRgn(0, 0, menuWidth, menuHeight, 15, 15); SetWindowRgn(hwndStartMenu, hRgn, TRUE);
    }
    if (isMenuVisible) ShowWindow(hwndStartMenu, SW_HIDE);
    else { ShowWindow(hwndStartMenu, SW_SHOW); SetForegroundWindow(hwndStartMenu); SetFocus(hwndStartMenu); }
    isMenuVisible = !isMenuVisible;
}

// --- Barra de Tareas (Win11 Replica) ---
LRESULT CALLBACK DockProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            openApps.clear(); EnumWindows(EnumWindowsProc, 0);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            int sw = GetSystemMetrics(SM_CXSCREEN);
            
            // Fondo de la barra
            HBRUSH bgBrush = CreateSolidBrush(RGB(24, 24, 28)); 
            RECT fullRect = {0, 0, sw, 48}; 
            FillRect(hdc, &fullRect, bgBrush); DeleteObject(bgBrush);

            int iconSpacing = 44;
            int totalIcons = openApps.size() + 1; 
            int blockWidth = totalIcons * iconSpacing;
            int startX = (sw - blockWidth) / 2;

            // 1. Logo Win11 Original
            SelectObject(hdc, GetStockObject(NULL_PEN));
            HBRUSH bTopLeft = CreateSolidBrush(RGB(61, 141, 227));
            HBRUSH bTopRight = CreateSolidBrush(RGB(46, 121, 211));
            HBRUSH bBotLeft = CreateSolidBrush(RGB(34, 109, 197));
            HBRUSH bBotRight = CreateSolidBrush(RGB(17, 82, 162));

            int sx = startX + 10; int sy = 12;
            SelectObject(hdc, bTopLeft);  RoundRect(hdc, sx, sy, sx+10, sy+10, 2, 2);
            SelectObject(hdc, bTopRight); RoundRect(hdc, sx+12, sy, sx+22, sy+10, 2, 2);
            SelectObject(hdc, bBotLeft);  RoundRect(hdc, sx, sy+12, sx+10, sy+22, 2, 2);
            SelectObject(hdc, bBotRight); RoundRect(hdc, sx+12, sy+12, sx+22, sy+22, 2, 2);

            DeleteObject(bTopLeft); DeleteObject(bTopRight); DeleteObject(bBotLeft); DeleteObject(bBotRight);

            // GDI+ Engine para Alta Calidad
            Gdiplus::Graphics graphics(hdc);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            // 2. Dibujar Iconos de Apps con Cero Pixelado
            int currentX = startX + iconSpacing;
            HWND activeApp = GetForegroundWindow();
            for (HWND app : openApps) {
                HICON hIcon = (HICON)SendMessage(app, WM_GETICON, ICON_BIG, 0);
                if (!hIcon) hIcon = (HICON)GetClassLongPtr(app, GCLP_HICON);
                if (!hIcon) hIcon = (HICON)SendMessage(app, WM_GETICON, ICON_SMALL2, 0);
                if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
                
                if (hIcon) {
                    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromHICON(hIcon);
                    if (bmp) {
                        graphics.DrawImage(bmp, currentX + 6, 8, 24, 24);
                        delete bmp;
                    }
                }
                
                // Indicador visual
                if (app == activeApp) {
                    Gdiplus::SolidBrush activeBrush(Gdiplus::Color(255, 0, 120, 215));
                    graphics.FillRectangle(&activeBrush, currentX + 12, 40, 12, 3);
                } else {
                    Gdiplus::SolidBrush inactiveBrush(Gdiplus::Color(255, 130, 130, 130));
                    graphics.FillRectangle(&inactiveBrush, currentX + 16, 40, 4, 3);
                }
                currentX += iconSpacing;
            }

            // 3. System Tray (Wi-Fi, Volumen) Dibujados Vectorialmente con GDI+
            Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
            Gdiplus::Pen whitePen(Gdiplus::Color(255, 255, 255, 255), 1.5f);

            // Dibujo de Red/Wi-Fi Infalible
            int wx = sw - 135; int wy = 26; 
            graphics.FillEllipse(&whiteBrush, wx - 2, wy, 4, 4); // Punto central
            graphics.DrawArc(&whitePen, wx - 6, wy - 4, 12, 12, 225, 90);
            graphics.DrawArc(&whitePen, wx - 10, wy - 8, 20, 20, 225, 90);
            graphics.DrawArc(&whitePen, wx - 14, wy - 12, 28, 28, 225, 90);

            // Dibujo de Volumen Infalible
            int vx = sw - 110; int vy = 16;
            graphics.FillRectangle(&whiteBrush, vx, vy + 4, 4, 6);
            Gdiplus::Point pts[3] = { Gdiplus::Point(vx + 4, vy + 4), Gdiplus::Point(vx + 9, vy), Gdiplus::Point(vx + 9, vy + 14) };
            graphics.FillPolygon(&whiteBrush, pts, 3);
            graphics.DrawArc(&whitePen, vx + 8, vy + 3, 6, 8, -90, 180);
            graphics.DrawArc(&whitePen, vx + 8, vy, 10, 14, -90, 180);

            // Reloj Nítido
            SetBkMode(hdc, TRANSPARENT); 
            SetTextColor(hdc, RGB(255, 255, 255));
            SYSTEMTIME st; GetLocalTime(&st);
            char timeStr[10]; GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeStr, sizeof(timeStr));
            HFONT hTimeFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, hTimeFont);
            TextOutA(hdc, sw - 80, 15, timeStr, strlen(timeStr));
            DeleteObject(hTimeFont);

            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int x = LOWORD(lParam);
            int totalWidth = (openApps.size() + 1) * 44;
            int startX = (sw - totalWidth) / 2;

            // Hitbox Inicio
            if (x >= startX && x <= startX + 44) ToggleStartMenu(); 
            // Hitbox Apps
            else if (x > startX + 44 && x < startX + totalWidth) {
                int idx = (x - (startX + 44)) / 44;
                if (idx >= 0 && idx < openApps.size()) { 
                    if (IsIconic(openApps[idx])) ShowWindow(openApps[idx], SW_RESTORE);
                    SetForegroundWindow(openApps[idx]); 
                }
            }
            // Hitbox Wi-Fi
            else if (x >= sw - 150 && x <= sw - 125) {
                ShellExecuteA(NULL, "open", "ms-availablenetworks:", NULL, NULL, SW_SHOWNORMAL);
            }
            // Hitbox Volumen
            else if (x >= sw - 120 && x <= sw - 95) {
                ShellExecuteA(NULL, "open", "sndvol.exe", NULL, NULL, SW_SHOWNORMAL);
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// --- Encender / Apagar el Sistema ---
void ToggleSystem(bool turnOn) {
    if (turnOn) {
        ShowWindow(FindWindowA("Shell_TrayWnd", NULL), SW_HIDE);
        
        WNDCLASSA wc = {0}; wc.lpfnWndProc = DockProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = "Win11TaskbarClass";
        RegisterClassA(&wc); 

        int sw = GetSystemMetrics(SM_CXSCREEN); int sh = GetSystemMetrics(SM_CYSCREEN);
        hwndDock = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            "Win11TaskbarClass", "Taskbar", WS_POPUP | WS_VISIBLE, 0, sh - 48, sw, 48, NULL, NULL, GetModuleHandle(NULL), NULL);

        SetLayeredWindowAttributes(hwndDock, 0, 240, LWA_ALPHA); EnableLiquidGlass(hwndDock);
        SetTimer(hwndDock, 1, 500, NULL); 
    } else {
        if (hwndDock) { DestroyWindow(hwndDock); hwndDock = NULL; }
        if (hwndStartMenu) { DestroyWindow(hwndStartMenu); hwndStartMenu = NULL; isMenuVisible = false; }
        ShowWindow(FindWindowA("Shell_TrayWnd", NULL), SW_SHOW);
    }
}

// --- Panel de Control ---
LRESULT CALLBACK ControlPanelProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 24)); FillRect(hdc, &ps.rcPaint, bgBrush); DeleteObject(bgBrush);
            SetBkMode(hdc, TRANSPARENT); 
            
            HFONT hFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, hFont); SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, 25, 25, "LiquidDock", 10);
            
            HFONT hDescFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, hDescFont); SetTextColor(hdc, RGB(150, 150, 150));
            TextOutA(hdc, 25, 60, "Interfaz Windows 11. Rendimiento Maximo.", 40);
            
            HBRUSH toggleBrush = isDockActive ? CreateSolidBrush(RGB(0, 120, 215)) : CreateSolidBrush(RGB(60, 60, 60));
            SelectObject(hdc, toggleBrush); SelectObject(hdc, GetStockObject(NULL_PEN)); RoundRect(hdc, 25, 120, 85, 150, 30, 30); 
            HBRUSH circleBrush = CreateSolidBrush(RGB(255, 255, 255)); SelectObject(hdc, circleBrush);
            if (isDockActive) Ellipse(hdc, 57, 122, 83, 148); else Ellipse(hdc, 27, 122, 53, 148);              
            SelectObject(hdc, hFont); SetTextColor(hdc, isDockActive ? RGB(0, 120, 215) : RGB(120, 120, 120));
            TextOutA(hdc, 100, 120, isDockActive ? "Barra Activada" : "Barra Desactivada", isDockActive ? 14 : 17);
            
            SetTextColor(hdc, RGB(0, 150, 255)); TextOutA(hdc, 25, 220, "Soporte (Abrir Gmail)", 21);
            
            DeleteObject(toggleBrush); DeleteObject(circleBrush); DeleteObject(hFont); DeleteObject(hDescFont);
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int xPos = LOWORD(lParam); int yPos = HIWORD(lParam);
            if (xPos >= 25 && xPos <= 85 && yPos >= 120 && yPos <= 150) {
                isDockActive = !isDockActive; ToggleSystem(isDockActive); InvalidateRect(hwnd, NULL, TRUE); 
            }
            if (xPos >= 25 && xPos <= 180 && yPos >= 220 && yPos <= 245) {
                ShellExecuteA(NULL, "open", "https://mail.google.com/mail/?view=cm&fs=1&to=hazedxz6@gmail.com", NULL, NULL, SW_SHOWNORMAL);
            }
            return 0;
        }
        case WM_DESTROY: {
            if (isDockActive) ToggleSystem(false);
            PostQuitMessage(0); return 0;
        }
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Inicializar GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSA wc = {0}; wc.lpfnWndProc = ControlPanelProc; wc.hInstance = hInst; wc.lpszClassName = "ControlPanelClass"; wc.hCursor = LoadCursor(NULL, IDC_HAND); RegisterClassA(&wc);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    hwndControlPanel = CreateWindowExA(0, "ControlPanelClass", "LiquidDock", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, (sw - 400) / 2, (sh - 300) / 2, 400, 300, NULL, NULL, hInst, NULL);
    ShowWindow(hwndControlPanel, SW_SHOW);
    
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    
    // Apagar GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}