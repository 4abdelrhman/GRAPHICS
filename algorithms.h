#pragma once
#include "globals.h"
#include <algorithm>
using namespace std;
inline void PutPixel(HDC hdc, int x, int y, COLORREF c)
{
    SetPixel(hdc, x, y, c);
}

// DDA
void DDALine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1, dy = y2 - y1;
    int steps = max(abs(dx), abs(dy));
    if (steps == 0) { PutPixel(hdc, x1, y1, c); return; }
    float xi = (float)dx / steps;
    float yi = (float)dy / steps;
    float x = (float)x1, y = (float)y1;
    for (int i = 0; i <= steps; ++i) {
        PutPixel(hdc, (int)round(x), (int)round(y), c);
        x += xi;  y += yi;
    }
}

// Midpoint (Bresenham)
void MidpointLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = abs(x2-x1), dy = abs(y2-y1);
    int sx = (x1<x2)?1:-1, sy = (y1<y2)?1:-1;
    int err = dx - dy;
    for(;;) {
        PutPixel(hdc, x1, y1, c);
        if (x1==x2 && y1==y2) break;
        int e2 = 2*err;
        if (e2 > -dy){ err -= dy; x1 += sx; }
        if (e2 <  dx){ err += dx; y1 += sy; }
    }
}

// Parametric 
void ParametricLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int steps = max(abs(x2-x1), abs(y2-y1));
    if (steps == 0) { PutPixel(hdc, x1, y1, c); return; }
    for (int i = 0; i <= steps; ++i) {
        float t = (float)i / steps;
        int x = (int)round(x1 + t*(x2-x1));
        int y = (int)round(y1 + t*(y2-y1));
        PutPixel(hdc, x, y, c);
    }
}


static void Plot8(HDC hdc, int cx, int cy, int x, int y, COLORREF c)
{
    PutPixel(hdc,cx+x,cy+y,c); PutPixel(hdc,cx-x,cy+y,c);
    PutPixel(hdc,cx+x,cy-y,c); PutPixel(hdc,cx-x,cy-y,c);
    PutPixel(hdc,cx+y,cy+x,c); PutPixel(hdc,cx-y,cy+x,c);
    PutPixel(hdc,cx+y,cy-x,c); PutPixel(hdc,cx-y,cy-x,c);
}

// Direct (equation)
void DirectCircle(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    for (int x = 0; x <= (int)(r/sqrt(2.0))+1; ++x) {
        int y = (int)round(sqrt((double)r*r - (double)x*x));
        Plot8(hdc, cx, cy, x, y, c);
    }
}

// Polar
void PolarCircle(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    float step = 1.0f / r;
    for (float t = 0; t < (float)(2*M_PI); t += step) {
        int x = (int)round(cx + r*cos(t));
        int y = (int)round(cy + r*sin(t));
        PutPixel(hdc, x, y, c);
    }
}

// Iterative Polar
void IterativePolarCircle(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    float eps = 1.0f / r;
    float x = (float)r, y = 0;
    while (x >= y) {
        Plot8(hdc, cx, cy, (int)round(x), (int)round(y), c);
        y += eps;
        x = (float)sqrt((double)r*r - (double)y*y);
    }
}

// Midpoint
void MidpointCircle(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    int x=0, y=r, d=1-r;
    Plot8(hdc,cx,cy,x,y,c);
    while (x < y) {
        if (d < 0) d += 2*x+3;
        else { d += 2*(x-y)+5; --y; }
        ++x;
        Plot8(hdc,cx,cy,x,y,c);
    }
}

// Modified Midpoint
void ModifiedMidpointCircle(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    int x=0, y=r;
    int d = 1 - r;
    int deltaE = 3, deltaSE = -2*r+5;
    Plot8(hdc,cx,cy,x,y,c);
    while (x < y) {
        if (d < 0) { d += deltaE; deltaE += 2; deltaSE += 2; }
        else { d += deltaSE; deltaE += 2; deltaSE += 4; --y; }
        ++x;
        Plot8(hdc,cx,cy,x,y,c);
    }
}

//  ELLIPSE
static void Plot4E(HDC hdc, int cx, int cy, int x, int y, COLORREF c)
{
    PutPixel(hdc,cx+x,cy+y,c); PutPixel(hdc,cx-x,cy+y,c);
    PutPixel(hdc,cx+x,cy-y,c); PutPixel(hdc,cx-x,cy-y,c);
}

