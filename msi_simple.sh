#!/bin/bash

# Configuration (edit these if needed)
CONFIG="ex_coher.config"
CACHE="simpleCache"
PROCESSOR="refProcessor"
BRANCH="refBranch"
MEMORY="refMemory"
COHERENCE="refCoherence"
TRACE_ROOT="traces/coher"  # Parent folder containing simple/, msi_simple/, etc.

echo "Running all coherence protocol tests..."

# Outer loop: Go through each protocol directory (simple/, msi_simple/, etc.)
for protocol_dir in "$TRACE_ROOT"/*/; do
    protocol_name=$(basename "$protocol_dir")
    echo "=== Testing protocol: $protocol_name ==="
    
    # Inner loop: Run each trace file in the current protocol directory
    for trace_file in "$protocol_dir"/*.trace; do
        trace_name=$(basename "$trace_file")
        echo "Running trace: $trace_name"
        
        ./cadss-engine -s "$CONFIG" -c "$CACHE" -t "$trace_file" \
                      -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" \
                      -o "$COHERENCE" -v
        
        echo "-----"
    done
done

echo "All tests completed!"