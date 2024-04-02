# Pipelined-Datapath-Simulator
This project is intended to understand in detail how a pipelined implementation works. I write a cycle-accurate behavioral simulator for a pipelined implementation of the LC-2K, complete with data forwarding and simple branch prediction. We have 5 pipeline register in this design: IF/ID, ID/EX, EX/MEM, MEM/WB, and WB/END. 
You will not implement the jalr instruction from LC2K. Taking out jalr eliminates several dependencies. No submitted test cases should include jalr.
At what point does the pipelined computer know to stop? It’s incorrect to stop as soon as a halt instruction is fetched because if an earlier branch was actually taken, then the halt would be squashed.
In the example below, beq 0 0 start will always branch to the start label. However, the halt instruction will enter our pipeline, as we don’t resolve branches until the MEM stage.
start   ...
        beq     0       0       start
done    halt
To solve this problem, the starter code stops when the halt instruction reaches the MEMWB register. This ensures that previously executed instructions have completed, and it also ensures that the machine won’t branch around this halt instruction. Note how the final printState() call will print the final state of the machine before the check for a halt instruction.

3.3: There are two types of data hazards that will need to be handled:
              Data hazards that do not involve stalling, and can be resolved using data forwarding.
              Data hazards that involve stalling, and still need forwarding

3.4:Use data forwarding to resolve most data hazards. The ALU should be able to take its inputs from any pipeline register (instead of just the IDEX register). To account for a lack of internal forwarding within the register file, you’ll instead forward data from the new WBEND pipeline register. Remember to take the most recent data (e.g., data in the EXMEM register gets priority over data in the MEMWB register). ONLY FORWARD DATA TO THE EX STAGE (not to memory).

3.5 Control Hazard: Predict branch-not-taken to speculate on branches, and decide whether or not to take the branch in the MEM stage. This requires you to discard instructions if it turns out that the branch prediction was incorrect. To discard instructions, change the relevant instructions in the pipeline to the noop instruction (0x1c00000). Do not use any other branch optimizations, e.g., resolving branches earlier, more advanced branch prediction, or special handling for short forward branches.
