# Arch preset: AVR 8-bit — ATmega328P (Arduino Uno, Nano, Pro Mini)
# SRAM: 2 KB total. Stack budget: 256 B. Flash: 32 KB.
#
# CONSTRAINTS — read before using:
#  - Only the ENCODE path (osfx_easy_encode_sensor_auto, osfx_fusion_encode)
#    is supported at this budget. Do NOT call osfx_fusion_decode_apply() or
#    osfx_secure_session_* — these exceed the available stack.
#  - Flash: the .a archive is 97 KB but the linker dead-strips unused symbols.
#    A TX-only sketch that calls only encode functions links to ~18-24 KB.
#    If you add decode + secure session, you will exceed 32 KB Flash.
#  - Use -ffunction-sections -fdata-sections -Wl,--gc-sections when linking.
#
# Static memory budget:
#   fusion_entry ≈ 48 B × 8 entries = 384 B
#   ID table     ≈ 24 B × 8 entries = 192 B
#   body scratch = 32 B
#   Total static ≈ 660 B — leaves ~1100 B for globals + stack

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "8"  CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "16" CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "2"  CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "8"  CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "1"  CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "8"  CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "8"  CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "1"  CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "32" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "1"  CACHE STRING "Arch: static scratch"    FORCE)

# All optional payload fields MUST be disabled — no SRAM budget for them
set(OSFX_CFG_PAYLOAD_GEOHASH_ID              OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            OFF CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
