#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stack>
#include <algorithm>
using namespace std;
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "globals.h"
#include "algorithms.h"

HWND             g_hwnd      = nullptr;
HDC              g_memDC     = nullptr;
HBITMAP          g_memBmp    = nullptr;
int              g_W         = 1000;
int              g_H         = 680;

DrawMode         g_mode      = MODE_NONE;
COLORREF         g_drawColor = RGB(0,0,0);
COLORREF         g_bgColor   = RGB(255,255,255);

bool             g_lbDown    = false;
POINT            g_ptStart   = {0,0};
POINT            g_ptPrev    = {0,0};

static bool  g_waitingSecondPt = false;
static POINT g_firstPt         = {0, 0};

vector<Shape>  g_shapes;
vector<POINT>  g_cardinalPts;
vector<POINT>  g_polyPts;

int  g_fillQuarter  = 1;
bool g_clipActive   = false;
RECT g_clipRect     = {150,150,650,450};
int  g_clipSqSide   = 150;

static POINT g_clipCircCenter = {400,300};
static int   g_clipCircR      = 180;

void CLog(const char* fmt,...)
{
    va_list a; va_start(a,fmt);
    vprintf(fmt,a);
    va_end(a);
    fflush(stdout);
}

static void CreateBackBuffer()
{
    HDC hdc=GetDC(g_hwnd);
    if(g_memBmp){DeleteObject(g_memBmp);DeleteDC(g_memDC);}
    g_memDC  = CreateCompatibleDC(hdc);
    g_memBmp = CreateCompatibleBitmap(hdc,g_W,g_H);
    SelectObject(g_memDC,g_memBmp);
    RECT r={0,0,g_W,g_H};
    HBRUSH br=CreateSolidBrush(g_bgColor);
    FillRect(g_memDC,&r,br); DeleteObject(br);
    ReleaseDC(g_hwnd,hdc);
}

static void ClearMemDC()
{
    RECT r={0,0,g_W,g_H};
    HBRUSH br=CreateSolidBrush(g_bgColor);
    FillRect(g_memDC,&r,br); DeleteObject(br);
}

