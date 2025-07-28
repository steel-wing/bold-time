#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1

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
} __attribute__((__packed__)) ClaySettings;

static bool ILLUMINATION_TABLE[10][15];

static void default_settings();
static void load_settings();
static void save_settings();

static void populate_illumination_table(void);
static int width_correction(int remainder, int index);
static int height_correction(int remainder, int index);
static void draw_digit(GContext *ctx, GPoint origin, GColor color, int width, int height, int digit);
static void watchface_update(Layer *layer, GContext *ctx);

static void inbox_received_handler(DictionaryIterator *iter, void *context);
static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);