// Direct 
void DirectEllipse(HDC hdc, int cx, int cy, int rx, int ry, COLORREF c)
{
    for (int x = -rx; x <= rx; ++x) {
        double y = ry * sqrt(1.0 - (double)x*x/((double)rx*rx));
        PutPixel(hdc, cx+x, cy+(int)round(y), c);
        PutPixel(hdc, cx+x, cy-(int)round(y), c);
    }
}

// Polar
void PolarEllipse(HDC hdc, int cx, int cy, int rx, int ry, COLORREF c)
{
    int steps = max(rx,ry)*4;
    for (int i = 0; i <= steps; ++i) {
        float t = (float)(2*M_PI*i/steps);
        int x = (int)round(cx + rx*cos(t));
        int y = (int)round(cy + ry*sin(t));
        PutPixel(hdc, x, y, c);
    }
}

// Midpoint
void MidpointEllipse(HDC hdc, int cx, int cy, int rx, int ry, COLORREF c)
{
    long long rx2 = (long long)rx*rx, ry2 = (long long)ry*ry;
    int x=0, y=ry;
    long long px=0, py=2*rx2*y;
    Plot4E(hdc,cx,cy,x,y,c);
    long long p1 = (long long)(ry2 - rx2*ry + 0.25*rx2);
    while (px < py) {
        ++x; px += 2*ry2;
        if (p1 < 0) p1 += ry2 + px;
        else { --y; py -= 2*rx2; p1 += ry2 + px - py; }
        Plot4E(hdc,cx,cy,x,y,c);
    }
    long long p2 = (long long)(ry2*(x+0.5)*(x+0.5) + rx2*(y-1)*(y-1) - rx2*ry2);
    while (y > 0) {
        --y;
        if (p2 > 0) p2 += rx2 - 2*rx2*y;
        else { ++x; p2 += 2*ry2*x - 2*rx2*y + rx2; }
        Plot4E(hdc,cx,cy,x,y,c);
    }
}

//  CARDINAL SPLINE
void CardinalSpline(HDC hdc, const vector<POINT>& pts, float tension, COLORREF c)
{
    if (pts.size() < 2) return;
    int n = (int)pts.size();
    int steps = 40;
    for (int i = 0; i < n-1; ++i) {
        POINT p0 = pts[max(0,i-1)];
        POINT p1 = pts[i];
        POINT p2 = pts[i+1];
        POINT p3 = pts[min(n-1,i+2)];
        for (int s = 0; s <= steps; ++s) {
            float t  = (float)s/steps;
            float t2 = t*t, t3 = t2*t;
            float h1 =  2*t3 - 3*t2 + 1;
            float h2 = -2*t3 + 3*t2;
            float h3 =     t3 - 2*t2 + t;
            float h4 =     t3 -   t2;
            float x = h1*p1.x + h2*p2.x
                    + tension*h3*(p2.x-p0.x) + tension*h4*(p3.x-p1.x);
            float y = h1*p1.y + h2*p2.y
                    + tension*h3*(p2.y-p0.y) + tension*h4*(p3.y-p1.y);
            PutPixel(hdc,(int)round(x),(int)round(y),c);
        }
    }
}

//  HERMITE CURVE  (for fill use)
static POINT HermitePoint(POINT p1,POINT p4,POINT r1,POINT r4,float t)
{
    float t2=t*t,t3=t2*t;
    float h1= 2*t3-3*t2+1, h2=-2*t3+3*t2, h3=t3-2*t2+t, h4=t3-t2;
    POINT p;
    p.x=(int)round(h1*p1.x+h2*p4.x+h3*r1.x+h4*r4.x);
    p.y=(int)round(h1*p1.y+h2*p4.y+h3*r1.y+h4*r4.y);
    return p;
}

// Bezier (De Casteljau, for fill)
static POINT BezierPoint(POINT p0,POINT p1,POINT p2,POINT p3,float t)
{
    float u=1-t;
    float x=u*u*u*p0.x+3*u*u*t*p1.x+3*u*t*t*p2.x+t*t*t*p3.x;
    float y=u*u*u*p0.y+3*u*u*t*p1.y+3*u*t*t*p2.y+t*t*t*p3.y;
    return {(int)round(x),(int)round(y)};
}

