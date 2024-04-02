/*
 * EECS 370, University of Michigan, Fall 2023
 * Project 3: LC-2K Pipeline Simulator
 * Instructions are found in the project spec: https://eecs370.github.io/project_3_spec/
 * Make sure NOT to modify printState or any of the associated functions
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 // maximum number of data words in memory
#define NUMREGS 8 // number of machine registers

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 // will not implemented for Project 3
#define HALT 6
#define NOOP 7

const char* opcode_to_str_map[] = {
    "add",
    "nor",
    "lw",
    "sw",
    "beq",
    "jalr",
    "halt",
    "noop"
};

#define NOOPINSTR (NOOP << 22)

typedef struct IFIDStruct {
	int pcPlus1;
	int instr;
} IFIDType;

typedef struct IDEXStruct {
	int pcPlus1;
	int valA;
	int valB;
	int offset;
	int instr;
} IDEXType;

typedef struct EXMEMStruct {
	int branchTarget;
    int eq;
	int aluResult;
	int valB;
	int instr;
} EXMEMType;

typedef struct MEMWBStruct {
	int writeData;
    int instr;
} MEMWBType;

typedef struct WBENDStruct {
	int writeData;
	int instr;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	unsigned int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	unsigned int cycles; // number of cycles run so far
} stateType;

static inline int opcode(int instruction) {
    return instruction>>22;
}

static inline int field0(int instruction) {
    return (instruction>>19) & 0x7;
}

static inline int field1(int instruction) {
    return (instruction>>16) & 0x7;
}

static inline int field2(int instruction) {
    return instruction & 0xFFFF;
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) {
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

void printState(stateType*);
void printInstruction(int);
void readMachineCode(stateType*, char*);
char check_datahazard(int current_instr, int last_instr);


int main(int argc, char *argv[]) {

    /* Declare state and newState.
       Note these have static lifetime so that instrMem and
       dataMem are not allocated on the stack. */

    static stateType state, newState;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    readMachineCode(&state, argv[1]);

    // Initialize state here
    // initialize all pipeline registers to the noop instruction ( 0x1c00000)
    state.IFID.instr = NOOPINSTR;
    state.IDEX.instr = NOOPINSTR;
    state.EXMEM.instr = NOOPINSTR;
    state.MEMWB.instr = NOOPINSTR;
    state.WBEND.instr = NOOPINSTR;
    state.cycles = 0;
    state.pc = 0;


    newState = state;
    int current_instr = 0;
    int last_instr = 0;
    while (opcode(state.MEMWB.instr) != HALT) {
        printState(&state);

        newState.cycles += 1;

        /* ---------------------- IF stage --------------------- */
        // stores instruction bits and PC + 1 into IF/ID register
        newState.IFID.pcPlus1 = state.pc + 1;
        newState.IFID.instr = state.instrMem[state.pc];
        current_instr = newState.IFID.instr;
        newState.pc++;
        
        /* ---------------------- ID stage --------------------- */
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
        newState.IDEX.valA = state.reg[field0(state.IFID.instr)];
        newState.IDEX.valB = state.reg[field1(state.IFID.instr)];
        newState.IDEX.offset = convertNum(field2(state.IFID.instr));
        newState.IDEX.instr = state.IFID.instr;
        // if (opcode(state.IFID.instr) == LW)
// int pcPlus1;
// 	int valA;
// 	int valB;
// 	int offset;
// 	int instr;
        /* ---------------------- EX stage --------------------- */
        newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;  // for beq target
        newState.EXMEM.instr = state.IDEX.instr;
        current_instr = newState.EXMEM.instr;
        newState.EXMEM.valB = state.IDEX.valB;

        //check data hazard

        //如果是LW， check hazard，需要insert noop
        if (opcode(current_instr) == LW) {
            //要和前面一个比较
            if (opcode(state.IFID.instr) == ADD || opcode(state.IFID.instr) == NOR || opcode(state.IFID.instr) == BEQ){
                if (field1(current_instr) == field0(state.IFID.instr) || field1(current_instr) == field1(state.IFID.instr)){
                    newState.IFID.instr = state.IFID.instr;
                    newState.IFID.pcPlus1 = state.IFID.pcPlus1;
                    newState.pc--;
                    newState.IDEX.instr = NOOPINSTR;
                }
            }
            else if (opcode(state.IFID.instr) == LW && field1(current_instr) == field0(state.IFID.instr)) {  // regA
                newState.IFID.instr = state.IFID.instr;
                newState.IFID.pcPlus1 = state.IFID.pcPlus1;
                newState.pc--;
                newState.IDEX.instr = NOOPINSTR;
            }
            else if (opcode(state.IFID.instr) == SW && field1(current_instr) == field1(state.IFID.instr)) {   // regB
                newState.IFID.instr = state.IFID.instr;
                newState.IFID.pcPlus1 = state.IFID.pcPlus1;
                newState.pc--;
                newState.IDEX.instr = NOOPINSTR;
            }
        }

        // check point, assume there is no hazard.
        // if (opcode(state.IDEX.instr) == LW || opcode(state.IDEX.instr) == SW){
        //     newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.offset;
        // }
        // else if (opcode(state.IDEX.instr) == ADD){
        //     newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
        // }
        // else if (opcode(state.IDEX.instr) == NOR){
        //     newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
        // }
        // else if (opcode(state.IDEX.instr) == BEQ){
        //     newState.EXMEM.eq = (state.IDEX.valA == state.IDEX.valB) ? 1 : 0;
        // }

        // 如果是第一个ins 是add nor， 看下一个ins ，如果不是lw，说明要用到
        int new_regA = state.IDEX.valA;
        int new_regB = state.IDEX.valB;
        if (opcode(current_instr) == ADD || opcode(current_instr) == NOR || 
        opcode(current_instr) == LW || opcode(current_instr) == SW || opcode(current_instr) == BEQ){
            // 看 WB 的 hazard
            if (opcode(state.WBEND.instr) == LW || opcode(state.WBEND.instr) == ADD || opcode(state.WBEND.instr) == NOR){
                last_instr = state.WBEND.instr;
                if (check_datahazard(current_instr, last_instr) == 'A'){
                    new_regA = state.WBEND.writeData;
                }
                else if (check_datahazard(current_instr, last_instr) == 'B'){
                    new_regB = state.WBEND.writeData;
                }
                else if (check_datahazard(current_instr, last_instr) == 'O'){
                    new_regA = state.WBEND.writeData;
                    new_regB = state.WBEND.writeData;
                }
            }
            // 在看 MEM 的 lw  hazard
            if (opcode(state.MEMWB.instr) == LW || opcode(state.MEMWB.instr) == ADD || opcode(state.MEMWB.instr) == NOR){
                last_instr = state.MEMWB.instr;
                if (check_datahazard(current_instr, last_instr) == 'A'){
                    new_regA = state.MEMWB.writeData;
                }
                else if (check_datahazard(current_instr, last_instr) == 'B'){
                    new_regB = state.MEMWB.writeData;
                }
                else if (check_datahazard(current_instr, last_instr) == 'O'){
                    new_regA = state.MEMWB.writeData;
                    new_regB = state.MEMWB.writeData;
                }
            }
            // 先看EX/MEM 的 instr 有没有hazard
            if (opcode(state.EXMEM.instr) == ADD || opcode(state.EXMEM.instr) == NOR){
                    last_instr = state.EXMEM.instr;
                if (check_datahazard(current_instr, last_instr) == 'A'){
                    new_regA = state.EXMEM.aluResult;
                }
                else if (check_datahazard(current_instr, last_instr) == 'B'){
                    new_regB = state.EXMEM.aluResult;
                }
                else if (check_datahazard(current_instr, last_instr) == 'O'){
                    new_regA = state.EXMEM.aluResult;
                    new_regB = state.EXMEM.aluResult;
                }
            }
            if (opcode(current_instr) == ADD) newState.EXMEM.aluResult = new_regA + new_regB;
            if (opcode(current_instr) == NOR) newState.EXMEM.aluResult = ~(new_regA | new_regB);
            if (opcode(current_instr) == BEQ) newState.EXMEM.eq = (new_regA == new_regB) ? 1 : 0;
            if (opcode(current_instr) == LW) newState.EXMEM.aluResult = new_regA + state.IDEX.offset;
            if (opcode(current_instr) == SW) {
                newState.EXMEM.aluResult = new_regA + state.IDEX.offset;
                newState.EXMEM.valB = new_regB;
            }
    
            
            
        }

    
 
// int branchTarget;
//  int eq;
// 	int aluResult;
// 	int valB;
// 	int instr;

        /* --------------------- MEM stage --------------------- */
        newState.MEMWB.instr = state.EXMEM.instr;
        
        if (opcode(state.EXMEM.instr) == LW) {
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        }
        else if (opcode(state.EXMEM.instr) == SW) {
            newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.valB;
        }
        // control hazard
        else if (opcode(state.EXMEM.instr) == BEQ) {
            if (state.EXMEM.eq == 1) {
                newState.pc = state.EXMEM.branchTarget;
                newState.EXMEM.instr = NOOPINSTR;
                newState.IDEX.instr = NOOPINSTR;
                newState.IFID.instr = NOOPINSTR;
            }
        }
        else if (opcode(state.EXMEM.instr) == ADD || opcode(state.EXMEM.instr) == NOR){
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }

        /* ---------------------- WB stage --------------------- */
        newState.WBEND.instr = state.MEMWB.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;
        if (opcode(state.MEMWB.instr) == ADD ||opcode(state.MEMWB.instr) == NOR){
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        else if (opcode(state.MEMWB.instr) == LW) {
            newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
        }

        /* ------------------------ END ------------------------ */
        state = newState; /* this is the last statement before end of the loop. It marks the end
        of the cycle and updates the current state with the values calculated in this cycle */
    }
    printf("Machine halted\n");
    printf("Total of %d cycles executed\n", state.cycles);
    printf("Final state of machine:\n");
    printState(&state);
}