void RenderShape(HDC hdc,const Shape&s)
{
    if(s.pts.empty()) return;
    int x1=s.pts[0].x, y1=s.pts[0].y;
    int x2=s.pts.size()>1?s.pts[1].x:x1;
    int y2=s.pts.size()>1?s.pts[1].y:y1;
    COLORREF c=s.color;
    switch(s.mode){
    // ── Lines
    case MODE_LINE_DDA:        DDALine(hdc,x1,y1,x2,y2,c); break;
    case MODE_LINE_MIDPOINT:   MidpointLine(hdc,x1,y1,x2,y2,c); break;
    case MODE_LINE_PARAMETRIC: ParametricLine(hdc,x1,y1,x2,y2,c); break;
    // ── Circles
    case MODE_CIRCLE_DIRECT:    { int r=(int)hypot(x2-x1,y2-y1); DirectCircle(hdc,x1,y1,r,c); } break;
    case MODE_CIRCLE_POLAR:     { int r=(int)hypot(x2-x1,y2-y1); PolarCircle(hdc,x1,y1,r,c); } break;
    case MODE_CIRCLE_ITER_POLAR:{ int r=(int)hypot(x2-x1,y2-y1); IterativePolarCircle(hdc,x1,y1,r,c); } break;
    case MODE_CIRCLE_MIDPOINT:  { int r=(int)hypot(x2-x1,y2-y1); MidpointCircle(hdc,x1,y1,r,c); } break;
    case MODE_CIRCLE_MOD_MID:   { int r=(int)hypot(x2-x1,y2-y1); ModifiedMidpointCircle(hdc,x1,y1,r,c); } break;
    // ── Ellipse
    case MODE_ELLIPSE_DIRECT:   { int rx=abs(x2-x1),ry=abs(y2-y1); DirectEllipse(hdc,x1,y1,rx,ry,c); } break;
    case MODE_ELLIPSE_POLAR:    { int rx=abs(x2-x1),ry=abs(y2-y1); PolarEllipse(hdc,x1,y1,rx,ry,c); } break;
    case MODE_ELLIPSE_MIDPOINT: { int rx=abs(x2-x1),ry=abs(y2-y1); MidpointEllipse(hdc,x1,y1,rx,ry,c); } break;
    case MODE_CURVE_CARDINAL:
    {
        auto cp=s.pts; CardinalSpline(hdc,cp,0.5f,c);
    } break;
    case MODE_FILL_CIRC_LINES:
    { int r=(int)hypot(x2-x1,y2-y1);
      FillCircleWithLines(hdc,x1,y1,r,s.pts.size()>2?(int)s.pts[2].x:1,c); } break;
    case MODE_FILL_CIRC_CIRCS:
    { int r=(int)hypot(x2-x1,y2-y1);
      FillCircleWithCircles(hdc,x1,y1,r,s.pts.size()>2?(int)s.pts[2].x:1,c); } break;
    case MODE_FILL_SQ_HERMITE:
    { int side=abs(x2-x1);
      FillSquareWithHermite(hdc,min(x1,x2),min(y1,y2),side,c); } break;
    case MODE_FILL_RECT_BEZIER:
    { int w=abs(x2-x1),h=abs(y2-y1);
      FillRectWithBezier(hdc,min(x1,x2),min(y1,y2),w,h,c); } break;
    case MODE_FILL_CONVEX:
    { auto cp=s.pts; ScanlineFill(hdc,cp,c); } break;
    case MODE_FILL_NONCONVEX:
    { auto cp=s.pts; NonConvexFill(hdc,cp,c); } break;
    case MODE_FILL_FLOOD_REC:
    { COLORREF target = GetPixel(hdc, x1, y1); if (target != c)
      FloodFillRec(hdc, x1, y1, c, target, 0, 0, g_W, g_H); } break;
    case MODE_FILL_FLOOD_NREC:
    { COLORREF target = GetPixel(hdc, x1, y1); if (target != c)
       FloodFillNonRec(hdc, x1, y1, c, 0, 0, g_W, g_H); } break;
    case MODE_CLIP_RECT_POINT:
    {
        RECT cr={(LONG)s.pts[2].x,(LONG)s.pts[2].y,
                 (LONG)s.pts[3].x,(LONG)s.pts[3].y};
        HPEN pen=CreatePen(PS_DASH,1,RGB(0,128,0));
        HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom);
        SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
        if(PointInRect({x1,y1},cr))
            Ellipse(hdc,x1-3,y1-3,x1+3,y1+3);
        else {
            HPEN gp=CreatePen(PS_SOLID,1,RGB(200,200,200));
            HPEN gop=(HPEN)SelectObject(hdc,gp);
            Ellipse(hdc,x1-3,y1-3,x1+3,y1+3);
            SelectObject(hdc,gop); DeleteObject(gp);
        }
    } break;
    case MODE_CLIP_RECT_LINE:
    {
        RECT cr={(LONG)s.pts[2].x,(LONG)s.pts[2].y,
                 (LONG)s.pts[3].x,(LONG)s.pts[3].y};
        HPEN pen=CreatePen(PS_DASH,1,RGB(0,128,0));
        HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom);
        SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
        HPEN gp=CreatePen(PS_DOT,1,RGB(180,180,180));
        HPEN gop=(HPEN)SelectObject(hdc,gp);
        MoveToEx(hdc,x1,y1,nullptr); LineTo(hdc,x2,y2);
        SelectObject(hdc,gop); DeleteObject(gp);
        int cx1=x1,cy1=y1,cx2=x2,cy2=y2;
        if(CohenSutherland(cx1,cy1,cx2,cy2,cr))
            DDALine(hdc,cx1,cy1,cx2,cy2,c);
    } break;
    case MODE_CLIP_RECT_POLY:
    {
        RECT cr={(LONG)s.pts[2].x,(LONG)s.pts[2].y,
                 (LONG)s.pts[3].x,(LONG)s.pts[3].y};
        HPEN pen=CreatePen(PS_DASH,1,RGB(0,128,0));
        HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom);
        SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
        vector<POINT> poly(s.pts.begin()+4,s.pts.end());
        vector<POINT> out;
        SutherlandHodgman(poly,cr,out);
        if(out.size()>1){
            HPEN pp=CreatePen(PS_SOLID,1,c);
            HPEN pop=(HPEN)SelectObject(hdc,pp);
            MoveToEx(hdc,out[0].x,out[0].y,nullptr);
            for(auto&p:out) LineTo(hdc,p.x,p.y);
            LineTo(hdc,out[0].x,out[0].y);
            SelectObject(hdc,pop); DeleteObject(pp);
        }
    } break;
    case MODE_CLIP_SQ_POINT:
    {
        RECT cr={(LONG)s.pts[2].x,(LONG)s.pts[2].y,
                 (LONG)s.pts[3].x,(LONG)s.pts[3].y};
        HPEN pen=CreatePen(PS_DASH,1,RGB(0,0,200));
        HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom);
        SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
        if(PointInRect({x1,y1},cr)) Ellipse(hdc,x1-3,y1-3,x1+3,y1+3);
    } break;
    case MODE_CLIP_SQ_LINE:
    {
        RECT cr={(LONG)s.pts[2].x,(LONG)s.pts[2].y,
                 (LONG)s.pts[3].x,(LONG)s.pts[3].y};
        HPEN pen=CreatePen(PS_DASH,1,RGB(0,0,200));
        HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom);
        SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
        HPEN gp=CreatePen(PS_DOT,1,RGB(180,180,180));
        HPEN gop=(HPEN)SelectObject(hdc,gp);
        MoveToEx(hdc,x1,y1,nullptr); LineTo(hdc,x2,y2);
        SelectObject(hdc,gop); DeleteObject(gp);
        int cx1=x1,cy1=y1,cx2=x2,cy2=y2;
        if(CohenSutherland(cx1,cy1,cx2,cy2,cr))
            DDALine(hdc,cx1,cy1,cx2,cy2,c);
    } break;
    case MODE_BONUS_CIRC_POINT:
    {
        POINT cc={(LONG)s.pts[2].x,(LONG)s.pts[2].y};
        int rr=(int)s.pts[3].x;
        MidpointCircle(hdc,cc.x,cc.y,rr,RGB(0,200,0));
        if(PointInCircle({x1,y1},cc,rr))
            { HPEN pp=CreatePen(PS_SOLID,3,c);
              HPEN op=(HPEN)SelectObject(hdc,pp);
              Ellipse(hdc,x1-4,y1-4,x1+4,y1+4);
              SelectObject(hdc,op); DeleteObject(pp);}
    } break;
    case MODE_BONUS_CIRC_LINE:
    {
        POINT cc={(LONG)s.pts[2].x,(LONG)s.pts[2].y};
        int rr=(int)s.pts[3].x;
        MidpointCircle(hdc,cc.x,cc.y,rr,RGB(0,200,0));
        HPEN gp=CreatePen(PS_DOT,1,RGB(180,180,180));
        HPEN gop=(HPEN)SelectObject(hdc,gp);
        MoveToEx(hdc,x1,y1,nullptr); LineTo(hdc,x2,y2);
        SelectObject(hdc,gop); DeleteObject(gp);
        int lx1=x1,ly1=y1,lx2=x2,ly2=y2;
        if(ClipLineCircle(lx1,ly1,lx2,ly2,cc,rr))
            DDALine(hdc,lx1,ly1,lx2,ly2,c);
    } break;
    case MODE_BONUS_HAPPY: { int r=(int)hypot(x2-x1,y2-y1); DrawHappyFace(hdc,x1,y1,r,c); } break;
    case MODE_BONUS_SAD:   { int r=(int)hypot(x2-x1,y2-y1); DrawSadFace(hdc,x1,y1,r,c); } break;
    default: break;
    }
}

