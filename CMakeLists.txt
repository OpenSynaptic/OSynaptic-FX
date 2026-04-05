cmake_minimum_required(VERSION 3.15)
project(osfx VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Architecture preset (optional)
# Sets per-arch memory limits and overrides payload feature defaults.
# Usage: cmake -B build -DOSFX_ARCH_PRESET=cortexm4
# Available presets: cortexm0plus cortexm3 cortexm4 cortexm7 cortexm33
#                    riscv32 esp32 avr328p avr2560 x86_64
# ---------------------------------------------------------------------------
set(OSFX_ARCH_PRESET "" CACHE STRING
    "Architecture memory preset (cortexm0plus|cortexm3|cortexm4|cortexm7|cortexm33|riscv32|esp32|avr328p|avr2560|x86_64)")

if(OSFX_ARCH_PRESET)
    set(_osfx_preset_file
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arch_presets/${OSFX_ARCH_PRESET}.cmake")
    if(NOT EXISTS "${_osfx_preset_file}")
        message(FATAL_ERROR
            "Unknown OSFX_ARCH_PRESET '${OSFX_ARCH_PRESET}'. "
            "Expected file: ${_osfx_preset_file}")
    endif()
    include("${_osfx_preset_file}")
    message(STATUS "OSFX arch preset: ${OSFX_ARCH_PRESET}")
endif()

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

option(OSFX_ENABLE_LTO         "Enable link-time optimization"       OFF)
option(OSFX_ENABLE_FILE_IO     "Enable persistent storage"           OFF)
option(OSFX_ENABLE_CLI         "Enable command line interface"        OFF)

# Easy-API and storage abstraction (pure C99, no Arduino SDK dependency).
# Disabled by default for bare cmake/native builds; set ON when you want the
# high-level osfx_easy_* helpers or the pluggable storage backend in a
# non-Arduino environment.
option(OSFX_BUILD_EASY_API     "Build osfx_easy + osfx_storage modules" OFF)

# Plugin options
option(OSFX_CFG_PLUGIN_TRANSPORT      "Enable transport plugin"        ON)
option(OSFX_CFG_PLUGIN_TEST_PLUGIN    "Enable test_plugin plugin"      ON)
option(OSFX_CFG_PLUGIN_PORT_FORWARDER "Enable port_forwarder plugin"   ON)
option(OSFX_CFG_AUTOLOAD_TRANSPORT      "Autoload transport plugin"    ON)
option(OSFX_CFG_AUTOLOAD_TEST_PLUGIN    "Autoload test_plugin plugin"  ON)
option(OSFX_CFG_AUTOLOAD_PORT_FORWARDER "Autoload port_forwarder"      ON)

# Payload field options
option(OSFX_CFG_PAYLOAD_DEVICE_ID          "Enable DeviceId"              ON)
option(OSFX_CFG_PAYLOAD_DEVICE_STATUS      "Enable DeviceStatus"          ON)
option(OSFX_CFG_PAYLOAD_TIMESTAMP          "Enable Timestamp"             ON)
option(OSFX_CFG_PAYLOAD_SUB_TEMPLATE_ID    "Enable SubTemplateId"         ON)
option(OSFX_CFG_PAYLOAD_SENSOR_ID          "Enable SensorId"              ON)
option(OSFX_CFG_PAYLOAD_SENSOR_STATUS      "Enable SensorStatus"          ON)
option(OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE "Enable PhysicalAttribute"    OFF)
option(OSFX_CFG_PAYLOAD_NORMALIZED_VALUE   "Enable NormalizedValue"       ON)
option(OSFX_CFG_PAYLOAD_GEOHASH_ID         "Enable GeohashId"            OFF)
option(OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE "Enable SupplementaryMessage" ON)
option(OSFX_CFG_PAYLOAD_RESOURCE_URL       "Enable ResourceUrl"          OFF)

# ---------------------------------------------------------------------------
# Library sources
# ---------------------------------------------------------------------------

set(OSFX_CORE_SOURCES
    src/osfx_solidity.c
    src/osfx_fusion_packet.c
    src/osfx_fusion_state.c
    src/osfx_standardization.c
    src/osfx_template_grammar.c
    src/osfx_secure_session.c
    src/osfx_id_allocator.c
    src/osfx_handshake_cmd.c
    src/osfx_handshake_dispatch.c
    src/osfx_payload_crypto.c
    src/osfx_protocol_matrix.c
    src/osfx_transporter_runtime.c
    src/osfx_service_runtime.c
    src/osfx_platform_runtime.c
    src/osfx_plugin_transport.c
    src/osfx_plugin_test.c
    src/osfx_plugin_port_forwarder.c
    src/osfx_cli_lite.c
    src/osfx_glue.c
    src/osfx_core_facade.c
    src/osfx_library_catalog.c
)

# osfx_easy.c and osfx_storage.c are pure C99 and compile fine with any
# toolchain. osfx_storage_littlefs.c depends on the Arduino LittleFS SDK and
# is intentionally excluded here — it is compiled by the Arduino IDE only.
if(OSFX_BUILD_EASY_API)
    list(APPEND OSFX_CORE_SOURCES
        src/osfx_easy.c
        src/osfx_storage.c
    )
endif()

add_library(osfx_core STATIC ${OSFX_CORE_SOURCES})
add_library(osfx::core ALIAS osfx_core)

target_include_directories(osfx_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(MSVC)
    target_compile_definitions(osfx_core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(OSFX_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT osfx_ipo_ok OUTPUT osfx_ipo_msg)
    if(osfx_ipo_ok)
        set_property(TARGET osfx_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "OSFX_ENABLE_LTO=ON but IPO/LTO not supported: ${osfx_ipo_msg}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Per-architecture size-limit defines (set by arch preset, if loaded)
# These override the #ifndef defaults in osfx_user_config.h and the
# individual module headers.
# ---------------------------------------------------------------------------
if(OSFX_ARCH_PRESET)
    foreach(_osfx_var
        OSFX_FUSION_MAX_ENTRIES
        OSFX_FUSION_MAX_PREFIX
        OSFX_FUSION_MAX_VALS
        OSFX_FUSION_MAX_VAL_LEN
        OSFX_FUSION_MAX_SENSORS
        OSFX_FUSION_MAX_TAG_LEN
        OSFX_ID_MAX_ENTRIES
        OSFX_TMPL_MAX_SENSORS
        OSFX_CFG_MULTI_SENSOR_BODY_CAP
        OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
    )
        # Map OSFX_FOO → OSFX_ARCH_FOO (set by preset file)
        string(REPLACE "OSFX_" "OSFX_ARCH_" _osfx_arch_var "${_osfx_var}")
        if(DEFINED ${_osfx_arch_var})
            target_compile_definitions(osfx_core PUBLIC
                ${_osfx_var}=${${_osfx_arch_var}})
        endif()
    endforeach()
endif()

target_compile_definitions(osfx_core PUBLIC
    OSFX_ENABLE_FILE_IO=$<BOOL:${OSFX_ENABLE_FILE_IO}>
    OSFX_ENABLE_CLI=$<BOOL:${OSFX_ENABLE_CLI}>
    OSFX_CFG_PLUGIN_TRANSPORT=$<BOOL:${OSFX_CFG_PLUGIN_TRANSPORT}>
    OSFX_CFG_PLUGIN_TEST_PLUGIN=$<BOOL:${OSFX_CFG_PLUGIN_TEST_PLUGIN}>
    OSFX_CFG_PLUGIN_PORT_FORWARDER=$<BOOL:${OSFX_CFG_PLUGIN_PORT_FORWARDER}>
    OSFX_CFG_AUTOLOAD_TRANSPORT=$<BOOL:${OSFX_CFG_AUTOLOAD_TRANSPORT}>
    OSFX_CFG_AUTOLOAD_TEST_PLUGIN=$<BOOL:${OSFX_CFG_AUTOLOAD_TEST_PLUGIN}>
    OSFX_CFG_AUTOLOAD_PORT_FORWARDER=$<BOOL:${OSFX_CFG_AUTOLOAD_PORT_FORWARDER}>
    OSFX_CFG_PAYLOAD_DEVICE_ID=$<BOOL:${OSFX_CFG_PAYLOAD_DEVICE_ID}>
    OSFX_CFG_PAYLOAD_DEVICE_STATUS=$<BOOL:${OSFX_CFG_PAYLOAD_DEVICE_STATUS}>
    OSFX_CFG_PAYLOAD_TIMESTAMP=$<BOOL:${OSFX_CFG_PAYLOAD_TIMESTAMP}>
    OSFX_CFG_PAYLOAD_SUB_TEMPLATE_ID=$<BOOL:${OSFX_CFG_PAYLOAD_SUB_TEMPLATE_ID}>
    OSFX_CFG_PAYLOAD_SENSOR_ID=$<BOOL:${OSFX_CFG_PAYLOAD_SENSOR_ID}>
    OSFX_CFG_PAYLOAD_SENSOR_STATUS=$<BOOL:${OSFX_CFG_PAYLOAD_SENSOR_STATUS}>
    OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE=$<BOOL:${OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE}>
    OSFX_CFG_PAYLOAD_NORMALIZED_VALUE=$<BOOL:${OSFX_CFG_PAYLOAD_NORMALIZED_VALUE}>
    OSFX_CFG_PAYLOAD_GEOHASH_ID=$<BOOL:${OSFX_CFG_PAYLOAD_GEOHASH_ID}>
    OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE=$<BOOL:${OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE}>
    OSFX_CFG_PAYLOAD_RESOURCE_URL=$<BOOL:${OSFX_CFG_PAYLOAD_RESOURCE_URL}>
)

set_target_properties(osfx_core PROPERTIES
    OUTPUT_NAME osfx_core
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# ---------------------------------------------------------------------------
# Install rules (native / cross-compile targets)
# ---------------------------------------------------------------------------

install(TARGETS osfx_core
    EXPORT osfx_coreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT osfx_coreTargets
    FILE osfx_coreTargets.cmake
    NAMESPACE osfx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osfx
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osfx-c99Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/osfxConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osfx
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/osfxConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/osfxConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/osfxConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osfx
)
