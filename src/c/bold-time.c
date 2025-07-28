#include <pebble.h>
#include "bold-time.h"

// important variables for below
static Window *window;
static Layer *watchface_layer;

// struct for holding the values of our settings
ClaySettings settings;

// the original values
static void default_settings() {
        settings.background_color = GColorBlack;
        settings.hour_one_color = GColorWhite;
        settings.hour_two_color = GColorLightGray;
        settings.minute_one_color = GColorLightGray;;
        settings.minute_two_color = GColorWhite;;

        settings.border_thickness = 2;
        settings.gap_thickness = 2;

        settings.six_tail = true;
        settings.seven_tail = false;
        settings.nine_tail = true;
}

// read settings from storage
static void load_settings() {
    // load default settings
    default_settings();

    // if they exist, read settings from internal storage
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// save settings to internal storage
static void save_settings() {
    // write the data over to internal storage
    persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// a set of clever defines so we don't have to go crazy in the next function
#define LOAD_INT(name)                                                         \
  Tuple *name = dict_find(iter, MESSAGE_KEY_##name);                           \
  if (name)                                                                    \
  settings.name = name->value->int32

#define LOAD_COLOR(name)                                                       \
  Tuple *name = dict_find(iter, MESSAGE_KEY_##name);                           \
  if (name)                                                                    \
  settings.name = GColorFromHEX(name->value->int32)

#define LOAD_BOOL(name)                                                        \
  Tuple *name = dict_find(iter, MESSAGE_KEY_##name);                           \
  if (name)                                                                    \
  settings.name = name->value->int32 == 1

// handle the settings sent from the phone
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    LOAD_COLOR(background_color);
    LOAD_COLOR(hour_one_color);
    LOAD_COLOR(hour_two_color);
    LOAD_COLOR(minute_one_color);
    LOAD_COLOR(minute_two_color);
    LOAD_INT(border_thickness);
    LOAD_INT(gap_thickness);
    LOAD_BOOL(six_tail);
    LOAD_BOOL(seven_tail);
    LOAD_BOOL(nine_tail);
    
    save_settings();

    // mark the layer as dirty so it gets refreshed
    layer_mark_dirty(watchface_layer);
}

// update the illumination table based on user settings
static void populate_illumination_table(void) {
    // yes I know 0 and 1 aren't bools but this looks better
    bool template[10][15] = {
        {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1},
        {1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
        {1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1},
        {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1},
        {1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1},
        {1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1} 
    };

    // copy template into ILLUMINATION TABLE memory
    memcpy(ILLUMINATION_TABLE, template, sizeof(template));

    // manually correct individual values following settings
    ILLUMINATION_TABLE[6][1] = settings.six_tail;
    ILLUMINATION_TABLE[6][2] = settings.six_tail;

    ILLUMINATION_TABLE[7][3] = settings.seven_tail;

    ILLUMINATION_TABLE[9][12] = settings.nine_tail;
    ILLUMINATION_TABLE[9][13] = settings.nine_tail;
}


// dynamically and symmetrically add pixels to cell lengths 
static int width_correction(int remainder, int index) {
    if (remainder == 1 && index == 1) {
        return 1;
    } else if (remainder == 2 && (index == 0 || index == 2)) {
        return 1;
    } else {
        return 0;
    }
}

// dynamically and symmetrically add pixels to cell heights
static int height_correction(int remainder, int index) {
    if (remainder == 1 && index == 2) {
        return 1;
    } else if (remainder == 2 && (index == 1 || index == 3)) {
        return 1;
    } else if (remainder == 3 && (index != 1 && index != 3)) { // hey that's almost clever!
        return 1;
    } else if (remainder == 4 && index != 2) {
        return 1;
    } else {
        return 0;
    }
}

// draws a single digit. GPoint is top left corner of box
static void draw_digit(GContext *ctx, GPoint origin, GColor color, int width, int height, int digit) {
    // set number color
    graphics_context_set_fill_color(ctx, color);

    // individual cell dimensions
    int cellw = width / 3;
    int cellh = height / 5;

    // previous cell end point
    int pcellw = 0;
    int pcellh = 0;

    // find out how many pixels we'll have to add back in
    int remainw = width - 3 * cellw;
    int remainh = height - 5 * cellh;

    // iterate across all 15 segments
    for (int segment = 0; segment < 15; segment++) {
        // get our little coordinate within the cells
        int xindex = segment % 3;
        int yindex = segment / 3;

        // new cell dimensions
        int ncellw = cellw + width_correction(remainw, xindex);
        int ncellh = cellh + height_correction(remainh, yindex);

        // light it up!
        if (ILLUMINATION_TABLE[digit][segment]) {
            // draw our rectangle
            graphics_fill_rect(ctx, GRect(origin.x + pcellw, origin.y + pcellh, ncellw, ncellh), 0, GCornerNone);
        }

        // update previous
        if (xindex != 2) {
            pcellw += ncellw;
        } else {
            pcellw = 0;
            pcellh += ncellh;
        }
    }
}

// update the watchface (runs every time-> call)
static void watchface_update(Layer *layer, GContext *ctx) {
    // get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    bool military = clock_is_24h_style();

    // get the right hour digits
    if (!military) {
        // 13:00 - 23:00 -> 1:00 - 11:00
        if (t->tm_hour > 12) {
            t->tm_hour -= 12;
        // 0:00 -> 12:00
        } else if (t->tm_hour == 0) {
            t->tm_hour += 12;
        }
    }
    
    // break it down
    int h1 = t->tm_hour / 10;
    int h2 = t->tm_hour % 10;
    int m1 = t->tm_min / 10;
    int m2 = t->tm_min % 10;
    
    // get screen dimensions (make this reactive to alerts)
    GRect bounds = layer_get_unobstructed_bounds(layer);
    int screenwidth = bounds.size.w;
    int screenheight = bounds.size.h;

    // set background color
    graphics_context_set_fill_color(ctx, settings.background_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // figure out number size
    int width = (screenwidth - settings.gap_thickness) / 2 - settings.border_thickness;
    int height = (screenheight - settings.gap_thickness) / 2 - settings.border_thickness;

    // correction for if someone sets gap to an odd value. Adds 1 to bottom and right
    int cor = (settings.gap_thickness % 2 == 0) ? 0 : 1;

    // set start point for first digit
    GPoint drawpoint = GPoint(settings.border_thickness, settings.border_thickness);

    // load user settings and get ILLUMINATION TABLE ready for reference
    populate_illumination_table();

    // write the time
    draw_digit(ctx, drawpoint, settings.hour_one_color, width, height, h1);
    drawpoint.x += width + settings.gap_thickness;

    draw_digit(ctx, drawpoint, settings.hour_two_color, width + cor, height, h2);
    drawpoint.x -= width + settings.gap_thickness;
    drawpoint.y += height + settings.gap_thickness;

    draw_digit(ctx, drawpoint, settings.minute_one_color, width, height + cor, m1);
    drawpoint.x += width + settings.gap_thickness;

    draw_digit(ctx, drawpoint, settings.minute_two_color, width + cor, height + cor, m2);
}

// signals to redraw the screen after a minute has occured
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    layer_mark_dirty(window_get_root_layer(window));
}

// window load function to initialize the watchface
void window_load(Window *window) {
    // get info for window size
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // construct the layer and set up its update proceedures
    watchface_layer = layer_create(bounds);
    layer_set_update_proc(watchface_layer, watchface_update);
    layer_add_child(window_get_root_layer(window), watchface_layer);
}

// Window unload function to clean up
void window_unload(Window *window) {
    layer_destroy(watchface_layer);
}

// init() to handle everything that has to get done at the startt
static void init(void) {
    // get our settings in
    load_settings();

    // handle getting the settings from the phone
    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(dict_calc_buffer_size(12), 0);

    // construct window and get it into position
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true);

    // subscribe us to the minute service
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
} 

// just destroys the window since we already handled the paths
static void deinit() {
    if (window) {
        window_destroy(window);
    }
}

// gotta love best practice
int main(void) {
    init();
    app_event_loop();
    deinit();
}