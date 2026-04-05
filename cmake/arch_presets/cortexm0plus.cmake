# Arch preset: ARM Cortex-M0+ (RP2040, STM32C0, SAM D21)
# Conservative baseline: STM32C0 (12 KB SRAM), stack budget 2048 B.
# RP2040 (264 KB) has more headroom but this preset targets the most
# constrained M0+ device in the family.
#
# osfx_fusion_entry size at these limits:
#   4 + 4 + 32 + 4*12 + 4*12 = 8 + 32 + 48 + 48 = 136 B  (approx)
#   32 entries = ~4.4 KB fusion state
#   ID table: 64 * ~24 B = ~1.5 KB
#   Body scratch: 256 B
#   Total static: ~6.2 KB  (well within 12 KB / 264 KB)

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "32"  CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "32"  CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "4"   CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "12"  CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "4"   CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "12"  CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "64"  CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "4"   CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "256" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "1"   CACHE STRING "Arch: static scratch"    FORCE)

# Disable optional payload fields to minimise struct size
set(OSFX_CFG_PAYLOAD_GEOHASH_ID              OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
