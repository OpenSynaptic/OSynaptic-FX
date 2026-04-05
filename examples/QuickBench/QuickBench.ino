/**
 * Quick Benchmark - Dual-Core Arduino Sketch
 * 
 * Measures OSFX packet encoding performance with dual cores:
 * - Core 0 + Core 1 parallel encoding
 * - 200KB memory buffer
 * - Aggregated throughput
 *
 * Suggested audience:
 * - Maintainers and performance diagnostics workflows
 * - Not intended as first-time API onboarding sketch
 * 
 * Usage: Compile & upload to ESP32, open Serial Monitor @ 115200 baud
 */

#include <OSynapticFX.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BENCH_COUNT 20000
#define BUFFER_PER_CORE (70 * 1024)  // 70KB per core, allocated on heap
#define IDLE_YIELD_INTERVAL 500      // Yield every N packets to avoid IDLE WDT
#define TIMESTAMP_STEP_US 1000ULL    // Synthetic timestamp step per packet
#define PAYLOAD_SIZE 150             // Payload size in bytes for benchmark
#define TASK_STACK_SIZE 8192         // Larger stack for big local payload buffers
#define PROGRESS_INTERVAL 1000       // Progress print interval in packets

// Heap-allocated buffers (pointers)
static uint8_t *g_packet_buf_core0 = NULL;
static uint8_t *g_packet_buf_core1 = NULL;
static int g_packet_len = 0;

// Shared result structure
typedef struct {
  uint32_t count;
  uint32_t total_bytes;
  uint32_t errors;
  uint32_t time_ms;
  float avg_packet_size;
  float packets_per_sec;
  int core_id;
} bench_result;

// Global results
static bench_result g_result_core0 = {0};
static bench_result g_result_core1 = {0};

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for Serial to stabilize
  
  Serial.println("\n\n========================================");
  Serial.println("OSFX Dual-Core Benchmark Starting...");
  Serial.println("========================================\n");
  
  delay(500);
  
  Serial.println("\n╔════════════════════════════════════════════════════╗");
  Serial.println("║   OSFX Dual-Core Benchmark (Heap-allocated)      ║");
  Serial.println("║              70KB per core (140KB total)          ║");
  Serial.println("║                   Single Run                      ║");
  Serial.println("╚════════════════════════════════════════════════════╝\n");
  
  // Run single iteration
  Serial.printf("\n========== Running Tests ==========\n\n");
  Serial.flush();
  
  run_single_iteration();
  
  Serial.println("\n========================================");
  Serial.println("Tests complete. System halted.");
  Serial.println("========================================\n");
  Serial.flush();
}

void run_single_iteration() {
  uint32_t free_heap_before = esp_get_free_heap_size();
  Serial.printf("Heap before: %u bytes\n", free_heap_before);
  Serial.flush();
  
  // Allocate buffers from heap
  Serial.println("Allocating buffers...");
  g_packet_buf_core0 = (uint8_t *)malloc(BUFFER_PER_CORE);
  g_packet_buf_core1 = (uint8_t *)malloc(BUFFER_PER_CORE);
  
  if (!g_packet_buf_core0 || !g_packet_buf_core1) {
    Serial.println("ERROR: Failed to allocate buffers!");
    Serial.printf("Core0 buf: %p, Core1 buf: %p\n", g_packet_buf_core0, g_packet_buf_core1);
    Serial.flush();
    free(g_packet_buf_core0);
    free(g_packet_buf_core1);
    return;
  }
  
  Serial.printf("✓ Buffer size:   %u KB per core\n", BUFFER_PER_CORE / 1024);
  Serial.printf("✓ Total buffers: %u KB\n\n", (BUFFER_PER_CORE * 2) / 1024);
  Serial.flush();
  
  // Test 2 only: Dual-core throughput
  test_dual_core_throughput();
  
  // Memory stats
  uint32_t free_heap_after = esp_get_free_heap_size();
  Serial.printf("Heap after:    %u bytes\n", free_heap_after);
  Serial.printf("Heap used:     %u bytes\n", free_heap_before - free_heap_after);
  Serial.flush();
  
  // Free buffers
  free(g_packet_buf_core0);
  free(g_packet_buf_core1);
  g_packet_buf_core0 = NULL;
  g_packet_buf_core1 = NULL;
}