/*
* DO NOT MODIFY ANY OF THE CODE BELOW.
*/
char check_datahazard(int current_instr, int last_instr){
    if (opcode(current_instr) == ADD || opcode(current_instr) == NOR || opcode(current_instr) == BEQ || opcode(current_instr) == SW){
        if (opcode(last_instr) == ADD || opcode(last_instr) == NOR){
            if (field2(last_instr) == field0(current_instr) && field2(last_instr) != field1(current_instr)) return 'A';
            if (field2(last_instr) == field1(current_instr) && field2(last_instr) != field0(current_instr)) return 'B';
            if (field2(last_instr) == field1(current_instr) && field2(last_instr) == field0(current_instr)) return 'O';
        }
        else if (opcode(last_instr) == LW){
            if (field1(last_instr) == field0(current_instr) && field1(last_instr) != field1(current_instr)) return 'A';
            if (field1(last_instr) == field1(current_instr) && field1(last_instr) != field0(current_instr)) return 'B';
            if (field1(last_instr) == field1(current_instr) && field1(last_instr) == field0(current_instr)) return 'O';
        }
    }
    else if (opcode(current_instr) == LW){
        if (opcode(last_instr) == ADD || opcode(last_instr) == NOR){
            if (field2(last_instr) == field0(current_instr)) return 'A';
        }
        else if (opcode(last_instr) == LW){
            if (field1(last_instr) == field0(current_instr)) return 'A';
        }
    }
    return 'N';
}

