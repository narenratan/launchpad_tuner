#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sstream>

#include "clap/clap.h"
#include "libMTSMaster.h"

#define OCTAVE_CENTS (1200.0)

// Parameters.
#define P_XSTEP (0)
#define P_YSTEP (1)
#define P_TRANSPOSE (2)
#define P_COUNT (3)

// GUI size.
#define GUI_WIDTH (450)
#define GUI_HEIGHT (400)

struct MyPlugin {
    clap_plugin_t plugin;
    const clap_host_t *host;
    float mainParameters[P_COUNT];
    struct GUI *gui;
    const clap_host_posix_fd_support_t *hostPOSIXFDSupport;
};

const char *_features[] = {
    CLAP_PLUGIN_FEATURE_UTILITY,
    NULL,
};

static const clap_plugin_descriptor_t pluginDescriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "naren.launchpad_tuner",
    .name = "Launchpad tuner",
    .vendor = "naren",
    .url = "https://github.com/narenratan",
    .manual_url = "https://github.com/narenratan",
    .support_url = "https://github.com/narenratan",
    .version = "1.0.0",
    .description = "Retune synths to use Launchpad as an isomorphic keyboard",

    .features = _features,
};

static const clap_plugin_params_t extensionParams = {
    .count = [] (const clap_plugin_t *plugin) -> uint32_t {
        return P_COUNT;
    },

    .get_info = [] (const clap_plugin_t *_plugin, uint32_t index, clap_param_info_t *information) -> bool {
        if (index == P_XSTEP) {
            memset(information, 0, sizeof(clap_param_info_t));
            information->id = index;
            information->flags = CLAP_PARAM_IS_AUTOMATABLE;
            information->min_value = 0.0f;
            information->max_value = 1.0f;
            information->default_value = 0.05664166714743746;
            strcpy(information->name, "X step");
            return true;
        } else if (index == P_YSTEP) {
            memset(information, 0, sizeof(clap_param_info_t));
            information->id = index;
            information->flags = CLAP_PARAM_IS_AUTOMATABLE;
            information->min_value = 0.0f;
            information->max_value = 1.0f;
            information->default_value = 0.26416041678685936;
            strcpy(information->name, "Y step");
            return true;
        } else if (index == P_TRANSPOSE) {
            memset(information, 0, sizeof(clap_param_info_t));
            information->id = index;
            information->flags = CLAP_PARAM_IS_AUTOMATABLE;
            information->min_value = 0.0f;
            information->max_value = 1.0f;
            information->default_value = 0.0f;
            strcpy(information->name, "Transpose");
            return true;
        } else {
            return false;
        }
    },

    .get_value = [] (const clap_plugin_t *_plugin, clap_id id, double *value) -> bool {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        uint32_t i = (uint32_t) id;
        if (i >= P_COUNT) return false;
        *value = plugin->mainParameters[i];
        return true;
    },

    .value_to_text = [] (const clap_plugin_t *_plugin, clap_id id, double value, char *display, uint32_t size) {
        uint32_t i = (uint32_t) id;
        if (i >= P_COUNT) return false;
        snprintf(display, size, "%f", value);
        return true;
    },

    .text_to_value = [] (const clap_plugin_t *_plugin, clap_id param_id, const char *display, double *value) {
        // TODO Implement this.
        return false;
    },

    .flush = [] (const clap_plugin_t *_plugin, const clap_input_events_t *in, const clap_output_events_t *out) {
    },
};

static const clap_plugin_state_t extensionState = {
    .save = [] (const clap_plugin_t *_plugin, const clap_ostream_t *stream) -> bool {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        return sizeof(float) * P_COUNT == stream->write(stream, plugin->mainParameters, sizeof(float) * P_COUNT);
    },

    .load = [] (const clap_plugin_t *_plugin, const clap_istream_t *stream) -> bool {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        bool success = sizeof(float) * P_COUNT == stream->read(stream, plugin->mainParameters, sizeof(float) * P_COUNT);
        return success;
    },
};

#if defined(_WIN32)
#include "gui_w32.cpp"
#elif defined(__linux__)
#include "gui_x11.cpp"
#elif defined(__APPLE__)
#include "gui_mac.cpp"
#endif

static void set_tuning(double x, double y, double t)
{
    int i, j;
    for(i = 0; i < 9; i++)
    {
        for(j = 0; j < 9; j++)
            {
                MTS_SetNoteTuning(std::pow(2, i*x + j*y + t) * 220.0, 10*j + i + 11);
            }
    }
}