static void RedrawAll()
{
    ClearMemDC();
    for(auto&s:g_shapes) RenderShape(g_memDC,s);
    if(g_clipActive){
        HPEN pen=CreatePen(PS_DASH,1,RGB(100,100,255));
        HPEN op=(HPEN)SelectObject(g_memDC,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob=(HBRUSH)SelectObject(g_memDC,nb);
        Rectangle(g_memDC,g_clipRect.left,g_clipRect.top,g_clipRect.right,g_clipRect.bottom);
        SelectObject(g_memDC,op); SelectObject(g_memDC,ob); DeleteObject(pen);
    }
    if(!g_cardinalPts.empty()){
        HPEN pen=CreatePen(PS_DOT,1,RGB(160,160,160));
        HPEN op=(HPEN)SelectObject(g_memDC,pen);
        for(int i=0;i+1<(int)g_cardinalPts.size();++i){
            MoveToEx(g_memDC,g_cardinalPts[i].x,g_cardinalPts[i].y,nullptr);
            LineTo(g_memDC,g_cardinalPts[i+1].x,g_cardinalPts[i+1].y);
        }
        SelectObject(g_memDC,op); DeleteObject(pen);
        for(auto&p:g_cardinalPts) Ellipse(g_memDC,p.x-3,p.y-3,p.x+3,p.y+3);
    }
    if(!g_polyPts.empty()){
        HPEN pen=CreatePen(PS_DOT,1,RGB(160,160,160));
        HPEN op=(HPEN)SelectObject(g_memDC,pen);
        for(int i=0;i+1<(int)g_polyPts.size();++i){
            MoveToEx(g_memDC,g_polyPts[i].x,g_polyPts[i].y,nullptr);
            LineTo(g_memDC,g_polyPts[i+1].x,g_polyPts[i+1].y);
        }
        SelectObject(g_memDC,op); DeleteObject(pen);
        for(auto&p:g_polyPts) Ellipse(g_memDC,p.x-3,p.y-3,p.x+3,p.y+3);
    }
    InvalidateRect(g_hwnd,nullptr,FALSE);
}

static bool PickColor(HWND hwnd,COLORREF&c)
{
    static COLORREF cust[16]={};
    CHOOSECOLORA cc{}; cc.lStructSize=sizeof(cc);
    cc.hwndOwner=hwnd; cc.rgbResult=c;
    cc.lpCustColors=cust; cc.Flags=CC_FULLOPEN|CC_RGBINIT;
    if(ChooseColorA(&cc)){c=cc.rgbResult;return true;}
    return false;
}

static INT_PTR CALLBACK IntDlgProc(HWND hd,UINT m,WPARAM w,LPARAM l)
{
    static int* out=nullptr;
    if(m==WM_INITDIALOG){out=(int*)l;return TRUE;}
    if(m==WM_COMMAND){
        if(LOWORD(w)==IDOK){
            char buf[64]; GetDlgItemTextA(hd,1001,buf,64);
            if(out) *out=atoi(buf);
            EndDialog(hd,IDOK); return TRUE;
        }
        if(LOWORD(w)==IDCANCEL){EndDialog(hd,IDCANCEL);return TRUE;}
    }
    return FALSE;
}

static int AskQuarter(HWND hwnd)
{
    int q=1;
    CLog("[INPUT] Enter fill quarter (1=TR,2=TL,3=BL,4=BR): ");
    scanf_s("%d",&q);
    q=max(1,min(4,q));
    CLog("[INPUT] Quarter=%d\n",q);
    return q;
}

static void SaveShapes(const string&path)
{
    ofstream f(path);
    if(!f){CLog("[FILE] Cannot open %s\n",path.c_str());return;}
    for(auto&s:g_shapes){
        f<<(int)s.mode<<" "<<s.color<<" "<<s.pts.size();
        for(auto&p:s.pts) f<<" "<<p.x<<" "<<p.y;
        f<<"\n";
    }
    CLog("[FILE] Saved %zu shapes to %s\n",g_shapes.size(),path.c_str());
}

static void LoadShapes(const string&path)
{
    ifstream f(path);
    if(!f){CLog("[FILE] Cannot open %s\n",path.c_str());return;}
    g_shapes.clear();
    string line;
    while(getline(f,line)){
        if(line.empty()) continue;
        istringstream ss(line);
        Shape sh; int m; COLORREF col; int np;
        ss>>m>>col>>np;
        sh.mode=(DrawMode)m; sh.color=col;
        for(int i=0;i<np;++i){POINT p;ss>>p.x>>p.y;sh.pts.push_back(p);}
        g_shapes.push_back(sh);
    }
    CLog("[FILE] Loaded %zu shapes from %s\n",g_shapes.size(),path.c_str());
    RedrawAll();
}

static string OpenFileDlg(HWND hwnd,bool save)
{
    char buf[MAX_PATH]={};
    OPENFILENAMEA ofn{}; ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=hwnd; ofn.lpstrFile=buf; ofn.nMaxFile=MAX_PATH;
    ofn.lpstrFilter="Drawing Files\0*.drw\0All Files\0*.*\0";
    ofn.lpstrDefExt="drw";
    if(save){ofn.Flags=OFN_OVERWRITEPROMPT;GetSaveFileNameA(&ofn);}
    else{ofn.Flags=OFN_FILEMUSTEXIST;GetOpenFileNameA(&ofn);}
    return buf;
}

static const char* ModeName(DrawMode m)
{
    switch(m){
    case MODE_LINE_DDA:         return "DDA Line";
    case MODE_LINE_MIDPOINT:    return "Midpoint Line";
    case MODE_LINE_PARAMETRIC:  return "Parametric Line";
    case MODE_CIRCLE_DIRECT:    return "Direct Circle";
    case MODE_CIRCLE_POLAR:     return "Polar Circle";
    case MODE_CIRCLE_ITER_POLAR:return "Iterative Polar Circle";
    case MODE_CIRCLE_MIDPOINT:  return "Midpoint Circle";
    case MODE_CIRCLE_MOD_MID:   return "Modified Midpoint Circle";
    case MODE_ELLIPSE_DIRECT:   return "Direct Ellipse";
    case MODE_ELLIPSE_POLAR:    return "Polar Ellipse";
    case MODE_ELLIPSE_MIDPOINT: return "Midpoint Ellipse";
    case MODE_CURVE_CARDINAL:   return "Cardinal Spline [click pts, RClick=done]";
    case MODE_FILL_CIRC_LINES:  return "Fill Circle w/ Lines";
    case MODE_FILL_CIRC_CIRCS:  return "Fill Circle w/ Circles";
    case MODE_FILL_SQ_HERMITE:  return "Fill Square w/ Hermite";
    case MODE_FILL_RECT_BEZIER: return "Fill Rect w/ Bezier";
    case MODE_FILL_CONVEX:      return "Convex Fill [click pts, RClick=done]";
    case MODE_FILL_NONCONVEX:   return "Non-Convex Fill [click pts, RClick=done]";
    case MODE_FILL_FLOOD_REC:   return "Recursive Flood Fill [click inside shape]";
    case MODE_FILL_FLOOD_NREC:  return "Non-Recursive Flood Fill [click inside shape]";
    case MODE_CLIP_RECT_POINT:  return "Rect Clip – Point";
    case MODE_CLIP_RECT_LINE:   return "Rect Clip – Line";
    case MODE_CLIP_RECT_POLY:   return "Rect Clip – Polygon [click pts, RClick=done]";
    case MODE_CLIP_SQ_POINT:    return "Square Clip – Point";
    case MODE_CLIP_SQ_LINE:     return "Square Clip – Line";
    case MODE_BONUS_CIRC_POINT: return "Circle Clip – Point";
    case MODE_BONUS_CIRC_LINE:  return "Circle Clip – Line";
    case MODE_BONUS_HAPPY:      return "Happy Face";
    case MODE_BONUS_SAD:        return "Sad Face";
    default:                    return "None";
    }
}
static void UpdateTitle()
{
    char buf[256];
    sprintf_s(buf,"2D Drawing Package  |  [%s]  Color: #%06X",
              ModeName(g_mode),(g_drawColor&0xFF)<<16|((g_drawColor>>8)&0xFF)|((g_drawColor>>16)&0xFF));
    SetWindowTextA(g_hwnd,buf);
}

static void DrawRubber(HDC hdc,POINT a,POINT b)
{
    int r2=(int)hypot(b.x-a.x,b.y-a.y);
    SetROP2(hdc,R2_NOT);
    HPEN pen=CreatePen(PS_SOLID,1,RGB(0,0,0));
    HPEN op=(HPEN)SelectObject(hdc,pen);
    HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
    switch(g_mode){
    case MODE_LINE_DDA: case MODE_LINE_MIDPOINT: case MODE_LINE_PARAMETRIC:
    case MODE_CLIP_RECT_LINE: case MODE_CLIP_SQ_LINE: case MODE_BONUS_CIRC_LINE:
        MoveToEx(hdc,a.x,a.y,nullptr); LineTo(hdc,b.x,b.y); break;
    case MODE_CIRCLE_DIRECT: case MODE_CIRCLE_POLAR: case MODE_CIRCLE_ITER_POLAR:
    case MODE_CIRCLE_MIDPOINT: case MODE_CIRCLE_MOD_MID:
    case MODE_FILL_CIRC_LINES: case MODE_FILL_CIRC_CIRCS:
    case MODE_BONUS_HAPPY: case MODE_BONUS_SAD:
        Ellipse(hdc,a.x-r2,a.y-r2,a.x+r2,a.y+r2); break;
    case MODE_ELLIPSE_DIRECT: case MODE_ELLIPSE_POLAR: case MODE_ELLIPSE_MIDPOINT:
        Ellipse(hdc,a.x-(b.x-a.x),a.y-(b.y-a.y),a.x+(b.x-a.x),a.y+(b.y-a.y)); break;
    case MODE_FILL_SQ_HERMITE:
    { int s=abs(b.x-a.x);
      Rectangle(hdc,a.x,a.y,a.x+s,a.y+s); } break;
    case MODE_FILL_RECT_BEZIER:
        Rectangle(hdc,a.x,a.y,b.x,b.y); break;
    default: break;
    }
    SelectObject(hdc,op); SelectObject(hdc,ob); DeleteObject(pen);
    SetROP2(hdc,R2_COPYPEN);
}

static void CommitShape(POINT a,POINT b)
{
    Shape s; s.mode=g_mode; s.color=g_drawColor;
    s.pts.push_back(a); s.pts.push_back(b);
    if(g_mode==MODE_CLIP_RECT_POINT||g_mode==MODE_CLIP_RECT_LINE||
       g_mode==MODE_CLIP_SQ_POINT ||g_mode==MODE_CLIP_SQ_LINE){
        s.pts.push_back({g_clipRect.left,g_clipRect.top});
        s.pts.push_back({g_clipRect.right,g_clipRect.bottom});
    }
    if(g_mode==MODE_FILL_CIRC_LINES||g_mode==MODE_FILL_CIRC_CIRCS)
        s.pts.push_back({(LONG)g_fillQuarter,0});
    if(g_mode==MODE_BONUS_CIRC_POINT||g_mode==MODE_BONUS_CIRC_LINE){
        s.pts.push_back({g_clipCircCenter.x,g_clipCircCenter.y});
        s.pts.push_back({g_clipCircR,0});
    }
    g_shapes.push_back(s);
    RenderShape(g_memDC,g_shapes.back());
    InvalidateRect(g_hwnd,nullptr,FALSE);
    CLog("[SHAPE] %s  (%d,%d)->(%d,%d)\n",ModeName(g_mode),a.x,a.y,b.x,b.y);
}

static HMENU BuildMenuBar()
{
    HMENU hBar=CreateMenu();
    // File
    HMENU hF=CreatePopupMenu();
    AppendMenuA(hF,MF_STRING,ID_FILE_CLEAR,"Clear Screen");
    AppendMenuA(hF,MF_STRING,ID_FILE_SAVE, "Save...");
    AppendMenuA(hF,MF_STRING,ID_FILE_LOAD, "Load...");
    AppendMenuA(hF,MF_SEPARATOR,0,nullptr);
    AppendMenuA(hF,MF_STRING,ID_FILE_EXIT, "Exit");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hF,"&File");
    // Preferences
    HMENU hP=CreatePopupMenu();
    AppendMenuA(hP,MF_STRING,ID_PREF_BGCOLOR,"Background Color (White)");
    AppendMenuA(hP,MF_STRING,ID_PREF_CURSOR, "Change Mouse Cursor");
    AppendMenuA(hP,MF_STRING,ID_PREF_COLOR,  "Choose Draw Color...");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hP,"&Preferences");
    // Lines
    HMENU hL=CreatePopupMenu();
    AppendMenuA(hL,MF_STRING,ID_LINE_DDA,       "DDA Line");
    AppendMenuA(hL,MF_STRING,ID_LINE_MIDPOINT,  "Midpoint Line");
    AppendMenuA(hL,MF_STRING,ID_LINE_PARAMETRIC,"Parametric Line");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hL,"&Lines");
    // Circles
    HMENU hC=CreatePopupMenu();
    AppendMenuA(hC,MF_STRING,ID_CIRCLE_DIRECT,    "Direct Circle");
    AppendMenuA(hC,MF_STRING,ID_CIRCLE_POLAR,     "Polar Circle");
    AppendMenuA(hC,MF_STRING,ID_CIRCLE_ITER_POLAR,"Iterative Polar Circle");
    AppendMenuA(hC,MF_STRING,ID_CIRCLE_MIDPOINT,  "Midpoint Circle");
    AppendMenuA(hC,MF_STRING,ID_CIRCLE_MOD_MID,   "Modified Midpoint Circle");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hC,"&Circles");
    // Ellipse
    HMENU hE=CreatePopupMenu();
    AppendMenuA(hE,MF_STRING,ID_ELLIPSE_DIRECT,  "Direct Ellipse");
    AppendMenuA(hE,MF_STRING,ID_ELLIPSE_POLAR,   "Polar Ellipse");
    AppendMenuA(hE,MF_STRING,ID_ELLIPSE_MIDPOINT,"Midpoint Ellipse");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hE,"&Ellipse");
    HMENU hCv=CreatePopupMenu();
    AppendMenuA(hCv,MF_STRING,ID_CURVE_CARDINAL,"Cardinal Spline");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hCv,"C&urves");
    HMENU hFi=CreatePopupMenu();
    AppendMenuA(hFi,MF_STRING,ID_FILL_CIRC_LINES, "Fill Circle w/ Lines");
    AppendMenuA(hFi,MF_STRING,ID_FILL_CIRC_CIRCS, "Fill Circle w/ Circles");
    AppendMenuA(hFi,MF_STRING,ID_FILL_SQ_HERMITE, "Fill Square w/ Hermite");
    AppendMenuA(hFi,MF_STRING,ID_FILL_RECT_BEZIER,"Fill Rectangle w/ Bezier");
    AppendMenuA(hFi,MF_SEPARATOR,0,nullptr);
    AppendMenuA(hFi,MF_STRING,ID_FILL_CONVEX,    "Convex Fill (Scan-line)");
    AppendMenuA(hFi,MF_STRING,ID_FILL_NONCONVEX, "Non-Convex Fill");
    AppendMenuA(hFi,MF_SEPARATOR,0,nullptr);
    AppendMenuA(hFi,MF_STRING,ID_FILL_FLOOD_REC, "Recursive Flood Fill");
    AppendMenuA(hFi,MF_STRING,ID_FILL_FLOOD_NREC,"Non-Recursive Flood Fill");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hFi,"F&illing");
    HMENU hCl=CreatePopupMenu();
    HMENU hClR=CreatePopupMenu();
    AppendMenuA(hClR,MF_STRING,ID_CLIP_RECT_POINT,"Point");
    AppendMenuA(hClR,MF_STRING,ID_CLIP_RECT_LINE, "Line");
    AppendMenuA(hClR,MF_STRING,ID_CLIP_RECT_POLY, "Polygon");
    AppendMenuA(hCl,MF_POPUP,(UINT_PTR)hClR,"Rectangle Window");
    HMENU hClS=CreatePopupMenu();
    AppendMenuA(hClS,MF_STRING,ID_CLIP_SQ_POINT,"Point");
    AppendMenuA(hClS,MF_STRING,ID_CLIP_SQ_LINE, "Line");
    AppendMenuA(hCl,MF_POPUP,(UINT_PTR)hClS,"Square Window");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hCl,"C&lipping");
    HMENU hBo=CreatePopupMenu();
    HMENU hBoC=CreatePopupMenu();
    AppendMenuA(hBoC,MF_STRING,ID_BONUS_CIRC_POINT,"Point");
    AppendMenuA(hBoC,MF_STRING,ID_BONUS_CIRC_LINE, "Line");
    AppendMenuA(hBo,MF_POPUP,(UINT_PTR)hBoC,"Circle Clip Window");
    AppendMenuA(hBo,MF_SEPARATOR,0,nullptr);
    AppendMenuA(hBo,MF_STRING,ID_BONUS_HAPPY,"Happy Face");
    AppendMenuA(hBo,MF_STRING,ID_BONUS_SAD,  "Sad Face");
    AppendMenuA(hBar,MF_POPUP,(UINT_PTR)hBo,"&Bonus");
    return hBar;
}

