#ifndef OSFX_USER_CONFIG_H
#define OSFX_USER_CONFIG_H

/*
 * DRAM budget tuning for Arduino/ESP32.
 *
 * These defaults apply when building with the Arduino IDE.
 * For CMake cross-compile builds, use -DOSFX_ARCH_PRESET=<name> which
 * passes per-architecture values as -D flags that take priority over these
 * #ifndef defaults.
 *
 * ID allocator table size:
 *   Sensor node  : 4
 *   Small gateway: 32
 *   Large gateway: 128
 */
#ifndef OSFX_ID_MAX_ENTRIES
#define OSFX_ID_MAX_ENTRIES 128
#endif

/*
 * Max sensors per template message (affects static scratch buffers).
 * g_multi_sensor_slots and osfx_template_msg.sensors are sized by this.
 * 4 covers the typical 1-4 sensor node; raise if needed.
 */
#ifndef OSFX_TMPL_MAX_SENSORS
#define OSFX_TMPL_MAX_SENSORS 4
#endif

/*
 * Body buffer capacity for multi-sensor encode/decode scratch.
 * 512 bytes handles ~6 sensors comfortably; default 4096 wastes ~7 KB.
 */
#ifndef OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
#define OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH 1
#endif
#ifndef OSFX_CFG_MULTI_SENSOR_BODY_CAP
#define OSFX_CFG_MULTI_SENSOR_BODY_CAP 512
#endif

#endif
