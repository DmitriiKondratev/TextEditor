#include "DisplayedModel.h"

// don't touch this parameter
// DISPLAY_STEP
#define STEP 1

static void InitModelPos(ModelPos* pMP, Block* block) {
    assert(pMP);

    pMP->block = block;
    pMP->pos.x = 0;
    pMP->pos.y = 0;
}

void InitDisplayedModel(DisplayedModel* dm, const TEXTMETRIC* tm) {
    assert(dm);
    assert(tm);

    dm->charMetric.x = tm->tmAveCharWidth;
    dm->charMetric.y = tm->tmHeight + tm->tmExternalLeading;

    dm->mode = FORMAT_MODE_DEFAULT;
    dm->doc = NULL;

    dm->clientArea.chars = 0;
    dm->clientArea.lines = 0;

    dm->documentArea.chars = 0;
    dm->documentArea.lines = 0;

    dm->wrapModel.isValid = 0;
    dm->wrapModel.lines = 0;

    InitScrollBar(&(dm->scrollBars.horizontal));
    InitScrollBar(&(dm->scrollBars.vertical));
    InitModelPos(&(dm->scrollBars.modelPos), NULL);

    #ifdef CARET_ON
        dm->caret.clientPos.x = 0;
        dm->caret.clientPos.y = 0;
        dm->caret.isHidden.x = 0;
        dm->caret.isHidden.y = 0;
        InitModelPos(&(dm->caret.modelPos), NULL);
    #endif
}

static void PassPrev(ModelPos* modelPos, size_t delta) {
    assert(modelPos);

    modelPos->pos.y -= delta;
    for(; delta > 0; --delta) {
        assert(modelPos->block);
        modelPos->block = modelPos->block->prev;
    }
}

static void PassNext(ModelPos* modelPos, size_t delta) {
    assert(modelPos);

    modelPos->pos.y += delta;
    for(; delta > 0; --delta) {
        assert(modelPos->block);
        modelPos->block = modelPos->block->next;
    }
}

#ifndef NDEBUG // ======================================= /
    static void PrintWrapModel(const WrapModel* wrapModel) {
        if (wrapModel->isValid) {
            printf("Wrap model: %u\n", wrapModel->lines);
        } else {
            printf("Wrap model is not valid\n");
        }
    }
#endif // ============================================== /

static void CountLines(Block* startBlock, const Block* lastBlock, DisplayedModel* dm) {
    assert(startBlock && dm);

    while (startBlock != lastBlock) {
        if (startBlock->data.len > 0) {
            dm->wrapModel.lines += DIV_WITH_ROUND_UP(startBlock->data.len, dm->clientArea.chars);
        } else {
            ++dm->wrapModel.lines;
        }
        startBlock = startBlock->next;
    }
}

static size_t BuildWrapModel(HWND hwnd, DisplayedModel* dm) {
    assert(dm);
    #ifndef NDEBUG // ================================/
        // printf("BuildWrapModel\n");
    #endif

    size_t absolutePos;
    Block* block = dm->doc->blocks->nodes;

    dm->scrollBars.modelPos.pos.x = 0;
    dm->wrapModel.lines = 0;

    #ifdef CARET_ON
        dm->caret.linePos = 0;

        if (dm->caret.modelPos.pos.y < dm->scrollBars.modelPos.pos.y) {
            CountLines(block, dm->caret.modelPos.block, dm);
            block = dm->caret.modelPos.block;

            dm->caret.linePos = dm->wrapModel.lines;
            dm->caret.linePos += dm->caret.modelPos.pos.x / dm->clientArea.chars;

            if (!dm->caret.isHidden.y) { CaretHide(hwnd, &(dm->caret.isHidden.y)); }
            dm->caret.clientPos.y = 0;
        }
    #endif

    CountLines(block, dm->scrollBars.modelPos.block, dm);
    block = dm->scrollBars.modelPos.block;

    absolutePos = dm->wrapModel.lines;

    #ifdef CARET_ON
        if (dm->caret.modelPos.pos.y >= dm->scrollBars.modelPos.pos.y) {
            CountLines(block, dm->caret.modelPos.block, dm);
            block = dm->caret.modelPos.block;

            dm->caret.linePos = dm->wrapModel.lines;
            dm->caret.linePos += dm->caret.modelPos.pos.x / dm->clientArea.chars;

            dm->caret.clientPos.y = dm->caret.linePos - absolutePos;

            if (dm->caret.clientPos.y > DECREMENT_OF(dm->clientArea.lines)) {
                if (!dm->caret.isHidden.y) { CaretHide(hwnd, &(dm->caret.isHidden.y)); }
                dm->caret.clientPos.y = DECREMENT_OF(dm->clientArea.lines);
            } else if (dm->caret.isHidden.y) {
                CaretShow(hwnd, &(dm->caret.isHidden.y));
            }
        }
    #endif

    CountLines(block, NULL, dm);
    block = NULL;

    dm->wrapModel.isValid = 1; // TODO: delete
    return absolutePos;
}

#ifndef NDEBUG // ======================================= /
    static void PrintSBs(const DisplayedModel* dm) {
        assert(dm);

        printf("\tHorizontal\n");
        PrintScrollBar(&(dm->scrollBars.horizontal));
        printf("\tVertical\n");
        PrintScrollBar(&(dm->scrollBars.vertical));
        putchar('\n');
    }
#endif // ============================================== /

void CoverDocument(HWND hwnd, DisplayedModel* dm, Document* doc) {
    #ifndef NDEBUG // ================================/
        // printf("Cover document\n");
    #endif // =======================================/
    assert(dm && doc);

    dm->doc = doc;
    dm->documentArea.lines = doc->blocks->len;
    dm->documentArea.chars = GetMaxBlockLen(doc->blocks);

    dm->wrapModel.isValid = 0; // TODO: delete

    InitScrollBar(&(dm->scrollBars.horizontal));
    InitScrollBar(&(dm->scrollBars.vertical));
    InitModelPos(&(dm->scrollBars.modelPos), doc->blocks->nodes);

    #ifdef CARET_ON
        dm->caret.clientPos.x = 0;
        dm->caret.clientPos.y = 0;
        if (dm->caret.isHidden.x) { CaretShow(hwnd, &(dm->caret.isHidden.x)); }
        if (dm->caret.isHidden.y) { CaretShow(hwnd, &(dm->caret.isHidden.y)); }
        InitModelPos(&(dm->caret.modelPos), doc->blocks->nodes);
    #endif

    switch (dm->mode) {
    case FORMAT_MODE_DEFAULT:
        dm->scrollBars.horizontal.maxPos = GetAbsoluteMaxPos(dm->documentArea.chars, dm->clientArea.chars);
        dm->scrollBars.vertical.maxPos = GetAbsoluteMaxPos(dm->documentArea.lines, dm->clientArea.lines);
        break;

    case FORMAT_MODE_WRAP:
        BuildWrapModel(hwnd, dm);

        #ifndef NDEBUG // ================================/
            // PrintWrapModel(&(dm->wrapModel));
        #endif // =======================================/

        dm->scrollBars.vertical.maxPos = GetAbsoluteMaxPos(dm->wrapModel.lines, dm->clientArea.lines);
        break;

    default:
        PrintError(NULL, ERR_PARAM, __FILE__, __LINE__);
        return;
    }

    SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
    SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);

    #ifndef NDEBUG // ================================/
        // PrintSBs(dm);
    #endif // =======================================/
}

