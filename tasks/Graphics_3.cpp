#include <Windows.h>
#include <vector>
#include <cmath>

using namespace std;

vector<POINT> pts;

void DrawBezier(HDC hdc, POINT p0, POINT p1, POINT p2, POINT p3){
    for (double t = 0; t <= 1; t += 0.001)
    {
        double x =
            pow(1 - t, 3) * p0.x +
            3 * t * pow(1 - t, 2) * p1.x +
            3 * pow(t, 2) * (1 - t) * p2.x +
            pow(t, 3) * p3.x;
        double y =
            pow(1 - t, 3) * p0.y +
            3 * t * pow(1 - t, 2) * p1.y +
            3 * pow(t, 2) * (1 - t) * p2.y +
            pow(t, 3) * p3.y;
        SetPixel(hdc, (int)x, (int)y, RGB(255, 0, 0));
    }
}

LRESULT WINAPI WndProc(HWND hwnd, UINT mcode, WPARAM wp, LPARAM lp){
    HDC hdc;
    switch (mcode)
    {
    case WM_LBUTTONDOWN:
    {
        if (pts.size() < 4)
        {
            POINT p;
            p.x = LOWORD(lp);
            p.y = HIWORD(lp);
            pts.push_back(p);
        }
        if (pts.size() == 4)
        {
            hdc = GetDC(hwnd);
            DrawBezier(hdc, pts[0], pts[1], pts[2], pts[3]);
            ReleaseDC(hwnd, hdc);
        }
    }
    break;
    case WM_RBUTTONDOWN:
        pts.clear();
        InvalidateRect(hwnd, NULL, TRUE);
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
    HWND hwnd = CreateWindow(L"myclass", L"Bezier Curve",
        WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,
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