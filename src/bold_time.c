#include <pebble.h>

#define BORDER 2    // border around the numbers
#define GAP 2       // gap between numbers (best kept even)
#define S 1         // the tail on the 6
#define Z 0         // the tail on the 7
#define N 1         // the tail on the 9
#define BACKGROUND GColorBlack
#define COLOR1 GColorWhite
#define COLOR2 GColorLightGray

// this may be bad practice, but I just wanted the big guy at the bottom, okay?
static const bool ILLUMINATION_TABLE[10][15];

// important variables for below
Window *window;
Layer *watchface_layer;
GRect window_get_unobstructed_area(Window *win);

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
    graphics_context_set_fill_color(ctx, BACKGROUND);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // figure out number size
    int width = (screenwidth - GAP) / 2 - BORDER;
    int height = (screenheight - GAP) / 2 - BORDER;

    // correction for if someone sets gap to an odd value. Adds 1 to bottom and right
    int cor = (GAP % 2 == 0) ? 0 : 1;

    // set start point for first digit
    GPoint drawpoint = GPoint(BORDER, BORDER);

    // write the time
    draw_digit(ctx, drawpoint, COLOR1, width, height, h1);
    drawpoint.x += width + GAP;

    draw_digit(ctx, drawpoint, COLOR2, width + cor, height, h2);
    drawpoint.x -= width + GAP;
    drawpoint.y += height + GAP;

    draw_digit(ctx, drawpoint, COLOR2, width, height + cor, m1);
    drawpoint.x += width + GAP;

    draw_digit(ctx, drawpoint, COLOR1, width + cor, height + cor, m2);
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
    window_destroy(window);
}

// gotta love best practice
int main(void) {
    init();
    app_event_loop();
    deinit();
}

// how we decide which cells to illuminate for which digits
static const bool ILLUMINATION_TABLE[10][15] = {
// yes I know these are the wrong datatypes but this looks better

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