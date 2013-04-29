/*Copyright (c) 2013 Bit Hangar
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "timer.h"

#define MY_UUID { 0x7D, 0xAF, 0xEB, 0x68, 0xBF, 0x5C, 0x4B, 0x9F, 0xB6, 0xA3, 0x73, 0x8F, 0x6A, 0xBC, 0xE7, 0xEF }

//function prototypes
void secondTick();
void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);
void start_button_handler(ClickRecognizerRef recognizer, Window *window);
void reset_button_handler(ClickRecognizerRef recognizer, Window *window);
void split_button_handler(ClickRecognizerRef recognizer, Window *window);
void config_provider(ClickConfig **config, Window *window);
void handle_init_app(AppContextRef app_ctx);
void handle_deinit(AppContextRef ctx);
void pbl_main(void *params);

PBL_APP_INFO(MY_UUID,
             "Stopwatch", "Bit Hangar",
             1, 1, /* App version */
             RESOURCE_ID_MENU_ICON,
             0);

//Application window
Window window;
AppContextRef global_ctx;

//Background layer for stopwatch
static BmpContainer backgroundImage;

//Text layers for time
static TextLayer countLayer;
static TextLayer dSecondsLayer;
static TextLayer splitLayer;

static AppTimerHandle timer_handle;

static char timeText[] = "00:00:00";

static int dSeconds = 0;


void secondTick() {
    //Recalculate time with Pebble time
    PblTm outTime = get_display_time();

   
    //format and display
    string_format_time(timeText, sizeof(timeText), "%T", &outTime);
    text_layer_set_text(&countLayer, timeText);
}

void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)app_ctx;
    (void)handle;

    dSeconds = update_timer();    

    //Static due to pebble 'set_text'
    static char out[] = ".0";

    //Convert decisecond to character
    out[1] = (char)dSeconds+48;

    text_layer_set_text(&dSecondsLayer, out);

    //After 10 deciseconds, get full time by Pebble time comparison
    if (dSeconds == 0)
        secondTick();

    //Keep timer going for each decisecond
    if (is_timer_running())
       timer_handle = app_timer_send_event(app_ctx, 100 , cookie);
}

void start_button_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;

    //Start if not running, Stop if running
    if (!is_timer_running() && !is_max_time_reached()) {
        timer_start();

        //Initialize timer for ever
        timer_handle = app_timer_send_event(global_ctx, 100, 1);
    } else
    {
        timer_stop();   

        app_timer_cancel_event(global_ctx, timer_handle); 
    }
}

void reset_button_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;

    //Cancel timer from firing
    if (is_timer_running())
        app_timer_cancel_event(global_ctx, timer_handle);

    //clear out static variables
    reset();
   
    //Reset all text on display
    text_layer_set_text(&countLayer, "00:00:00");
    text_layer_set_text(&dSecondsLayer, ".0");
    text_layer_set_text(&splitLayer, "");

    //If reset while running, continute to run
    if (is_timer_running()) {
        //get_time(&startTime);
        timer_handle = app_timer_send_event(global_ctx, 100, 1);
    }
    
}

void split_button_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;

    if (is_timer_running())
    {
        //Static due to pebble 'set_text'
        static char out[] = "Split: 00:00:00.0";

        //Used to hold time from Pebble clock
        char splitTime[] = "00:00:00";
        //*out = 0;

        //Build up text, start with 'split'
        strcpy(out, "Split: ");

        //Add total time
        PblTm outTime = get_pebble_time();
        string_format_time(splitTime, sizeof(splitTime), "%T", &outTime);
        strcat(out, splitTime);

        //Add decisecond
        strcat(out, ".");
        out[16] = (char)dSeconds+48;

        //Update display
        text_layer_set_text(&splitLayer, out);
    }
}

void config_provider(ClickConfig **config, Window *window) {
    (void)window;
    
    //Set button handlers
    config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) start_button_handler;
    config[BUTTON_ID_UP]->click.handler = (ClickHandler) reset_button_handler;
    config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) split_button_handler;
}

void handle_init_app(AppContextRef app_ctx) {
    (void)app_ctx;
    (void)global_ctx;

    global_ctx = app_ctx;

    resource_init_current_app(&APP_RESOURCES);

    window_init(&window, "Stopwatch");
    window_stack_push(&window, true /* Animated */);

    //Set up the layout of the application
    bmp_init_container(RESOURCE_ID_WATCH_BACKGROUND, &backgroundImage);
    layer_add_child(&window.layer, &backgroundImage.layer.layer);

    text_layer_init(&countLayer, GRect(2, 95, 120, 40));
    text_layer_set_text_color(&countLayer, GColorBlack);
    text_layer_set_background_color(&countLayer, GColorClear);
    text_layer_set_font(&countLayer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
    text_layer_set_text(&countLayer, "00:00:00");
    layer_add_child(&window.layer, &countLayer.layer);

    text_layer_init(&dSecondsLayer, GRect(115, 95, 28, 40));
    dSecondsLayer.text_alignment = GTextAlignmentLeft;
    text_layer_set_text_color(&dSecondsLayer, GColorBlack);
    text_layer_set_background_color(&dSecondsLayer, GColorClear);
    text_layer_set_font(&dSecondsLayer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
    text_layer_set_text(&dSecondsLayer, ".0");
    layer_add_child(&window.layer, &dSecondsLayer.layer);

    text_layer_init(&splitLayer, GRect(50, 75, 90, 40));
    splitLayer.text_alignment = GTextAlignmentLeft;
    text_layer_set_text_color(&splitLayer, GColorBlack);
    text_layer_set_background_color(&splitLayer, GColorClear);
    text_layer_set_font(&splitLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    layer_add_child(&window.layer, &splitLayer.layer);

    // Attach our desired button functionality
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

    // Initialize timer with AppContextRef, AppTimerHandle, and max seconds
    init_timer(app_ctx, timer_handle, 359999);
}

void handle_deinit(AppContextRef ctx) {
    (void)ctx;

    //Remove bmp
    bmp_deinit_container(&backgroundImage);
}

void pbl_main(void *params) {
    //Set up handlers
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init_app,
        .deinit_handler = &handle_deinit,
        .timer_handler = &handle_timer,
    };
    app_event_loop(params, &handlers);
}
