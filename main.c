#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_pedo_layer;
static TextLayer *s_time_layer;


static void update_time(){
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char buffer[] = "00:00";

  if (clock_is_24h_style() == true){
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

static void main_window_load(){  
  //Create the TextLayer...
  s_time_layer = text_layer_create(GRect(0,10,144,50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);  
  //Prettify the layout...
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);


  //Create the layer for pedometer
  s_pedo_layer = text_layer_create(GRect(0,150,144,50));
  text_layer_set_background_color(s_pedo_layer, GColorBlack);
  text_layer_set_text_color(s_pedo_layer, GColorWhite);
  //Prettify the layout...
  text_layer_set_font(s_pedo_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  //Set the text!
  text_layer_set_text(s_pedo_layer, "Pedometer count here");

  //add it to the windows root layer
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_pedo_layer));
}

static void main_window_unload(Window *Window){
  	text_layer_destroy(s_time_layer);
    text_layer_destroy(s_pedo_layer);
}

static void init(){
  s_main_window = window_create();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);



  window_set_window_handlers(s_main_window, (WindowHandlers){
  	.load = main_window_load,
  	.unload = main_window_unload
  });

  //Show window on the watch... with animated = true
  window_stack_push(s_main_window, true);

  //Make sure the time is displayed from the start
  update_time();

}

static void deinit(){
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}