//  FILLING

// helper: quarter mask  (1=TR,2=TL,3=BL,4=BR)
static bool InQuarter(int dx, int dy, int q)
{
    switch(q){
        case 1: return dx>=0 && dy<=0;
        case 2: return dx<=0 && dy<=0;
        case 3: return dx<=0 && dy>=0;
        case 4: return dx>=0 && dy>=0;
    }
    return true;
}

// Fill circle with lines
void FillCircleWithLines(HDC hdc, int cx, int cy, int r, int quarter, COLORREF c)
{
    for (int y=-r; y<=r; ++y) {
        int x=(int)sqrt(max(0.0,(double)r*r-(double)y*y));
        int xStart=-x, xEnd=x;
        for (int xx=xStart; xx<=xEnd; ++xx) {
            if (InQuarter(xx,y,quarter))
                PutPixel(hdc,cx+xx,cy+y,c);
        }
    }
    MidpointCircle(hdc,cx,cy,r,c);
}

// Fill circle with circles
void FillCircleWithCircles(HDC hdc, int cx, int cy, int r, int quarter, COLORREF c)
{
    MidpointCircle(hdc,cx,cy,r,c);
    for (int ri=r-1; ri>0; ri-=3) {
        float step = 1.0f/ri;
        for (float t=0; t<(float)(2*M_PI); t+=step) {
            int dx=(int)round(ri*cos(t));
            int dy=(int)round(ri*sin(t));
            if (InQuarter(dx,dy,quarter))
                PutPixel(hdc,cx+dx,cy+dy,c);
        }
    }
}

// Fill square with Hermite curves (vertical)
void FillSquareWithHermite(HDC hdc, int x1, int y1, int side, COLORREF c)
{
    int x2=x1+side, y2=y1+side;
    DDALine(hdc,x1,y1,x2,y1,c);
    DDALine(hdc,x2,y1,x2,y2,c);
    DDALine(hdc,x2,y2,x1,y2,c);
    DDALine(hdc,x1,y2,x1,y1,c);
    for (int x=x1; x<=x2; x+=4) {
        POINT p1={x,y1},p4={x,y2};
        POINT r1={side/2,0},r4={-side/2,0};
        int steps=60;
        for (int s=0;s<steps;++s){
            POINT a=HermitePoint(p1,p4,r1,r4,(float)s/steps);
            POINT b=HermitePoint(p1,p4,r1,r4,(float)(s+1)/steps);
            PutPixel(hdc,a.x,a.y,c);
            PutPixel(hdc,b.x,b.y,c);
        }
    }
}

// Fill rectangle with Bezier curves (horizontal)
void FillRectWithBezier(HDC hdc, int x1, int y1, int w, int h, COLORREF c)
{
    int x2=x1+w, y2=y1+h;
    DDALine(hdc,x1,y1,x2,y1,c);
    DDALine(hdc,x2,y1,x2,y2,c);
    DDALine(hdc,x2,y2,x1,y2,c);
    DDALine(hdc,x1,y2,x1,y1,c);
    for (int y=y1; y<=y2; y+=4) {
        POINT p0={x1,y},p1={x1+w/3,y-h/3},p2={x1+2*w/3,y+h/3},p3={x2,y};
        int steps=60;
        for (int s=0;s<steps;++s){
            POINT a=BezierPoint(p0,p1,p2,p3,(float)s/steps);
            POINT b=BezierPoint(p0,p1,p2,p3,(float)(s+1)/steps);
            DDALine(hdc,a.x,a.y,b.x,b.y,c);
        }
    }
}


//  Scan-line convex fill 
void ScanlineFill(HDC hdc, vector<POINT> poly, COLORREF c)
{
    if (poly.size()<3) return;
    int yMin=poly[0].y, yMax=poly[0].y;
    for(auto&p:poly){yMin = min(yMin, static_cast<int>(p.y));yMax = max<int>(yMax, p.y);}
    int n=(int)poly.size();
    for (int y=yMin;y<=yMax;++y){
        vector<int> xs;
        for(int i=0;i<n;++i){
            POINT a=poly[i],b=poly[(i+1)%n];
            if((a.y<=y&&b.y>y)||(b.y<=y&&a.y>y)){
                int x=a.x+(y-a.y)*(b.x-a.x)/(b.y-a.y);
                xs.push_back(x);
            }
        }
        sort(xs.begin(),xs.end());
        for(int i=0;i+1<(int)xs.size();i+=2)
            for(int x=xs[i];x<=xs[i+1];++x)
                PutPixel(hdc,x,y,c);
    }
}

