## Multicore Cache Simulator
## Steven Guo and Mihail Alexandrov

Project URL: https://github.com/Agardragon5/Parallel-Cache-Sim 

## Summary:
Our project idea revolves around developing a multicore processor and cache simulator that can accurately model the behavior of a multi-core CPU system, including its cache hierarchy and memory coherence protocols. The primary goal is to create a tool that simulates the execution of a program across multiple cores while providing detailed statistics on cache performance, coherence overhead, stall times, and other critical metrics. 


## Background:

Modern multi-core processors rely on private per-core caches (L1/L2) and shared last-level caches (L3) to bridge the performance gap between fast CPU cores and slower main memory. However, this cache hierarchy introduces the critical challenge of maintaining cache coherence - ensuring all cores see a consistent view of shared memory. Without proper coherence mechanisms, different cores could simultaneously hold conflicting versions of the same data in their private caches, leading to incorrect program execution. 

Our simulator will model a system with multiple cores. It will implement cache coherence protocols such as MESI (Modified, Exclusive, Shared, Invalid) or MOESI to ensure data consistency across cores. Users will be able to input a program or workload, and the simulator will execute it across the cores, tracking how data moves through the cache hierarchy and how coherence protocols manage shared data. Key metrics like cache hit/miss rates, stall times due to cache coherence, memory access latency, and will be reported to help users analyze the system's performance.

If time permits, we plan to expand our investigation into different memory models for parallel systems, including the implementation of a directory-based cache coherence system. The primary objective of this project is not to develop a parallel program, but rather to create an accurate simulation model that demonstrates how parallel systems maintain cache coherence. This simulator will enable users to analyze and profile code performance under various architectural conditions and coherence protocols.









## The Challenge:

The challenge of this project comes from needing to think about parallel systems from an architectural, rather than software based perspective. Rather than assuming coherency models work correctly, we must instead design and implement the underlying mechanisms which enable them to work. Checking correctness is a significant challenge, as we don’t have any reference simulator to validate our own model against, making it necessary to create our own verification strategies. Additionally, memory accesses across multiple threads is non-deterministic, meaning that there can be multiple correct outcomes, which we must differentiate from outcomes that result from an incorrect consistency model. 

## Resources:
We intend to use Professor Railing’s Computer Architecture Design Simulator for Students (CADSS) as a framework for the project. The nature of the simulator means that we will not need any specialized hardware, and will be using the GHC machines.

## Goals and Deliverables:

To ensure a successful project, we aim to deliver the following:  

1. Functional Multi-Core Cache Simulator
   - A simulator that models a multi-core CPU system with private L1/L2 caches per core and a shared L3 cache (or main memory).  
   - Support for configurable cache parameters (size, associativity, replacement policies like LRU, FIFO, random).  
   - Accurate modeling of memory access latencies and cache hierarchy behavior.  

2. Implementation of Cache Coherence Protocols
   - Support for MSI, MESI, and MOESI coherence protocols.  
   - Correct handling of read/write operations, invalidations, and state transitions.  
   - Verification of correctness through test cases (e.g., race conditions, false sharing).  

3. Performance Metrics and Analysis
   - Detailed statistics on cache hit/miss rates, coherence overhead, and memory access latency.  
   - Measurement of stall cycles due to cache misses and coherence transactions.  
   - Comparison of different coherence protocols under varying workloads.  

4. Benchmarking and Evaluation
   - Execution of standard benchmarks to evaluate performance.  
   - Analysis of how different protocols affect throughput, latency, and scalability.  






Hope to Achieve (Stretch Goals) 
If the project progresses ahead of schedule, we aim to extend the simulator with:  

1. Directory-Based Coherence Protocol
   - Implementation of a directory-based protocol to compare with snooping-based approaches (MESI, MOESI).  
   - Analysis of scalability for larger core counts (e.g., 8+ cores).  

Expected Performance & Analysis Outcomes  
We aim to answer the following key questions:  
- How does the choice of coherence protocol impact performance?  
- What are the bottlenecks in multi-core cache systems?  

By the end of the project, we will have a fully functional simulator capable of providing insights into multi-core caching behavior, along with quantitative comparisons of different coherence mechanisms.  

Our final goal for the project is to have a fully implemented multicore coherent caching system supporting MSI, MESI, MOESI, MESIF cache coherence protocols and benchmarking their performance against one another. If time permits we plan to explore how other cache coherence protocols, such as directory-based, may impact performance. 

## Platform Choice:
Using the CADSS framework is a good choice as implementing an entire processor is out of scope for the nature of this project, which intends to focus on specifically parallel systems. By using this starting point, we can spend more time working on the main focus of this project, which is investigating different coherency protocols, rather than designing a full processor from scratch.














## Schedule:

Week 1 (3/31 - Carnival Week)
Complete architectural design document:
Define message types and bus transaction protocol
Create detailed FSM diagrams for MSI/MESI
Set up CADSS simulation infrastructure:
Understanding the existing simulator for coherence tracking
Implement basic statistics collection framework
Deliverable: Design doc 

Week 2 (4/7)
Core protocol implementation:
Complete MSI baseline (all state transitions)
Implement bus snooping logic with atomic operations
Add stall cycle accounting for coherence operations
Verification: 
Create basic test cases
Deliverable: Working MSI prototype with basic validation

Week 3 (4/14)
Protocol enhancement:
Upgrade to MESI (add Exclusive state)
Implement silent upgrades (M→E transitions)
Deliverable: Validated MESI implementation with performance stats

Week 4 (4/21)
Advanced features (priority order):
MOESIF extension
Directory protocol skeleton (basic implementation)
Comprehensive evaluation:
Compare protocols using CPI and bandwidth metrics
Generate final performance reports
Deliverable: Final system with comparative analysis



