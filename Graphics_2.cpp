#include <windows.h>
#include <vector>
#include <cmath>

using namespace std;

vector<POINT> points;

//Distance for radius
int Distance(POINT a, POINT b)
{
    return (int)sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

// Midpoint Line
void DrawMidPointLine(HDC hdc,int x1,int y1,int x2,int y2,COLORREF color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    //If moving right --> +1 If moving left --> -1
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;
    dx = abs(dx);
    dy = abs(dy);
    int x = x1;
    int y = y1;
    SetPixel(hdc, x, y, color);
    // |slope| <= 1
    if (dx >= dy)
    {
        int d = 2 * dy - dx;
        int dE = 2 * dy;
        int dNE = 2 * (dy - dx);
        for (int i = 0; i < dx; i++)
        {
            if (d <= 0) // yn = y
            {
                d += dE;
                x += sx;
            }
            else //yn = y + 1
            {
                d += dNE;
                x += sx;
                y += sy;
            }
            SetPixel(hdc, x, y, color);
        }
    }
        // |slope| > 1
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

// Draw 8 Circle Points
void Draw8Points(HDC hdc,int xc,int yc,int x,int y,COLORREF c)
{
    SetPixel(hdc, xc+x, yc+y, c);
    SetPixel(hdc, xc-x, yc+y, c);
    SetPixel(hdc, xc+x, yc-y, c);
    SetPixel(hdc, xc-x, yc-y, c);
    SetPixel(hdc, xc+y, yc+x, c);
    SetPixel(hdc, xc-y, yc+x, c);
    SetPixel(hdc, xc+y, yc-x, c);
    SetPixel(hdc, xc-y, yc-x, c);
}

// Bresenham's algo based on second order difference
void DrawCircleSecondOrder(HDC hdc,int xc,int yc,int R,COLORREF c)
{
    int x = 0;
    int y = R;
    int d = 1 - R;
    int ch1 = 3;
    int ch2 = 5 - 2*R;
    Draw8Points(hdc,xc,yc,x,y,c);
    while(x < y)
    {
        if(d < 0)
        {
            d += ch1;
            ch2 += 2;
        }
        else
        {
            d += ch2;
            ch2 += 4;
            y--;
        }
        ch1 += 2;
        x++;
        Draw8Points(hdc,xc,yc,x,y,c);
    }
}

void FillBetweenCircles(HDC hdc,int xc,int yc,int r1,int r2)
{
    COLORREF colors[8] =
            {
                    RGB(244, 162, 97),   // soft orange
                    RGB(233, 196, 106),  // warm yellow
                    RGB(144, 190, 109),  // soft green
                    RGB(77, 144, 142),   // teal
                    RGB(100, 149, 237),  // soft blue
                    RGB(155, 89, 182),   // soft purple
                    RGB(231, 111, 81),   // coral
                    RGB(181, 131, 141)   // dusty pink
            };
    int r2sq = r2 * r2;
    int r1sq = r1 * r1;
    for(int y = yc - r2; y <= yc + r2; y++)
    {
        for(int x = xc - r2; x <= xc + r2; x++)
        {
            int dx = x - xc;
            int dy = y - yc;
            int d = dx*dx + dy*dy;
            if(d >= r1sq && d <= r2sq)
            {
                double angle = atan2(dy,dx);
                if(angle < 0) angle += 2 * 3.14159265;
                int sector = (int)(angle / (3.14159265/4)); // 8 sectors
                SetPixel(hdc,x,y,colors[sector]);
            }
        }
    }
}
// Draw Full Shape
void DrawShape(HDC hdc)
{
    if(points.size()!=3)
        return;
    POINT center = points[0];
    POINT p1 = points[1];
    POINT p2 = points[2];
    int r1 = Distance(center,p1);
    int r2 = Distance(center,p2);
    if(r1>r2)
        swap(r1,r2);
    FillBetweenCircles(hdc,center.x,center.y,r1,r2);
    DrawCircleSecondOrder(hdc,center.x,center.y,r1,RGB(0,0,0));
    DrawCircleSecondOrder(hdc,center.x,center.y,r2,RGB(0,0,0));
    DrawMidPointLine(hdc,center.x,center.y,p1.x,p1.y,RGB(0,0,0));
    DrawMidPointLine(hdc,center.x,center.y,p2.x,p2.y,RGB(0,0,0));
}

// Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LBUTTONDOWN: // left click
        {
            if(points.size()<3)
            {
                POINT p;
                p.x = LOWORD(lParam);
                p.y = HIWORD(lParam);
                points.push_back(p);
                InvalidateRect(hwnd,nullptr,TRUE);
            }
        }
            break;
        case WM_RBUTTONDOWN:
        {
            points.clear();
            InvalidateRect(hwnd,nullptr,TRUE);
        }
            break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd,&ps);
            DrawShape(hdc);
            EndPaint(hwnd,&ps);
        }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

// WinMain
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR,int nCmdShow)
{
    const char CLASS_NAME[]="CircleTask";
    WNDCLASS wc={};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(80,80,80));
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            "CircleTask",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,CW_USEDEFAULT,
            700,700,
            nullptr,nullptr,hInstance,nullptr
    );
    ShowWindow(hwnd,nCmdShow);
    MSG msg={};
    while(GetMessage(&msg, nullptr,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}