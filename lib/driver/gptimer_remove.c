/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/lock.h>
#include "sdkconfig.h"

#if CONFIG_GPTIMER_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_pm.h"
//#include "driver/gptimer.h"
#include "gptimer.h"
#include "hal/timer_types.h"
#include "hal/timer_hal.h"
//#include "timer_halJMH.h"
#include "hal/timer_ll.h"
#include "soc/timer_periph.h"
#include "soc/soc_memory_types.h"
//#include "esp_memory_utils.h"
//#include "esp_private/periph_ctrl.h"
#include <periph_ctrl.h>
#include "esp_private/esp_clk.h"
#include "clk_ctrl_os.h"
#include "gptimer_priv.h"
#include "gptimer_types.h"

static const char *TAG = "gptimer";

typedef struct gptimer_platform_t {
    _lock_t mutex;                             // platform level mutex lock
    gptimer_group_t *groups[SOC_TIMER_GROUPS]; // timer group pool
    int group_ref_counts[SOC_TIMER_GROUPS];    // reference count used to protect group install/uninstall
} gptimer_platform_t;

// gptimer driver platform, it's always a singleton
static gptimer_platform_t s_platform;

static gptimer_group_t *gptimer_acquire_group_handle(int group_id);
static void gptimer_release_group_handle(gptimer_group_t *group);
static esp_err_t gptimer_select_periph_clock(gptimer_t *timer, gptimer_clock_source_t src_clk, uint32_t resolution_hz);
static void gptimer_default_isr(void *args);

/**
 * JMH Added this function; originally found @ ~ESP-IDF/components/hal/esp32/include/hal/timer_ll.h 
 * @brief Set clock source for timer
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param clk_src Clock source
 */
//static inline void timer_ll_set_clock_source(timg_dev_t *hw, uint32_t timer_num, gptimer_clock_source_t clk_src)//JMH removed for Windows 10
// {
//     switch (clk_src) {
//     case GPTIMER_CLK_SRC_APB:
//         break;
//     default:
//         HAL_ASSERT(false && "unsupported clock source");
//         break;
//     }
// }
/**
 * JMH Added this function; originally found @ ~ESP-IDF/components/hal/timer_hal.c
 *
 * @brief Deinit timer hal context.
 *
 * @param hal Context of HAL layer
 */

void timer_hal_deinit(timer_hal_context_t *hal)
{
    //timer_idx_t timer_num = (timer_idx_t)hal->dev->hw_timer;// JMH removed for Windows 10 version
    // disable peripheral clock
    //timer_ll_enable_clock(hal->dev, hal->timer_id, false);// JMH addeded for Windows 10 version
    // ensure counter, alarm, auto-reload are disabled
    timer_ll_enable_counter(hal->dev, hal->timer_id, false);// JMH addeded for Windows 10 version
    //timer_ll_set_counter_enable(timg_dev_t *hw, timer_idx_t timer_num, bool counter_en)
    //timer_ll_set_counter_enable(hal->dev, timer_num, false);// JMH removed for Windows 10 version
    timer_ll_enable_auto_reload(hal->dev, hal->timer_id, false);// JMH addeded for Windows 10 version
    //timer_ll_set_auto_reload(timg_dev_t *hw, timer_idx_t timer_num, bool auto_reload_en)
    //timer_ll_set_auto_reload(hal->dev, timer_num, false);// JMH removed for Windows 10 version
    timer_ll_enable_alarm(hal->dev, hal->timer_id, false);// JMH addeded for Windows 10 version
    //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
    //timer_ll_set_alarm_enable(hal->dev, timer_num, false);// JMH removed for Windows 10 version
#if SOC_TIMER_SUPPORT_ETM
    timer_ll_enable_etm(hal->dev, ftimer_ll_set_counter_enable(timg_dev_t *hw, timer_idx_t timer_num, bool counter_en)alse);
#endif
    hal->dev = NULL;
}