#ifndef NDEBUG // ======================================= /
    static void PrintPos(const DisplayedModel* dm) {
        assert(dm);
        printf("blockPos = %u, ", dm->scrollBars.modelPos.pos.y);
        printf("charPos = %u\n", dm->scrollBars.modelPos.pos.x);
    }
#endif // ============================================== /

static size_t PrintLine(size_t lineIndex, HDC hdc, const DisplayedModel* dm, Fragment** fragment, size_t displayedChars, size_t delta) {
    assert(dm && fragment && *fragment);
    assert(displayedChars);

    size_t length = 0;

    for (size_t j = 0; j < displayedChars; j += length) {
        if ((*fragment)->data.len - delta <= displayedChars - j) {
            length = (*fragment)->data.len - delta;

            TextOut(hdc,
                j * dm->charMetric.x,
                lineIndex * dm->charMetric.y,
                dm->doc->text->data + (*fragment)->data.pos + delta,
                length);
            
            delta = 0;
            *fragment = (*fragment)->next;
        } else {
            length = displayedChars - j;

            TextOut(hdc,
                j * dm->charMetric.x,
                lineIndex * dm->charMetric.y,
                dm->doc->text->data + (*fragment)->data.pos + delta,
                length);
            
            delta += length;
            break;
        }
    }

    return delta;
}

void DisplayModel(HDC hdc, const DisplayedModel* dm) {
    assert(dm && dm->doc && dm->doc->text);

    Block* block = dm->scrollBars.modelPos.block;
    Fragment* fragment;
    size_t displayedLines, displayedChars;
    size_t linesBlock, nextLine;
    size_t delta;

    #ifndef NDEBUG // ================================/
        // printf("Display model:\n");
        // printf("\tHorizontal\n");
        // PrintScrollBar(&(dm->scrollBars.horizontal));
        // printf("\tVertical\n");
        // PrintScrollBar(&(dm->scrollBars.vertical));
        // putchar('\n');
    #endif // =======================================/

    switch (dm->mode) {
    case FORMAT_MODE_DEFAULT:
        displayedLines = min(dm->clientArea.lines, dm->documentArea.lines - dm->scrollBars.vertical.pos);

        // print
        for (size_t i = 0; i < displayedLines; ++i) {
            if (dm->scrollBars.horizontal.pos < block->data.len) {
                fragment = block->data.fragments->nodes;
                displayedChars = min(dm->clientArea.chars, block->data.len - dm->scrollBars.horizontal.pos);
                
                // pass
                delta = dm->scrollBars.horizontal.pos;
                while (delta > fragment->data.len) {
                    delta -= fragment->data.len;
                    fragment = fragment->next;
                }

                PrintLine(i, hdc, dm, &fragment, displayedChars, delta);
            }
            
            block = block->next;
        }
        break;

    case FORMAT_MODE_WRAP:
        nextLine = dm->scrollBars.modelPos.pos.x;
        // nextLine = DIV_WITH_ROUND_UP(dm->scrollBars.modelPos.pos.x, dm->clientArea.chars);

        displayedLines = min(dm->clientArea.lines, dm->wrapModel.lines - dm->scrollBars.vertical.pos);
        fragment = block->data.fragments->nodes;

        // pass
        delta = nextLine * dm->clientArea.chars;
        while (delta > fragment->data.len) {
            delta -= fragment->data.len;
            fragment = fragment->next;
        }

        // print
        for (size_t i = 0; i < displayedLines; nextLine = 0,
            block = block->next, fragment = block ? block->data.fragments->nodes : NULL) {

            // empty line
            if (!block->data.len) {
                ++i;
                continue;
            }

            linesBlock = DIV_WITH_ROUND_UP(block->data.len, dm->clientArea.chars);
            displayedChars = dm->clientArea.chars;

            if (linesBlock - nextLine <= displayedLines - i) {

                for (; nextLine < linesBlock - 1; ++i, ++nextLine) {
                    delta = PrintLine(i, hdc, dm, &fragment, displayedChars, delta);
                }

                displayedChars = (block->data.len % dm->clientArea.chars) ? (block->data.len % dm->clientArea.chars) : dm->clientArea.chars;
                delta = PrintLine(i, hdc, dm, &fragment, displayedChars, delta);
                
                ++i;
            } else {
                for (; i < displayedLines; ++i, ++nextLine) {
                    delta = PrintLine(i, hdc, dm, &fragment, displayedChars, delta);
                }
            }
        }
        break;

    default:
        PrintError(NULL, ERR_PARAM, __FILE__, __LINE__);
        return;
    }
}

static void UpdateScrollPos_Back(DisplayedModel* dm, size_t count) {
    assert(dm);

    switch (dm->mode) {
    case FORMAT_MODE_DEFAULT:
        // for remaining
        while (dm->scrollBars.modelPos.pos.y != dm->scrollBars.vertical.pos) {
            dm->scrollBars.modelPos.block = dm->scrollBars.modelPos.block->prev;
            --dm->scrollBars.modelPos.pos.y;
        }
        break;

    case FORMAT_MODE_WRAP:
        // for remaining
        for (; count > 0;) {
            if (dm->scrollBars.modelPos.pos.x + 1 <= count) {
                count -= dm->scrollBars.modelPos.pos.x + 1;
            } else {
                dm->scrollBars.modelPos.pos.x -= count;
                break;
            }

            // prev block
            dm->scrollBars.modelPos.block = dm->scrollBars.modelPos.block->prev;
            --(dm->scrollBars.modelPos.pos.y);
            if (dm->scrollBars.modelPos.block->data.len > 0) {
                dm->scrollBars.modelPos.pos.x = DIV_WITH_ROUND_UP(dm->scrollBars.modelPos.block->data.len, dm->clientArea.chars) - 1;
            } else {
                dm->scrollBars.modelPos.pos.x = 0;
            }
        }
        break;

    default:
        PrintError(NULL, ERR_PARAM, __FILE__, __LINE__);
        break;
    }
}

