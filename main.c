#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_pedo_layer;
static TextLayer *s_count_layer;
static TextLayer *s_time_layer;
static BitmapLayer *s_bmap_layer;
static GBitmap *mm1;
static GBitmap *mm2;
static GBitmap *mm3;
static GBitmap *currBmap;

// Timer used to determine next step check
static AppTimer *timer;

int X_DELTA = 35;
int Y_DELTA, Z_DELTA = 185;
int YZ_DELTA_MIN = 175;
int YZ_DELTA_MAX = 195; 
int X_DELTA_TEMP, Y_DELTA_TEMP, Z_DELTA_TEMP = 0;
int lastX, lastY, lastZ, currX, currY, currZ = 0;
int sensitivity = 1;

long pedometerCount = 0;

bool did_pebble_vibrate = false;
bool validX, validY, validZ = false;
bool startedSession = false;

// interval to check for next step (in ms)
const int ACCEL_STEP_MS = 475;
// value to auto adjust step acceptance 
const int PED_ADJUST = 2;

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

void autoCorrectZ(){
  if (Z_DELTA > YZ_DELTA_MAX){
    Z_DELTA = YZ_DELTA_MAX; 
  } else if (Z_DELTA < YZ_DELTA_MIN){
    Z_DELTA = YZ_DELTA_MIN;
  }
}

void autoCorrectY(){
  if (Y_DELTA > YZ_DELTA_MAX){
    Y_DELTA = YZ_DELTA_MAX; 
  } else if (Y_DELTA < YZ_DELTA_MIN){
    Y_DELTA = YZ_DELTA_MIN;
  }
}

void pedometer_update() {
  if (startedSession) {
    X_DELTA_TEMP = abs(abs(currX) - abs(lastX));
    if (X_DELTA_TEMP >= X_DELTA) {
      validX = true;
    }
    Y_DELTA_TEMP = abs(abs(currY) - abs(lastY));
    if (Y_DELTA_TEMP >= Y_DELTA) {
      validY = true;
      if (Y_DELTA_TEMP - Y_DELTA > 200){
        autoCorrectY();
        Y_DELTA = (Y_DELTA < YZ_DELTA_MAX) ? Y_DELTA + PED_ADJUST : Y_DELTA;
      } else if (Y_DELTA - Y_DELTA_TEMP > 175){
        autoCorrectY();
        Y_DELTA = (Y_DELTA > YZ_DELTA_MIN) ? Y_DELTA - PED_ADJUST : Y_DELTA;
      }
    }
    Z_DELTA_TEMP = abs(abs(currZ) - abs(lastZ));
    if (abs(abs(currZ) - abs(lastZ)) >= Z_DELTA) {
      validZ = true;
      if (Z_DELTA_TEMP - Z_DELTA > 200){
        autoCorrectZ();
        Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
      } else if (Z_DELTA - Z_DELTA_TEMP > 175){
        autoCorrectZ();
        Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
      }
    }
  } else {
    startedSession = true;
  }
}

void resetUpdate() {
  lastX = currX;
  lastY = currY;
  lastZ = currZ;
  validX = false;
  validY = false;
  validZ = false;
}

void update_ui_callback() {
  if ((validX && validY && !did_pebble_vibrate) || (validX && validZ && !did_pebble_vibrate)) {
    pedometerCount++;

    static char buf[] = "123456890abcdefghijkl";
    snprintf(buf, sizeof(buf), "%ld", pedometerCount);
    text_layer_set_text(s_count_layer, buf);
    
    //Destroy old image if there is one, and update.
    if (bitmap_layer_get_bitmap(s_bmap_layer) != NULL){
      currBmap = bitmap_layer_get_bitmap(s_bmap_layer);
    } else {
      bitmap_layer_set_bitmap(s_bmap_layer, mm1);
    }
    
    if (currBmap == mm1){
      gbitmap_destroy(mm1);
      bitmap_layer_set_bitmap(s_bmap_layer, mm1);
    } else if (currBmap == mm2){
      gbitmap_destroy(mm2);
      bitmap_layer_set_bitmap(s_bmap_layer, mm3);      
    } else if (currBmap == mm3){
      gbitmap_destroy(mm3);
      bitmap_layer_set_bitmap(s_bmap_layer, mm1);      
    }     
  }
  resetUpdate();
}

static void timer_callback(void *data) {
  AccelData accel = (AccelData ) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  if (!startedSession) {
    lastX = accel.x;
    lastY = accel.y;
    lastZ = accel.z;
  } else {
    currX = accel.x;
    currY = accel.y;
    currZ = accel.z;
  }
  
  did_pebble_vibrate = accel.did_vibrate;

  pedometer_update();
  update_ui_callback();

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
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
  s_pedo_layer = text_layer_create(GRect(0,110,144,25));
  text_layer_set_background_color(s_pedo_layer, GColorBlack);
  text_layer_set_text_color(s_pedo_layer, GColorWhite);
  //Prettify the layout...
  text_layer_set_font(s_pedo_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_pedo_layer, GTextAlignmentCenter);
  //Set the text!
  text_layer_set_text(s_pedo_layer, "Step counts...");

  s_count_layer = text_layer_create(GRect(0,133,144,35));
  text_layer_set_background_color(s_count_layer, GColorClear);
  text_layer_set_text_color(s_count_layer, GColorBlack); 
  //Prettify the layout...
  text_layer_set_font(s_count_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
  
  s_bmap_layer = bitmap_layer_create(GRect(70,70,53,50));
  
  //add it to the windows root layer
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_pedo_layer));
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_count_layer));
  layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(s_bmap_layer));
  
}

static void main_window_unload(Window *Window){
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_pedo_layer);
}

static void init(){
  accel_data_service_subscribe(0, NULL);
  s_main_window = window_create();
  
  mm1 = gbitmap_create_with_resource(res1);
  mm2 = gbitmap_create_with_resource(res2);
  mm3 = gbitmap_create_with_resource(res3);
  
  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
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
  gbitmap_destroy(mm1);
  gbitmap_destroy(mm2);
  gbitmap_destroy(mm3);
  bitmap_layer_destroy(s_bmap_layer);
  window_destroy(s_main_window);  
}

int main(void){
  init();
  app_event_loop();
  deinit();
}