static esp_err_t gptimer_register_to_group(gptimer_t *timer)
{
    gptimer_group_t *group = NULL;
    int timer_id = -1;
    for (int i = 0; i < SOC_TIMER_GROUPS; i++) {
        group = gptimer_acquire_group_handle(i);
        ESP_RETURN_ON_FALSE(group, ESP_ERR_NO_MEM, TAG, "no mem for group (%d)", i);
        // loop to search free timer in the group
        portENTER_CRITICAL(&group->spinlock);
        for (int j = 0; j < SOC_TIMER_GROUP_TIMERS_PER_GROUP; j++) {
            if (!group->timers[j]) {
                timer_id = j;
                group->timers[j] = timer;
                break;
            }
        }
        portEXIT_CRITICAL(&group->spinlock);
        if (timer_id < 0) {
            gptimer_release_group_handle(group);
            group = NULL;
        } else {
            timer->timer_id = timer_id;
            timer->group = group;
            break;;
        }
    }
    ESP_RETURN_ON_FALSE(timer_id != -1, ESP_ERR_NOT_FOUND, TAG, "no free timer");
    return ESP_OK;
}

static void gptimer_unregister_from_group(gptimer_t *timer)
{
    gptimer_group_t *group = timer->group;
    int timer_id = timer->timer_id;
    portENTER_CRITICAL(&group->spinlock);
    group->timers[timer_id] = NULL;
    portEXIT_CRITICAL(&group->spinlock);
    // timer has a reference on group, release it now
    gptimer_release_group_handle(group);
}

static esp_err_t gptimer_destory(gptimer_t *timer)
{
    if (timer->pm_lock) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_delete(timer->pm_lock), TAG, "delete pm_lock failed");
    }
    if (timer->intr) {
        ESP_RETURN_ON_ERROR(esp_intr_free(timer->intr), TAG, "delete interrupt service failed");
    }
    if (timer->group) {
        gptimer_unregister_from_group(timer);
    }
    free(timer);
    return ESP_OK;
}

esp_err_t gptimer_new_timer(const gptimer_config_t *config, gptimer_handle_t *ret_timer)
{
#if CONFIG_GPTIMER_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    gptimer_t *timer = NULL;
    ESP_GOTO_ON_FALSE(config && ret_timer, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    //ESP_GOTO_ON_FALSE(config->resolution_hz, ESP_ERR_INVALID_ARG, err, TAG, "invalid timer resolution:%PRIu32", config->resolution_hz);
    ESP_GOTO_ON_FALSE(config->resolution_hz, ESP_ERR_INVALID_ARG, err, TAG, "invalid timer resolution:%d", config->resolution_hz);


    timer = heap_caps_calloc(1, sizeof(gptimer_t), GPTIMER_MEM_ALLOC_CAPS);
    ESP_GOTO_ON_FALSE(timer, ESP_ERR_NO_MEM, err, TAG, "no mem for gptimer");
    // register timer to the group (because one group can have several timers)
    ESP_GOTO_ON_ERROR(gptimer_register_to_group(timer), err, TAG, "register timer failed");
    gptimer_group_t *group = timer->group;
    int group_id = group->group_id;
    int timer_id = timer->timer_id;

    // initialize HAL layer
    timer_hal_init(&timer->hal, group_id, timer_id);
    // select clock source, set clock resolution
    ESP_GOTO_ON_ERROR(gptimer_select_periph_clock(timer, config->clk_src, config->resolution_hz), err, TAG, "set periph clock failed");
    // initialize counter value to zero
    timer_hal_set_counter_value(&timer->hal, 0);
    // set counting direction
    timer_ll_set_count_direction(timer->hal.dev, timer_id, config->direction);// JMH added for Windows 10 version
    //timer_ll_set_counter_increase(timg_dev_t *hw, timer_idx_t timer_num, bool increase_en)
    //timer_ll_set_counter_increase(timer->hal.dev, timer_id, config->direction);// JMH removed for Windows 10 version

    // interrupt register is shared by all timers in the same group
    portENTER_CRITICAL(&group->spinlock);
    timer_ll_enable_intr(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer_id), false); // disable interrupt// JMH added for Windows 10 version
    //timer_ll_intr_disable(timg_dev_t *hw, timer_idx_t timer_num)
    //timer_ll_intr_disable(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer_id));
    //timer_ll_intr_disable(timer->hal.dev, timer_id); // disable interrupt// JMH removed for Windows 10 version
    //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
    timer_ll_set_alarm_enable(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer_id), false);// JMH added for Windows 10 version
    //timer_ll_intr_disable(timg_dev_t *hw, timer_idx_t timer_num)
    timer_ll_intr_disable(timer->hal.dev, timer_id);
    timer_ll_clear_intr_status(timer->hal.dev, timer_id); // clear pending interrupt event
    portEXIT_CRITICAL(&group->spinlock);
    // initialize other members of timer
    timer->spinlock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    timer->fsm = GPTIMER_FSM_INIT; // put the timer into init state
    timer->direction = config->direction;
    timer->flags.intr_shared = config->flags.intr_shared;
    //ESP_LOGD(TAG, "new gptimer (%d,%d) at %p, resolution=%"PRIu32"Hz", group_id, timer_id, timer, timer->resolution_hz);
    ESP_LOGD(TAG, "new gptimer (%d,%d) at %p, resolution=%dHz", group_id, timer_id, timer, timer->resolution_hz);
    *ret_timer = timer;
    return ESP_OK;

