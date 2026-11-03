#pragma once
#include "canvas.h"
#include <input/input.h>
#include <stdbool.h>
typedef struct View View;
typedef void (*ViewDrawCallback)(Canvas* canvas, void* model);
typedef bool (*ViewInputCallback)(InputEvent* event, void* context);
typedef enum { ViewModelTypeNone, ViewModelTypeLockFree, ViewModelTypeLocking } ViewModelType;
View* view_alloc(void);
void view_free(View* v);
void view_allocate_model(View* v, ViewModelType t, size_t size);
void* view_get_model(View* v);
void view_commit_model(View* v, bool update);
void view_set_context(View* v, void* ctx);
void view_set_draw_callback(View* v, ViewDrawCallback cb);
void view_set_input_callback(View* v, ViewInputCallback cb);
#define with_view_model(view, type, code, update) \
    {                                             \
        type = view_get_model(view);              \
        { code };                                 \
        view_commit_model(view, update);          \
    }
/* Test-Helfer */
void view_test_draw(View* v, Canvas* canvas);
bool view_test_input(View* v, InputKey key, InputType type);