//  Non-convex (same scan-line, handles concavities too) 
void NonConvexFill(HDC hdc,vector<POINT>& poly, COLORREF c)
{
    ScanlineFill(hdc, poly, c);
}

//  Recursive flood fill 
void FloodFillRec(HDC hdc, int x, int y, COLORREF fill, COLORREF target,
                  int x0, int y0, int x1, int y1, int depth=0)
{
    if (depth > 5000) return;
    if (x<x0||x>x1||y<y0||y>y1) return;
    if (GetPixel(hdc,x,y) != target) return;
    PutPixel(hdc,x,y,fill);
    FloodFillRec(hdc,x+1,y,fill,target,x0,y0,x1,y1,depth+1);
    FloodFillRec(hdc,x-1,y,fill,target,x0,y0,x1,y1,depth+1);
    FloodFillRec(hdc,x,y+1,fill,target,x0,y0,x1,y1,depth+1);
    FloodFillRec(hdc,x,y-1,fill,target,x0,y0,x1,y1,depth+1);
}

//  Non-recursive (stack-based) flood fill 
void FloodFillNonRec(HDC hdc, int sx, int sy, COLORREF fill,
                     int x0, int y0, int x1, int y1)
{
    COLORREF target = GetPixel(hdc, sx, sy);
    if (target == fill) return;
    std::stack<POINT> stk;
    stk.push({sx, sy});
    while (!stk.empty()){
        POINT p = stk.top(); stk.pop();
        int x=p.x, y=p.y;
        if (x<x0||x>x1||y<y0||y>y1) continue;
        if (GetPixel(hdc,x,y) != target) continue;
        PutPixel(hdc,x,y,fill);
        if (x+1<=x1 && GetPixel(hdc,x+1,y)==target) stk.push({x+1,y});
        if (x-1>=x0 && GetPixel(hdc,x-1,y)==target) stk.push({x-1,y});
        if (y+1<=y1 && GetPixel(hdc,x,y+1)==target) stk.push({x,y+1});
        if (y-1>=y0 && GetPixel(hdc,x,y-1)==target) stk.push({x,y-1});
    }
}

//  CLIPPING

//  Point inside rect 
bool PointInRect(POINT p, RECT r)
{
    return p.x>=r.left && p.x<=r.right && p.y>=r.top && p.y<=r.bottom;
}

//  Cohen-Sutherland 
enum CS_CODE { INSIDE=0,LEFT=1,RIGHT=2,BOTTOM=4,TOP=8 };
static int CSCode(int x,int y,RECT r){
    int c=INSIDE;
    if(x<r.left)c|=LEFT; else if(x>r.right)c|=RIGHT;
    if(y<r.top) c|=TOP;  else if(y>r.bottom)c|=BOTTOM;
    return c;
}
bool CohenSutherland(int&x1,int&y1,int&x2,int&y2,RECT r)
{
    int c1=CSCode(x1,y1,r),c2=CSCode(x2,y2,r);
    while(true){
        if(!(c1|c2)) return true;
        if(c1&c2)    return false;
        int c=c1?c1:c2;
        int x,y;
        if(c&BOTTOM){x=x1+(x2-x1)*(r.bottom-y1)/(y2-y1);y=r.bottom;}
        else if(c&TOP){x=x1+(x2-x1)*(r.top-y1)/(y2-y1);y=r.top;}
        else if(c&RIGHT){y=y1+(y2-y1)*(r.right-x1)/(x2-x1);x=r.right;}
        else{y=y1+(y2-y1)*(r.left-x1)/(x2-x1);x=r.left;}
        if(c==c1){x1=x;y1=y;c1=CSCode(x1,y1,r);}
        else{x2=x;y2=y;c2=CSCode(x2,y2,r);}
    }
}

