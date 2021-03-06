#include <freertos/FreeRTOS.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <string.h>

#include "odroid_system.h"
#include "odroid_input.h"

#define BATT_VOLTAGE_FULL        (4.2f)
#define BATT_VOLTAGE_EMPTY       (3.5f)
#define BATT_DIVIDER_R1          (10000)
#define BATT_DIVIDER_R2          (10000)

static volatile bool input_task_is_running = false;
static volatile uint last_gamepad_read = 0;
static odroid_gamepad_state gamepad_state;
static SemaphoreHandle_t xSemaphore;

odroid_gamepad_state odroid_input_gamepad_read_raw()
{
    odroid_gamepad_state state = {0};
    memset(&state, 0, sizeof(state));

    int joyX = adc1_get_raw(ODROID_PIN_GAMEPAD_X);
    int joyY = adc1_get_raw(ODROID_PIN_GAMEPAD_Y);

    if (joyX > 2048 + 1024)
        state.values[ODROID_INPUT_LEFT] = 1;
    else if (joyX > 1024)
        state.values[ODROID_INPUT_RIGHT] = 1;

    if (joyY > 2048 + 1024)
        state.values[ODROID_INPUT_UP] = 1;
    else if (joyY > 1024)
        state.values[ODROID_INPUT_DOWN] = 1;

    state.values[ODROID_INPUT_SELECT] = !(gpio_get_level(ODROID_PIN_GAMEPAD_SELECT));
    state.values[ODROID_INPUT_START] = !(gpio_get_level(ODROID_PIN_GAMEPAD_START));

    state.values[ODROID_INPUT_A] = !(gpio_get_level(ODROID_PIN_GAMEPAD_A));
    state.values[ODROID_INPUT_B] = !(gpio_get_level(ODROID_PIN_GAMEPAD_B));

    state.values[ODROID_INPUT_MENU] = !(gpio_get_level(ODROID_PIN_GAMEPAD_MENU));
    state.values[ODROID_INPUT_VOLUME] = !(gpio_get_level(ODROID_PIN_GAMEPAD_VOLUME));

    return state;
}

static void input_task(void *arg)
{
    input_task_is_running = true;

    uint8_t debounce[ODROID_INPUT_MAX];

    // Initialize debounce state
    memset(debounce, 0xFF, ODROID_INPUT_MAX);

    while (input_task_is_running)
    {
        // Read hardware
        odroid_gamepad_state state = odroid_input_gamepad_read_raw();

        for(int i = 0; i < ODROID_INPUT_MAX; ++i)
		{
            // Shift current values
			debounce[i] <<= 1;
		}

        // Debounce
        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        gamepad_state.bitmask = 0;

        for(int i = 0; i < ODROID_INPUT_MAX; ++i)
		{
            debounce[i] |= state.values[i] ? 1 : 0;
            uint8_t val = debounce[i] & 0x03; //0x0f;
            switch (val) {
                case 0x00:
                    gamepad_state.values[i] = 0;
                    break;

                case 0x03: //0x0f:
                    gamepad_state.values[i] = 1;
                    gamepad_state.bitmask |= 1 << i;
                    break;

                default:
                    // ignore
                    break;
            }
		}

        xSemaphoreGive(xSemaphore);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vSemaphoreDelete(xSemaphore);
    vTaskDelete(NULL);
}

void odroid_input_gamepad_init()
{
    assert(input_task_is_running == false);

    xSemaphore = xSemaphoreCreateMutex();

	gpio_set_direction(ODROID_PIN_GAMEPAD_SELECT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_PIN_GAMEPAD_SELECT, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_PIN_GAMEPAD_START, GPIO_MODE_INPUT);

	gpio_set_direction(ODROID_PIN_GAMEPAD_A, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_PIN_GAMEPAD_A, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_PIN_GAMEPAD_B, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_PIN_GAMEPAD_B, GPIO_PULLUP_ONLY);

	adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ODROID_PIN_GAMEPAD_X, ADC_ATTEN_11db);
	adc1_config_channel_atten(ODROID_PIN_GAMEPAD_Y, ADC_ATTEN_11db);

	gpio_set_direction(ODROID_PIN_GAMEPAD_MENU, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_PIN_GAMEPAD_MENU, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_PIN_GAMEPAD_VOLUME, GPIO_MODE_INPUT);

    // Start background polling
    xTaskCreatePinnedToCore(&input_task, "input_task", 2048, NULL, 5, NULL, 1);

  	printf("odroid_input_gamepad_init done.\n");
}

void odroid_input_gamepad_terminate()
{
    input_task_is_running = false;
}

long odroid_input_gamepad_last_polled()
{
    if (!last_gamepad_read)
        return 0;

    return get_elapsed_time_since(last_gamepad_read);
}

void odroid_input_gamepad_read(odroid_gamepad_state* out_state)
{
    assert(input_task_is_running == true);

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    *out_state = gamepad_state;
    xSemaphoreGive(xSemaphore);

    last_gamepad_read = get_elapsed_time();
}

bool odroid_input_key_is_pressed(int key)
{
    odroid_gamepad_state joystick;
    odroid_input_gamepad_read(&joystick);

    if (key == ODROID_INPUT_ANY) {
        for (int i = 0; i < ODROID_INPUT_MAX; i++) {
            if (joystick.values[i] == true) {
                return true;
            }
        }
        return false;
    }

    return joystick.values[key];
}

void odroid_input_wait_for_key(int key, bool pressed)
{
	while (odroid_input_key_is_pressed(key) != pressed)
    {
        vTaskDelay(1);
    }
}

odroid_battery_state odroid_input_battery_read()
{
    static esp_adc_cal_characteristics_t adc_chars;
    static float adcValue = 0.0f;

    short sampleCount = 4;
    float adcSample = 0.0f;

    // ADC not initialized
    if (adc_chars.vref == 0)
    {
        adc1_config_width(ADC_WIDTH_12Bit);
        adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    }

    for (int i = 0; i < sampleCount; ++i)
    {
        adcSample += esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc_chars) * 0.001f;
    }
    adcSample /= sampleCount;

    if (adcValue == 0.0f)
    {
        adcValue = adcSample;
    }
    else
    {
        adcValue += adcSample;
        adcValue /= 2.0f;
    }

    const float Vs = (adcValue / BATT_DIVIDER_R2 * (BATT_DIVIDER_R1 + BATT_DIVIDER_R2));
    const float Vconst = MAX(BATT_VOLTAGE_EMPTY, MIN(Vs, BATT_VOLTAGE_FULL));

    odroid_battery_state out_state = {
        .millivolts = (int)(Vs * 1000),
        .percentage = (int)((Vconst - BATT_VOLTAGE_EMPTY) / (BATT_VOLTAGE_FULL - BATT_VOLTAGE_EMPTY) * 100.0f),
    };

    return out_state;
}
