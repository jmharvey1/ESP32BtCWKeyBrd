/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "hal/dac_types.h"
//20231216 JMH Added typedef struct
/**
 * @brief The multiple of the amplitude of the cosine wave generator. The max amplitude is VDD3P3_RTC.
 */
typedef enum {
    DAC_CW_SCALE_1 = 0x0,   /*!< 1/1. Default. */
    DAC_CW_SCALE_2 = 0x1,   /*!< 1/2. */
    DAC_CW_SCALE_4 = 0x2,   /*!< 1/4. */
    DAC_CW_SCALE_8 = 0x3,   /*!< 1/8. */
} dac_cw_scale_t;
//20231216 JMH Added typedef struct
/**
 * @brief Set the phase of the cosine wave generator output.
 */
typedef enum {
    DAC_CW_PHASE_0   = 0x2, /*!< Phase shift +0° */
    DAC_CW_PHASE_180 = 0x3, /*!< Phase shift +180° */
} dac_cw_phase_t;

//20231216 JMH Added typedef struct
/**
 * @brief Config the cosine wave generator function in DAC module.
 */
typedef struct {
    dac_channel_t en_ch;    /*!< Enable the cosine wave generator of DAC channel. */
    dac_cw_scale_t scale;   /*!< Set the amplitude of the cosine wave generator output. */
    dac_cw_phase_t phase;   /*!< Set the phase of the cosine wave generator output. */
    uint32_t freq;          /*!< Set frequency of cosine wave generator output. Range: 130(130Hz) ~ 55000(100KHz). */
    int8_t offset;          /*!< Set the voltage value of the DC component of the cosine wave generator output.
                                 Note: Unreasonable settings can cause waveform to be oversaturated. Range: -128 ~ 127. */
} dac_cw_config_t;

/**
 * @brief Get the GPIO number of a specific DAC channel.
 *
 * @param channel Channel to get the gpio number
 * @param gpio_num output buffer to hold the gpio number
 * @return
 *   - ESP_OK if success
 */
esp_err_t dac_pad_get_io_num(dac_channel_t channel, gpio_num_t *gpio_num);

/**
 * @brief Set DAC output voltage.
 *        DAC output is 8-bit. Maximum (255) corresponds to VDD3P3_RTC.
 *
 * @note Need to configure DAC pad before calling this function.
 *       DAC channel 1 is attached to GPIO25, DAC channel 2 is attached to GPIO26
 * @param channel DAC channel
 * @param dac_value DAC output value
 *
 * @return
 *     - ESP_OK success
 */
esp_err_t dac_output_voltage(dac_channel_t channel, uint8_t dac_value);

/**
 * @brief DAC pad output enable
 *
 * @param channel DAC channel
 * @note DAC channel 1 is attached to GPIO25, DAC channel 2 is attached to GPIO26
 *       I2S left channel will be mapped to DAC channel 2
 *       I2S right channel will be mapped to DAC channel 1
 */
esp_err_t dac_output_enable(dac_channel_t channel);

/**
 * @brief DAC pad output disable
 *
 * @param channel DAC channel
 * @note DAC channel 1 is attached to GPIO25, DAC channel 2 is attached to GPIO26
 * @return
 *     - ESP_OK success
 */
esp_err_t dac_output_disable(dac_channel_t channel);

/**
 * @brief Enable cosine wave generator output.
 *
 * @return
 *     - ESP_OK success
 */
esp_err_t dac_cw_generator_enable(void);

/**
 * @brief Disable cosine wave generator output.
 *
 * @return
 *     - ESP_OK success
 */
esp_err_t dac_cw_generator_disable(void);

/**
 * @brief Config the cosine wave generator function in DAC module.
 *
 * @param cw Configuration.
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG The parameter is NULL.
 */
esp_err_t dac_cw_generator_config(dac_cw_config_t *cw);

#ifdef __cplusplus
}
#endif