static void UpdateScrollPos_Forward(DisplayedModel* dm, size_t count) {
    assert(dm);

    size_t delta;
    size_t linesBlock = 0;
    Block* block = dm->scrollBars.modelPos.block;

    switch (dm->mode) {
    case FORMAT_MODE_DEFAULT:
        // for remaining
        while (dm->scrollBars.modelPos.pos.y != dm->scrollBars.vertical.pos) {
            dm->scrollBars.modelPos.block = dm->scrollBars.modelPos.block->next;
            ++dm->scrollBars.modelPos.pos.y;
        }
        break;

    case FORMAT_MODE_WRAP:
        // for remaining
        if (block->data.len > 0) {
            linesBlock = DIV_WITH_ROUND_UP(block->data.len, dm->clientArea.chars);
        } else {
            linesBlock = 1;
        }

        delta = linesBlock - dm->scrollBars.modelPos.pos.x;
        dm->scrollBars.modelPos.pos.x = 0;

        for (; count > 0; linesBlock = 0) {
            if (count >= delta) {
                count -= delta;
            } else {
                dm->scrollBars.modelPos.pos.x = linesBlock - (delta - count);
                break;
            }

            block = block->next;

            // next block
            dm->scrollBars.modelPos.block = block;
            ++dm->scrollBars.modelPos.pos.y;

            if (block->data.len > 0) {
                linesBlock = DIV_WITH_ROUND_UP(block->data.len, dm->clientArea.chars);
            } else {
                linesBlock = 1;
            }
            delta = linesBlock;
        }
        break;

    default:
        PrintError(NULL, ERR_PARAM, __FILE__, __LINE__);
        break;
    }
}

static void InitRect(RECT* rectangle, const DisplayedModel* dm) {
    assert(rectangle);

    rectangle->top = 0;
    rectangle->left = 0;

    if (dm) {
        rectangle->right = (long) (dm->charMetric.x * dm->clientArea.chars);
        rectangle->bottom = (long) (dm->charMetric.y * dm->clientArea.lines);
    } else {
        rectangle->right = 0;
        rectangle->bottom = 0;
    }
}

size_t Scroll(HWND hwnd, DisplayedModel* dm, size_t count, Direction direction, RECT* rectangle) {
    assert(dm);
    assert(rectangle);

    int xScroll = 0;
    int yScroll = 0;

    InitRect(rectangle, dm);

    switch (direction) {
    case UP:
        count = min(count, dm->scrollBars.vertical.pos);
        if (count) {
            dm->scrollBars.vertical.pos -= count;

            UpdateScrollPos_Back(dm, count);
            SetRelativePos(hwnd, &(dm->scrollBars.vertical), SB_VERT);

            yScroll = (int) (count * dm->charMetric.y);
            rectangle->bottom = dm->charMetric.y * min(count, dm->clientArea.lines);
        }
        break;
    // UP

    case DOWN:
        count = min(count, dm->scrollBars.vertical.maxPos - dm->scrollBars.vertical.pos);
        if (count) {
            dm->scrollBars.vertical.pos += count;

            UpdateScrollPos_Forward(dm, count);
            SetRelativePos(hwnd, &(dm->scrollBars.vertical), SB_VERT);

            yScroll = - (int) (count * dm->charMetric.y);
            rectangle->top = dm->charMetric.y * (dm->clientArea.lines - min(count, dm->clientArea.lines));
        }
        break;
    // DOWN

    case LEFT:
        assert(dm->mode == FORMAT_MODE_DEFAULT);

        count = min(count, dm->scrollBars.horizontal.pos);
        if (count) {
            dm->scrollBars.horizontal.pos -= count;

            SetRelativePos(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);

            xScroll = (int) (count * dm->charMetric.x);
            rectangle->left = dm->charMetric.x * (dm->clientArea.chars - min(count, dm->clientArea.chars));
        }
        break;
    // LEFT

    case RIGHT:
        assert(dm->mode == FORMAT_MODE_DEFAULT);

        count = min(count, dm->scrollBars.horizontal.maxPos - dm->scrollBars.horizontal.pos);
        if (count) {
            dm->scrollBars.horizontal.pos += count;
            SetRelativePos(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);

            xScroll = - (int) (count * dm->charMetric.x);
            rectangle->right = dm->charMetric.x * min(count, dm->clientArea.chars);
        }
        break;
    // RIGHT

    default:
    // TODO: error
        return 0;
    }

    ScrollWindow(hwnd, xScroll, yScroll, NULL, NULL);

    // Repaint rectangle
    InvalidateRect(hwnd, rectangle, TRUE);
    UpdateWindow(hwnd);

    #ifndef NDEBUG // ==============================================/
        // PrintPos(dm);
        // PrintScrollBar(&(dm->scrollBars.vertical));
        // PrintScrollBar(&(dm->scrollBars.horizontal));
    #endif // =====================================================/

    return count;
}

static void UpdateHorizontalSB_Default(HWND hwnd, DisplayedModel* dm) {
    assert(dm);

    dm->scrollBars.horizontal.maxPos = GetAbsoluteMaxPos(dm->documentArea.chars, dm->clientArea.chars);

    if (dm->scrollBars.horizontal.pos > dm->scrollBars.horizontal.maxPos) {
        #ifdef CARET_ON
            size_t scrollValue = dm->scrollBars.horizontal.pos - dm->scrollBars.horizontal.maxPos;
        #endif

        dm->scrollBars.horizontal.pos = dm->scrollBars.horizontal.maxPos;

    #ifdef CARET_ON
            // left border
            CaretHandleTopLeftBorder(hwnd, &(dm->caret.isHidden.x), scrollValue,
                                dm->caret.modelPos.pos.x, dm->scrollBars.horizontal.pos,
                                &(dm->caret.clientPos.x), DECREMENT_OF(dm->clientArea.chars));
        } else {
            // right border
            CaretHandleBottomRightBorder(hwnd, &(dm->caret.isHidden.x), 0,
                                dm->caret.modelPos.pos.x, dm->scrollBars.horizontal.pos,
                                &(dm->caret.clientPos.x), DECREMENT_OF(dm->clientArea.chars));
    #endif
    }
}

static void UpdateVerticalSB_Default(HWND hwnd, DisplayedModel* dm) {
    assert(dm);

    dm->scrollBars.vertical.maxPos = GetAbsoluteMaxPos(dm->documentArea.lines, dm->clientArea.lines);

    if (dm->scrollBars.vertical.pos > dm->scrollBars.vertical.maxPos) {
        size_t scrollValue = dm->scrollBars.vertical.pos - dm->scrollBars.vertical.maxPos;

        // pass
        PassPrev(&(dm->scrollBars.modelPos), scrollValue);
        dm->scrollBars.vertical.pos = dm->scrollBars.vertical.maxPos;

    #ifdef CARET_ON
            // top
            CaretHandleTopLeftBorder(hwnd, &(dm->caret.isHidden.y), scrollValue,
                                dm->caret.modelPos.pos.y, dm->scrollBars.vertical.pos,
                                &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines));
        } else {
            // bottom
            CaretHandleBottomRightBorder(hwnd, &(dm->caret.isHidden.y), 0,
                                dm->caret.modelPos.pos.y, dm->scrollBars.vertical.pos,
                                &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines));
    #endif
    }
}