void printInstruction(int instr) {
    const char* instr_opcode_str;
    int instr_opcode = opcode(instr);
    if(ADD <= instr_opcode && instr_opcode <= NOOP) {
        instr_opcode_str = opcode_to_str_map[instr_opcode];
    }

    switch (instr_opcode) {
        case ADD:
        case NOR:
        case LW:
        case SW:
        case BEQ:
            printf("%s %d %d %d", instr_opcode_str, field0(instr), field1(instr), convertNum(field2(instr)));
            break;
        case JALR:
            printf("%s %d %d", instr_opcode_str, field0(instr), field1(instr));
            break;
        case HALT:
        case NOOP:
            printf("%s", instr_opcode_str);
            break;
        default:
            printf(".fill %d", instr);
            return;
    }
}

void printState(stateType *statePtr) {
    printf("\n@@@\n");
    printf("state before cycle %d starts:\n", statePtr->cycles);
    printf("\tpc = %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (int i=0; i<statePtr->numMemory; ++i) {
        printf("\t\tdataMem[ %d ] = %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (int i=0; i<NUMREGS; ++i) {
        printf("\t\treg[ %d ] = %d\n", i, statePtr->reg[i]);
    }

    // IF/ID
    printf("\tIF/ID pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IFID.instr);
    printInstruction(statePtr->IFID.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IFID.pcPlus1);
    if(opcode(statePtr->IFID.instr) == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");

    // ID/EX
    int idexOp = opcode(statePtr->IDEX.instr);
    printf("\tID/EX pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IDEX.instr);
    printInstruction(statePtr->IDEX.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IDEX.pcPlus1);
    if(idexOp == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegA = %d", statePtr->IDEX.valA);
    if (idexOp >= HALT || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->IDEX.valB);
    if(idexOp == LW || idexOp > BEQ || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\toffset = %d", statePtr->IDEX.offset);
    if (idexOp != LW && idexOp != SW && idexOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // EX/MEM
    int exmemOp = opcode(statePtr->EXMEM.instr);
    printf("\tEX/MEM pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->EXMEM.instr);
    printInstruction(statePtr->EXMEM.instr);
    printf(" )\n");
    printf("\t\tbranchTarget %d", statePtr->EXMEM.branchTarget);
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\teq ? %s", (statePtr->EXMEM.eq ? "True" : "False"));
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\taluResult = %d", statePtr->EXMEM.aluResult);
    if (exmemOp > SW || exmemOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->EXMEM.valB);
    if (exmemOp != SW) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // MEM/WB
	int memwbOp = opcode(statePtr->MEMWB.instr);
    printf("\tMEM/WB pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->MEMWB.instr);
    printInstruction(statePtr->MEMWB.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->MEMWB.writeData);
    if (memwbOp >= SW || memwbOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // WB/END
	int wbendOp = opcode(statePtr->WBEND.instr);
    printf("\tWB/END pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->WBEND.instr);
    printInstruction(statePtr->WBEND.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->WBEND.writeData);
    if (wbendOp >= SW || wbendOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    printf("end state\n");
    fflush(stdout);
}

// File
#define MAXLINELENGTH 1000 // MAXLINELENGTH is the max number of characters we read

void readMachineCode(stateType *state, char* filename) {
    char line[MAXLINELENGTH];
    FILE *filePtr = fopen(filename, "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", filename);
        exit(1);
    }

    printf("instruction memory:\n");
    for (state->numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; ++state->numMemory) {
        if (sscanf(line, "%d", state->instrMem+state->numMemory) != 1) {
            printf("error in reading address %d\n", state->numMemory);
            exit(1);
        }
        printf("\tinstrMem[ %d ]\t= 0x%08x\t= %d\t= ", state->numMemory, 
            state->instrMem[state->numMemory], state->instrMem[state->numMemory]);
        printInstruction(state->dataMem[state->numMemory] = state->instrMem[state->numMemory]);
        printf("\n");
    }
}
