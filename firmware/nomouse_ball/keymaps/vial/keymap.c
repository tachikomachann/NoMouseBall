#include QMK_KEYBOARD_H
#include "eeconfig.h"

// Modify these values to adjust the scrolling speed
#define SCROLL_DIVISOR_H 50.0
#define SCROLL_DIVISOR_V 50.0
// #define POINTING_DEVICE_INVERT_H
#define POINTING_DEVICE_INVERT_V

enum custom_keycodes {
    DRAG_SCROLL = QK_KB_0,
    DRAG_SCROLL_H,
    DRAG_SCROLL_V, 
    DPI_CYCLE,
    ALT_TAB,
};

static const uint16_t dpi_table[] = {800, 1600, 2400, 3200};
#define DPI_LEVEL_COUNT (sizeof(dpi_table) / sizeof(dpi_table[0]))
#define EEPROM_ADDR_DPI 10  // avoid conflict with QMK internal configs

static uint8_t dpi_index = 0;
static bool dpi_ready = false;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(MS_BTN1, MS_BTN2 ,MS_BTN3, MS_BTN4)
};

// const uint16_t PROGMEM combo_key12[] = {DRAG_SCROLL, ALT_TAB, COMBO_END};
// const uint16_t PROGMEM combo_key24[] = {MS_BTN2, MS_BTN4, COMBO_END};
// const uint16_t PROGMEM combo_key13[] = {MS_BTN1, MS_BTN3, COMBO_END};
// const uint16_t PROGMEM combo_key124[] = {MS_BTN1, MS_BTN2, MS_BTN4, COMBO_END};
// const uint16_t PROGMEM combo_key1234[] = {MS_BTN1,MS_BTN2,MS_BTN3, MS_BTN4, COMBO_END};
// __attribute__((weak)) combo_t key_combos[] = {
//     COMBO(combo_key12, QK_BOOT),
//     COMBO(combo_key24, DRAG_SCROLL_V),
//     COMBO(combo_key13, DRAG_SCROLL_H),
//     COMBO(combo_key124, DPI_CYCLE),
//     COMBO(combo_key1234, QK_BOOT),
// };

bool set_scrolling = false;
bool set_scrolling_h_only = false;
bool set_scrolling_v_only = false;
// Variables to store accumulated scroll values
float scroll_accumulated_h = 0;
float scroll_accumulated_v = 0;
static uint16_t alt_tab_timer;

void pointing_device_init_user(void) {
    // Ensure EEPROM initialized
    if (!eeconfig_is_enabled()) {
        eeconfig_init();
    }

    uint8_t saved = eeprom_read_byte((void*)EEPROM_ADDR_DPI);
    if (saved >= DPI_LEVEL_COUNT) {
        saved = 0;
    }

    dpi_index = saved;
    pointing_device_set_cpi(dpi_table[dpi_index]);
    dpi_ready = true;
    uprintf("DPI init: %u\n", dpi_table[dpi_index]);
}
static void apply_dpi(void) {
    if (!dpi_ready) return;
    pointing_device_set_cpi(dpi_table[dpi_index]);
    eeprom_update_byte((void*)EEPROM_ADDR_DPI, dpi_index);
    uprintf("DPI set to %u\n", dpi_table[dpi_index]);
}


// Function to handle mouse reports and perform drag scrolling
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // Check if drag scrolling is active
    if (set_scrolling || set_scrolling_h_only || set_scrolling_v_only) {
        if(set_scrolling || set_scrolling_h_only){
            // Calculate and accumulate scroll values based on mouse movement and divisors
            scroll_accumulated_h += (float)mouse_report.x / SCROLL_DIVISOR_H;
            // Assign integer parts of accumulated scroll values to the mouse report
            mouse_report.h = (int8_t)scroll_accumulated_h;
            // Update accumulated scroll values by subtracting the integer parts
            scroll_accumulated_h -= (int8_t)scroll_accumulated_h;
        }


        if(set_scrolling ||set_scrolling_v_only){
            scroll_accumulated_v += (float)mouse_report.y / SCROLL_DIVISOR_V;
            mouse_report.v = (int8_t)scroll_accumulated_v;
            scroll_accumulated_v -= (int8_t)scroll_accumulated_v;
        }

        // Clear the X and Y values of the mouse report
        mouse_report.x = 0;
        mouse_report.y = 0;
        #if defined(POINTING_DEVICE_INVERT_H)
            mouse_report.h = -mouse_report.h;
        #endif
        #if defined(POINTING_DEVICE_INVERT_V)
            mouse_report.v = -mouse_report.v;
        #endif
    }
    return mouse_report;
}

// Function to handle key events and enable/disable drag scrolling
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case DRAG_SCROLL:
            // Toggle set_scrolling when DRAG_SCROLL key is pressed or released
            set_scrolling = record->event.pressed;
            break;
        case DRAG_SCROLL_H:
            // Toggle set_scrolling when DRAG_SCROLL key is pressed or released
            set_scrolling_h_only = record->event.pressed;
            break;
        case DRAG_SCROLL_V:
            // Toggle set_scrolling when DRAG_SCROLL key is pressed or released
            set_scrolling_v_only = record->event.pressed;
            break;
        case DPI_CYCLE:
            if(record->event.pressed){
                dpi_index++;
                if (dpi_index >= DPI_LEVEL_COUNT) dpi_index = 0;
                apply_dpi();
                return false;
            }
            break;
        case ALT_TAB:
            if (record->event.pressed) {
                alt_tab_timer = timer_read();
                register_code(KC_LALT);
                tap_code(KC_TAB);
            } else {
                if (timer_elapsed(alt_tab_timer) < 300) {
                    unregister_code(KC_LALT);
                } else {
                    wait_ms(50);  
                    tap_code(KC_MS_BTN1);  
                    wait_ms(30);  
                    unregister_code(KC_LALT);  
                }
            }
            break;
        default:
            break;
    }
    return true;
}

// // Function to handle layer changes and disable drag scrolling when not in AUTO_MOUSE_DEFAULT_LAYER
// layer_state_t layer_state_set_user(layer_state_t state) {
//     // Disable set_scrolling if the current layer is not the AUTO_MOUSE_DEFAULT_LAYER
//     if (get_highest_layer(state) != AUTO_MOUSE_DEFAULT_LAYER) {
//         set_scrolling = false;
//     }
//     return state;
// }