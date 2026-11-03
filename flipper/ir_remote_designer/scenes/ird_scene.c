#include "ird_scene.h"

// Generate scene on_enter handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const ird_on_enter_handlers[])(void*) = {
#include "ird_scene_config.h"
};
#undef ADD_SCENE

// Generate scene on_event handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const ird_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "ird_scene_config.h"
};
#undef ADD_SCENE

// Generate scene on_exit handlers array
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const ird_on_exit_handlers[])(void* context) = {
#include "ird_scene_config.h"
};
#undef ADD_SCENE

// Initialize scene handlers configuration structure
const SceneManagerHandlers ird_scene_handlers = {
    .on_enter_handlers = ird_on_enter_handlers,
    .on_event_handlers = ird_on_event_handlers,
    .on_exit_handlers = ird_on_exit_handlers,
    .scene_num = IrdSceneNum,
};
