#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "lv_log.h"

#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)
#define LV_COLOR_SILVER lv_color_make(0xC0, 0xC0, 0xC0)
#define LV_COLOR_GRAY lv_color_make(0x80, 0x80, 0x80)
#define LV_COLOR_BLACK lv_color_make(0x00, 0x00, 0x00)
#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)
#define LV_COLOR_MAROON lv_color_make(0x80, 0x00, 0x00)
#define LV_COLOR_YELLOW lv_color_make(0xFF, 0xFF, 0x00)
#define LV_COLOR_OLIVE lv_color_make(0x80, 0x80, 0x00)
#define LV_COLOR_LIME lv_color_make(0x00, 0xFF, 0x00)
#define LV_COLOR_GREEN lv_color_make(0x00, 0x80, 0x00)
#define LV_COLOR_CYAN lv_color_make(0x00, 0xFF, 0xFF)
#define LV_COLOR_AQUA LV_COLOR_CYAN
#define LV_COLOR_TEAL lv_color_make(0x00, 0x80, 0x80)
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)
#define LV_COLOR_NAVY lv_color_make(0x00, 0x00, 0x80)
#define LV_COLOR_MAGENTA lv_color_make(0xFF, 0x00, 0xFF)
#define LV_COLOR_PURPLE lv_color_make(0x80, 0x00, 0x80)
#define LV_COLOR_ORANGE lv_color_make(0xFF, 0xA5, 0x00)
#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 320
#define DISP_BUF_SIZE (480 * 320 * 16)
#define __Sizeof(arr) (sizeof(arr)/sizeof(arr[0]))
LV_FONT_DECLARE(Font_Impact_120);

uint32_t custom_tick_get(void);
/*Clock*/
typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t ms;
    uint16_t year;
    uint8_t  month;
    uint8_t  date;
    uint8_t  week;
} Clock_Value_t;

typedef struct
{
	uint8_t temp;
	uint8_t hum;
}Weather_Value_t;
typedef struct{
    lv_obj_t * label_1;
    lv_obj_t * label_2;
    lv_anim_t anim_now;
    lv_anim_t anim_next; 
    lv_coord_t y_offset;
    uint8_t value_last;
    lv_coord_t x_offset;
}lv_label_time_effect_t;

Clock_Value_t Clock;
Weather_Value_t Weather;

static lv_label_time_effect_t labelTimeEffect[4];
static lv_obj_t* ledSecGrp[2];
static lv_obj_t* Cont;
static lv_obj_t* contTime;
static lv_obj_t* labelDate;
static lv_obj_t* labelWeather;