void test_single_packet() {
  Serial.printf("[TEST 1] Single Packet (%u bytes random data)\n", PAYLOAD_SIZE);
  Serial.println("──────────────────────────────────────────────");
  Serial.flush();
  
  // Generate random payload data
  uint8_t random_data[PAYLOAD_SIZE];
  for (int i = 0; i < PAYLOAD_SIZE; i++) {
    random_data[i] = (uint8_t)rand();
  }
  
  // Encode packet on Core 0
  g_packet_len = osfx_packet_encode_full(
      0xABCDEF00,      // cmd (packet type)
      0x12345678,      // source_aid (device ID)
      1,               // tid (transaction ID)
      1234567890ULL,   // timestamp
        random_data,
        PAYLOAD_SIZE,
      g_packet_buf_core0,
      BUFFER_PER_CORE);
  
  Serial.printf("  Running on: Core %d\n", xPortGetCoreID());
      Serial.printf("  Random data size: %u bytes\n", PAYLOAD_SIZE);
  Serial.printf("  Packet size:      %d bytes\n", g_packet_len);
      Serial.printf("  Overhead:         %d bytes\n\n", g_packet_len - PAYLOAD_SIZE);
  Serial.flush();
}

// Task for Core 0
void core0_benchmark_task(void *param) {
  Serial.println("    [Core 0 Task] START");
  Serial.flush();
  
  bench_result *result = (bench_result *)param;
  result->core_id = 0;
  uint32_t start_time = millis();
  uint64_t ts_us = (uint64_t)millis() * 1000ULL;
  
  uint8_t fixed_data[PAYLOAD_SIZE];
  for (int j = 0; j < PAYLOAD_SIZE; j++) {
    fixed_data[j] = (uint8_t)((j * 37 + 11) & 0xFF);
  }
  
  for (int i = 0; i < BENCH_COUNT; i++) {
    // Encode packet with fixed payload to measure encoder upper-bound PPS
    int packet_len = osfx_packet_encode_full(
        0x3F,
        0x11223344,
        i % 8,
        ts_us,
        fixed_data,
        PAYLOAD_SIZE,
        g_packet_buf_core0,
        BUFFER_PER_CORE);
    
    if (packet_len > 0) {
      result->count++;
      result->total_bytes += packet_len;
    } else {
      result->errors++;
    }

    ts_us += TIMESTAMP_STEP_US;

    // Let IDLE task run periodically so Task WDT does not fire on IDLE0
    if ((i + 1) % IDLE_YIELD_INTERVAL == 0) {
      vTaskDelay(1);
    }
    
    // Print progress + heap periodically
    if ((i + 1) % PROGRESS_INTERVAL == 0) {
      uint32_t free_heap = esp_get_free_heap_size();
      Serial.printf("    [Core 0] %u packets, Free Heap: %u bytes\n", i + 1, free_heap);
      Serial.flush();
    }
  }
  
  result->time_ms = millis() - start_time;
  Serial.println("    [Core 0 Task] END");
  Serial.flush();
  vTaskDelete(NULL);  // Self-delete
}

// Task for Core 1
void core1_benchmark_task(void *param) {
  Serial.println("    [Core 1 Task] START");
  Serial.flush();
  
  bench_result *result = (bench_result *)param;
  result->core_id = 1;
  uint32_t start_time = millis();
  uint64_t ts_us = (uint64_t)millis() * 1000ULL;
  
  uint8_t fixed_data[PAYLOAD_SIZE];
  for (int j = 0; j < PAYLOAD_SIZE; j++) {
    fixed_data[j] = (uint8_t)((j * 37 + 11) & 0xFF);
  }
  
  for (int i = 0; i < BENCH_COUNT; i++) {
    // Encode packet with fixed payload to measure encoder upper-bound PPS
    int packet_len = osfx_packet_encode_full(
        0x3F,
        0x55667788,
        i % 8,
        ts_us,
        fixed_data,
        PAYLOAD_SIZE,
        g_packet_buf_core1,
        BUFFER_PER_CORE);
    
    if (packet_len > 0) {
      result->count++;
      result->total_bytes += packet_len;
    } else {
      result->errors++;
    }

    ts_us += TIMESTAMP_STEP_US;

    // Let IDLE task run periodically so Task WDT does not fire on IDLE0
    if ((i + 1) % IDLE_YIELD_INTERVAL == 0) {
      vTaskDelay(1);
    }
    
    // Print progress + heap periodically
    if ((i + 1) % PROGRESS_INTERVAL == 0) {
      uint32_t free_heap = esp_get_free_heap_size();
      Serial.printf("    [Core 1] %u packets, Free Heap: %u bytes\n", i + 1, free_heap);
      Serial.flush();
    }
  }
  
  result->time_ms = millis() - start_time;
  Serial.println("    [Core 1 Task] END");
  Serial.flush();
  vTaskDelete(NULL);  // Self-delete
}

