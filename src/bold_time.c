#include <pebble.h>
#include "gcolor_definitions.h"

#define SETTINGS_KEY 1

// important variables for below
Window *window;
Layer *watchface_layer;
GRect window_get_unobstructed_area(Window *win);

// struct for holding the values of our settings
typedef struct ClaySettings {
    GColor background_color;
    GColor hour_one_color;
    GColor hour_two_color;
    GColor minute_one_color;
    GColor minute_two_color;

    int border_thickness;
    int gap_thickness;
    
    bool six_tail;
    bool seven_tail;
    bool nine_tail;
} ClaySettings;

// instantiation of that struct
static ClaySettings settings;

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

// necessary sacrifices
static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// given to the pebble gods
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

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

  layer_mark_dirty(watchface_layer);
}

// how we decide which cells to illuminate for which digits
static const bool ILLUMINATION_TABLE[10][15] = {
// yes I know these are the wrong datatypes but this looks better

    bool S = 1; //settings.six_tail;
    bool Z = 1; //settings.seven_tail;
    bool N = 1; //settings.nine_tail;

//   a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, 
    {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1},   // 0
    {1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1},   // 1
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},   // 2         a b c
    {1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1},   // 3         d e f
    {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1},   // 4         g h i
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1},   // 5         j k l
    {1, S, S, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1},   // 6         m n o        
    {1, 1, 1, Z, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},   // 7 
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},   // 8
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, N, N, 1},   // 9
};

// dynamically and symmetrically add pixels to cell lengths 
int width_correction(int remainder, int index) {
    if (remainder == 1 && index == 1) {
        return 1;
    } else if (remainder == 2 && (index == 0 || index == 2)) {
        return 1;
    } else {
        return 0;
    }
}

// dynamically and symmetrically add pixels to cell heights
int height_correction(int remainder, int index) {
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
void draw_digit(GContext *ctx, GPoint origin, GColor color, int width, int height, int digit) {
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
void watchface_update(Layer *layer, GContext *ctx) {
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

// clear out the stuff for time reception? not really sure about this one
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
static void init() {
    // get our settings in
    load_settings();

    // construct window and get it into position
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true);

    // subscribe us to the minute service
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    // handle getting the settings from the phone
    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(dict_calc_buffer_size(12), 0);
} 

// just destroys the window since we already handled the paths
static void deinit() {
    window_destroy(window);
}

// gotta love best practice
int main(void) {
    init();
    app_event_loop();
    deinit();
}