static int g_cursorIdx=0;
LPCSTR IDC_PENCIL;
static LPCSTR g_cursors[]={IDC_CROSS,IDC_ARROW,IDC_HAND,IDC_PENCIL};
static const char* g_cursorNames[]={"Crosshair","Arrow","Hand","Pencil"};
static void CycleCursor()
{
    g_cursorIdx=(g_cursorIdx+1)%4;
    SetClassLongPtrA(g_hwnd,GCLP_HCURSOR,(LONG_PTR)LoadCursorA(nullptr,g_cursors[g_cursorIdx]));
    CLog("[PREF] Cursor changed to: %s\n",g_cursorNames[g_cursorIdx]);
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    switch(msg)
    {
    case WM_SIZE:
        g_W=LOWORD(lp); g_H=HIWORD(lp);
        CreateBackBuffer();
        RedrawAll();
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd,&ps);
        BitBlt(hdc,0,0,g_W,g_H,g_memDC,0,0,SRCCOPY);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
        if (g_mode == MODE_CURVE_CARDINAL) {
            g_cardinalPts.push_back({mx, my});
            CLog("[CARDINAL] Added pt (%d,%d)  total=%zu\n", mx, my, g_cardinalPts.size());
            RedrawAll();
            if (g_cardinalPts.size() == 4) {
                Shape s; s.mode=g_mode; s.color=g_drawColor;
                s.pts = g_cardinalPts;
                g_shapes.push_back(s);
                RenderShape(g_memDC, g_shapes.back());
                CLog("[CARDINAL] Auto-committed 4 pts\n");
                g_cardinalPts.clear();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        if (g_mode == MODE_FILL_CONVEX   ||
            g_mode == MODE_FILL_NONCONVEX ||
            g_mode == MODE_CLIP_RECT_POLY) {
            g_polyPts.push_back({mx, my});
            CLog("[POLY] Added pt (%d,%d)  total=%zu\n", mx, my, g_polyPts.size());
            RedrawAll();
            return 0;
        }
        if (g_mode == MODE_FILL_FLOOD_REC || g_mode == MODE_FILL_FLOOD_NREC) {
            CommitShape({mx,my},{mx,my});
            return 0;
        }
        bool isTwoClickMode = (g_mode == MODE_LINE_DDA          ||
                            g_mode == MODE_LINE_MIDPOINT      ||
                            g_mode == MODE_LINE_PARAMETRIC    ||
                            g_mode == MODE_CIRCLE_DIRECT      ||
                            g_mode == MODE_CIRCLE_POLAR       ||
                            g_mode == MODE_CIRCLE_ITER_POLAR  ||
                            g_mode == MODE_CIRCLE_MIDPOINT    ||
                            g_mode == MODE_CIRCLE_MOD_MID     ||
                            g_mode == MODE_ELLIPSE_DIRECT     ||
                            g_mode == MODE_ELLIPSE_POLAR      ||
                            g_mode == MODE_ELLIPSE_MIDPOINT   ||
                            g_mode == MODE_FILL_CIRC_LINES    ||
                            g_mode == MODE_FILL_CIRC_CIRCS    ||
                            g_mode == MODE_FILL_SQ_HERMITE    ||
                            g_mode == MODE_FILL_RECT_BEZIER   ||
                            g_mode == MODE_BONUS_HAPPY        ||
                            g_mode == MODE_BONUS_SAD          ||
                            g_mode == MODE_CLIP_RECT_POINT    ||
                            g_mode == MODE_CLIP_RECT_LINE     ||
                            g_mode == MODE_CLIP_SQ_POINT      ||
                            g_mode == MODE_CLIP_SQ_LINE       ||
                            g_mode == MODE_BONUS_CIRC_LINE);
        if (isTwoClickMode) {
            if (!g_waitingSecondPt) {
                g_firstPt         = {mx, my};
                g_waitingSecondPt = true;
                HDC hdc = GetDC(hwnd);
                Ellipse(hdc, mx-3, my-3, mx+3, my+3);
                ReleaseDC(hwnd, hdc);
                CLog("[DRAW] P1=(%d,%d) – click P2\n", mx, my);
            } else {
                g_waitingSecondPt = false;
                CommitShape(g_firstPt, {mx, my});
                CLog("[DRAW] P2=(%d,%d)\n", mx, my);
            }
            return 0;
        }
        g_ptStart = {mx, my};
        g_ptPrev  = g_ptStart;
        g_lbDown  = true;
        SetCapture(hwnd);
        CLog("[MOUSE] Down (%d,%d)  mode=%s\n", mx, my, ModeName(g_mode));
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if(!g_lbDown) return 0;
        int mx=GET_X_LPARAM(lp), my=GET_Y_LPARAM(lp);
        HDC hdc=GetDC(hwnd);
        DrawRubber(hdc,g_ptStart,g_ptPrev);
        DrawRubber(hdc,g_ptStart,{mx,my});
        ReleaseDC(hwnd,hdc);
        g_ptPrev={mx,my};
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if(!g_lbDown) return 0;
        g_lbDown=false; ReleaseCapture();
        int mx=GET_X_LPARAM(lp), my=GET_Y_LPARAM(lp);
        // erase last rubber
        HDC hdc=GetDC(hwnd);
        DrawRubber(hdc,g_ptStart,g_ptPrev);
        ReleaseDC(hwnd,hdc);
        if(g_mode==MODE_NONE){return 0;}
        POINT end={mx,my};
        CommitShape(g_ptStart,end);
        return 0;
    }
    case WM_RBUTTONDOWN:
    {
        if (g_mode == MODE_CURVE_CARDINAL && g_cardinalPts.size() == 4) {
            Shape s; s.mode=g_mode; s.color=g_drawColor;
            s.pts = g_cardinalPts;
            g_shapes.push_back(s);
            RenderShape(g_memDC, g_shapes.back());
            CLog("[CARDINAL] Committed 4 pts\n");
            g_cardinalPts.clear();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if((g_mode==MODE_FILL_CONVEX||g_mode==MODE_FILL_NONCONVEX) && g_polyPts.size()>=3){
            Shape s; s.mode=g_mode; s.color=g_drawColor;
            s.pts=g_polyPts;
            g_shapes.push_back(s);
            RenderShape(g_memDC,g_shapes.back());
            CLog("[POLY] Filled %zu pts\n",g_polyPts.size());
            g_polyPts.clear();
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        else if(g_mode==MODE_CLIP_RECT_POLY && g_polyPts.size()>=3){
            Shape s; s.mode=g_mode; s.color=g_drawColor;
            s.pts.push_back({0,0}); s.pts.push_back({0,0});
            s.pts.push_back({g_clipRect.left,g_clipRect.top});
            s.pts.push_back({g_clipRect.right,g_clipRect.bottom});
            for(auto&p:g_polyPts) s.pts.push_back(p);
            g_shapes.push_back(s);
            RenderShape(g_memDC,g_shapes.back());
            g_polyPts.clear();
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        else if(g_lbDown){ g_lbDown=false; ReleaseCapture();
            HDC hdc=GetDC(hwnd); DrawRubber(hdc,g_ptStart,g_ptPrev); ReleaseDC(hwnd,hdc);
        }
        return 0;
    }
    case WM_COMMAND:
    {
        int id=LOWORD(wp);
        if(id==ID_FILE_CLEAR){
            g_shapes.clear(); g_cardinalPts.clear(); g_polyPts.clear();
            ClearMemDC(); InvalidateRect(hwnd,nullptr,FALSE);
            CLog("[FILE] Canvas cleared\n");
        }
        else if(id==ID_FILE_SAVE){
            string p=OpenFileDlg(hwnd,true);
            if(!p.empty()) SaveShapes(p);
        }
        else if(id==ID_FILE_LOAD){
            string p=OpenFileDlg(hwnd,false);
            if(!p.empty()) LoadShapes(p);
        }
        else if(id==ID_FILE_EXIT) PostQuitMessage(0);
        else if(id==ID_PREF_BGCOLOR){
            if(PickColor(hwnd,g_bgColor)){
                CLog("[PREF] Background color changed\n");
                RedrawAll();
            }
        }
        else if(id==ID_PREF_CURSOR) CycleCursor();
        else if(id==ID_PREF_COLOR){
            if(PickColor(hwnd,g_drawColor))
                CLog("[PREF] Draw color changed to #%06X\n",
                    GetRValue(g_drawColor)<<16|GetGValue(g_drawColor)<<8|GetBValue(g_drawColor));
        }
        // Lines
        else if(id==ID_LINE_DDA)        { g_mode=MODE_LINE_DDA;        g_waitingSecondPt=false; CLog("[MODE] DDA Line\n"); }
        else if(id==ID_LINE_MIDPOINT)   { g_mode=MODE_LINE_MIDPOINT;   g_waitingSecondPt=false; CLog("[MODE] Midpoint Line\n"); }
        else if(id==ID_LINE_PARAMETRIC) { g_mode=MODE_LINE_PARAMETRIC; g_waitingSecondPt=false; CLog("[MODE] Parametric Line\n"); }
        // Circles
        else if(id==ID_CIRCLE_DIRECT)    { g_mode=MODE_CIRCLE_DIRECT;     g_waitingSecondPt=false; }
        else if(id==ID_CIRCLE_POLAR)     { g_mode=MODE_CIRCLE_POLAR;      g_waitingSecondPt=false; }
        else if(id==ID_CIRCLE_ITER_POLAR){ g_mode=MODE_CIRCLE_ITER_POLAR; g_waitingSecondPt=false; }
        else if(id==ID_CIRCLE_MIDPOINT)  { g_mode=MODE_CIRCLE_MIDPOINT;   g_waitingSecondPt=false; }
        else if(id==ID_CIRCLE_MOD_MID)   { g_mode=MODE_CIRCLE_MOD_MID;    g_waitingSecondPt=false; }
        // Ellipse
        else if(id==ID_ELLIPSE_DIRECT)   { g_mode=MODE_ELLIPSE_DIRECT;    g_waitingSecondPt=false; }
        else if(id==ID_ELLIPSE_POLAR)    { g_mode=MODE_ELLIPSE_POLAR;     g_waitingSecondPt=false; }
        else if(id==ID_ELLIPSE_MIDPOINT) { g_mode=MODE_ELLIPSE_MIDPOINT;  g_waitingSecondPt=false; }
        // Curves
        else if(id==ID_CURVE_CARDINAL){
            g_mode=MODE_CURVE_CARDINAL; g_cardinalPts.clear();
            CLog("[MODE] Cardinal Spline – left-click to add pts, right-click to finish\n");
        }
        // Filling
        else if(id==ID_FILL_CIRC_LINES) { g_fillQuarter=1; g_mode=MODE_FILL_CIRC_LINES; g_waitingSecondPt=false; CLog("[MODE] Fill Circle w/ Lines\n"); }
        else if(id==ID_FILL_CIRC_CIRCS) { g_fillQuarter=1; g_mode=MODE_FILL_CIRC_CIRCS; g_waitingSecondPt=false; CLog("[MODE] Fill Circle w/ Circles\n"); }
        else if(id==ID_FILL_SQ_HERMITE) { g_mode=MODE_FILL_SQ_HERMITE;  CLog("[MODE] Fill Square w/ Hermite\n"); }
        else if(id==ID_FILL_RECT_BEZIER){ g_mode=MODE_FILL_RECT_BEZIER; CLog("[MODE] Fill Rect w/ Bezier\n"); }
        else if(id==ID_FILL_CONVEX)     { g_mode=MODE_FILL_CONVEX;   g_polyPts.clear(); CLog("[MODE] Convex Fill – click pts, RClick done\n"); }
        else if(id==ID_FILL_NONCONVEX)  { g_mode=MODE_FILL_NONCONVEX;g_polyPts.clear(); CLog("[MODE] Non-Convex Fill – click pts, RClick done\n"); }
        else if(id==ID_FILL_FLOOD_REC)  { g_mode=MODE_FILL_FLOOD_REC;  CLog("[MODE] Recursive Flood Fill – click inside a closed shape\n"); }
        else if(id==ID_FILL_FLOOD_NREC) { g_mode=MODE_FILL_FLOOD_NREC; CLog("[MODE] Non-Recursive Flood Fill – click inside a closed shape\n"); }
        // Clipping
        else if(id==ID_CLIP_RECT_POINT){ g_mode=MODE_CLIP_RECT_POINT; CLog("[MODE] Rect Clip-Point. Clip window: (%d,%d)-(%d,%d)\n",g_clipRect.left,g_clipRect.top,g_clipRect.right,g_clipRect.bottom); }
        else if(id==ID_CLIP_RECT_LINE) { g_mode=MODE_CLIP_RECT_LINE;  CLog("[MODE] Rect Clip-Line. Draw a line with mouse.\n"); }
        else if(id==ID_CLIP_RECT_POLY) { g_mode=MODE_CLIP_RECT_POLY;  g_polyPts.clear(); CLog("[MODE] Rect Clip-Poly. Click pts, RClick=clip.\n"); }
        else if(id==ID_CLIP_SQ_POINT)  {
            int half=g_clipSqSide;
            int cx=(g_clipRect.left+g_clipRect.right)/2;
            int cy=(g_clipRect.top+g_clipRect.bottom)/2;
            g_clipRect={cx-half,cy-half,cx+half,cy+half};
            g_mode=MODE_CLIP_SQ_POINT; CLog("[MODE] Square Clip-Point\n");
        }
        else if(id==ID_CLIP_SQ_LINE)   {
            int half=g_clipSqSide;
            int cx=(g_clipRect.left+g_clipRect.right)/2;
            int cy=(g_clipRect.top+g_clipRect.bottom)/2;
            g_clipRect={cx-half,cy-half,cx+half,cy+half};
            g_mode=MODE_CLIP_SQ_LINE; CLog("[MODE] Square Clip-Line\n");
        }
        // Bonus
        else if(id==ID_BONUS_CIRC_POINT){ g_mode=MODE_BONUS_CIRC_POINT; CLog("[MODE] Circle Clip-Point. Circle center=(%d,%d) r=%d\n",g_clipCircCenter.x,g_clipCircCenter.y,g_clipCircR); }
        else if(id==ID_BONUS_CIRC_LINE) { g_mode=MODE_BONUS_CIRC_LINE;  CLog("[MODE] Circle Clip-Line\n"); }
        else if(id==ID_BONUS_HAPPY)     { g_mode=MODE_BONUS_HAPPY; CLog("[MODE] Happy Face – drag center+radius\n"); }
        else if(id==ID_BONUS_SAD)       { g_mode=MODE_BONUS_SAD;   CLog("[MODE] Sad Face – drag center+radius\n"); }
        UpdateTitle();
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd,msg,wp,lp);
}
DWORD WINAPI ConsoleThread(LPVOID)
{
    CLog("\n=================================================\n");
    CLog("  2D Drawing Package - Console\n");
    CLog("=================================================\n");
    CLog("  Commands:\n");
    CLog("  color  <R> <G> <B>   - set draw color\n");
    CLog("  bg     <R> <G> <B>   - set background color\n");
    CLog("  clip   <x1 y1 x2 y2> - set clip rectangle\n");
    CLog("  clipr  <r>           - set clip circle radius\n");
    CLog("  clipc  <x> <y>       - set clip circle center\n");
    CLog("  sq     <half-side>   - set square clip side\n");
    CLog("  clear                - clear canvas\n");
    CLog("  count                - print shape count\n");
    CLog("  quit                 - exit\n");
    CLog("=================================================\n\n");
    char buf[256];
    while(true){
        CLog("> ");
        if(!fgets(buf,sizeof(buf),stdin)) break;
        string cmd(buf);
        if(!cmd.empty()&&cmd.back()=='\n') cmd.pop_back();
        if(cmd=="quit"||cmd=="exit"){ PostMessage(g_hwnd,WM_CLOSE,0,0); break; }
        else if(cmd=="clear"){
            g_shapes.clear(); g_cardinalPts.clear(); g_polyPts.clear();
            RedrawAll(); CLog("[CONSOLE] Canvas cleared\n");
        }
        else if(cmd=="count") CLog("[CONSOLE] Shapes: %zu\n",g_shapes.size());
        else if(cmd.substr(0,5)=="color"){
            int r,g,b; sscanf_s(cmd.c_str()+5,"%d %d %d",&r,&g,&b);
            g_drawColor=RGB(r,g,b); UpdateTitle();
            CLog("[CONSOLE] Color set to RGB(%d,%d,%d)\n",r,g,b);
        }
        else if(cmd.substr(0,2)=="bg"){
            int r,g,b; sscanf_s(cmd.c_str()+2,"%d %d %d",&r,&g,&b);
            g_bgColor=RGB(r,g,b); RedrawAll();
            CLog("[CONSOLE] BG color set\n");
        }
        else if(cmd.substr(0,4)=="clip"&&cmd[4]==' '){
            int x1,y1,x2,y2; sscanf_s(cmd.c_str()+5,"%d %d %d %d",&x1,&y1,&x2,&y2);
            g_clipRect={x1,y1,x2,y2};
            CLog("[CONSOLE] Clip rect set to (%d,%d)-(%d,%d)\n",x1,y1,x2,y2);
        }
        else if(cmd.substr(0,4)=="clipr"){
            int r; sscanf_s(cmd.c_str()+5,"%d",&r); g_clipCircR=r;
            CLog("[CONSOLE] Clip circle radius=%d\n",r);
        }
        else if(cmd.substr(0,4)=="clipc"){
            int x,y; sscanf_s(cmd.c_str()+5,"%d %d",&x,&y);
            g_clipCircCenter={x,y};
            CLog("[CONSOLE] Clip circle center=(%d,%d)\n",x,y);
        }
        else if(cmd.substr(0,2)=="sq"){
            int s; sscanf_s(cmd.c_str()+2,"%d",&s); g_clipSqSide=s;
            CLog("[CONSOLE] Square half-side=%d\n",s);
        }
        else if(!cmd.empty()) CLog("[CONSOLE] Unknown: '%s' (type help)\n",cmd.c_str());
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int nCmdShow)
{
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy,"CONOUT$","w",stdout);
    freopen_s(&dummy,"CONOUT$","w",stderr);
    freopen_s(&dummy,"CONIN$", "r",stdin);
    SetConsoleTitleA("2D Drawing Package – Console");
    WNDCLASSEXA wc={};
    wc.cbSize=sizeof(wc);
    wc.style=CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=WndProc;
    wc.hInstance=hInst;
    wc.hCursor=LoadCursorA(nullptr,IDC_CROSS);
    wc.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName="Draw2D";
    RegisterClassExA(&wc);
    g_hwnd=CreateWindowExA(0,"Draw2D","2D Drawing Package",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1000,680,
        nullptr,nullptr,hInst,nullptr);
    SetMenu(g_hwnd,BuildMenuBar());
    CreateBackBuffer();
    ShowWindow(g_hwnd,nCmdShow);
    UpdateWindow(g_hwnd);
    UpdateTitle();
    CLog("[APP] Ready. Select a tool from the menu, then draw with the mouse.\n");
    HANDLE hThread=CreateThread(nullptr,0,ConsoleThread,nullptr,0,nullptr);
    MSG msg;
    while(GetMessageA(&msg,nullptr,0,0)){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if(hThread){TerminateThread(hThread,0);CloseHandle(hThread);}
    FreeConsole();
    return (int)msg.wParam;
}