static void UpdateVerticalSB_Wrap(HWND hwnd, DisplayedModel* dm) {
    assert(dm);

    dm->scrollBars.vertical.maxPos = GetAbsoluteMaxPos(dm->wrapModel.lines, dm->clientArea.lines);

    if (dm->scrollBars.vertical.pos > dm->scrollBars.vertical.maxPos) {
        size_t scrollValue = dm->scrollBars.vertical.pos - dm->scrollBars.vertical.maxPos;

        // pass
        for (size_t delta = dm->scrollBars.vertical.pos - dm->scrollBars.vertical.maxPos; delta > 0;) {
            if (dm->scrollBars.modelPos.pos.x + 1 <= delta) {
                delta -= dm->scrollBars.modelPos.pos.x + 1;
            } else {
                dm->scrollBars.modelPos.pos.x -= delta;
                break;
            }

            // prev block
            dm->scrollBars.modelPos.block = dm->scrollBars.modelPos.block->prev;
            --dm->scrollBars.modelPos.pos.y;
            if (dm->scrollBars.modelPos.block->data.len > 0) {
                dm->scrollBars.modelPos.pos.x = DIV_WITH_ROUND_UP(dm->scrollBars.modelPos.block->data.len, dm->clientArea.chars) - 1;
            } else {
                dm->scrollBars.modelPos.pos.x = 0;
            }
        }
        dm->scrollBars.vertical.pos = dm->scrollBars.vertical.maxPos;

    #ifdef CARET_ON
            // top
            CaretHandleTopLeftBorder(hwnd, &(dm->caret.isHidden.y), scrollValue,
                                dm->caret.linePos, dm->scrollBars.vertical.pos,
                                &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines));
        } else {
            // bottom
            CaretHandleBottomRightBorder(hwnd, &(dm->caret.isHidden.y), 0,
                                dm->caret.linePos, dm->scrollBars.vertical.pos,
                                &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines));
    #endif
    }
}

