#ifndef LITT_GUI_H
#define LITT_GUI_H

#include <imgui.h>
#include "engine_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create and initialise the GUI. Returns non-null on success. */
void  *litt_gui_create(void);

/* Destroy and free all resources. */
void   litt_gui_destroy(void *handle);

/* One frame of rendering – call from your GLFW/Vulkan draw loop. */
void   litt_gui_render_frame(void *handle);

/* Accessors for the engine handle inside the GUI (used by tests). */
LittEngine *litt_gui_get_engine(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* LITT_GUI_H */
