# Arch preset: x86_64 (Linux, Windows, macOS) — development and test host
# No SRAM constraint. Stack budget: 16384 B. Used for protocol development
# and integration testing against the OpenSynaptic Python/Rust server.

set(OSFX_ARCH_FUSION_MAX_ENTRIES  "256"  CACHE STRING "Arch: fusion entries"    FORCE)
set(OSFX_ARCH_FUSION_MAX_PREFIX   "64"   CACHE STRING "Arch: sig prefix len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_VALS     "16"   CACHE STRING "Arch: val slots"         FORCE)
set(OSFX_ARCH_FUSION_MAX_VAL_LEN  "32"   CACHE STRING "Arch: val string len"    FORCE)
set(OSFX_ARCH_FUSION_MAX_SENSORS  "16"   CACHE STRING "Arch: sensors per entry" FORCE)
set(OSFX_ARCH_FUSION_MAX_TAG_LEN  "16"   CACHE STRING "Arch: tag name len"      FORCE)
set(OSFX_ARCH_ID_MAX_ENTRIES      "512"  CACHE STRING "Arch: ID table size"     FORCE)
set(OSFX_ARCH_TMPL_MAX_SENSORS    "16"   CACHE STRING "Arch: template sensors"  FORCE)
set(OSFX_ARCH_BODY_CAP            "4096" CACHE STRING "Arch: body buffer bytes" FORCE)
set(OSFX_ARCH_STATIC_SCRATCH      "0"    CACHE STRING "Arch: static scratch"    FORCE)

set(OSFX_CFG_PAYLOAD_GEOHASH_ID              ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE   ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_RESOURCE_URL            ON  CACHE BOOL "" FORCE)
set(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE      OFF CACHE BOOL "" FORCE)
