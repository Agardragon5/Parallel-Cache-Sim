#!/bin/bash

# Configuration
CONFIG="ex_coher.config"
CACHE="simpleCache"
PROCESSOR="refProcessor"
BRANCH="refBranch"
MEMORY="refMemory"
TRACE_ROOT="traces/coher/"

compare_outputs() {
    local trace_file=$1
    local trace_name=$(basename "$trace_file")
    
    echo "=== Testing: $trace_name ==="
    
    # Run reference implementation
    echo "Running REFERENCE (with refCoherence)..."
    ref_output=$(./cadss-engine -s "$CONFIG" -c "$CACHE" -t "$trace_file" \
                -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" -o "refCoherence" -v)
    
    # Run custom implementation
    echo "Running CUSTOM (without refCoherence)..."
    custom_output=$(./cadss-engine -s "$CONFIG" -c "$CACHE" -t "$trace_file" \
                  -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" -v)
    
    # Compare outputs
    echo "=== DIFFERENCES ==="
    diff -y <(echo "$ref_output") <(echo "$custom_output") \
    --suppress-common-lines --ignore-trailing-space --ignore-space-change \
    --ignore-blank-lines| head -n 20
    echo "=================="
    echo ""
}

# Main execution loop
for protocol_dir in "$TRACE_ROOT"/*/; do
    protocol_name=$(basename "$protocol_dir")
    echo "==== PROTOCOL: $protocol_name ===="
    
    for trace_file in "$protocol_dir"/*.trace; do
        compare_outputs "$trace_file"
    done
done