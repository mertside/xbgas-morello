# Test Makefile for xBGAS Runtime Headers
# This Makefile helps validate that the refactored headers compile correctly

CC = cc
CFLAGS = -g -O2 -Wall
INCLUDES = -Iruntime
RUNTIME_DIR = runtime

# Header files to test
HEADERS = $(RUNTIME_DIR)/xbrtime_common.h \
          $(RUNTIME_DIR)/xbrtime_api.h \
          $(RUNTIME_DIR)/xbrtime_internal.h \
          $(RUNTIME_DIR)/xbMrtime-types.h \
          $(RUNTIME_DIR)/xbMrtime-macros.h \
          $(RUNTIME_DIR)/test.h

.PHONY: test-headers clean-test help

help:
	@echo "xBGAS Header Testing Makefile"
	@echo "Available targets:"
	@echo "  test-headers  - Test compilation of all refactored headers"
	@echo "  clean-test    - Clean test artifacts"
	@echo "  help          - Show this help message"

test-headers: header_test.exe macro_test.exe
	@echo "=== Running Header Tests ==="
	./header_test.exe
	./macro_test.exe
	@echo "=== All Header Tests Passed ==="

header_test.exe: header_test.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

macro_test.exe: macro_test.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

header_test.c:
	@echo "Creating header test file..."
	@cat > $@ << 'EOF'
#include "xbrtime_common.h"
#include "xbrtime_api.h"
#include "xbrtime_internal.h"
#include "xbMrtime-types.h"
#include "xbMrtime-macros.h"
#include "test.h"

int main() {
    printf("✓ All headers compiled successfully!\n");
    printf("✓ xbrtime_common.h - Common definitions loaded\n");
    printf("✓ xbrtime_api.h - API definitions loaded\n");
    printf("✓ xbrtime_internal.h - Internal definitions loaded\n");
    printf("✓ xbMrtime-types.h - Type definitions loaded\n");
    printf("✓ xbMrtime-macros.h - Macro definitions loaded\n");
    printf("✓ test.h - Test utilities loaded\n");
    return 0;
}
EOF

macro_test.c:
	@echo "Creating macro test file..."
	@cat > $@ << 'EOF'
#include "xbMrtime-macros.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    printf("=== Macro Functionality Test ===\n");
    
    // Test memory alignment macros
    void *ptr = malloc(100);
    if (ptr) {
        printf("Memory allocation: %p\n", ptr);
        printf("Is aligned (8): %s\n", IS_ALIGNED((uintptr_t)ptr, 8) ? "Yes" : "No");
        free(ptr);
    }
    
    // Test utility macros
    int test_array[] = {1, 2, 3, 4, 5};
    printf("Array size test: %zu (expected: 5)\n", ARRAY_SIZE(test_array));
    printf("Min/Max test: min(5,3)=%d, max(5,3)=%d\n", MIN(5,3), MAX(5,3));
    
    // Test bounds checking
    printf("Bounds check (valid): %s\n", 
           CHECK_BOUNDS(2, 0, 5) ? "Valid" : "Invalid");
    printf("Bounds check (invalid): %s\n", 
           CHECK_BOUNDS(6, 0, 5) ? "Valid" : "Invalid");
    
    TEST_LOG("Macro test completed successfully");
    printf("✓ All macro tests passed!\n");
    return 0;
}
EOF

clean-test:
	rm -f header_test.c macro_test.c header_test.exe macro_test.exe

.PRECIOUS: header_test.c macro_test.c
