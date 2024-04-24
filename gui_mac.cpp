#define GUI_API CLAP_WINDOW_API_COCOA

#include <elements.hpp>

namespace ce = cycfi::elements;

struct GUI {
    ce::window *win;
    ce::view *view;
};

extern "C" void MacSetParent(void *_mainView, void *_parentView);
extern "C" void MacSetVisible(void *_mainView, bool show);


static void GUICreate(MyPlugin *plugin) {
    assert(!plugin->gui);
    plugin->gui = (GUI *) calloc(1, sizeof(GUI));
    plugin->gui->win = new ce::window("tuneBfree", 0, ce::rect{0, 0, GUI_WIDTH, GUI_HEIGHT});
    plugin->gui->win->on_close = []() {};
    plugin->gui->view = new ce::view(*plugin->gui->win);
}

static void GUIDestroy(MyPlugin *plugin) {
    assert(plugin->gui);
    free(plugin->gui);
    plugin->gui = nullptr;
}

static void GUISetParent(MyPlugin *plugin, const clap_window_t *parent) { MacSetParent(plugin->gui->view->host(), parent->cocoa); }
static void GUISetVisible(MyPlugin *plugin, bool visible) { MacSetVisible(plugin->gui->view->host(), visible); }
static void GUIOnPOSIXFD(MyPlugin *) {}
