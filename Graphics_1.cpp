#include <Windows.h>
#include <vector>
#include <cmath>

using namespace std;

vector<POINT> points;
bool starReady = false;

void DrawDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = max(abs(dx), abs(dy));
    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++)
    {
        SetPixel(hdc, round(x), round(y), color);
        x += xInc;
        y += yInc;
    }
}

LRESULT WINAPI WndProc(HWND hwnd, UINT mcode, WPARAM wp, LPARAM lp)
{
    HDC hdc;
    switch (mcode)
    {
    case WM_LBUTTONDOWN:
    {
        if (points.size() < 5)
        {
            POINT p;
            p.x = LOWORD(lp);
            p.y = HIWORD(lp);
            points.push_back(p);
        }
        if (points.size() == 5)
            starReady = true;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);
        if (starReady)
        {
            COLORREF colors[5] = {
                RGB(255, 0, 0),
                RGB(0, 255, 40),
                RGB(0, 0, 255),
                RGB(0, 0, 0),
                RGB(255, 127, 0)
            };
            int order[6] = {0, 2, 4, 1, 3, 0};
            for (int i = 0; i < 5; i++)
            {
                POINT p1 = points[order[i]];
                POINT p2 = points[order[i + 1]];
                DrawDDA(hdc, p1.x, p1.y, p2.x, p2.y, colors[i]);
            }
        }
        EndPaint(hwnd, &ps);
    }
    break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, mcode, wp, lp);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE h, HINSTANCE p, LPSTR c, int nsh)
{
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hInstance = h;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"myclass";
    wc.lpszMenuName = NULL;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&wc);
    HWND hwnd = CreateWindow(
        L"myclass",
        L"Star Shape - DDA",
        WS_OVERLAPPEDWINDOW,
        0, 0, 800, 600,
        NULL, NULL, h, 0);
    ShowWindow(hwnd, nsh);
    UpdateWindow(hwnd);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}