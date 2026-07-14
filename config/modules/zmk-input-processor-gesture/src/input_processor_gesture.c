/*
 * zmk-input-processor-gesture
 *
 * 試作コード（2026-07-14、Skinner39 Studio連携で作成）。実機でのビルド・書き込み・
 * 動作確認はまだ行っていません。ZMK公式の scaler / behaviors input-processor
 * （app/src/pointing/input_processor_scaler.c, input_processor_behaviors.c）を
 * 参考に実装しています。
 *
 * 動作: 監視対象の軸（code）について、event->value をインスタンスごとの
 * remainder（zmk_input_processor_state）に累積し、閾値(threshold)を超えたら
 * bindings[0]（負方向）または bindings[1]（正方向）を1回だけ押して離す。
 * このプロセッサを通した軸のイベントは常に消費し（cursor移動としては伝播させない）、
 * 専用のジェスチャーレイヤーで使うことを想定している。
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_gesture

#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/virtual_key_position.h>

struct ip_gesture_config {
    uint8_t index;
    uint16_t type;
    uint16_t code;
    int16_t threshold;
    const struct zmk_behavior_binding *bindings; // [0]=負方向 [1]=正方向、必ず2個
};

static int ip_gesture_handle_event(const struct device *dev, struct input_event *event,
                                    uint32_t param1, uint32_t param2,
                                    struct zmk_input_processor_state *state) {
    const struct ip_gesture_config *cfg = dev->config;

    if (event->type != cfg->type || event->code != cfg->code) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int32_t total = event->value;
    if (state && state->remainder) {
        total += *state->remainder;
    }

    if (total <= -cfg->threshold || total >= cfg->threshold) {
        int dir = (total > 0) ? 1 : 0;

        struct zmk_behavior_binding_event behavior_event = {
            .position = ZMK_VIRTUAL_KEY_POSITION_BEHAVIOR_INPUT_PROCESSOR(
                state ? state->input_device_index : 0, cfg->index),
            .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
            .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
        };

        // 通常のキー押下と同じくpress→releaseの2回invokeでワンタップを合成する。
        // 端末側で反応しない場合はここに短いk_msleep()を挟む必要があるかもしれない
        // （実機検証待ち）
        zmk_behavior_invoke_binding(&cfg->bindings[dir], behavior_event, true);
        zmk_behavior_invoke_binding(&cfg->bindings[dir], behavior_event, false);

        total = 0;
    }

    if (state && state->remainder) {
        *state->remainder = (int16_t)total;
    }

    // ジェスチャー用の軸として扱い、カーソル移動としては伝播させない
    return ZMK_INPUT_PROC_STOP;
}

static struct zmk_input_processor_driver_api ip_gesture_driver_api = {
    .handle_event = ip_gesture_handle_event,
};

#define IP_GESTURE_INST(n)                                                                        \
    static const struct zmk_behavior_binding ip_gesture_bindings_##n[] = {                        \
        LISTIFY(DT_INST_PROP_LEN(n, bindings), ZMK_KEYMAP_EXTRACT_BINDING, (, ), DT_DRV_INST(n))}; \
    BUILD_ASSERT(ARRAY_SIZE(ip_gesture_bindings_##n) == 2,                                         \
                 "zmk,input-processor-gesture requires exactly 2 bindings: "                       \
                 "[0]=negative direction, [1]=positive direction");                                 \
    static const struct ip_gesture_config ip_gesture_config_##n = {                                \
        .index = n,                                                                                \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .code = DT_INST_PROP(n, code),                                                             \
        .threshold = DT_INST_PROP_OR(n, threshold, 40),                                            \
        .bindings = ip_gesture_bindings_##n,                                                       \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &ip_gesture_config_##n, POST_KERNEL,                \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ip_gesture_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IP_GESTURE_INST)
