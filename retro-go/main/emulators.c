#include <esp_partition.h>
#include <esp_system.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <odroid_system.h>

#include "bitmaps/all.h"
#include "emulators.h"
#include "gui.h"

retro_emulators_t emulators_stack;
retro_emulators_t *emulators = &emulators_stack;

static void add_emulator(const char *system, const char *dirname, const char* ext, const char *part,
                          uint16_t crc_offset, const void *logo, const void *header)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, part);

    if (partition == NULL)
    {
        return;
    }

    retro_emulator_t *p = &emulators->entries[emulators->count++];
    strcpy(p->system_name, system);
    strcpy(p->dirname, dirname);
    strcpy(p->ext, ext);
    p->partition = partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN;
    p->roms.selected = 0;
    p->roms.count = 0;
    p->roms.files = NULL;
    p->image_logo = (uint16_t *)logo;
    p->image_header = (uint16_t *)header;
    p->initialized = false;
    p->crc_offset = crc_offset;
}

static int file_sort_comparator(const void *p, const void *q)
{
    retro_emulator_file_t *l = (retro_emulator_file_t*)p;
    retro_emulator_file_t *r = (retro_emulator_file_t*)q;
    return strcasecmp(l->name, r->name);
}

retro_emulator_t *emu_get_selected(void)
{
    return &emulators->entries[emulators->selected];
}

retro_emulator_file_t *emu_get_selected_file(retro_emulator_t *emu)
{
    if (emu->roms.count == 0 || emu->roms.selected > emu->roms.count) {
        return NULL;
    }
    return &emu->roms.files[emu->roms.selected];
}

void emulators_init_emu(retro_emulator_t *emu)
{
    if (emu->initialized)
    {
        return;
    }

    printf("emulators_init_emu: Initializing %s\n", emu->system_name);

    emu->initialized = true;

    char path[128];
    char keyBuffer[24];

    sprintf(keyBuffer, "Sel.%s", emu->dirname);
    emu->roms.selected = odroid_settings_int32_get(keyBuffer, 0);

    odroid_system_spi_lock_acquire(SPI_LOCK_SDCARD);

    sprintf(path, ODROID_BASE_PATH_CRC_CACHE "/%s", emu->dirname);
    odroid_sdcard_mkdir(path);

    sprintf(path, ODROID_BASE_PATH_SAVES "/%s", emu->dirname);
    odroid_sdcard_mkdir(path);

    sprintf(path, ODROID_BASE_PATH_ROMS "/%s", emu->dirname);
    odroid_sdcard_mkdir(path);

    DIR* dir = opendir(path);
    if (dir)
    {
        char *selected_file = odroid_settings_RomFilePath_get();
        struct dirent* in_file;

        while ((in_file = readdir(dir)))
        {
            const char *ext = odroid_sdcard_get_extension(in_file->d_name);

            if (!ext) continue;

            if (strcasecmp(emu->ext, ext) != 0 && strcasecmp("zip", ext) != 0)
                continue;

            if (emu->roms.count % 100 == 0) {
                emu->roms.files = (retro_emulator_file_t *)realloc(emu->roms.files,
                    (emu->roms.count + 100) * sizeof(retro_emulator_file_t));
            }
            retro_emulator_file_t *file = &emu->roms.files[emu->roms.count++];
            sprintf(file->path, "%s/%s", path, in_file->d_name);
            strcpy(file->name, in_file->d_name);
            strcpy(file->ext, ext);
            file->name[strlen(file->name)-strlen(ext)-1] = 0;
            file->checksum = 0;
        }

        if (emu->roms.count > 0)
        {
            qsort((void*)emu->roms.files, emu->roms.count, sizeof(retro_emulator_file_t), file_sort_comparator);

            // for (int i = 0; i < emu->roms.count; ++i) {
            //     if (selected_file && strcmp(file->path, selected_file) == 0) {
            //         emu->roms.selected = emu->roms.count - 1;
            //         break;
            //     }
            // }

            if (emu->roms.selected > emu->roms.count - 1)
            {
                emu->roms.selected = emu->roms.count - 1;
            }
        }

        free(selected_file);
        closedir(dir);
    }

    odroid_system_spi_lock_release(SPI_LOCK_SDCARD);
}

void emulators_init()
{
    add_emulator("Nintendo Entertainment System", "nes", "nes", "nofrendo", 16, logo_nes, header_nes);
    add_emulator("Nintendo Gameboy", "gb", "gb", "gnuboy", 0, logo_gb, header_gb);
    add_emulator("Nintendo Gameboy Color", "gbc", "gbc", "gnuboy", 0, logo_gbc, header_gbc);
    add_emulator("Sega Master System", "sms", "sms", "smsplusgx", 0, logo_sms, header_sms);
    add_emulator("Sega Game Gear", "gg", "gg", "smsplusgx", 0, logo_gg, header_gg);
    add_emulator("ColecoVision", "col", "col", "smsplusgx", 0, logo_col, header_col);
    add_emulator("PC Engine", "pce", "pce", "huexpress", 0, logo_pce, header_pce);
    add_emulator("Atari Lynx", "lnx", "lnx", "handy", 0, logo_lnx, header_lnx);
    // add_emulator("Atari 2600", "a26", "a26", "stella", 0, logo_a26, header_a26);

    emulators->selected = odroid_settings_int32_get("SelectedEmu", 0);
}

void emulators_start_emu(retro_emulator_t *emu)
{
    retro_emulator_file_t *file = emu_get_selected_file(emu);
    char keyBuffer[16];

    printf("Starting game: %s\n", file->path);

    sprintf(keyBuffer, "Sel.%s", emu->dirname);
    odroid_settings_int32_set(keyBuffer, emu->roms.selected);
    odroid_settings_int32_set("SelectedEmu", emulators->selected);

    odroid_settings_RomFilePath_set(file->path);
    odroid_system_switch_app(emu->partition);
}
