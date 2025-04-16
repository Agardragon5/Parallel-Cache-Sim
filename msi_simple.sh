#!/bin/bash
#This is the bash script to run all traces located in traces/coher 

# Configuration 
# These are the parameters you can modify to change what parts you're running 
# with. To run the different coherence protocols, you need to modify the 
# ex_coher.config file with the number next to __coherence -s num. Num can be 
# a value 0-4 where 0 is MI, 1 is MSI, 2 is MESI, 3 is MOESI, and 4 is MESIF. 
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
        
        #Actual script to run (for individual tests)
        #Ex: ./cadss-engine -s ex_coher.config -c simpleCache -p refProcessor
        # -t traces/coher/msi_multiple_lines/p2.trace -m refMemory -b refBranch -v 
        # can use -h for more info 
        ./cadss-engine -s "$CONFIG" -c "$CACHE" -t "$trace_file" \
                      -p "$PROCESSOR" -b "$BRANCH" -m "$MEMORY" \
                      -o "$COHERENCE" \
                      -v 
                      #delete the -o "$COHERENCE line if you don't want to run with reference"
        
        echo "-----"
    done
done

echo "All tests completed!"