std::string diagram_entry(float *mainParameters, int i, int j)
{
    std::string s = std::to_string((int) std::round(std::fmod(OCTAVE_CENTS * (i * mainParameters[P_XSTEP] + j * mainParameters[P_YSTEP] + mainParameters[P_TRANSPOSE]), OCTAVE_CENTS)));
    s.insert(s.begin(), 5 - s.size(), ' ');
    return s;
}

float check(float x)
{
    if (std::isnan(x))
    {
        throw std::runtime_error("nan");
    }
    if (std::isinf(x))
    {
        throw std::runtime_error("inf");
    }
    return x;
}

float parse_step(std::string s)
{
    int slash, top, bottom;

    slash = s.find('/');
    if (slash != std::string::npos)
    {
        top = std::stoi(s.substr(0, slash));
        bottom = std::stoi(s.substr(slash + 1));
        return check(std::log2(((float) top)/((float) bottom)));
    }

    slash = s.find('\\');
    if (slash != std::string::npos)
    {
        top = std::stoi(s.substr(0, slash));
        bottom = std::stoi(s.substr(slash + 1));
        return check(((float) top)/((float) bottom));
    }

    return check(std::stof(s)/OCTAVE_CENTS);
}

void GUISetup(MyPlugin *plugin)
{
    float *mainParameters = plugin->mainParameters;
    ce::view *view = plugin->gui->view;

    auto xStep = ce::input_box(std::to_string(OCTAVE_CENTS * mainParameters[P_XSTEP]));

    std::shared_ptr<ce::label> labels[9][9];
    int i, j;
    for(i = 0; i < 9; i++)
    {
        for(j = 0; j < 9; j++)
        {
            labels[i][j] = share(ce::label(diagram_entry(plugin->mainParameters, i, j)));
        }
    }

    xStep.second->on_enter = [mainParameters, labels, view] (std::string_view text)->bool {
        try {
            mainParameters[P_XSTEP] = parse_step(std::string(text));
        } catch (std::exception&) {
            return true;
        }
        set_tuning(mainParameters[P_XSTEP], mainParameters[P_YSTEP], mainParameters[P_TRANSPOSE]);
        for(int i = 0; i < 9; i++) {
            for(int j = 0; j < 9; j++) {
                labels[i][j]->set_text(diagram_entry(mainParameters, i, j));
                view->refresh(*labels[i][j]);
            }
        }
        return true;
    };

    auto yStep = ce::input_box(std::to_string(OCTAVE_CENTS * mainParameters[P_YSTEP]));

    yStep.second->on_enter = [mainParameters, labels, view] (std::string_view text)->bool {
        try {
            mainParameters[P_YSTEP] = parse_step(std::string(text));
        } catch (std::exception&) {
            return true;
        }
        set_tuning(mainParameters[P_XSTEP], mainParameters[P_YSTEP], mainParameters[P_TRANSPOSE]);
        for(int i = 0; i < 9; i++) {
            for(int j = 0; j < 9; j++) {
                labels[i][j]->set_text(diagram_entry(mainParameters, i, j));
                view->refresh(*labels[i][j]);
            }
        }
        return true;
    };

    auto transpose = ce::input_box(std::to_string(OCTAVE_CENTS * mainParameters[P_TRANSPOSE]));

    transpose.second->on_enter = [mainParameters, labels, view] (std::string_view text)->bool {
        try {
            mainParameters[P_TRANSPOSE] = parse_step(std::string(text));
        } catch (std::exception&) {
            return true;
        }
        set_tuning(mainParameters[P_XSTEP], mainParameters[P_YSTEP], mainParameters[P_TRANSPOSE]);
        for(int i = 0; i < 9; i++) {
            for(int j = 0; j < 9; j++) {
                labels[i][j]->set_text(diagram_entry(mainParameters, i, j));
                view->refresh(*labels[i][j]);
            }
        }
        return true;
    };

    int m = 3;
    auto diagram = ce::vgrid(
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][8]),
            hold(labels[1][8]),
            hold(labels[2][8]),
            hold(labels[3][8]),
            hold(labels[4][8]),
            hold(labels[5][8]),
            hold(labels[6][8]),
            hold(labels[7][8]),
            hold(labels[8][8])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][7]),
            hold(labels[1][7]),
            hold(labels[2][7]),
            hold(labels[3][7]),
            hold(labels[4][7]),
            hold(labels[5][7]),
            hold(labels[6][7]),
            hold(labels[7][7]),
            hold(labels[8][7])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][6]),
            hold(labels[1][6]),
            hold(labels[2][6]),
            hold(labels[3][6]),
            hold(labels[4][6]),
            hold(labels[5][6]),
            hold(labels[6][6]),
            hold(labels[7][6]),
            hold(labels[8][6])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][5]),
            hold(labels[1][5]),
            hold(labels[2][5]),
            hold(labels[3][5]),
            hold(labels[4][5]),
            hold(labels[5][5]),
            hold(labels[6][5]),
            hold(labels[7][5]),
            hold(labels[8][5])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][4]),
            hold(labels[1][4]),
            hold(labels[2][4]),
            hold(labels[3][4]),
            hold(labels[4][4]),
            hold(labels[5][4]),
            hold(labels[6][4]),
            hold(labels[7][4]),
            hold(labels[8][4])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][3]),
            hold(labels[1][3]),
            hold(labels[2][3]),
            hold(labels[3][3]),
            hold(labels[4][3]),
            hold(labels[5][3]),
            hold(labels[6][3]),
            hold(labels[7][3]),
            hold(labels[8][3])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][2]),
            hold(labels[1][2]),
            hold(labels[2][2]),
            hold(labels[3][2]),
            hold(labels[4][2]),
            hold(labels[5][2]),
            hold(labels[6][2]),
            hold(labels[7][2]),
            hold(labels[8][2])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][1]),
            hold(labels[1][1]),
            hold(labels[2][1]),
            hold(labels[3][1]),
            hold(labels[4][1]),
            hold(labels[5][1]),
            hold(labels[6][1]),
            hold(labels[7][1]),
            hold(labels[8][1])
        )),
        ce::vmargin(m, ce::hgrid(
            hold(labels[0][0]),
            hold(labels[1][0]),
            hold(labels[2][0]),
            hold(labels[3][0]),
            hold(labels[4][0]),
            hold(labels[5][0]),
            hold(labels[6][0]),
            hold(labels[7][0]),
            hold(labels[8][0])
        ))
    );

    plugin->gui->view->content(
        ce::align_center_middle(ce::vmargin(10, ce::pane(
            "Launchpad tuner",
            ce::vtile(
                ce::htile(
                    ce::margin({20, 20, 20, 10}, ce::vtile(ce::label("X step"), ce::vmargin(10, ce::hmin_size(100, xStep.first)))),
                    ce::margin({20, 20, 20, 10}, ce::vtile(ce::label("Y step"), ce::vmargin(10, ce::hmin_size(100, yStep.first)))),
                    ce::margin({20, 20, 20, 10}, ce::vtile(ce::label("Transpose"), ce::vmargin(10, ce::hmin_size(100, transpose.first))))
                ),
                ce::margin({20, 0, 20, 20}, diagram)
            )
        )
    )));