// Caret
#ifdef CARET_ON

    void CaretPrintParams(DisplayedModel* dm) {
        assert(dm);

        printf("Caret params:\n");
        printf("\tIs hidden: [%u; %u]\n", dm->caret.isHidden.x, dm->caret.isHidden.y);
        printf("\tClient pos: [%u; %u]\n", dm->caret.clientPos.x, dm->caret.clientPos.y);
        printf("\tModel pos: [%u; %u]\n", dm->caret.modelPos.pos.x, dm->caret.modelPos.pos.y);
        printf("\tLine pos: %u\n", dm->caret.linePos);
    }

    void FindHome_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.x);

        if (dm->caret.modelPos.pos.x > dm->caret.clientPos.x) {
            Scroll(hwnd, dm, dm->caret.modelPos.pos.x - dm->caret.clientPos.x, LEFT, rectangle);
        }
        dm->caret.modelPos.pos.x = 0;

        dm->caret.clientPos.x = 0;
    }

    void FindHome_Wrap(DisplayedModel* dm) {
        assert(dm);
        assert(dm->caret.clientPos.x);

        size_t remainder = dm->caret.modelPos.block->data.len % dm->clientArea.chars;

        if (dm->caret.modelPos.pos.x <= dm->caret.modelPos.block->data.len - remainder) {
            dm->caret.modelPos.pos.x -= dm->caret.clientPos.x;
        } else {
            dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len - remainder;
        }

        dm->caret.clientPos.x = 0;
    }

    void FindLeftEnd_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.x > dm->caret.modelPos.block->data.len);

        size_t deltaModel = dm->caret.modelPos.pos.x - dm->caret.modelPos.block->data.len;

        if (dm->caret.clientPos.x < deltaModel) {
            Scroll(hwnd, dm, deltaModel - dm->caret.clientPos.x, LEFT, rectangle);
            dm->caret.clientPos.x = 0;
        } else {
            dm->caret.clientPos.x -= deltaModel;
        }

        dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
    }

    void FindLeftEnd_Wrap(DisplayedModel* dm) {
        assert(dm);
        size_t deltaModel = dm->caret.modelPos.pos.x - dm->caret.modelPos.block->data.len;

        if (dm->caret.modelPos.pos.x > dm->caret.modelPos.block->data.len) {
            dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
            dm->caret.clientPos.x = dm->caret.modelPos.block->data.len % dm->clientArea.chars;
        }
    }

    void FindRightEnd_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);

        if (dm->caret.modelPos.pos.x < dm->caret.modelPos.block->data.len) {
            size_t deltaModel = dm->caret.modelPos.block->data.len - dm->caret.modelPos.pos.x;
            size_t deltaClient = DECREMENT_OF(dm->clientArea.chars) - dm->caret.clientPos.x;

            if (deltaModel > deltaClient) {
                Scroll(hwnd, dm, deltaModel - deltaClient, RIGHT, rectangle);
                dm->caret.clientPos.x = DECREMENT_OF(dm->clientArea.chars);
            } else {
                dm->caret.clientPos.x += deltaModel;
            }

            dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
        }
    }

    void FindRightEnd_Wrap(DisplayedModel* dm) {
        assert(dm);

        size_t remainder = dm->caret.modelPos.block->data.len % dm->clientArea.chars;

        if (dm->caret.modelPos.pos.x < dm->caret.modelPos.block->data.len - remainder) {
            dm->caret.clientPos.x = dm->clientArea.chars;
            dm->caret.modelPos.pos.x += dm->clientArea.chars - dm->caret.modelPos.pos.x % dm->clientArea.chars;
        } else if (dm->caret.clientPos.x < dm->clientArea.chars) {
            dm->caret.clientPos.x = remainder;
            dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
        }
    }


    // CaretMoveToTop
    static void MoveToTop(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);

        --dm->caret.linePos;
        if (dm->caret.clientPos.y > 0) {
            --dm->caret.clientPos.y;
        } else {
            Scroll(hwnd, dm, STEP, UP, rectangle);
        }
    }

    void CaretMoveToTop_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.y > 0);

        PassPrev(&(dm->caret.modelPos), 1);
        MoveToTop(hwnd, dm, rectangle);
    }

    void CaretMoveToTop_Wrap(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.linePos > 0);

        size_t blockLinePos = dm->caret.modelPos.pos.x / dm->clientArea.chars - (dm->caret.clientPos.x == dm->clientArea.chars);

        if (blockLinePos > 0) {
            // split string
            dm->caret.modelPos.pos.x -= dm->clientArea.chars;
        } else {
            // one line or first line of a split string
            PassPrev(&(dm->caret.modelPos), 1);

            if (dm->caret.modelPos.block->data.len) {
                size_t remainder = dm->caret.modelPos.block->data.len % dm->clientArea.chars;

                if (dm->caret.clientPos.x < remainder) {
                    // the same position
                    dm->caret.modelPos.pos.x = dm->caret.clientPos.x + dm->caret.modelPos.block->data.len - remainder;
                } else if (!remainder) {
                    // the same position when line is full
                    dm->caret.modelPos.pos.x = dm->caret.clientPos.x + dm->caret.modelPos.block->data.len - dm->clientArea.chars;
                } else {
                    // find end of line
                    dm->caret.clientPos.x = remainder;
                    dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
                }
            } else {
                dm->caret.clientPos.x = 0;
                dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
            }
        }
        MoveToTop(hwnd, dm, rectangle);
    }
    // ========================================================================


    // CaretMoveToBottom
    static void MoveToBottom(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);

        ++dm->caret.linePos;
        if (dm->clientArea.lines == 1 || dm->caret.clientPos.y == DECREMENT_OF(dm->clientArea.lines - 1)) {
            Scroll(hwnd, dm, STEP, DOWN, rectangle);
        } else {
            ++dm->caret.clientPos.y;
        }
    }

    void CaretMoveToBottom_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.y < DECREMENT_OF(dm->documentArea.lines));

        PassNext(&(dm->caret.modelPos), 1);
        MoveToBottom(hwnd, dm, rectangle);
    }

    void CaretMoveToBottom_Wrap(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.linePos < DECREMENT_OF(dm->wrapModel.lines));

        size_t linesNum = 1;
        size_t blockLinePos = dm->caret.modelPos.pos.x / dm->clientArea.chars - (dm->caret.clientPos.x == dm->clientArea.chars);

        if (dm->caret.modelPos.block->data.len > 0) {
            linesNum = DIV_WITH_ROUND_UP(dm->caret.modelPos.block->data.len, dm->clientArea.chars);
        }

        if (blockLinePos < DECREMENT_OF(linesNum)) {
            // split string
            size_t deltaModel = dm->caret.modelPos.block->data.len - dm->caret.modelPos.pos.x;

            if (deltaModel < dm->clientArea.chars) {
                dm->caret.modelPos.pos.x += deltaModel;
                dm->caret.clientPos.x = dm->caret.modelPos.block->data.len % dm->clientArea.chars;
            } else {
                dm->caret.modelPos.pos.x += dm->clientArea.chars;
            }
        } else {
            // one line or last line of a split string
            PassNext(&(dm->caret.modelPos), 1);

            if (dm->caret.modelPos.block->data.len < dm->caret.clientPos.x) {
                dm->caret.modelPos.pos.x = dm->caret.modelPos.block->data.len;
                dm->caret.clientPos.x = dm->caret.modelPos.block->data.len % dm->clientArea.chars;
            } else {
                dm->caret.modelPos.pos.x = dm->caret.clientPos.x;
            }
        }
        MoveToBottom(hwnd, dm, rectangle);
    }
    // ========================================================================

    void CaretMoveToLeft_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.x);

        --dm->caret.modelPos.pos.x;

        if (dm->caret.clientPos.x > 0) {
            --dm->caret.clientPos.x;
        } else {
            Scroll(hwnd, dm, STEP, LEFT, rectangle);
        }
    }

    void CaretMoveToLeft_Wrap(DisplayedModel* dm) {
        assert(dm);
        assert(dm->caret.modelPos.pos.x);
        assert(dm->caret.clientPos.x);

        --dm->caret.modelPos.pos.x;
        --dm->caret.clientPos.x;
    }

    void CaretMoveToRight_Default(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->caret.modelPos.pos.x < dm->caret.modelPos.block->data.len);

        ++dm->caret.modelPos.pos.x;
        if (dm->caret.clientPos.x < DECREMENT_OF(dm->clientArea.chars)) {
            ++dm->caret.clientPos.x;
        } else {
            Scroll(hwnd, dm, STEP, RIGHT, rectangle);
        }
    }

    void CaretMoveToRight_Wrap(DisplayedModel* dm) {
        assert(dm);
        assert(dm->caret.modelPos.pos.x < dm->caret.modelPos.block->data.len);
        assert(dm->caret.clientPos.x < dm->clientArea.chars);

        ++dm->caret.modelPos.pos.x;
        ++dm->caret.clientPos.x;
    }

    void CaretPageUp(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->scrollBars.vertical.pos);

        size_t linePos, delta;

        switch(dm->mode) {
        case FORMAT_MODE_DEFAULT:
            linePos = dm->caret.modelPos.pos.y;
            break;

        case FORMAT_MODE_WRAP:
            linePos = dm->caret.linePos;
            break;

        default:
            return;
        }

        if (dm->clientArea.lines > 1) {
            delta = min(DECREMENT_OF(dm->clientArea.lines - 1), dm->scrollBars.vertical.pos);
        } else {
            delta = 1;
        }

        if (dm->caret.modelPos.pos.y > 0) {
            PassPrev(&(dm->caret.modelPos), delta);
            Scroll(hwnd, dm, delta, UP, rectangle);
        }
    }

    void CaretPageDown(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm && rectangle);
        assert(dm->scrollBars.vertical.maxPos - dm->scrollBars.vertical.pos);

        size_t linePos, delta;

        switch(dm->mode) {
        case FORMAT_MODE_DEFAULT:
            linePos = dm->caret.modelPos.pos.y;
            break;

        case FORMAT_MODE_WRAP:
            linePos = dm->caret.linePos;
            break;

        default:
            return;
        }

        if (dm->clientArea.lines > 1) {
            delta = min(DECREMENT_OF(dm->clientArea.lines - 1),
                        dm->scrollBars.vertical.maxPos - dm->scrollBars.vertical.pos);
        } else {
            delta = 1;
        }

        if (linePos - dm->caret.clientPos.y + delta <= dm->scrollBars.vertical.maxPos) {
            PassNext(&(dm->caret.modelPos), delta);
            Scroll(hwnd, dm, delta, DOWN, rectangle);
        }
    }

    void CaretSetPos(DisplayedModel* dm) {
        assert(dm);

        if (!(dm->caret.isHidden.y | dm->caret.isHidden.x)) {
            SetCaretPos(dm->caret.clientPos.x * dm->charMetric.x, dm->caret.clientPos.y * dm->charMetric.y);
        }
    }

    void CaretCreate(HWND hwnd, DisplayedModel* dm) {
        assert(dm);

        CreateCaret(hwnd, NULL, 1, dm->charMetric.y);
        CaretSetPos(dm);
        ShowCaret(hwnd);
    }

    void CaretHandleTopLeftBorder(HWND hwnd, int* p_isHidden, size_t scrollValue, size_t modelPos, size_t scrollBarPos, size_t* pClientPos, size_t clientPosMax) {
        assert(p_isHidden && pClientPos);

        // caret is over the top/left border
        if (*p_isHidden && *pClientPos == 0) {
            if (scrollBarPos <= modelPos) {
                // caret is in the client area
                *pClientPos = modelPos - scrollBarPos;
                CaretShow(hwnd, p_isHidden);
            }
        } else if (!*p_isHidden) {
            if (clientPosMax >= *pClientPos + scrollValue) {
                // caret is in the client area
                *pClientPos += scrollValue;

            } else {
                // caret is over the bottom/right border
                *pClientPos = clientPosMax;
                CaretHide(hwnd, p_isHidden);
            }
        }
    }

    void CaretHandleBottomRightBorder(HWND hwnd, int* p_isHidden, size_t scrollValue, size_t modelPos, size_t scrollBarPos, size_t* pClientPos, size_t clientPosMax) {
        assert(p_isHidden && pClientPos);

        if (*p_isHidden && *pClientPos != 0) {
            if (scrollBarPos + clientPosMax >= modelPos) {
                // caret is in the client area
                *pClientPos = modelPos - scrollBarPos;
                CaretShow(hwnd, p_isHidden);

            } else {
                // caret is over the bottom/right border
                *pClientPos = clientPosMax;
            }
        } else if (!*p_isHidden) {
            if (*pClientPos > clientPosMax) {
                // caret is over the bottom/right border
                *pClientPos = clientPosMax;
                CaretHide(hwnd, p_isHidden);

            } else if (*pClientPos >= scrollValue) {
                *pClientPos -= scrollValue;

            } else {
                // caret is over the top/left border
                *pClientPos = 0;
                CaretHide(hwnd, p_isHidden);
            }
        }
    }

    void FindCaret(HWND hwnd, DisplayedModel* dm, RECT* rectangle) {
        assert(dm);
        size_t scrollValue;

        if (dm->caret.isHidden.x) {
            // left border
            if (dm->caret.modelPos.pos.x < dm->scrollBars.horizontal.pos) {
                scrollValue = dm->scrollBars.horizontal.pos - dm->caret.modelPos.pos.x;
                scrollValue = Scroll(hwnd, dm, scrollValue, LEFT, rectangle);
                CaretHandleTopLeftBorder(hwnd, &(dm->caret.isHidden.x), scrollValue,
                                            dm->caret.modelPos.pos.x, dm->scrollBars.horizontal.pos,
                                            &(dm->caret.clientPos.x), DECREMENT_OF(dm->clientArea.chars));
            }

            // right border
            if (dm->caret.modelPos.pos.x > dm->scrollBars.horizontal.pos + DECREMENT_OF(dm->clientArea.chars)) {
                scrollValue = dm->caret.modelPos.pos.x - (dm->scrollBars.horizontal.pos + DECREMENT_OF(dm->clientArea.chars));
                scrollValue = Scroll(hwnd, dm, scrollValue, RIGHT, rectangle);
                CaretHandleBottomRightBorder(hwnd, &(dm->caret.isHidden.x), scrollValue,
                                            dm->caret.modelPos.pos.x, dm->scrollBars.horizontal.pos,
                                            &(dm->caret.clientPos.x), DECREMENT_OF(dm->clientArea.chars));
            }
        }

        if (dm->caret.isHidden.y) {
            size_t numLines;

            switch (dm->mode) {
            case FORMAT_MODE_DEFAULT:
                numLines = dm->caret.modelPos.pos.y;
                break;

            case FORMAT_MODE_WRAP:
                numLines = dm->caret.linePos;
                break;
                
            default:
                break;
            }

            // top
            if (numLines < dm->scrollBars.vertical.pos) {
                scrollValue = dm->scrollBars.vertical.pos - numLines;
                scrollValue = Scroll(hwnd, dm, scrollValue, UP, rectangle);
                CaretHandleTopLeftBorder(hwnd, &(dm->caret.isHidden.y), scrollValue,
                                            numLines, dm->scrollBars.vertical.pos,
                                            &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines - 1));
            }

            // bottom
            if (numLines > dm->scrollBars.vertical.pos + DECREMENT_OF(dm->clientArea.lines - 1)) {
                scrollValue = numLines - (dm->scrollBars.vertical.pos + DECREMENT_OF(dm->clientArea.lines - 1));
                scrollValue = Scroll(hwnd, dm, scrollValue, DOWN, rectangle);
                CaretHandleBottomRightBorder(hwnd, &(dm->caret.isHidden.y), scrollValue,
                                            numLines, dm->scrollBars.vertical.pos,
                                            &(dm->caret.clientPos.y), DECREMENT_OF(dm->clientArea.lines - 1));
            }
        }

        if (dm->clientArea.lines > STEP && dm->caret.clientPos.y == DECREMENT_OF(dm->clientArea.lines)) {
            --dm->caret.clientPos.y;
            Scroll(hwnd, dm, STEP, DOWN, rectangle);
        }
    }

    static size_t FindPos(Fragment** fragment, size_t* delta) {
        assert(fragment && *fragment);
        assert(delta);

        size_t counter = 0;

        while (*delta > (*fragment)->data.len) {
            *delta -= (*fragment)->data.len;
            *fragment = (*fragment)->next;

            ++counter;
        }

        return counter;
    }

    static int SplitFragment(ListFragment* fragments, Fragment* prevFragment, size_t delta) {
        FragmentData_t fragmentData = { prevFragment->data.len - delta, prevFragment->data.pos + delta };
        Fragment* newFragment = CreateFragment(prevFragment, &fragmentData);

        if (!newFragment) { return ERR_NOMEM; }

        InsertFragments(fragments, newFragment);
        prevFragment->data.len = delta;

        return ERR_SUCCESS;
    }

    int CaretAddChar(HWND hwnd, DisplayedModel* dm, char c) {
        assert(dm);
        
        ListFragment* fragments = dm->caret.modelPos.block->data.fragments;
        Fragment* fragment = fragments->nodes;
        Fragment* newFragment = NULL;
        FragmentData_t fragmentData = {1, dm->doc->text->len};

        if (AddChar(dm->doc->text, c) < 0) {
            PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
            return ERR_NOMEM;
        }

        // find place
        size_t delta = dm->caret.modelPos.pos.x;
        FindPos(&fragment, &delta);

        // split
        if (delta && delta < fragment->data.len) {
            if (SplitFragment(fragments, fragment, delta)) {
                --dm->doc->text->len;
                dm->doc->text->data[dm->doc->text->len] = '\0';

                PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
                return ERR_NOMEM;
            }
        }

        // insert
        if (!fragment->data.len) {
            ++fragment->data.len;
            fragment->data.pos = dm->doc->text->len - 1;
        } else if (delta == fragment->data.len && dm->doc->text->len == (fragment->data.pos + fragment->data.len + 1)) {
            ++fragment->data.len;
        } else {
            newFragment = CreateFragment(!delta ? NULL : fragment, &fragmentData);

            if (!newFragment) {
                --dm->doc->text->len;
                dm->doc->text->data[dm->doc->text->len] = '\0';

                PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
                return ERR_NOMEM;
            }

            InsertFragments(fragments, newFragment);
        }

        // update
        ++dm->caret.modelPos.block->data.len;

        if (dm->documentArea.chars < dm->caret.modelPos.block->data.len) {
            dm->documentArea.chars = dm->caret.modelPos.block->data.len;

            if (dm->mode == FORMAT_MODE_DEFAULT) {
                UpdateHorizontalSB_Default(hwnd, dm);
                SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
            }
        }

        if (dm->mode == FORMAT_MODE_WRAP) {
            size_t linesBlock = 1;

            if (dm->caret.modelPos.block->data.len != 1) {
                linesBlock = DIV_WITH_ROUND_UP(dm->caret.modelPos.block->data.len - 1, dm->clientArea.chars);
            }

            if (linesBlock < DIV_WITH_ROUND_UP(dm->caret.modelPos.block->data.len, dm->clientArea.chars)) {
                ++dm->wrapModel.lines;
                UpdateVerticalSB_Wrap(hwnd, dm);
                SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            }
        }

        return ERR_SUCCESS;
    }

    int CaretAddBlock(HWND hwnd, DisplayedModel* dm) {
        assert(dm);
        int isSplitted = 0;

        ListFragment* fragments = dm->caret.modelPos.block->data.fragments;
        Fragment* fragment = fragments->nodes;

        ListFragment* newFragments = CreateListFragment();
        Fragment* newFragment;
        if (!newFragments) {
            PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
            return ERR_NOMEM;
        }

        // find place
        size_t delta = dm->caret.modelPos.pos.x;
        size_t countPassFragments = FindPos(&fragment, &delta);

        if (delta && delta < fragment->data.len) {
            // split
            if (SplitFragment(dm->caret.modelPos.block->data.fragments, fragment, delta)) {
                DestroyListFragment(&newFragments);
                PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
                return ERR_NOMEM;
            }

            newFragment = fragment->next;
            isSplitted = 1;
            ++countPassFragments;
        } else if (delta == fragment->data.len && fragment->next) {
            newFragment = fragment->next;
            isSplitted = 1;
            ++countPassFragments;
        } else {
            FragmentData_t fragmentData = { 0, dm->doc->text->len };
            newFragment = CreateFragment(NULL, &fragmentData);

            if (!newFragment) {
                DestroyListFragment(&newFragments);
                PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
                return ERR_NOMEM;
            }
        }

        // insert
        Block* block = dm->caret.modelPos.block;
        BlockData_t blockData = { 0, newFragments };
        int isChangeLen = block->data.len == dm->documentArea.chars;

        if (isSplitted) {
            blockData.len = block->data.len - dm->caret.modelPos.pos.x;
        } else if (!delta) {
            block = block->prev;
        }

        Block* newBlock = CreateBlock(block, &blockData);

        if (!newBlock) {
            DestroyListFragment(&newFragments);
            if (!isSplitted) { DestroyFragment(&newFragment); }

            PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
            return ERR_NOMEM;
        }

        if (isSplitted) {
            newFragment->prev = NULL;

            fragment->next = NULL;
            fragments->len = countPassFragments;

            block->data.len = dm->caret.modelPos.pos.x;
        } else if (!delta) {
            if (dm->caret.modelPos.block == dm->scrollBars.modelPos.block) {
                switch (dm->mode) {
                case FORMAT_MODE_DEFAULT:
                    dm->scrollBars.modelPos.block = newBlock;
                    break;

                case FORMAT_MODE_WRAP:
                    if (dm->scrollBars.modelPos.pos.x == 1) {
                        dm->scrollBars.modelPos.block = newBlock;
                    }
                    break;

                default:
                    break;
                }
            }
            
            dm->caret.modelPos.block = newBlock;
        }

        InsertFragments(newFragments, newFragment);
        InsertBlocks(dm->doc->blocks, newBlock);

        // update
        ++dm->documentArea.lines;

        if (isChangeLen) {
            dm->documentArea.chars = GetMaxBlockLen(dm->doc->blocks);

            if (dm->mode == FORMAT_MODE_DEFAULT) {
                UpdateHorizontalSB_Default(hwnd, dm);
                SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
            }
        }

        switch (dm->mode) {
        case FORMAT_MODE_DEFAULT:
            UpdateVerticalSB_Default(hwnd, dm);
            SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            break;

        case FORMAT_MODE_WRAP:
            dm->scrollBars.vertical.pos = BuildWrapModel(hwnd, dm);
            UpdateVerticalSB_Wrap(hwnd, dm);
            SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            break;
        
        default:
            break;
        }

        return ERR_SUCCESS;
    }

    int CaretDeleteChar(HWND hwnd, DisplayedModel* dm) {
        assert(dm);
        assert(dm->caret.modelPos.block->data.len);

        ListFragment* fragments = dm->caret.modelPos.block->data.fragments;
        Fragment* fragment = fragments->nodes;

        // find place
        size_t delta = dm->caret.modelPos.pos.x;
        FindPos(&fragment, &delta);

        // split
        if (delta) {
            if (delta < fragment->data.len && SplitFragment(fragments, fragment, delta)) {
                PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
                return ERR_NOMEM;
            }

            assert (fragment->next);
            fragment = fragment->next;
        }

        // delete
        --fragment->data.len;
        ++fragment->data.pos;

        if (!fragment->data.len && fragments->len > 1) {
            DeleteFragment(fragments, fragment);
        }

        // update
        --dm->caret.modelPos.block->data.len;

        if (dm->documentArea.chars == dm->caret.modelPos.block->data.len + 1) {
            dm->documentArea.chars = GetMaxBlockLen(dm->doc->blocks);

            if (dm->mode == FORMAT_MODE_DEFAULT) {
                UpdateHorizontalSB_Default(hwnd, dm);
                SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
            }
        }

        if (dm->mode == FORMAT_MODE_WRAP && ((dm->caret.modelPos.block->data.len + 1) > dm->clientArea.chars)) {
            size_t linesBlock = DIV_WITH_ROUND_UP(dm->caret.modelPos.block->data.len + 1, dm->clientArea.chars);

            if (linesBlock > DIV_WITH_ROUND_UP(dm->caret.modelPos.block->data.len, dm->clientArea.chars)) {
                --dm->wrapModel.lines;
                UpdateVerticalSB_Wrap(hwnd, dm);
                SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            }
        }

        return ERR_SUCCESS;
    }

    void CaretDeleteBlock(HWND hwnd, DisplayedModel* dm) {
        assert(dm);
        assert(dm->caret.modelPos.pos.x == dm->caret.modelPos.block->data.len);
        assert(dm->caret.modelPos.block->next);

        Block* block = dm->caret.modelPos.block;
        Block* nextBlock = block->next;

        ListFragment* fragments = block->data.fragments;

        if (!block->data.len) {
            if (dm->scrollBars.modelPos.block == block) {
                dm->scrollBars.modelPos.block = nextBlock;
            }
            dm->caret.modelPos.block = nextBlock;

            DeleteBlock(dm->doc->blocks, block);
        } else if (!nextBlock->data.len) {
            DeleteBlock(dm->doc->blocks, nextBlock);
        } else {
            ListFragment* fragments = block->data.fragments;
            nextBlock->data.fragments->nodes->prev = fragments->last;
            InsertFragments(fragments, nextBlock->data.fragments->nodes);

            nextBlock->data.fragments = NULL;
            block->data.len += nextBlock->data.len;

            DeleteBlock(dm->doc->blocks, nextBlock);

            // update horizontal params
            if (block->data.len > dm->documentArea.chars) {
                dm->documentArea.chars = GetMaxBlockLen(dm->doc->blocks);

                if (dm->mode == FORMAT_MODE_DEFAULT) {
                    UpdateHorizontalSB_Default(hwnd, dm);
                    SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
                }
            }
        }

        // update
        --dm->documentArea.lines;

        switch (dm->mode) {
        case FORMAT_MODE_DEFAULT:
            UpdateVerticalSB_Default(hwnd, dm);
            SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            break;

        case FORMAT_MODE_WRAP:
            dm->scrollBars.vertical.pos = BuildWrapModel(hwnd, dm);
            UpdateVerticalSB_Wrap(hwnd, dm);
            SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
            break;
        
        default:
            break;
        }
    }
