#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")


struct ACCENTPOLICY { int nAccentState; int nFlags; int nColor; int nAnimationId; };
struct WINCOMPATTRDATA { int nAttribute; PVOID pData; ULONG ulDataSize; };

void EnableLiquidGlass(HWND hwnd) {
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetAttr)(HWND, WINCOMPATTRDATA*);
        pSetAttr SetAttr = (pSetAttr)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (SetAttr) {
            
            ACCENTPOLICY policy = { 4, 2, 0x40000000, 0 }; 
            WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
            SetAttr(hwnd, &data);
        }
    }
}


bool isDockActive = false;
HWND hwndDock = NULL;
HWND hwndStartMenu = NULL;
HWND hwndControlPanel = NULL;
bool isMenuVisible = false;
std::vector<HWND> openApps;
ULONG_PTR gdiplusToken;
int hoveredIndex = -1; 


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
            TextOutA(hdc, 50, 42, "Type here to search", 19);
            DeleteObject(searchBrush); DeleteObject(fontSearch);

            HFONT titleFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, titleFont); SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, 40, 100, "Pinned", 6);
            
            HICON hApp = LoadIcon(NULL, IDI_APPLICATION);
            int startX = 60; int startY = 140;
            for(int i = 0; i < 3; i++) {
                DrawIconEx(hdc, startX + (i * 120), startY, hApp, 32, 32, 0, NULL, DI_NORMAL);
                DrawIconEx(hdc, startX + (i * 120), startY + 100, hApp, 32, 32, 0, NULL, DI_NORMAL);
            }

            HPEN linePen = CreatePen(PS_SOLID, 1, RGB(60, 60, 65)); SelectObject(hdc, linePen);
            MoveToEx(hdc, 0, 580, NULL); LineTo(hdc, 600, 580); DeleteObject(linePen);

            HFONT fontUser = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Segoe UI");
            SelectObject(hdc, fontUser); TextOutA(hdc, 80, 605, "User Profile", 12);
            
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