#if defined(_WIN32)
    plugin->gui->window = plugin->gui->view->host();
#elif defined(__linux__)
    plugin->gui->display = ce::get_display();
    plugin->gui->window = plugin->gui->view->host()->x_window;

    if (plugin->hostPOSIXFDSupport && plugin->hostPOSIXFDSupport->register_fd)
    {
        plugin->hostPOSIXFDSupport->register_fd(
            plugin->host, ConnectionNumber(plugin->gui->display), CLAP_POSIX_FD_READ);
    }
#endif
}

static const clap_plugin_gui_t extensionGUI = {
    .is_api_supported = [] (const clap_plugin_t *plugin, const char *api, bool isFloating) -> bool {
        return 0 == strcmp(api, GUI_API) && !isFloating;
    },

    .get_preferred_api = [] (const clap_plugin_t *plugin, const char **api, bool *isFloating) -> bool {
        *api = GUI_API;
        *isFloating = false;
        return true;
    },

    .create = [] (const clap_plugin_t *_plugin, const char *api, bool isFloating) -> bool {
        if (!extensionGUI.is_api_supported(_plugin, api, isFloating)) return false;
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        GUICreate(plugin);
        GUISetup(plugin);
        return true;
    },

    .destroy = [] (const clap_plugin_t *_plugin) {
        GUIDestroy((MyPlugin *) _plugin->plugin_data);
    },

    .set_scale = [] (const clap_plugin_t *plugin, double scale) -> bool {
        return false;
    },

    .get_size = [] (const clap_plugin_t *plugin, uint32_t *width, uint32_t *height) -> bool {
        *width = GUI_WIDTH;
        *height = GUI_HEIGHT;
        return true;
    },

    .can_resize = [] (const clap_plugin_t *plugin) -> bool {
        return false;
    },

    .get_resize_hints = [] (const clap_plugin_t *plugin, clap_gui_resize_hints_t *hints) -> bool {
        return false;
    },

    .adjust_size = [] (const clap_plugin_t *plugin, uint32_t *width, uint32_t *height) -> bool {
        return extensionGUI.get_size(plugin, width, height);
    },

    .set_size = [] (const clap_plugin_t *plugin, uint32_t width, uint32_t height) -> bool {
        return true;
    },

    .set_parent = [] (const clap_plugin_t *_plugin, const clap_window_t *window) -> bool {
        assert(0 == strcmp(window->api, GUI_API));
        GUISetParent((MyPlugin *) _plugin->plugin_data, window);
        return true;
    },

    .set_transient = [] (const clap_plugin_t *plugin, const clap_window_t *window) -> bool {
        return false;
    },

    .suggest_title = [] (const clap_plugin_t *plugin, const char *title) {
    },

    .show = [] (const clap_plugin_t *_plugin) -> bool {
        GUISetVisible((MyPlugin *) _plugin->plugin_data, true);
        return true;
    },

    .hide = [] (const clap_plugin_t *_plugin) -> bool {
        GUISetVisible((MyPlugin *) _plugin->plugin_data, false);
        return true;
    },
};

