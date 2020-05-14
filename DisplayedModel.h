#ifndef DISPLAYED_MODEL_H_INCLUDED
#define DISPLAYED_MODEL_H_INCLUDED

#include <windows.h>
#include <math.h>
#include <limits.h>

#include "Document.h"

#define MAX_POS INT_MAX

typedef enum {
    FORMAT_MODE_DEFAULT,
    FORMAT_MODE_WRAP
} FormatMode;

typedef enum {
         UP,
    LEFT,   RIGHT,
        DOWN
} Direction;

typedef struct {
    size_t x;
    size_t y;
} metric_t;

typedef struct {
    size_t x;
    size_t y;
} position_t;

typedef struct {
    size_t lines;
    size_t chars;
} area_t;

typedef struct {
    size_t lines;
    size_t chars;
} DefaultModel;

typedef struct {
    int isValid;
    size_t lines;
} WrapModel;

typedef struct {
    metric_t charMetric;

    area_t clientArea;
    area_t documentArea;
    WrapModel wrapModel;

    FormatMode mode;
    Document* doc;
    
    position_t shift;

    struct {
        Block* block;
        size_t blockPos;
        size_t charPos;
    } currentPos;   
} DisplayedModel;

void InitDisplayedModel(DisplayedModel* dm, TEXTMETRIC* tm);
void UpdateDisplayedModel(HWND hwnd, DisplayedModel* dm, LPARAM lParam);
void CoverDocument(HWND hwnd, DisplayedModel* dm, Document* doc);

void SwitchMode(HWND hwnd, DisplayedModel* dm, FormatMode mode);
void DisplayModel(HDC hdc, const DisplayedModel* dm);

void Scroll(HWND hwnd, DisplayedModel* dm, size_t count, Direction dir);
size_t GetCurrentPos(int pos, size_t requiredMax);
#endif // DISPLAYED_MODEL_H_INCLUDED