LRESULT CALLBACK DockProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            openApps.clear(); EnumWindows(EnumWindowsProc, 0);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_ERASEBKGND:
            return 1; 

        case WM_MOUSEMOVE: {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int x = LOWORD(lParam);
            int totalWidth = (openApps.size() + 1) * 44;
            int startX = (sw - totalWidth) / 2;

            int newHover = -1;
            if (x >= startX && x <= startX + 44) newHover = 0; 
            else if (x > startX + 44 && x < startX + totalWidth) {
                newHover = 1 + (x - (startX + 44)) / 44;
            }

            if (newHover != hoveredIndex) {
                hoveredIndex = newHover;
                InvalidateRect(hwnd, NULL, FALSE); 
            }

            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            return 0;
        }

        case WM_MOUSELEAVE: {
            hoveredIndex = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            int sw = GetSystemMetrics(SM_CXSCREEN);
            
            
            HDC hdcMem = CreateCompatibleDC(hdc);
            BITMAPINFO bmi = {0};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = sw;
            bmi.bmiHeader.biHeight = -48; // Top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            void* pBits;
            HBITMAP hbmMem = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
            HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);

            Gdiplus::Graphics graphics(hdcMem);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

            
            graphics.Clear(Gdiplus::Color(100, 24, 24, 28));

            int iconSpacing = 44;
            int totalIcons = openApps.size() + 1; 
            int blockWidth = totalIcons * iconSpacing;
            int startX = (sw - blockWidth) / 2;

            
            if (hoveredIndex == 0) {
                Gdiplus::SolidBrush hoverBrush(Gdiplus::Color(40, 255, 255, 255));
                graphics.FillRectangle(&hoverBrush, startX + 2, 4, 40, 40);
            }

            
            int sx = startX + 10; int sy = 12; int gap = 1; int sqSize = 10;
            Gdiplus::SolidBrush bTopLeft(Gdiplus::Color(255, 61, 141, 227));
            Gdiplus::SolidBrush bTopRight(Gdiplus::Color(255, 46, 121, 211));
            Gdiplus::SolidBrush bBotLeft(Gdiplus::Color(255, 34, 109, 197));
            Gdiplus::SolidBrush bBotRight(Gdiplus::Color(255, 17, 82, 162));

            graphics.FillRectangle(&bTopLeft, sx, sy, sqSize, sqSize);
            graphics.FillRectangle(&bTopRight, sx + sqSize + gap, sy, sqSize, sqSize);
            graphics.FillRectangle(&bBotLeft, sx, sy + sqSize + gap, sqSize, sqSize);
            graphics.FillRectangle(&bBotRight, sx + sqSize + gap, sy + sqSize + gap, sqSize, sqSize);

            
            int currentX = startX + iconSpacing;
            HWND activeApp = GetForegroundWindow();
            for (int i = 0; i < openApps.size(); i++) {
                HWND app = openApps[i];
                
                
                if (hoveredIndex == i + 1) {
                    Gdiplus::SolidBrush hoverBrush(Gdiplus::Color(40, 255, 255, 255));
                    graphics.FillRectangle(&hoverBrush, currentX + 2, 4, 40, 40);
                }

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
                
                if (app == activeApp) {
                    Gdiplus::SolidBrush activeBrush(Gdiplus::Color(255, 0, 120, 215));
                    graphics.FillRectangle(&activeBrush, currentX + 12, 40, 12, 3);
                } else {
                    Gdiplus::SolidBrush inactiveBrush(Gdiplus::Color(255, 130, 130, 130));
                    graphics.FillRectangle(&inactiveBrush, currentX + 16, 40, 4, 3);
                }
                currentX += iconSpacing;
            }

           
            Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
            Gdiplus::Pen whitePen(Gdiplus::Color(255, 255, 255, 255), 1.5f);

            int wx = sw - 135; int wy = 26; 
            graphics.FillEllipse(&whiteBrush, wx - 2, wy, 4, 4); 
            graphics.DrawArc(&whitePen, wx - 6, wy - 4, 12, 12, 225, 90);
            graphics.DrawArc(&whitePen, wx - 10, wy - 8, 20, 20, 225, 90);
            Gdiplus::Pen whitePenThin(Gdiplus::Color(255, 255, 255, 255), 1.2f); 
            graphics.DrawArc(&whitePenThin, wx - 14, wy - 12, 28, 28, 225, 90);

            int vx = sw - 110; int vy = 16;
            graphics.FillRectangle(&whiteBrush, vx, vy + 4, 4, 6);
            Gdiplus::Point pts[3] = { Gdiplus::Point(vx + 4, vy + 4), Gdiplus::Point(vx + 9, vy), Gdiplus::Point(vx + 9, vy + 14) };
            graphics.FillPolygon(&whiteBrush, pts, 3);
            graphics.DrawArc(&whitePen, vx + 8, vy + 3, 6, 8, -90, 180); 
            graphics.DrawArc(&whitePen, vx + 8, vy, 10, 14, -90, 180);

            SYSTEMTIME st; GetLocalTime(&st);
            wchar_t timeWStr[10]; GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeWStr, sizeof(timeWStr) / sizeof(timeWStr[0]));
            
            Gdiplus::FontFamily fontFamily(L"Segoe UI");
            Gdiplus::Font hTimeFont(&fontFamily, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::PointF timePoint(sw - 80.0f, 15.0f);
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));
            graphics.DrawString(timeWStr, -1, &hTimeFont, timePoint, &textBrush);

            BitBlt(hdc, 0, 0, sw, 48, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int x = LOWORD(lParam);
            int totalWidth = (openApps.size() + 1) * 44;
            int startX = (sw - totalWidth) / 2;

            if (x >= startX && x <= startX + 44) ToggleStartMenu(); 
            else if (x > startX + 44 && x < startX + totalWidth) {
                int idx = (x - (startX + 44)) / 44;
                if (idx >= 0 && idx < openApps.size()) { 
                    if (IsIconic(openApps[idx])) ShowWindow(openApps[idx], SW_RESTORE);
                    SetForegroundWindow(openApps[idx]); 
                }
            }
            else if (x >= sw - 150 && x <= sw - 125) {
                ShellExecuteA(NULL, "open", "ms-availablenetworks:", NULL, NULL, SW_SHOWNORMAL);
            }
            else if (x >= sw - 120 && x <= sw - 95) {
                ShellExecuteA(NULL, "open", "sndvol.exe", NULL, NULL, SW_SHOWNORMAL);
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}


void ToggleSystem(bool turnOn) {
    if (turnOn) {
        ShowWindow(FindWindowA("Shell_TrayWnd", NULL), SW_HIDE);
        
        WNDCLASSA wc = {0}; wc.lpfnWndProc = DockProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = "Win11TaskbarClass";
        wc.hbrBackground = NULL; 
        RegisterClassA(&wc); 

        int sw = GetSystemMetrics(SM_CXSCREEN); int sh = GetSystemMetrics(SM_CYSCREEN);
        hwndDock = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            "Win11TaskbarClass", "Taskbar", WS_POPUP | WS_VISIBLE, 0, sh - 48, sw, 48, NULL, NULL, GetModuleHandle(NULL), NULL);

        SetLayeredWindowAttributes(hwndDock, 0, 240, LWA_ALPHA); 
        EnableLiquidGlass(hwndDock);
        SetTimer(hwndDock, 1, 500, NULL); 
    } else {
        if (hwndDock) { DestroyWindow(hwndDock); hwndDock = NULL; }
        if (hwndStartMenu) { DestroyWindow(hwndStartMenu); hwndStartMenu = NULL; isMenuVisible = false; }
        ShowWindow(FindWindowA("Shell_TrayWnd", NULL), SW_SHOW);
    }
}


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
            TextOutA(hdc, 25, 60, "Windows 11 Interface. Maximum Performance.", 42);
            
            HBRUSH toggleBrush = isDockActive ? CreateSolidBrush(RGB(0, 120, 215)) : CreateSolidBrush(RGB(60, 60, 60));
            SelectObject(hdc, toggleBrush); SelectObject(hdc, GetStockObject(NULL_PEN)); RoundRect(hdc, 25, 120, 85, 150, 30, 30); 
            HBRUSH circleBrush = CreateSolidBrush(RGB(255, 255, 255)); SelectObject(hdc, circleBrush);
            if (isDockActive) Ellipse(hdc, 57, 122, 83, 148); else Ellipse(hdc, 27, 122, 53, 148);              
            SelectObject(hdc, hFont); SetTextColor(hdc, isDockActive ? RGB(0, 120, 215) : RGB(120, 120, 120));
            
            if (isDockActive) TextOutA(hdc, 100, 120, "Taskbar Active", 14);
            else TextOutA(hdc, 100, 120, "Taskbar Inactive", 16);
            
            SetTextColor(hdc, RGB(0, 150, 255)); TextOutA(hdc, 25, 220, "Support (Open Gmail)", 20);
            
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
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSA wc = {0}; wc.lpfnWndProc = ControlPanelProc; wc.hInstance = hInst; wc.lpszClassName = "ControlPanelClass"; wc.hCursor = LoadCursor(NULL, IDC_HAND); RegisterClassA(&wc);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    hwndControlPanel = CreateWindowExA(0, "ControlPanelClass", "LiquidDock", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, (sw - 400) / 2, (sh - 300) / 2, 400, 300, NULL, NULL, hInst, NULL);
    ShowWindow(hwndControlPanel, SW_SHOW);
    
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