//  Sutherland-Hodgman polygon clip
static vector<POINT> ClipEdge(vector<POINT>& in,POINT a,POINT b)
{
    vector<POINT> out;
    int n=(int)in.size();
    auto inside=[&](POINT p){
        return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x)>=0;
    };
    auto intersect=[&](POINT p,POINT q)->POINT{
        float dx1=b.x-a.x,dy1=b.y-a.y;
        float dx2=q.x-p.x,dy2=q.y-p.y;
        float t=((a.x-p.x)*dy2-(a.y-p.y)*dx2)/(dx1*dy2-dy1*dx2);
        return{(int)round(a.x+t*dx1),(int)round(a.y+t*dy1)};
    };
    for(int i=0;i<n;++i){
        POINT cur=in[i],prev=in[(i+n-1)%n];
        bool ci=inside(cur),pi=inside(prev);
        if(ci){if(!pi)out.push_back(intersect(prev,cur));out.push_back(cur);}
        else if(pi) out.push_back(intersect(prev,cur));
    }
    return out;
}
void SutherlandHodgman(vector<POINT> poly,RECT r,vector<POINT>&out)
{
    POINT tl={r.left,r.top},tr={r.right,r.top};
    POINT br={r.right,r.bottom},bl={r.left,r.bottom};
    poly=ClipEdge(poly,tl,tr);
    poly=ClipEdge(poly,tr,br);
    poly=ClipEdge(poly,br,bl);
    poly=ClipEdge(poly,bl,tl);
    out=poly;
}

//  Circle clipping (bonus)
bool PointInCircle(POINT p, POINT center, int r)
{
    int dx=p.x-center.x, dy=p.y-center.y;
    return dx*dx+dy*dy <= r*r;
}

// Clip line to circle using parametric intersection
bool ClipLineCircle(int&x1,int&y1,int&x2,int&y2,POINT c,int r)
{
    float dx=float(x2-x1),dy=float(y2-y1);
    float fx=float(x1-c.x),fy=float(y1-c.y);
    float a=dx*dx+dy*dy;
    float b=2*(fx*dx+fy*dy);
    float cc2=fx*fx+fy*fy-(float)r*r;
    float disc=b*b-4*a*cc2;
    if(disc<0) return false;
    float sq=sqrt(disc);
    float t1=(-b-sq)/(2*a), t2=(-b+sq)/(2*a);
    float tMin=max(0.0f,t1), tMax=min(1.0f,t2);
    if(tMin>tMax) return false;
    x2=(int)round(x1+tMax*dx); y2=(int)round(y1+tMax*dy);
    x1=(int)round(x1+tMin*dx); y1=(int)round(y1+tMin*dy);
    return true;
}

//  SMILEY FACES
void DrawHappyFace(HDC hdc,int cx,int cy,int r,COLORREF c)
{
    MidpointCircle(hdc,cx,cy,r,c);
    int ey=cy-r/3, ex=r/3;
    MidpointCircle(hdc,cx-ex,ey,r/10,c);
    MidpointCircle(hdc,cx+ex,ey,r/10,c);
    DDALine(hdc,cx,cy-r/8,cx,cy+r/8,c);
    vector<POINT> mouth={
        {cx-(int)(r*0.45f),cy+(int)(r*0.15f)},
        {cx-(int)(r*0.25f),cy+(int)(r*0.45f)},
        {cx,               cy+(int)(r*0.55f)},
        {cx+(int)(r*0.25f),cy+(int)(r*0.45f)},
        {cx+(int)(r*0.45f),cy+(int)(r*0.15f)}
    };
    CardinalSpline(hdc,mouth,0.5f,c);
}

// SAD FACE
void DrawSadFace(HDC hdc,int cx,int cy,int r,COLORREF c)
{
    MidpointCircle(hdc,cx,cy,r,c);
    int ey=cy-r/3, ex=r/3;
    MidpointCircle(hdc,cx-ex,ey,r/10,c);
    MidpointCircle(hdc,cx+ex,ey,r/10,c);
    DDALine(hdc,cx,cy,cx,cy+r/8,c);
    vector<POINT> mouth={
        {cx-(int)(r*0.45f),cy+(int)(r*0.50f)},
        {cx-(int)(r*0.25f),cy+(int)(r*0.25f)},
        {cx,               cy+(int)(r*0.20f)},
        {cx+(int)(r*0.25f),cy+(int)(r*0.25f)},
        {cx+(int)(r*0.45f),cy+(int)(r*0.50f)}
    };
    CardinalSpline(hdc,mouth,0.5f,c);
}

void RenderShape(HDC hdc, const Shape& s);