//"Sun Mon Tue Wed Thu Fri Sat "
const char* week_str[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static void lv_label_time_effect_check_value(lv_label_time_effect_t * effect, uint8_t value);
static void lv_label_time_effect_init(
    lv_label_time_effect_t* effect,
    lv_obj_t* cont,
    lv_obj_t* label_copy,
    uint16_t anim_time,
    lv_coord_t x_offset
);

static void LabelDate_Create(lv_obj_t* par)
{
    lv_obj_t* label = lv_label_create(par);
    lv_label_set_text(label, "2023 . 6 . 1 [4]");
    lv_obj_set_height(label, 32);
    lv_obj_align_to(label, contTime, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(label, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
    labelDate = label;
}

static void LabelWeather_Create(lv_obj_t* par)
{
    lv_obj_t* label = lv_label_create(par);
    lv_label_set_text(label, "TMP: 0C HUM: 0%");
    lv_obj_set_height(label, 32);
    lv_obj_align_to(label, contTime, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(label, LV_COLOR_GRAY, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
    labelWeather = label;
}

static void label_date_update(int year, int mon, int day, int week)
{
	
    lv_label_set_text_fmt(labelDate, "%d . %d . %d  %s", year, mon, day, week_str[week]);
}

static void label_weather_update(int temp, int hum)
{
    lv_label_set_text_fmt(labelWeather, "TMP: %dc HUM: %d%", temp, hum);
}

static void LabelTime_Create(lv_obj_t* par)
{
    const lv_coord_t x_mod[4] = {-120, -50, 50, 120};
    for(int i = 0; i < __Sizeof(labelTimeEffect); i++)
    {
        lv_obj_t * label = lv_label_create(par);
        lv_label_set_text(label, "0");
        lv_obj_align(label, LV_ALIGN_CENTER, x_mod[i], 0);
        lv_obj_set_style_text_color(label, LV_COLOR_BLACK, LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &Font_Impact_120, LV_PART_MAIN);
        lv_label_time_effect_init(&labelTimeEffect[i], par, label, 500, x_mod[i]);
    }
    
    lv_obj_t* led = lv_led_create(par);
    lv_obj_set_size(led, 8, 8);
    lv_obj_align(led, LV_ALIGN_CENTER, 0, -15);
    lv_led_set_color(led, LV_COLOR_BLACK);
    ledSecGrp[0] = led;
    
    led = lv_led_create(par);
    lv_obj_set_size(led, 8, 8);
    lv_obj_align(led, LV_ALIGN_CENTER, 0, 15);
    lv_led_set_color(led, LV_COLOR_BLACK);
    ledSecGrp[1] = led;


}

void lv_label_time_effect_init(
    lv_label_time_effect_t* effect,
    lv_obj_t* cont,
    lv_obj_t* label_copy,
    uint16_t anim_time,
    lv_coord_t x_offset
)
{
    lv_obj_t* label = lv_label_create(cont);
    lv_label_set_text(label, lv_label_get_text(label_copy));
    
    lv_obj_set_style_text_color(label, LV_COLOR_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &Font_Impact_120, LV_PART_MAIN);

    effect->y_offset = (160 + 120) / 2 + 1;
    printf("cont_h %d, label_copy_h %d\n", lv_obj_get_height(cont), lv_obj_get_height(label_copy));
    lv_obj_align(label, LV_ALIGN_CENTER, x_offset, -effect->y_offset);
    //lv_obj_align(label, label_copy, LV_ALIGN_CENTER, 0, 0);
    effect->label_1 = label_copy;
    effect->label_2 = label;

    effect->value_last = 0;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_time(&a, anim_time);
    lv_anim_set_delay(&a, 0);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    effect->anim_now = a;
    effect->anim_next = a;
    printf("effect->y_offset [%d], effect->label_1 y [%d],effect->label_2 y [%d] \n",
        effect->y_offset,lv_obj_get_y(effect->label_1),
        lv_obj_get_y(effect->label_2));
}

static void ContTime_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);

    lv_obj_set_size(cont, 350, 160);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cont, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);//Propagate the events to the parent too
    contTime = cont;

    LabelTime_Create(contTime);
}

static void lv_label_time_effect_check_value(lv_label_time_effect_t * effect, uint8_t value)
{
    if(value == effect->value_last)
        return;
    lv_obj_t * next_label;
    lv_obj_t * now_label;

    if(lv_obj_get_y(effect->label_2) > lv_obj_get_y(effect->label_1))
    {
        now_label = effect->label_1;
        next_label = effect->label_2;
        printf("effect->label_2 %d < effect->label_1 %d \n", lv_obj_get_y(effect->label_2) ,lv_obj_get_y(effect->label_1));
    }
    else 
    {
        now_label = effect->label_2;
        next_label = effect->label_1;
        printf("effect->label_2 %d > effect->label_1 %d \n", lv_obj_get_y(effect->label_2), lv_obj_get_y(effect->label_1));
    }

    lv_label_set_text_fmt(now_label, "%d", effect->value_last);
    lv_label_set_text_fmt(next_label, "%d", value);
    effect->value_last = value;

    //lv_obj_align(next_label, LV_ALIGN_CENTER, 0, -effect->y_offset);

    lv_obj_set_y(next_label, -effect->y_offset);
    //lv_obj_align_to(next_label, now_label, LV_ALIGN_OUT_TOP_MID, 0, -effect->y_offset);

    lv_coord_t y_offset = abs(lv_obj_get_y(now_label) - lv_obj_get_y(next_label));

    lv_anim_t* a;
    a = &(effect->anim_now);
    lv_anim_set_var(a, now_label);
    lv_anim_set_values(a, 0, effect->y_offset);
    lv_anim_start(a);

    a = &(effect->anim_next);
    lv_anim_set_var(a, next_label);
    lv_anim_set_values(a, -effect->y_offset, 0);
    lv_anim_start(a);

    LV_LOG_USER("y_offset [%d], now_label y [%d],next_label y [%d]",
        y_offset, lv_obj_get_y(now_label),
        lv_obj_get_y(next_label));
}

static void LabelTime_Update(int h,int m)
{

    lv_label_time_effect_check_value(&labelTimeEffect[3], Clock.min % 10);

    lv_label_time_effect_check_value(&labelTimeEffect[2], Clock.min / 10);

    lv_label_time_effect_check_value(&labelTimeEffect[1], Clock.hour % 10);

    lv_label_time_effect_check_value(&labelTimeEffect[0], Clock.hour / 10);

    lv_led_toggle(ledSecGrp[0]);
    lv_led_toggle(ledSecGrp[1]);
}

static void Cont_Create(lv_obj_t* par)
{
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    //lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_size(obj, WINDOW_WIDTH, WINDOW_HEIGHT);
    lv_obj_set_style_bg_color(obj, LV_COLOR_BLACK, LV_PART_MAIN);
	lv_obj_set_style_radius(obj,0,LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);//Propagate the events to the parent too
    //lv_obj_add_event_cb(obj, (lv_event_cb_t)ContApps_EventHandler, LV_EVENT_GESTURE, NULL);
    Cont = obj;
}

static void dial_update()
{
	struct tm *Time;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    Time = localtime(&tv.tv_sec);
    Clock.year = Time->tm_year+1900;
    Clock.month = Time->tm_mon+1;
    Clock.date = Time->tm_mday;
    Clock.week = Time->tm_wday;
    Clock.hour = Time->tm_hour;
    Clock.min = Time->tm_min;
    Clock.sec = Time->tm_sec;
    label_date_update(Clock.year, Clock.month, Clock.date, Clock.week);
    LabelTime_Update(Clock.hour,Clock.min);
}

static void Setup()
{
	Cont_Create(lv_scr_act());
    ContTime_Create(Cont);
    LabelDate_Create(Cont);
    LabelWeather_Create(Cont);
}

static void Exit()
{
    lv_obj_clean(lv_scr_act());
}

int main(void)
{
    LV_LOG_USER("LittlevGL init\n");
    /*LittlevGL init*/
    lv_init();

    printf("Linux frame buffer device init\n");
    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    printf("Initialize a descriptor for the buffer\n");
    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);
    printf("Initialize and register a display driver\n");
    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 480;
    disp_drv.ver_res    = 320;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);
    printf("evdev_init\n");
    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = evdev_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

    printf("Set a cursor for the mouse\n");
    /*Set a cursor for the mouse*/
    LV_IMG_DECLARE(mouse_cursor_icon)
    lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/

    LV_LOG_USER("LV_COLOR_DEPTH %d", LV_COLOR_DEPTH);

    /*Create a Demo*/
    printf("Create a Demo\n");

    Setup();

    while(1) {
        static int last_tick = 0;
        if(last_tick + 1000 <= custom_tick_get())
        {
            dial_update();
            last_tick = custom_tick_get();
        }
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
