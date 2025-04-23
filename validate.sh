#!/bin/bash

# Configuration
CONFIG="ex_coher.config"
NUM_PROCESSORS="4" 
CACHE="simpleCache"
PROCESSOR="refProcessor"
BRANCH="refBranch"
MEMORY="refMemory"
TRACE_ROOT="traces/coher/mesif/temp" 
compare_outputs() {
    local protocol_dir=$1

    # Run reference implementation
    echo "Running REFERENCE (with refCoherence)..."
    ref_output=$(./cadss-engine -n "$NUM_PROCESSORS" -s "$CONFIG" -c "$CACHE" -t "$protocol_dir" \
                -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" -o "refCoherence" -v)
    
    # Run custom implementation
    echo "Running CUSTOM (without refCoherence)..."
    custom_output=$(./cadss-engine -n "$NUM_PROCESSORS" -s "$CONFIG" -c "$CACHE" -t "$protocol_dir" \
                  -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" -v)
    
    # Compare outputs
    echo "=== DIFFERENCES ==="
    diff -y <(echo "$ref_output") <(echo "$custom_output") \
    --ignore-trailing-space --ignore-space-change \
    --ignore-blank-lines| head -n 30
    echo "=================="
    echo ""
}

# Main execution loop
for protocol_dir in "$TRACE_ROOT"/*/; do
    protocol_name=$(basename "$protocol_dir")
    echo "==== PROTOCOL: $protocol_name ===="
    compare_outputs $protocol_dir 
done