#include "otap.h"
#include <stdint.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(OTAP_LOG, LOG_LEVEL_INF);

struct flash_img_context ctx;

void initSwapping()
{
    int err;

    uint8_t areaID = flash_img_get_upload_slot();

    err = flash_img_init_id(&ctx,
                            areaID);
    if (err) {
        LOG_ERR("flash_img_init_id failed: %d", err);
        return;
    }
 
    err = boot_erase_img_bank(areaID);
    if (err) {
        LOG_ERR("Failed to erase image bank: %d", err);
        return;
    }
}


int writeToBackup(uint8_t * newData, size_t len, bool flush){
    
    size_t bytesWritten = flash_img_bytes_written(&ctx);

    if (bytesWritten + len > ctx.flash_area->fa_size) {
        return -1;
    }

    return flash_img_buffered_write(&ctx, newData, len, flush);

}

// TODO: this should have a return value to indicate success or failure
void setWriteToBackupDone(){

   int err = boot_request_upgrade(BOOT_UPGRADE_TEST);

    if (err == -1) {
        LOG_ERR("Failed to request upgrade: %d", err);
    }

// rturrn err;
// TODO: make sure all memmory is freed

}
