# SPDX-License-Identifier: MIT
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURE src/input_processor_gesture.c)
