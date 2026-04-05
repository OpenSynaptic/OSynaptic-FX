# Arch preset: RISC-V 32-bit generic (ESP32-C3/C6, CH32V, GD32VF103)
# Baseline: ESP32-C3 (400 KB SRAM), stack budget 4096 B.
# Covers rv32imc (ESP32-C3) and rv32imac (GD32VF, CH32V) targets;
# memory limits match the more capable end of this family.

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "64"  CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "64"  CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "8"   CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "16"  CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "8"   CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "12"  CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "128" CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "8"   CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "512" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "1"   CACHE STRING "Arch: static scratch"    FORCE)

set(OSFX_CFG_PAYLOAD_GEOHASH_ID              OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