void test_dual_core_throughput() {
  Serial.printf("\n[TEST 2] Dual-Core Throughput - Fixed Payload (%uB, %u pkt × 2 cores, 140KB total)\n", PAYLOAD_SIZE, BENCH_COUNT);
  Serial.println("──────────────────────────────────────────────────────────────");
  Serial.flush();
  
  // Reset results
  memset(&g_result_core0, 0, sizeof(bench_result));
  memset(&g_result_core1, 0, sizeof(bench_result));
  
  uint32_t global_start = millis();
  
  Serial.println("  Launching Core 0 task...");
  Serial.flush();
  
  // Launch tasks on both cores
  BaseType_t task0 = xTaskCreatePinnedToCore(
      core0_benchmark_task,
      "Core0_Bench",
      TASK_STACK_SIZE,
      (void *)&g_result_core0,
      1,
      NULL,
      0);  // Pin to Core 0
  
  Serial.println("  Launching Core 1 task...");
  Serial.flush();
  
  BaseType_t task1 = xTaskCreatePinnedToCore(
      core1_benchmark_task,
      "Core1_Bench",
      TASK_STACK_SIZE,
      (void *)&g_result_core1,
      1,
      NULL,
      1);  // Pin to Core 1
  
  if (task0 != pdPASS || task1 != pdPASS) {
    Serial.println("  ERROR: Failed to create tasks!");
    Serial.printf("  task0=%d, task1=%d\n", task0, task1);
    Serial.flush();
    return;
  }
  
  Serial.println("  Tasks created. Waiting for completion...");
  Serial.flush();
  
  // Wait for BENCH_COUNT packets per core to complete
  uint32_t start_wait = millis();
  uint32_t max_wait = 180000;  // Increased timeout for larger payloads
  uint32_t target_count = BENCH_COUNT;
  uint32_t completion_threshold = (BENCH_COUNT * 95) / 100;  // 95% completion
  
  while (millis() - start_wait < max_wait) {
    delay(1000);
    
    // Check every second
    if (g_result_core0.count >= completion_threshold && 
        g_result_core1.count >= completion_threshold) {
      delay(2000);  // Extra buffer
      break;
    }
    
    // Print progress every 5 seconds
    if ((millis() - start_wait) % 5000 < 1000) {
      uint32_t elapsed = (millis() - start_wait) / 1000;
      Serial.printf("  [%u s] Core0: %u/%u, Core1: %u/%u\n", 
                    elapsed, g_result_core0.count, target_count,
                    g_result_core1.count, target_count);
      Serial.flush();
    }
  }
  
  if (g_result_core0.count < completion_threshold || g_result_core1.count < completion_threshold) {
    Serial.println("  WARNING: Timeout waiting for tasks. Some packets may not be encoded.");
    Serial.flush();
  }
  
  uint32_t global_time = millis() - global_start;
  
  Serial.println("  Results ready.");
  Serial.flush();
  
  // Calculate combined results
  uint32_t total_count = g_result_core0.count + g_result_core1.count;
  uint32_t total_bytes = g_result_core0.total_bytes + g_result_core1.total_bytes;
  uint32_t total_errors = g_result_core0.errors + g_result_core1.errors;
  
  float combined_pps = (total_count > 0) ? (float)total_count * 1000.0 / global_time : 0.0;
  float combined_throughput_kbs = (total_bytes > 0) ? (total_bytes / 1024.0) / (global_time / 1000.0) : 0.0;
  
  // Core 0 results
  Serial.printf("\n  ┌─ Core 0 Results\n");
  Serial.printf("  │  Packets: %u, Errors: %u\n", g_result_core0.count, g_result_core0.errors);
  float core0_pps = (g_result_core0.time_ms > 0) ? (float)g_result_core0.count * 1000.0 / g_result_core0.time_ms : 0.0;
  Serial.printf("  │  Time: %u ms, PPS: %.1f\n", g_result_core0.time_ms, core0_pps);
  Serial.flush();
  
  // Core 1 results
  Serial.printf("  ├─ Core 1 Results\n");
  Serial.printf("  │  Packets: %u, Errors: %u\n", g_result_core1.count, g_result_core1.errors);
  float core1_pps = (g_result_core1.time_ms > 0) ? (float)g_result_core1.count * 1000.0 / g_result_core1.time_ms : 0.0;
  Serial.printf("  │  Time: %u ms, PPS: %.1f\n", g_result_core1.time_ms, core1_pps);
  Serial.flush();
  
  // Combined results - use actual task times, not wall time
  uint32_t actual_time = (g_result_core0.time_ms > g_result_core1.time_ms) ? g_result_core0.time_ms : g_result_core1.time_ms;
  float actual_combined_pps = (actual_time > 0) ? (float)total_count * 1000.0 / actual_time : 0.0;
  
  Serial.printf("  └─ Combined Results\n");
  Serial.printf("     Total packets:  %u\n", total_count);
  Serial.printf("     Total errors:   %u\n", total_errors);
  Serial.printf("     Actual time:    %u ms (parallel, not wall time)\n", actual_time);
  Serial.printf("     Combined PPS:   %.1f  ← MAIN METRIC (both cores)\n", actual_combined_pps);
  Serial.printf("     Total data:     %u bytes\n", total_bytes);
  Serial.printf("     Throughput:     %.1f KB/s\n\n", combined_throughput_kbs);
  Serial.flush();
}

