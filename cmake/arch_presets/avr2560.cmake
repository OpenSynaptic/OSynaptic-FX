# Arch preset: AVR 8-bit large — ATmega2560 (Arduino Mega 2560)
# SRAM: 8 KB total. Stack budget: 512 B. Flash: 256 KB.
#
# More headroom than ATmega328P but still resource-constrained.
# Secure session and supplementary message are disabled to preserve SRAM.
# Both encode and decode paths are supported at this configuration.
#
# Static memory budget:
#   fusion_entry ≈ 4+4+24+2*8+4*12 = 8+24+16+48 = 96 B × 16 = 1536 B
#   ID table     ≈ 24 B × 16 entries = 384 B
#   body scratch = 64 B
#   Total static ≈ 2.0 KB — leaves ~6 KB for globals + 512 B stack

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "16" CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "24" CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "4"  CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "12" CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "2"  CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "8"  CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "16" CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "2"  CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "64" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "1"  CACHE STRING "Arch: static scratch"    FORCE)

set(OSFX_CFG_PAYLOAD_GEOHASH_ID              OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
