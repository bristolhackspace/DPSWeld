
#include "hw.h"
#include "ili9163c.h"
#include "tft.h"
#include "event.h"
#include "mini-printf.h"
#include "pwrctl.h"
#include "tick.h"

#include <libopencm3/stm32/dac.h>

#define UI_UPDATE_INTERVAL_MS 50
#define STARTUP_TIMEOUT_MS 500

typedef enum {
	control_state_startup,
    control_state_error,
    control_state_output_disabled,
    control_state_charging,
    control_state_ready,
    control_state_discharging,
} control_state_t;

static control_state_t control_state;
static const char* error_message = NULL;

static uint64_t state_timeout = 0;
static bool state_timeout_set = false;

static uint32_t target_voltage = 10000;

static uint32_t last_vin;
static uint32_t last_vout;
static uint32_t last_iout;

static void set_state_timeout(uint64_t delay)
{
    state_timeout = get_ticks() + delay;
    state_timeout_set = true;
}

static void state_timeout_check(void)
{
    if(state_timeout_set) {
        bool timed_out = (int64_t)(get_ticks() - state_timeout) > 0;
        if(timed_out) {
            state_timeout_set = false;
            event_put(event_state_timeout, 0);
        }
    }
}

static void refresh_target_voltage(void)
{
    char buf[10];
    tft_puts(FONT_METER_SMALL, "Target voltage", 5, 15, 120, 15, WHITE, false);
    mini_snprintf(buf, sizeof(buf), "%02u.%03uV", target_voltage/1000, (target_voltage%1000));
    tft_puts(FONT_METER_MEDIUM, buf, 5, 37, 120, 22, WHITE, false);
}

static void refresh_measured_voltage(void)
{
    char buf[10];
    tft_puts(FONT_METER_SMALL, "Measured", 5, 52, 120, 15, WHITE, false);
    mini_snprintf(buf, sizeof(buf), "%02u.%03uV", last_vout/1000, (last_vout%1000));
    tft_puts(FONT_METER_MEDIUM, buf, 5, 74, 120, 22, WHITE, false);
}

static void set_ui_status(const char* message, uint16_t colour)
{
    tft_fill(0, 105, 128, 20, BLACK);
    tft_puts(FONT_METER_SMALL, message, 5, 125, 120, 20, colour, false);
}

static void ui_handle_event(event_t event, uint8_t data)
{
    (void)data;

    switch(event) {
        case event_state_transition:
            control_state = (control_state_t)data;
            state_timeout_set = false;
            break;
        case event_brownout:
            control_state = control_state_error;
            state_timeout_set = false;
            break;
        case event_rot_right:
            if(target_voltage < 13000) {
                target_voltage += 100;
                pwrctl_set_vout(target_voltage);
                refresh_target_voltage();
            }
            break;
        case event_rot_left:
            if(target_voltage > 3000) {
                target_voltage -= 100;
                pwrctl_set_vout(target_voltage);
                refresh_target_voltage();
            }
            break;
        default:
            break;
    }

    switch(control_state) {
        case control_state_startup:
            if (event == event_state_timeout) {
                pwrctl_set_vin_limit(14000);
                event_put(event_state_transition, control_state_output_disabled);
            }
            break;
        case control_state_error:
            if(event == event_brownout) {
                set_ui_status("Input brownout", RED);
            }
            break;
        case control_state_output_disabled:
            if(event == event_state_transition) {
                hw_set_discharge();
                hw_clear_trigger();
                pwrctl_enable_vout(false);
                set_ui_status("Output disabled", WHITE);
                refresh_target_voltage();
            }
            if(event == event_button_enable) {
                event_put(event_state_transition, control_state_charging);
            }
            break;
        case control_state_charging:
            if(event == event_state_transition) {
                set_ui_status("Charging...", BLUE);
                hw_clear_discharge();
                pwrctl_enable_vout(true);
            }
            if(event == event_button_enable) {
                event_put(event_state_transition, control_state_output_disabled);
            }
            break;
        case control_state_ready:
            if(event == event_state_transition) {
                set_ui_status("Charged", GREEN);
            }
            if(event == event_button_enable) {
                event_put(event_state_transition, control_state_output_disabled);
            }
            if(event == event_button_sel) {
                event_put(event_state_transition, control_state_discharging);
            }
            break;
        case control_state_discharging:
            if(event == event_state_transition) {
                pwrctl_enable_vout(false);
                hw_set_trigger();
                set_state_timeout(1000);
                set_ui_status("Discharging", MAGENTA);
            }
            if(event == event_button_enable) {
                event_put(event_state_transition, control_state_output_disabled);
            }
            if (event == event_state_timeout) {
                hw_clear_trigger();
                event_put(event_state_transition, control_state_charging);
            }
        default:
            break;
    }
}

static void ui_tick(void)
{
    static uint64_t last = 0;
    /** Update on the first call and every UI_UPDATE_INTERVAL_MS ms */
    if (last > 0 && get_ticks() - last < UI_UPDATE_INTERVAL_MS) {
        return;
    }

    last = get_ticks();

    uint16_t iout_raw;
    uint16_t vin_raw;
    uint16_t vout_raw;
    hw_get_adc_values(&iout_raw, &vin_raw, &vout_raw);
    last_vin = pwrctl_calc_vin(vin_raw);
    last_vout = pwrctl_calc_vout(vout_raw);
    last_iout = pwrctl_calc_iout(iout_raw);

    char buf[10];

    switch(control_state) {
        case control_state_charging:
            if (last_iout < 20 && last_vout + 100 > target_voltage) {
                event_put(event_state_transition, control_state_ready);
            }
            break;
        default:
            break;
    }

    refresh_measured_voltage();

}

static void event_handler(void)
{
    while(1) {
        event_t event;
        uint8_t data = 0;
        if (!event_get(&event, &data)) {
            hw_longpress_check();
            state_timeout_check();
            ui_tick();
        } else {
            // if (event) {
            //     emu_printf(" Event %d 0x%02x\n", event, data);
            // }
            // switch(event) {
            //     case event_none:
            //         dbg_printf("Weird, should not receive 'none events'\n");
            //         break;
            //     case event_uart_rx:
            //         serial_handle_rx_char(data);
            //         break;
            //     case event_ocp:
            //         break;
            //     default:
            //         break;
            // }
            ui_handle_event(event, data);
        }

        // if(hw_sel_button_pressed()) {
        //     ili9163c_fill_rect(32, 32, 64, 64, GREEN);
        // }

#ifdef CONFIG_WDOG
        wdog_kick();
#endif // CONFIG_WDOG
    }
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    hw_init();
    event_init();
    tft_init();

    hw_enable_backlight(50);

    //ili9163c_fill_rect(0, 0, 40, 50, WHITE);

    pwrctl_init();
    pwrctl_set_ilimit(2000);

    pwrctl_set_iout(500);
    pwrctl_set_vout(target_voltage);
    pwrctl_enable_vout(false);

    // dac_load_data_buffer_single(DAC1, 880, DAC_ALIGN_RIGHT12, DAC_CHANNEL1);
    control_state = control_state_startup;
    set_state_timeout(STARTUP_TIMEOUT_MS);

    event_handler();
}