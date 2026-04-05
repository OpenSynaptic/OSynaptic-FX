# Arch preset: ARM Cortex-M7 (STM32H7, i.MX RT, SAM E70)
# High-memory configuration; 1 MB+ SRAM available, stack budget 8192 B.
#
# osfx_fusion_entry ≈ 4+4+64+16*32+16*16 = 8+64+512+256 = 840 B
# 128 entries = ~107 KB fusion state
# ID table: 256 * ~24 B = ~6.1 KB
# Body scratch: 1024 B
# Total static: ~114 KB  (well within 1 MB)
# Static scratch disabled — can use task stacks or heap for body buffers.

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "128"  CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "64"   CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "16"   CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "32"   CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "16"   CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "16"   CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "256"  CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "16"   CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "1024" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "0"    CACHE STRING "Arch: static scratch"    FORCE)

set(OSFX_CFG_PAYLOAD_GEOHASH_ID              ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
