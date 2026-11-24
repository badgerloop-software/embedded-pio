#include "dac.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_submodule, LOG_LEVEL_INF);

static void _Error_Handler(const char * file, int line) {
    LOG_ERR("DAC HAL call failed at %s:%d", file, line);
    k_panic();
}

DAC_HandleTypeDef dacHandle;
DAC_ChannelConfTypeDef dacConfig;

void initDAC(DAC_TypeDef* instance, uint32_t* channels, uint8_t num_channels) {
    dacHandle.Instance = instance;
    if (HAL_DAC_Init(&dacHandle) != HAL_OK) {
        _Error_Handler(__FILE__, __LINE__);
    }
    dacConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    dacConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    for (int i = 0; i < num_channels; i++) {
        if (HAL_DAC_ConfigChannel(&dacHandle, &dacConfig, channels[i]) != HAL_OK) {
            LOG_ERR("channel %d not initialized\n", i);
        } else {
            LOG_INF("channel %d working\n", i);
        }
        HAL_DAC_Start(&dacHandle, channels[i]);
    }
}

void writeDAC(uint32_t channel, float value) {
    HAL_DAC_SetValue(&dacHandle, channel, DAC_ALIGN_12B_R, value * DAC_MAX_VALUE);
}
