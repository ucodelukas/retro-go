#include <nvs_flash.h>
#include <string.h>

#include "odroid_system.h"
#include "odroid_settings.h"

static const char* NvsNamespace = "Odroid";
// Global
static const char* NvsKey_RomFilePath  = "RomFilePath";
static const char* NvsKey_StartAction  = "StartAction";
static const char* NvsKey_Backlight    = "Backlight";
static const char* NvsKey_AudioSink    = "AudioSink";
static const char* NvsKey_Volume       = "Volume";
static const char* NvsKey_StartupApp   = "StartupApp";
static const char* NvsKey_FontSize     = "FontSize";
// Per-app
static const char* NvsKey_Region       = "Region";
static const char* NvsKey_Palette      = "Palette";
static const char* NvsKey_DispScaling  = "DispScale";
static const char* NvsKey_DispFilter   = "DispFilter";
static const char* NvsKey_DispRotation = "DispRotate";
static const char* NvsKey_DispOverscan = "Overscan";
static const char* NvsKey_SpriteLimit  = "SpriteL";

static nvs_handle my_handle;

void odroid_settings_init()
{
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        nvs_flash_erase();
        if (nvs_flash_init() != ESP_OK) {
            printf("odroid_system_init: Failed to init NVS");
            abort();
        }
    }

	err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
	assert(err == ESP_OK);
}

char* odroid_settings_string_get(const char *key, char *default_value)
{
    char* result = default_value;

    size_t required_size;
    esp_err_t err = nvs_get_str(my_handle, key, NULL, &required_size);
    if (err == ESP_OK)
    {
        char* value = rg_alloc(required_size, MEM_ANY);

        esp_err_t err = nvs_get_str(my_handle, key, value, &required_size);
        if (err == ESP_OK)
        {
            result = value;
        }
    }

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("%s: key='%s' err=%d\n", __func__, key, err);
    }

    return result;
}

void odroid_settings_string_set(const char *key, char *value)
{
    // To do: Check if value is same and avoid overwrite
    esp_err_t ret = nvs_set_str(my_handle, key, value);
    nvs_commit(my_handle);

    if (ret != ESP_OK)
    {
        printf("%s: key='%s' err=%d\n", __func__, key, ret);
    }
}

int32_t odroid_settings_int32_get(const char *key, int32_t default_value)
{
    int current = default_value;
    esp_err_t ret = ESP_OK;

    ret = nvs_get_i32(my_handle, key, &current);

    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("%s: key='%s' err=%d\n", __func__, key, ret);
    }

    return current;
}

void odroid_settings_int32_set(const char *key, int32_t value)
{
    esp_err_t ret = ESP_OK;

    if (odroid_settings_int32_get(key, 0) != value)
    {
        ret = nvs_set_i32(my_handle, key, value);
        nvs_commit(my_handle);
    }

    if (ret != ESP_OK)
    {
        printf("%s: key='%s' err=%d\n", __func__, key, ret);
    }
}


int32_t odroid_settings_app_int32_get(const char *key, int32_t default_value)
{
    char app_key[16];
    sprintf(app_key, "%.12s.%d", key, odroid_system_get_app_id());
    return odroid_settings_int32_get(app_key, default_value);
}

void odroid_settings_app_int32_set(const char *key, int32_t value)
{
    char app_key[16];
    sprintf(app_key, "%.12s.%d", key, odroid_system_get_app_id());
    odroid_settings_int32_set(app_key, value);
}


int32_t odroid_settings_FontSize_get()
{
    return odroid_settings_int32_get(NvsKey_FontSize, 1);
}
void odroid_settings_FontSize_set(int32_t value)
{
    odroid_settings_int32_set(NvsKey_FontSize, value);
}


char* odroid_settings_RomFilePath_get()
{
    return odroid_settings_string_get(NvsKey_RomFilePath, NULL);
}
void odroid_settings_RomFilePath_set(char* value)
{
    odroid_settings_string_set(NvsKey_RomFilePath, value);
}


int32_t odroid_settings_Volume_get()
{
    return odroid_settings_int32_get(NvsKey_Volume, ODROID_AUDIO_VOLUME_DEFAULT);
}
void odroid_settings_Volume_set(int32_t value)
{
    odroid_settings_int32_set(NvsKey_Volume, value);
}


int32_t odroid_settings_AudioSink_get()
{
    return odroid_settings_int32_get(NvsKey_AudioSink, ODROID_AUDIO_SINK_SPEAKER);
}
void odroid_settings_AudioSink_set(int32_t value)
{
    odroid_settings_int32_set(NvsKey_AudioSink, value);
}


int32_t odroid_settings_Backlight_get()
{
    return odroid_settings_int32_get(NvsKey_Backlight, 2);
}
void odroid_settings_Backlight_set(int32_t value)
{
    odroid_settings_int32_set(NvsKey_Backlight, value);
}


ODROID_START_ACTION odroid_settings_StartAction_get()
{
    return odroid_settings_int32_get(NvsKey_StartAction, 0);
}
void odroid_settings_StartAction_set(ODROID_START_ACTION value)
{
    odroid_settings_int32_set(NvsKey_StartAction, value);
}


int32_t odroid_settings_StartupApp_get()
{
    return odroid_settings_int32_get(NvsKey_StartupApp, 1);
}
void odroid_settings_StartupApp_set(int32_t value)
{
    odroid_settings_int32_set(NvsKey_StartupApp, value);
}


int32_t odroid_settings_Palette_get()
{
    return odroid_settings_app_int32_get(NvsKey_Palette, 0);
}
void odroid_settings_Palette_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_Palette, value);
}


int32_t odroid_settings_SpriteLimit_get()
{
    return odroid_settings_app_int32_get(NvsKey_SpriteLimit, 1);
}
void odroid_settings_SpriteLimit_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_SpriteLimit, value);
}


ODROID_REGION odroid_settings_Region_get()
{
    return odroid_settings_app_int32_get(NvsKey_Region, ODROID_REGION_AUTO);
}
void odroid_settings_Region_set(ODROID_REGION value)
{
    odroid_settings_app_int32_set(NvsKey_Region, value);
}


int32_t odroid_settings_DisplayScaling_get()
{
    return odroid_settings_app_int32_get(NvsKey_DispScaling, ODROID_DISPLAY_SCALING_FILL);
}
void odroid_settings_DisplayScaling_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_DispScaling, value);
}


int32_t odroid_settings_DisplayFilter_get()
{
    return odroid_settings_app_int32_get(NvsKey_DispFilter, ODROID_DISPLAY_FILTER_OFF);
}
void odroid_settings_DisplayFilter_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_DispFilter, value);
}


int32_t odroid_settings_DisplayRotation_get()
{
    return odroid_settings_app_int32_get(NvsKey_DispRotation, ODROID_DISPLAY_ROTATION_OFF);
}
void odroid_settings_DisplayRotation_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_DispRotation, value);
}


int32_t odroid_settings_DisplayOverscan_get()
{
    return odroid_settings_app_int32_get(NvsKey_DispOverscan, 1);
}
void odroid_settings_DisplayOverscan_set(int32_t value)
{
    odroid_settings_app_int32_set(NvsKey_DispOverscan, value);
}
