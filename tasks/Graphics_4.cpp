#include <windows.h>
#include <vector>
#include <cmath>

using namespace std;

vector<POINT> points;

bool polygonReady = false;
bool fillReady = false;
bool filled = false;

POINT seedPoint;

void DrawMidPointLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;
    dx = abs(dx);
    dy = abs(dy);
    int x = x1;
    int y = y1;
    SetPixel(hdc, x, y, color);
    if (dx >= dy)
    {
        int d = 2 * dy - dx;
        int dE = 2 * dy;
        int dNE = 2 * (dy - dx);
        for (int i = 0; i < dx; i++)
        {
            if (d <= 0)
            {
                d += dE;
                x += sx;
            }
            else
            {
                d += dNE;
                x += sx;
                y += sy;
            }
            SetPixel(hdc, x, y, color);
        }
    }
    else
    {
        int d = 2 * dx - dy;
        int dN = 2 * dx;
        int dNE = 2 * (dx - dy);
        for (int i = 0; i < dy; i++)
        {
            if (d <= 0)
            {
                d += dN;
                y += sy;
            }
            else
            {
                d += dNE;
                x += sx;
                y += sy;
            }
            SetPixel(hdc, x, y, color);
        }
    }
}

void FloodFill(HDC hdc, int x, int y, COLORREF boundaryColor, COLORREF fillColor)
{
    COLORREF c = GetPixel(hdc, x, y);
    if (c == boundaryColor || c == fillColor)
        return;
    SetPixel(hdc, x, y, fillColor);
    FloodFill(hdc, x + 1, y, boundaryColor, fillColor);
    FloodFill(hdc, x - 1, y, boundaryColor, fillColor);
    FloodFill(hdc, x, y + 1, boundaryColor, fillColor);
    FloodFill(hdc, x, y - 1, boundaryColor, fillColor);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT p;
        p.x = LOWORD(lParam);
        p.y = HIWORD(lParam);
        if (!polygonReady)
        {
            points.push_back(p);
            if (points.size() == 5)
                polygonReady = true;
        }
        else if (!filled)
        {
            seedPoint = p;
            filled = true;
        }
        
        InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        COLORREF borderColor = RGB(255, 255, 255);
        if (polygonReady)
        {
            for (int i = 0; i < 5; i++)
            {
                POINT p1 = points[i];
                POINT p2 = points[(i + 1) % 5];
                DrawMidPointLine(hdc, p1.x, p1.y, p2.x, p2.y, borderColor);
            }
        }
        EndPaint(hwnd, &ps);
        if (filled)
        {
            HDC hdcFill = GetDC(hwnd);
            FloodFill(hdcFill,
                      seedPoint.x,
                      seedPoint.y,
                      RGB(255, 255, 255),
                      RGB(0, 255, 0));
            ReleaseDC(hwnd, hdcFill);
        }
    }
    return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE,
                   LPSTR,
                   int nCmdShow)
{
    const char CLASS_NAME[] = "PolygonFloodFill";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Polygon + Flood Fill",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL,
        hInstance, NULL);

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}