static const clap_plugin_posix_fd_support_t extensionPOSIXFDSupport = {
    .on_fd = [] (const clap_plugin_t *_plugin, int fd, clap_posix_fd_flags_t flags) {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        GUIOnPOSIXFD(plugin);
    },
};

static const clap_plugin_t pluginClass = {
    .desc = &pluginDescriptor,
    .plugin_data = nullptr,

    .init = [] (const clap_plugin *_plugin) -> bool {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->hostPOSIXFDSupport = (const clap_host_posix_fd_support_t *) plugin->host->get_extension(plugin->host, CLAP_EXT_POSIX_FD_SUPPORT);

        for (uint32_t i = 0; i < P_COUNT; i++) {
            clap_param_info_t information = {};
            extensionParams.get_info(_plugin, i, &information);
            plugin->mainParameters[i] = information.default_value;
        }

        set_tuning(
            plugin->mainParameters[P_XSTEP],
            plugin->mainParameters[P_YSTEP],
            plugin->mainParameters[P_TRANSPOSE]
        );

        return true;
    },

    .destroy = [] (const clap_plugin *_plugin) {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        MTS_DeregisterMaster();

        free(plugin);
    },

    .activate = [] (const clap_plugin *_plugin, double sampleRate, uint32_t minimumFramesCount, uint32_t maximumFramesCount) -> bool {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        if (MTS_CanRegisterMaster())
        {
            MTS_RegisterMaster();
        }

        return true;
    },

    .deactivate = [] (const clap_plugin *_plugin) {
        MTS_DeregisterMaster();
    },

    .start_processing = [] (const clap_plugin *_plugin) -> bool {
        return true;
    },

    .stop_processing = [] (const clap_plugin *_plugin) {
    },

    .reset = [] (const clap_plugin *_plugin) {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
    },

    .process = [] (const clap_plugin *_plugin, const clap_process_t *process) -> clap_process_status {
        return CLAP_PROCESS_CONTINUE;
    },

    .get_extension = [] (const clap_plugin *plugin, const char *id) -> const void * {
        if (0 == strcmp(id, CLAP_EXT_PARAMS          )) return &extensionParams;
        if (0 == strcmp(id, CLAP_EXT_GUI             )) return &extensionGUI;
        if (0 == strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT)) return &extensionPOSIXFDSupport;
        if (0 == strcmp(id, CLAP_EXT_STATE           )) return &extensionState;
        return nullptr;
    },

    .on_main_thread = [] (const clap_plugin *_plugin) {
    },
};

static const clap_plugin_factory_t pluginFactory = {
    .get_plugin_count = [] (const clap_plugin_factory *factory) -> uint32_t { 
        return 1; 
    },

    .get_plugin_descriptor = [] (const clap_plugin_factory *factory, uint32_t index) -> const clap_plugin_descriptor_t * { 
        return index == 0 ? &pluginDescriptor : nullptr; 
    },

    .create_plugin = [] (const clap_plugin_factory *factory, const clap_host_t *host, const char *pluginID) -> const clap_plugin_t * {
        if (!clap_version_is_compatible(host->clap_version) || strcmp(pluginID, pluginDescriptor.id)) {
            return nullptr;
        }

        MyPlugin *plugin = (MyPlugin *) calloc(1, sizeof(MyPlugin));
        plugin->host = host;
        plugin->plugin = pluginClass;
        plugin->plugin.plugin_data = plugin;
        return &plugin->plugin;
    },
};

extern "C" const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,

    .init = [] (const char *path) -> bool { 
        return true; 
    },

    .deinit = [] () {},

    .get_factory = [] (const char *factoryID) -> const void * {
        return strcmp(factoryID, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &pluginFactory;
    },
};