void test_single_vs_batch_processing() {
  Serial.println("\n[TEST 3] Single vs Batch Processing (512 packets total)");
  Serial.println("──────────────────────────────────────────────────────");
  Serial.flush();
  
  uint8_t random_data[PAYLOAD_SIZE];
  int packet_len;
  uint32_t time_ms;
  float pps;
  
  // === Mode 1: Single packet, repeated 512 times ===
  Serial.println("\n  Mode 1: Single at a time (512 × 1 packet)");
  Serial.flush();
  
  uint32_t start = millis();
  for (int i = 0; i < 512; i++) {
    for (int j = 0; j < PAYLOAD_SIZE; j++) {
      random_data[j] = (uint8_t)rand();
    }
    packet_len = osfx_packet_encode_full(
        0x3F, 0x12345678, i % 8, (uint64_t)millis() * 1000,
        random_data, PAYLOAD_SIZE, g_packet_buf_core0, BUFFER_PER_CORE);
  }
  time_ms = millis() - start;
  pps = (time_ms > 0) ? (512.0 * 1000.0 / time_ms) : 0.0;
  Serial.printf("     Time: %u ms, PPS: %.1f\n", time_ms, pps);
  Serial.flush();
  
  // === Mode 2: Batch 64 at a time (8 × 64 packets) ===
  Serial.println("\n  Mode 2: Batch mode (8 × 64 packets)");
  Serial.flush();
  
  start = millis();
  for (int batch = 0; batch < 8; batch++) {
    for (int i = 0; i < 64; i++) {
      for (int j = 0; j < PAYLOAD_SIZE; j++) {
        random_data[j] = (uint8_t)rand();
      }
      packet_len = osfx_packet_encode_full(
          0x3F, 0x87654321, (batch * 64 + i) % 8, (uint64_t)millis() * 1000,
          random_data, PAYLOAD_SIZE, g_packet_buf_core0, BUFFER_PER_CORE);
    }
    // Optional: Small yield after each batch
    yield();
  }
  time_ms = millis() - start;
  pps = (time_ms > 0) ? (512.0 * 1000.0 / time_ms) : 0.0;
  Serial.printf("     Time: %u ms, PPS: %.1f\n", time_ms, pps);
  Serial.flush();
  
  // === Analysis ===
  Serial.println("\n  Interpretation:");
  Serial.println("    If Mode 2 ≈ Mode 1: No significant batch overhead");
  Serial.println("    If Mode 1 > Mode 2: Batch mode is faster (cache?)\n");
  Serial.flush();
}

void loop() {
  // Tests complete in setup(), halt here forever
  while(1) {
    delay(10000);
  }
}
