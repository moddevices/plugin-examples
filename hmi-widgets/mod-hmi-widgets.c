#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"

#include "lv2-hmi.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLUGIN_URI "http://moddevices.com/plugins/mod-devel/mod-hmi-widgets"

typedef enum {
    PLUGIN_TOGGLE_ASSIGNMENT = 0,
    PLUGIN_LINEAR_ASSIGNMENT,
} PortIndex;

#define LYRIC_WORD_COUNT 74

const char* all_star[] =
{
    (char*)("Somebody"),
    (char*)("Once"),
    (char*)("Told"),
    (char*)("Me"),
    (char*)("The"),
    (char*)("World"),
    (char*)("Is"),
    (char*)("Gonna"),
    (char*)("Roll"),
    (char*)("Me I"),
    (char*)("ain't"),
    (char*)("the"),
    (char*)("sharpest"),
    (char*)("tool"),
    (char*)("in"),
    (char*)("the"),
    (char*)("shed"),
    (char*)("She"),
    (char*)("was"),
    (char*)("looking"),
    (char*)("kind"),
    (char*)("of"),
    (char*)("dumb"),
    (char*)("with"),
    (char*)("her"),
    (char*)("finger"),
    (char*)("and"),
    (char*)("her"),
    (char*)("thumb"),
    (char*)("In"),
    (char*)("the"),
    (char*)("shape"),
    (char*)("of"),
    (char*)("an"),
    (char*)("L"),
    (char*)("on"),
    (char*)("her"),
    (char*)("forehead"),
    (char*)("Well"),
    (char*)("the"),
    (char*)("years"),
    (char*)("start"),
    (char*)("coming"),
    (char*)("and"),
    (char*)("they"),
    (char*)("don't"),
    (char*)("stop"),
    (char*)("coming"),
    (char*)("Fed"),
    (char*)("to"),
    (char*)("the"),
    (char*)("rules"),
    (char*)("and"),
    (char*)("I"),
    (char*)("hit"),
    (char*)("the"),
    (char*)("ground"),
    (char*)("running"),
    (char*)("Didn't"),
    (char*)("make"),
    (char*)("sense"),
    (char*)("not"),
    (char*)("to"),
    (char*)("live"),
    (char*)("for"),
    (char*)("fun"),
    (char*)("Your"),
    (char*)("brain"),
    (char*)("gets"),
    (char*)("smart"),
    (char*)("but"),
    (char*)("your"),
    (char*)("head"),
    (char*)("gets"),
    (char*)("dumb"),
};

typedef struct {
    // Control ports
    float* toggle;
    float* linear;

    // Features
    LV2_URID_Map*  map;
    LV2_Log_Logger logger;
    LV2_HMI_WidgetControl* hmi;

    // Internal state
    int sample_counter;
    int sample_rate;

    // To send to lables
    int run_count;

    // HMI Widgets stuff
    LV2_HMI_Addressing toggle_addressing;
    LV2_HMI_Addressing linear_addressing;
    LV2_HMI_LED_Colour colour;
} Control;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
    Control* self = (Control*)calloc(sizeof(Control), 1);

    // Get host features
    // clang-format off
    const char* missing = lv2_features_query(
            features,
            LV2_LOG__log,           &self->logger.log, false,
            LV2_URID__map,          &self->map,        true,
            LV2_HMI__WidgetControl, &self->hmi,        true,
            NULL);
    // clang-format on

    lv2_log_logger_set_map(&self->logger, self->map);

    if (missing) {
        lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
        free(self);
        return NULL;
    }

    self->sample_rate = (int)rate;
    self->colour = LV2_HMI_LED_Colour_Off;
    self->run_count = 0;

    return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
    Control* self = (Control*)instance;

    switch ((PortIndex)port) {
        case PLUGIN_TOGGLE_ASSIGNMENT:
            self->toggle = (float*)data;
            break;
        case PLUGIN_LINEAR_ASSIGNMENT:
            self->linear = (float*)data;
            break;
    }
}

static void
activate(LV2_Handle instance)
{
    Control* self = (Control*) instance;

    self->sample_counter = 0;
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
    Control* self = (Control*) instance;

    for (unsigned i = 0; i < n_samples; ++i, ++self->sample_counter) {
        if (self->sample_counter == self->sample_rate) {

            if (self->run_count > LYRIC_WORD_COUNT)
                self->run_count = 0;

            self->sample_counter = 0;

            if (self->linear_addressing != NULL)  {
                self->hmi->set_label(self->hmi->handle, self->linear_addressing, all_star[self->run_count++]);
                self->hmi->set_value(self->hmi->handle, self->linear_addressing, all_star[self->run_count++]);
                self->hmi->set_unit(self->hmi->handle, self->linear_addressing, all_star[self->run_count++]);

                self->hmi->set_indicator(self->hmi->handle, self->linear_addressing, (float)((float)self->run_count / (float)LYRIC_WORD_COUNT));
            }

            if (self->toggle_addressing != NULL) {
                if (++self->colour > LV2_HMI_LED_Colour_White) {
                    self->colour = LV2_HMI_LED_Colour_Off;
                }
                lv2_log_trace(&self->logger, "HMI set color to %i", self->colour);
                if ((int)*self->toggle)
                    self->hmi->set_led(self->hmi->handle, self->toggle_addressing, self->colour, 100, 100);
                else
                    self->hmi->set_led(self->hmi->handle, self->toggle_addressing, self->colour, 0, 0);

                self->hmi->set_label(self->hmi->handle, self->toggle_addressing, all_star[self->run_count++]);
            }

            if ((self->linear_addressing == NULL) && (self->toggle_addressing == NULL) ) {
                lv2_log_trace(&self->logger, "HMI not addressed");
            }
        }
    }
}

static void
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
    free(instance);
}

static void
addressed(LV2_Handle handle, uint32_t index, LV2_HMI_Addressing addressing, const LV2_HMI_AddressingInfo* info)
{
    Control* self = (Control*) handle;

    if (index == 0)
        self->toggle_addressing = addressing;
    else
        self->linear_addressing = addressing;
}

static void
unaddressed(LV2_Handle handle, uint32_t index)
{
    Control* self = (Control*) handle;

    if (index == 0)
        self->toggle_addressing = NULL;
    else
        self->linear_addressing = NULL;
}

static const void*
extension_data(const char* uri)
{
    static const LV2_HMI_PluginNotification hmiNotif = {
        addressed,
        unaddressed,
    };
    if (!strcmp(uri, LV2_HMI__PluginNotification))
        return &hmiNotif;
    return NULL;
}

static const LV2_Descriptor descriptor = {
    PLUGIN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    switch (index) {
        case 0:  return &descriptor;
        default: return NULL;
    }
}
