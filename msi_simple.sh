#!/bin/bash
#This is the bash script to run all traces located in traces/coher 

# Configuration 
# These are the parameters you can modify to change what parts you're running 
# with. To run the different coherence protocols, you need to modify the 
# ex_coher.config file with the number next to __coherence -s num. Num can be 
# a value 0-4 where 0 is MI, 1 is MSI, 2 is MESI, 3 is MOESI, and 4 is MESIF. 
CONFIG="ex_coher.config"
NUM_PROCESSORS="3" 
CACHE="simpleCache"
PROCESSOR="refProcessor"
BRANCH="refBranch"
MEMORY="refMemory"
COHERENCE="refCoherence"
TRACE_ROOT="traces/coher/msi/temp/" 
 # Parent folder containing simple/, msi_simple/, etc.

echo "Running all coherence protocol tests..."

# Outer loop: Go through each protocol directory (simple/, msi_simple/, etc.)
for protocol_dir in "$TRACE_ROOT"/*/; do
    protocol_name=$(basename "$protocol_dir")
    echo "=== Testing protocol: $protocol_name ==="
    ./cadss-engine -n "$NUM_PROCESSORS" -s "$CONFIG" -c "$CACHE" -t "$protocol_dir" \
                    -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" \
                    -o "$COHERENCE" \
                    -v 
    echo "-----"
    # done
done

echo "All tests completed!"