err:
    if (timer) {
        gptimer_destory(timer);
    }
    return ret;
}

esp_err_t gptimer_del_timer(gptimer_handle_t timer)
{
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(timer->fsm == GPTIMER_FSM_INIT, ESP_ERR_INVALID_STATE, TAG, "timer not in init state");
    gptimer_group_t *group = timer->group;
    gptimer_clock_source_t clk_src = timer->clk_src;
    int group_id = group->group_id;
    int timer_id = timer->timer_id;
    ESP_LOGD(TAG, "del timer (%d,%d)", group_id, timer_id);
    timer_hal_deinit(&timer->hal);
    // recycle memory resource
    ESP_RETURN_ON_ERROR(gptimer_destory(timer), TAG, "destory gptimer failed");

    switch (clk_src) {
#if SOC_TIMER_GROUP_SUPPORT_RC_FAST
    case GPTIMER_CLK_SRC_RC_FAST:
        periph_rtc_dig_clk8m_disable();
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_RC_FAST
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t gptimer_set_raw_count(gptimer_handle_t timer, unsigned long long value)
{
    ESP_RETURN_ON_FALSE_ISR(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    portENTER_CRITICAL_SAFE(&timer->spinlock);
    timer_hal_set_counter_value(&timer->hal, value);
    portEXIT_CRITICAL_SAFE(&timer->spinlock);
    return ESP_OK;
}

esp_err_t gptimer_get_raw_count(gptimer_handle_t timer, unsigned long long *value)
{
    ESP_RETURN_ON_FALSE_ISR(timer && value, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    portENTER_CRITICAL_SAFE(&timer->spinlock);
    //*value = timer_hal_capture_and_get_counter_value(&timer->hal);
    //timer_ll_get_counter_value(timg_dev_t *hw, timer_idx_t timer_num, uint64_t *alarm_value)
    timer_ll_get_counter_value(&timer->hal, (timer_idx_t)timer->timer_id, value);
    portEXIT_CRITICAL_SAFE(&timer->spinlock);
    return ESP_OK;
}

esp_err_t gptimer_get_resolution(gptimer_handle_t timer, uint32_t *out_resolution)
{
    ESP_RETURN_ON_FALSE(timer && out_resolution, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    *out_resolution = timer->resolution_hz;
    return ESP_OK;
}

esp_err_t gptimer_get_captured_count(gptimer_handle_t timer, uint64_t *value)
{
    ESP_RETURN_ON_FALSE_ISR(timer && value, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    portENTER_CRITICAL_SAFE(&timer->spinlock);
    //*value = timer_ll_get_counter_value(timer->hal.dev, timer->timer_id);
    timer_ll_get_counter_value(timer->hal.dev, timer->timer_id, value);
    portEXIT_CRITICAL_SAFE(&timer->spinlock);
    return ESP_OK;
}

esp_err_t gptimer_register_event_callbacks(gptimer_handle_t timer, const gptimer_event_callbacks_t *cbs, void *user_data)
{
    gptimer_group_t *group = NULL;
    ESP_RETURN_ON_FALSE(timer && cbs, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    group = timer->group;
    int group_id = group->group_id;
    int timer_id = timer->timer_id;

#if CONFIG_GPTIMER_ISR_IRAM_SAFE
    if (cbs->on_alarm) {
        ESP_RETURN_ON_FALSE(esp_ptr_in_iram(cbs->on_alarm), ESP_ERR_INVALID_ARG, TAG, "on_alarm callback not in IRAM");
    }
    if (user_data) {
        ESP_RETURN_ON_FALSE(esp_ptr_internal(user_data), ESP_ERR_INVALID_ARG, TAG, "user context not in internal RAM");
    }
#endif

    // lazy install interrupt service
    if (!timer->intr) {
        ESP_RETURN_ON_FALSE(timer->fsm == GPTIMER_FSM_INIT, ESP_ERR_INVALID_STATE, TAG, "timer not in init state");
        // if user wants to control the interrupt allocation more precisely, we can expose more flags in `gptimer_config_t`
        int isr_flags = timer->flags.intr_shared ? ESP_INTR_FLAG_SHARED | GPTIMER_INTR_ALLOC_FLAGS : GPTIMER_INTR_ALLOC_FLAGS;
        // ESP_RETURN_ON_ERROR(esp_intr_alloc_intrstatus(timer_group_periph_signals.groups[group_id].timer_irq_id[timer_id], isr_flags,
        //                     (uint32_t)timer_ll_get_intr_status_reg(timer->hal.dev), TIMER_LL_EVENT_ALARM(timer_id),
        //                     gptimer_default_isr, timer, &timer->intr), TAG, "install interrupt service failed");
        ESP_RETURN_ON_ERROR(esp_intr_alloc_intrstatus(timer_group_periph_signals.groups[group_id].t0_irq_id, isr_flags,
                            (uint32_t)timer_ll_get_intr_status_reg(timer->hal.dev), timer_id,
                            gptimer_default_isr, timer, &timer->intr), TAG, "install interrupt service failed");
    }

    // enable/disable GPTimer interrupt events
    portENTER_CRITICAL(&group->spinlock);
    //timer_ll_enable_intr(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id), cbs->on_alarm != NULL); // enable timer interrupt
    timer_ll_intr_enable(timer->hal.dev, timer->timer_id);
    portEXIT_CRITICAL(&group->spinlock);

    timer->on_alarm = cbs->on_alarm;
    timer->user_ctx = user_data;
    return ESP_OK;
}

esp_err_t gptimer_set_alarm_action(gptimer_handle_t timer, const gptimer_alarm_config_t *config)
{
    ESP_RETURN_ON_FALSE_ISR(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    if (config) {
        // When auto_reload is enabled, alarm_count should not be equal to reload_count
        bool valid_auto_reload = !config->flags.auto_reload_on_alarm || config->alarm_count != config->reload_count;
        ESP_RETURN_ON_FALSE_ISR(valid_auto_reload, ESP_ERR_INVALID_ARG, TAG, "reload count can't equal to alarm count");

        portENTER_CRITICAL_SAFE(&timer->spinlock);
        timer->reload_count = config->reload_count;
        timer->alarm_count = config->alarm_count;
        timer->flags.auto_reload_on_alarm = config->flags.auto_reload_on_alarm;
        timer->flags.alarm_en = true;

        //timer_ll_set_reload_value(timer->hal.dev, timer->timer_id, config->reload_count);
        //timer_ll_set_counter_value(timg_dev_t *hw, timer_idx_t timer_num, uint64_t load_val)
        timer_ll_set_counter_value(timer->hal.dev, timer->timer_id, config->reload_count);
        timer_ll_set_alarm_value(timer->hal.dev, timer->timer_id, config->alarm_count);
        portEXIT_CRITICAL_SAFE(&timer->spinlock);
    } else {
        portENTER_CRITICAL_SAFE(&timer->spinlock);
        timer->flags.auto_reload_on_alarm = false;
        timer->flags.alarm_en = false;
        portEXIT_CRITICAL_SAFE(&timer->spinlock);
    }

    portENTER_CRITICAL_SAFE(&timer->spinlock);
    //timer_ll_enable_auto_reload(timer->hal.dev, timer->timer_id, timer->flags.auto_reload_on_alarm);
    //timer_ll_set_auto_reload(timg_dev_t *hw, timer_idx_t timer_num, bool auto_reload_en)
    timer_ll_set_auto_reload(timer->hal.dev, timer->timer_id, timer->flags.auto_reload_on_alarm);

    //timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, timer->flags.alarm_en);
    //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
    timer_ll_set_alarm_enable(timer->hal.dev, timer->timer_id, timer->flags.alarm_en);
    portEXIT_CRITICAL_SAFE(&timer->spinlock);
    return ESP_OK;
}

esp_err_t gptimer_enable(gptimer_handle_t timer)
{
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(timer->fsm == GPTIMER_FSM_INIT, ESP_ERR_INVALID_STATE, TAG, "timer not in init state");

    // acquire power manager lock
    if (timer->pm_lock) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_acquire(timer->pm_lock), TAG, "acquire pm_lock failed");
    }
    // enable interrupt service
    if (timer->intr) {
        ESP_RETURN_ON_ERROR(esp_intr_enable(timer->intr), TAG, "enable interrupt service failed");
    }

    timer->fsm = GPTIMER_FSM_ENABLE;
    return ESP_OK;
}

esp_err_t gptimer_disable(gptimer_handle_t timer)
{
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(timer->fsm == GPTIMER_FSM_ENABLE, ESP_ERR_INVALID_STATE, TAG, "timer not in enable state");

    // disable interrupt service
    if (timer->intr) {
        ESP_RETURN_ON_ERROR(esp_intr_disable(timer->intr), TAG, "disable interrupt service failed");
    }
    // release power manager lock
    if (timer->pm_lock) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_release(timer->pm_lock), TAG, "release pm_lock failed");
    }

    timer->fsm = GPTIMER_FSM_INIT;
    return ESP_OK;
}

esp_err_t gptimer_start(gptimer_handle_t timer)
{
    ESP_RETURN_ON_FALSE_ISR(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE_ISR(timer->fsm == GPTIMER_FSM_ENABLE, ESP_ERR_INVALID_STATE, TAG, "timer not enabled yet");

    portENTER_CRITICAL_SAFE(&timer->spinlock);
    //timer_ll_enable_counter(timer->hal.dev, timer->timer_id, true);
    //timer_ll_set_counter_enable(timg_dev_t *hw, timer_idx_t timer_num, bool counter_en)
    timer_ll_set_counter_enable(timer->hal.dev, timer->timer_id, true);

    //timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, timer->flags.alarm_en);
    //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
    timer_ll_set_alarm_enable(timer->hal.dev, timer->timer_id, timer->flags.alarm_en);

    portEXIT_CRITICAL_SAFE(&timer->spinlock);

    return ESP_OK;
}

esp_err_t gptimer_stop(gptimer_handle_t timer)
{
    ESP_RETURN_ON_FALSE_ISR(timer, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE_ISR(timer->fsm == GPTIMER_FSM_ENABLE, ESP_ERR_INVALID_STATE, TAG, "timer not enabled yet");

    // disable counter, alarm, auto-reload
    portENTER_CRITICAL_SAFE(&timer->spinlock);
    // timer_ll_enable_counter(timer->hal.dev, timer->timer_id, false);
    //timer_ll_set_counter_enable(timg_dev_t *hw, timer_idx_t timer_num, bool counter_en)
    timer_ll_set_counter_enable(timer->hal.dev, timer->timer_id, false);

    // timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, false);
    //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
    timer_ll_set_alarm_enable(timer->hal.dev, timer->timer_id, false);

    portEXIT_CRITICAL_SAFE(&timer->spinlock);

    return ESP_OK;
}

static gptimer_group_t *gptimer_acquire_group_handle(int group_id)
{
    bool new_group = false;
    gptimer_group_t *group = NULL;

    // prevent install timer group concurrently
    _lock_acquire(&s_platform.mutex);
    if (!s_platform.groups[group_id]) {
        group = heap_caps_calloc(1, sizeof(gptimer_group_t), GPTIMER_MEM_ALLOC_CAPS);
        if (group) {
            new_group = true;
            s_platform.groups[group_id] = group;
            // initialize timer group members
            group->group_id = group_id;
            group->spinlock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
            // enable APB access timer registers
            periph_module_enable(timer_group_periph_signals.groups[group_id].module);
        }
    } else {
        group = s_platform.groups[group_id];
    }
    if (group) {
        // someone acquired the group handle means we have a new object that refer to this group
        s_platform.group_ref_counts[group_id]++;
    }
    _lock_release(&s_platform.mutex);

    if (new_group) {
        ESP_LOGD(TAG, "new group (%d) @%p", group_id, group);
    }

    return group;
}

static void gptimer_release_group_handle(gptimer_group_t *group)
{
    int group_id = group->group_id;
    bool do_deinitialize = false;

    _lock_acquire(&s_platform.mutex);
    s_platform.group_ref_counts[group_id]--;
    if (s_platform.group_ref_counts[group_id] == 0) {
        assert(s_platform.groups[group_id]);
        do_deinitialize = true;
        s_platform.groups[group_id] = NULL;
        // Theoretically we need to disable the peripheral clock for the timer group
        // However, next time when we enable the peripheral again, the registers will be reset to default value, including the watchdog registers inside the group
        // Then the watchdog will go into reset state, e.g. the flash boot watchdog is enabled again and reset the system very soon
        // periph_module_disable(timer_group_periph_signals.groups[group_id].module);
    }
    _lock_release(&s_platform.mutex);

    if (do_deinitialize) {
        free(group);
        ESP_LOGD(TAG, "del group (%d)", group_id);
    }
}

static esp_err_t gptimer_select_periph_clock(gptimer_t *timer, gptimer_clock_source_t src_clk, uint32_t resolution_hz)
{
    unsigned int counter_src_hz = 0;
    esp_err_t ret = ESP_OK;
    int timer_id = timer->timer_id;
    // [clk_tree] TODO: replace the following switch table by clk_tree API
    switch (src_clk) {
#if SOC_TIMER_GROUP_SUPPORT_APB
    case GPTIMER_CLK_SRC_APB:
        counter_src_hz = esp_clk_apb_freq();
#if CONFIG_PM_ENABLE
        sprintf(timer->pm_lock_name, "gptimer_%d_%d", timer->group->group_id, timer_id); // e.g. gptimer_0_0
        ret  = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, timer->pm_lock_name, &timer->pm_lock);
        ESP_RETURN_ON_ERROR(ret, TAG, "create APB_FREQ_MAX lock failed");
        ESP_LOGD(TAG, "install APB_FREQ_MAX lock for timer (%d,%d)", timer->group->group_id, timer_id);
#endif
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_APB
#if SOC_TIMER_GROUP_SUPPORT_PLL_F40M
    case GPTIMER_CLK_SRC_PLL_F40M:
        counter_src_hz = 40 * 1000 * 1000;
#if CONFIG_PM_ENABLE
        sprintf(timer->pm_lock_name, "gptimer_%d_%d", timer->group->group_id, timer_id); // e.g. gptimer_0_0
        // on ESP32C2, PLL_F40M is unavailable when CPU clock source switches from PLL to XTAL, so we're acquiring a "APB" lock here to prevent the clock switch
        ret  = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, timer->pm_lock_name, &timer->pm_lock);
        ESP_RETURN_ON_ERROR(ret, TAG, "create APB_FREQ_MAX lock failed");
        ESP_LOGD(TAG, "install APB_FREQ_MAX lock for timer (%d,%d)", timer->group->group_id, timer_id);
#endif
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_PLL_F40M
#if SOC_TIMER_GROUP_SUPPORT_PLL_F80M
    case GPTIMER_CLK_SRC_PLL_F80M:
        counter_src_hz = 80 * 1000 * 1000;
#if CONFIG_PM_ENABLE
        sprintf(timer->pm_lock_name, "gptimer_%d_%d", timer->group->group_id, timer_id); // e.g. gptimer_0_0
        // ESP32C6 PLL_F80M is available when SOC_ROOT_CLK switches to XTAL
        ret  = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, timer->pm_lock_name, &timer->pm_lock);
        ESP_RETURN_ON_ERROR(ret, TAG, "create NO_LIGHT_SLEEP lock failed");
        ESP_LOGD(TAG, "install NO_LIGHT_SLEEP lock for timer (%d,%d)", timer->group->group_id, timer_id);
#endif
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_PLL_F80M
#if SOC_TIMER_GROUP_SUPPORT_AHB
    case GPTIMER_CLK_SRC_AHB:
        // TODO: decide which kind of PM lock we should use for such clock
        counter_src_hz = 48 * 1000 * 1000;
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_AHB
#if SOC_TIMER_GROUP_SUPPORT_XTAL
    case GPTIMER_CLK_SRC_XTAL:
        counter_src_hz = esp_clk_xtal_freq();
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_XTAL
#if SOC_TIMER_GROUP_SUPPORT_RC_FAST
    case GPTIMER_CLK_SRC_RC_FAST:
        periph_rtc_dig_clk8m_enable();
        counter_src_hz = periph_rtc_dig_clk8m_get_freq();
        break;
#endif // SOC_TIMER_GROUP_SUPPORT_RC_FAST
    default:
        ESP_RETURN_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, TAG, "clock source %d is not support", src_clk);
        break;
    }
    timer_ll_set_clock_source(timer->hal.dev, timer_id, src_clk);
    timer->clk_src = src_clk;
    unsigned int prescale = counter_src_hz / resolution_hz; // potential resolution loss here
    //timer_ll_set_clock_prescale(timer->hal.dev, timer_id, prescale);
    //timer_ll_set_divider(timg_dev_t *hw, timer_idx_t timer_num, uint32_t divider)
    timer_ll_set_divider(timer->hal.dev, timer_id, prescale);
    timer->resolution_hz = counter_src_hz / prescale; // this is the real resolution
    if (timer->resolution_hz != resolution_hz) {
        ESP_LOGW(TAG, "resolution lost, expect %d, real %d", resolution_hz, timer->resolution_hz);
    }
    return ret;
}

// Put the default ISR handler in the IRAM for better performance
IRAM_ATTR static void gptimer_default_isr(void *args)
{
    bool need_yield = false;
    gptimer_t *timer = (gptimer_t *)args;
    gptimer_group_t *group = timer->group;
    gptimer_alarm_cb_t on_alarm_cb = timer->on_alarm;
    uint32_t intr_status=0;// = timer_ll_get_intr_status(timer->hal.dev);
    timer_ll_get_intr_status(timer->hal.dev, &intr_status);

    //if (intr_status & TIMER_LL_EVENT_ALARM(timer->timer_id)) {
    if (intr_status & timer->timer_id) {    
        // Note: when alarm event happens, the alarm will be disabled automatically by hardware
        uint64_t TcntVal=0;
        //timer_ll_get_counter_value(&timer->hal->dev,&timer->hal->idx, TcntVal);
        timer_hal_get_counter_value(&timer->hal, &TcntVal);
        gptimer_alarm_event_data_t edata = {
            //.count_value = timer_hal_capture_and_get_counter_value(&timer->hal),
            .count_value = TcntVal,
            .alarm_value = timer->alarm_count,
        };

        portENTER_CRITICAL_ISR(&group->spinlock);
        // timer_ll_clear_intr_status(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id));
        timer_ll_clear_intr_status(timer->hal.dev, timer->timer_id);
        // for auto-reload, we need to re-enable the alarm manually
        if (timer->flags.auto_reload_on_alarm) {
            //timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, true);
            //timer_ll_set_alarm_enable(timg_dev_t *hw, timer_idx_t timer_num, bool alarm_en)
            timer_ll_set_alarm_enable(timer->hal.dev, timer->timer_id, true);
        }
        portEXIT_CRITICAL_ISR(&group->spinlock);

        if (on_alarm_cb) {
            if (on_alarm_cb(timer, &edata, timer->user_ctx)) {
                need_yield = true;
            }
        }
    }

    if (need_yield) {
        portYIELD_FROM_ISR();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// The Following APIs are for internal use only (e.g. unit test)    /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t gptimer_get_intr_handle(gptimer_handle_t timer, intr_handle_t *ret_intr_handle)
{
    ESP_RETURN_ON_FALSE(timer && ret_intr_handle, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    *ret_intr_handle = timer->intr;
    return ESP_OK;
}

esp_err_t gptimer_get_pm_lock(gptimer_handle_t timer, esp_pm_lock_handle_t *ret_pm_lock)
{
    ESP_RETURN_ON_FALSE(timer && ret_pm_lock, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    *ret_pm_lock = timer->pm_lock;
    return ESP_OK;
}
