/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include "esp_etm.h"
//#include "driver/gptimer_types.h"
#include "gptimer_types.h"
#include "timer_types.h"
#include "soc_etm_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/*JMH added the following taken from from timer_ll.h*/
// Get timer group register base address with giving group number
#define TIMER_LL_GET_HW(group_id) ((group_id == 0) ? (&TIMERG0) : (&TIMERG1))
#define TIMER_LL_EVENT_ALARM(timer_id) (1 << (timer_id))

#define TIMER_LL_ETM_TASK_TABLE(group, timer, task)                                        \
    (uint32_t [2][1][GPTIMER_ETM_TASK_MAX]){{{                                             \
                            [GPTIMER_ETM_TASK_START_COUNT] = TIMER0_TASK_CNT_START_TIMER0, \
                            [GPTIMER_ETM_TASK_STOP_COUNT] = TIMER0_TASK_CNT_STOP_TIMER0,   \
                            [GPTIMER_ETM_TASK_EN_ALARM] = TIMER0_TASK_ALARM_START_TIMER0,  \
                            [GPTIMER_ETM_TASK_RELOAD] = TIMER0_TASK_CNT_RELOAD_TIMER0,     \
                            [GPTIMER_ETM_TASK_CAPTURE] = TIMER0_TASK_CNT_CAP_TIMER0,       \
                        }},                                                                \
                        {{                                                                 \
                            [GPTIMER_ETM_TASK_START_COUNT] = TIMER1_TASK_CNT_START_TIMER0, \
                            [GPTIMER_ETM_TASK_STOP_COUNT] = TIMER1_TASK_CNT_STOP_TIMER0,   \
                            [GPTIMER_ETM_TASK_EN_ALARM] = TIMER1_TASK_ALARM_START_TIMER0,  \
                            [GPTIMER_ETM_TASK_RELOAD] = TIMER1_TASK_CNT_RELOAD_TIMER0,     \
                            [GPTIMER_ETM_TASK_CAPTURE] = TIMER1_TASK_CNT_CAP_TIMER0,       \
                        }},                                                                \
    }[group][timer][task]

#define TIMER_LL_ETM_EVENT_TABLE(group, timer, event)                                      \
    (uint32_t [2][1][GPTIMER_ETM_EVENT_MAX]){{{                                            \
                            [GPTIMER_ETM_EVENT_ALARM_MATCH] = TIMER0_EVT_CNT_CMP_TIMER0,   \
                        }},                                                                \
                        {{                                                                 \
                            [GPTIMER_ETM_EVENT_ALARM_MATCH] = TIMER1_EVT_CNT_CMP_TIMER0,   \
                        }},                                                                \
    }[group][timer][event]





/**
 * @brief GPTimer ETM event configuration
 */
typedef struct {
    gptimer_etm_event_type_t event_type; /*!< GPTimer ETM event type */
} gptimer_etm_event_config_t;

/**
 * @brief Get the ETM event for GPTimer
 *
 * @note The created ETM event object can be deleted later by calling `esp_etm_del_event`
 *
 * @param[in] timer Timer handle created by `gptimer_new_timer`
 * @param[in] config GPTimer ETM event configuration
 * @param[out] out_event Returned ETM event handle
 * @return
 *      - ESP_OK: Get ETM event successfully
 *      - ESP_ERR_INVALID_ARG: Get ETM event failed because of invalid argument
 *      - ESP_FAIL: Get ETM event failed because of other error
 */
esp_err_t gptimer_new_etm_event(gptimer_handle_t timer, const gptimer_etm_event_config_t *config, esp_etm_event_handle_t *out_event);

/**
 * @brief GPTimer ETM task configuration
 */
typedef struct {
    gptimer_etm_task_type_t task_type; /*!< GPTimer ETM task type */
} gptimer_etm_task_config_t;

/**
 * @brief Get the ETM task for GPTimer
 *
 * @note The created ETM task object can be deleted later by calling `esp_etm_del_task`
 *
 * @param[in] timer Timer handle created by `gptimer_new_timer`
 * @param[in] config GPTimer ETM task configuration
 * @param[out] out_task Returned ETM task handle
 * @return
 *      - ESP_OK: Get ETM task successfully
 *      - ESP_ERR_INVALID_ARG: Get ETM task failed because of invalid argument
 *      - ESP_FAIL: Get ETM task failed because of other error
 */
esp_err_t gptimer_new_etm_task(gptimer_handle_t timer, const gptimer_etm_task_config_t *config, esp_etm_task_handle_t *out_task);

#ifdef __cplusplus
}
#endif
