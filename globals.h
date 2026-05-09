#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <cmath>
#include <stack>
#include <algorithm>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─── Menu IDs ────────────────────────────────────────────────────────────────
// File
#define ID_FILE_CLEAR        1001
#define ID_FILE_SAVE         1002
#define ID_FILE_LOAD         1003
#define ID_FILE_EXIT         1004

// Preferences
#define ID_PREF_BGCOLOR      2001
#define ID_PREF_CURSOR       2002
#define ID_PREF_COLOR        2003

// Lines
#define ID_LINE_DDA          3001
#define ID_LINE_MIDPOINT     3002
#define ID_LINE_PARAMETRIC   3003

// Circles
#define ID_CIRCLE_DIRECT     4001
#define ID_CIRCLE_POLAR      4002
#define ID_CIRCLE_ITER_POLAR 4003
#define ID_CIRCLE_MIDPOINT   4004
#define ID_CIRCLE_MOD_MID    4005

// Ellipse
#define ID_ELLIPSE_DIRECT    5001
#define ID_ELLIPSE_POLAR     5002
#define ID_ELLIPSE_MIDPOINT  5003

// Curves
#define ID_CURVE_CARDINAL    6001

// Filling
#define ID_FILL_CIRC_LINES   7001
#define ID_FILL_CIRC_CIRCS   7002
#define ID_FILL_SQ_HERMITE   7003
#define ID_FILL_RECT_BEZIER  7004
#define ID_FILL_CONVEX       7005
#define ID_FILL_NONCONVEX    7006
#define ID_FILL_FLOOD_REC    7007
#define ID_FILL_FLOOD_NREC   7008

// Clipping
#define ID_CLIP_RECT_POINT   8001
#define ID_CLIP_RECT_LINE    8002
#define ID_CLIP_RECT_POLY    8003
#define ID_CLIP_SQ_POINT     8004
#define ID_CLIP_SQ_LINE      8005

// Bonus
#define ID_BONUS_CIRC_POINT  9001
#define ID_BONUS_CIRC_LINE   9002
#define ID_BONUS_HAPPY       9003
#define ID_BONUS_SAD         9004

// ─── Drawing modes ────────────────────────────────────────────────────────────
enum DrawMode {
    MODE_NONE = 0,
    MODE_LINE_DDA, MODE_LINE_MIDPOINT, MODE_LINE_PARAMETRIC,
    MODE_CIRCLE_DIRECT, MODE_CIRCLE_POLAR, MODE_CIRCLE_ITER_POLAR,
    MODE_CIRCLE_MIDPOINT, MODE_CIRCLE_MOD_MID,
    MODE_ELLIPSE_DIRECT, MODE_ELLIPSE_POLAR, MODE_ELLIPSE_MIDPOINT,
    MODE_CURVE_CARDINAL,
    MODE_FILL_CIRC_LINES, MODE_FILL_CIRC_CIRCS,
    MODE_FILL_SQ_HERMITE, MODE_FILL_RECT_BEZIER,
    MODE_FILL_CONVEX, MODE_FILL_NONCONVEX,
    MODE_FILL_FLOOD_REC, MODE_FILL_FLOOD_NREC,
    MODE_CLIP_RECT_POINT, MODE_CLIP_RECT_LINE, MODE_CLIP_RECT_POLY,
    MODE_CLIP_SQ_POINT, MODE_CLIP_SQ_LINE,
    MODE_BONUS_CIRC_POINT, MODE_BONUS_CIRC_LINE,
    MODE_BONUS_HAPPY, MODE_BONUS_SAD
};

// ─── Shape record ─────────────────────────────────────────────────────────────
struct Shape {
    DrawMode         mode;
    std::vector<POINT> pts;
    COLORREF         color;
};

// ─── Globals ──────────────────────────────────────────────────────────────────
extern HWND              g_hwnd;
extern HDC               g_memDC;
extern HBITMAP           g_memBmp;
extern int               g_W, g_H;

extern DrawMode          g_mode;
extern COLORREF          g_drawColor;
extern COLORREF          g_bgColor;

extern bool              g_lbDown;
extern POINT             g_ptStart, g_ptPrev;

extern std::vector<Shape>  g_shapes;
extern std::vector<POINT>  g_cardinalPts;
extern std::vector<POINT>  g_polyPts;

extern int  g_fillQuarter;
extern bool g_clipActive;
extern RECT g_clipRect;
extern int  g_clipSqSide;

// console log
void CLog(const char* fmt, ...);