#endif

void SwitchMode(HWND hwnd, DisplayedModel* dm, FormatMode mode) {
    // printf("Switch mode\n");
    assert(dm);

    #ifndef NDEBUG // ====/
        // PrintPos(dm);
    #endif // ===========/

    switch(mode) {
    case FORMAT_MODE_DEFAULT:
        // printf("Default mode is activated\n");
        dm->mode = FORMAT_MODE_DEFAULT;
        ++dm->clientArea.chars;

        // horizontal scroll-bar
        #ifdef CARET_ON
            if (dm->caret.modelPos.pos.x > DECREMENT_OF(dm->clientArea.chars)) {
                CaretHide(hwnd, &(dm->caret.isHidden.x));
                dm->caret.clientPos.x = DECREMENT_OF(dm->clientArea.chars);
            } else {
                dm->caret.clientPos.x = dm->caret.modelPos.pos.x;
            }
        #endif
        dm->scrollBars.horizontal.maxPos = GetAbsoluteMaxPos(dm->documentArea.chars, dm->clientArea.chars);

        // vertical scroll-bar
        dm->scrollBars.vertical.pos = dm->scrollBars.modelPos.pos.y;
        dm->scrollBars.vertical.maxPos = GetAbsoluteMaxPos(dm->documentArea.lines, dm->clientArea.lines);

        if (dm->scrollBars.vertical.pos > dm->scrollBars.vertical.maxPos) {
            size_t scrollValue = dm->scrollBars.vertical.pos - dm->scrollBars.vertical.maxPos;

            // pass
            PassPrev(&(dm->scrollBars.modelPos), scrollValue);
            dm->scrollBars.vertical.pos = dm->scrollBars.vertical.maxPos;

            #ifdef CARET_ON
                if  (dm->caret.modelPos.pos.y >= dm->scrollBars.vertical.pos) {
                    dm->caret.clientPos.y = dm->caret.modelPos.pos.y - dm->scrollBars.vertical.pos;

                    if (dm->caret.clientPos.y > DECREMENT_OF(dm->clientArea.lines)) {
                        dm->caret.clientPos.y = DECREMENT_OF(dm->clientArea.lines);
                        CaretHide(hwnd, &(dm->caret.isHidden.y));
                    } else if (dm->caret.isHidden.y) {
                        CaretShow(hwnd, &(dm->caret.isHidden.y));
                    }
                } else {
                    dm->caret.clientPos.y = 0;
                    CaretHide(hwnd, &(dm->caret.isHidden.y));
                }
            #endif
        }
        #ifdef CARET_ON
            else {
                if  (dm->caret.modelPos.pos.y >= dm->scrollBars.vertical.pos) {
                    dm->caret.clientPos.y = dm->caret.modelPos.pos.y - dm->scrollBars.vertical.pos;

                    if (dm->caret.clientPos.y > DECREMENT_OF(dm->clientArea.lines)) {
                        CaretHide(hwnd, &(dm->caret.isHidden.y));
                        dm->caret.clientPos.y = DECREMENT_OF(dm->clientArea.lines);
                    } else if (dm->caret.isHidden.y) {
                        CaretShow(hwnd, &(dm->caret.isHidden.y));
                    }
                } else {
                    CaretHide(hwnd, &(dm->caret.isHidden.y));
                    dm->caret.clientPos.y = 0;
                }
            }
        #endif
        break;

    case FORMAT_MODE_WRAP:
        printf("Wrap mode is activated\n");
        dm->mode = FORMAT_MODE_WRAP;
        --dm->clientArea.chars;

        // horizontal scroll-bar
        dm->scrollBars.horizontal.pos = 0;
        dm->scrollBars.horizontal.maxPos = 0;
        #ifdef CARET_ON
            dm->caret.clientPos.x = dm->caret.modelPos.pos.x % dm->clientArea.chars;
            if (dm->caret.isHidden.x) { CaretShow(hwnd, &(dm->caret.isHidden.x)); }
        #endif


        // vertical scroll-bar
        dm->scrollBars.vertical.pos = BuildWrapModel(hwnd, dm);
        UpdateVerticalSB_Wrap(hwnd, dm);
        break;

    default:
        printf("Unknown mode\n");
        return;
    }

    SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
    SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);

    #ifndef NDEBUG // ====/
        // PrintPos(dm);
    #endif // ===========/
}

static int UpdateParameter(size_t* pOldValue, size_t newValue) {
    assert(pOldValue);

    if (*pOldValue != newValue) {
        *pOldValue = newValue;
        return 1;
    }

    return 0;
}

void UpdateDisplayedModel(HWND hwnd, DisplayedModel* dm, LPARAM lParam) {
    // printf("UpdateDisplayedModel\n");
    assert(dm);

    size_t chars = DIV_WITH_ROUND_UP(LOWORD(lParam), dm->charMetric.x) - (dm->mode == FORMAT_MODE_WRAP);
    size_t lines = DIV_WITH_ROUND_UP(HIWORD(lParam), dm->charMetric.y);

    int isCharsChanged = UpdateParameter(&(dm->clientArea.chars), chars);
    int isLinesChanged = UpdateParameter(&(dm->clientArea.lines), lines);

    if (!isCharsChanged && !isLinesChanged) { return; }

    switch (dm->mode) {
    case FORMAT_MODE_DEFAULT:
        // Horizontal scroll-bar
        if (isCharsChanged) { UpdateHorizontalSB_Default(hwnd, dm); }

        // Vertical scroll-bar
        if (isLinesChanged) { UpdateVerticalSB_Default(hwnd, dm); }
        break;
    // FORMAT_MODE_DEFAULT

    case FORMAT_MODE_WRAP:
        if (dm->documentArea.chars >= dm->clientArea.chars && isCharsChanged) {
            // Horizontal scroll-bar
            dm->scrollBars.horizontal.pos = 0;
            dm->scrollBars.horizontal.maxPos = 0;

            #ifdef CARET_ON
                dm->caret.clientPos.x = dm->caret.modelPos.pos.x % dm->clientArea.chars;
            #endif

            dm->wrapModel.isValid = 0; // for what?

            // Vertical scroll-bar
            dm->scrollBars.vertical.pos = BuildWrapModel(hwnd, dm);
        }

        // Vertical scroll-bar
        UpdateVerticalSB_Wrap(hwnd, dm);
        break;
    // FORMAT_MODE_WRAP

    default:
        return;
    }

    SetRelativeParam(hwnd, &(dm->scrollBars.horizontal), SB_HORZ);
    SetRelativeParam(hwnd, &(dm->scrollBars.vertical), SB_VERT);
}
