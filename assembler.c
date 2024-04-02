/**
 * Project 1
 * Assembler code fragment for LC-2K
 */
//problem: 1. 为什么有一个output不对， 2. 所有的都要用fprintf吗，如果用的话，outputfile还没define怎么办



#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);

int findLabelAddress(const char* searchLabel,int num_labels, const char labels[][7], FILE* outFilePtr) {
    for (int i = 0; i < num_labels; i++) {
        if (strcmp(labels[i], searchLabel) == 0) {
            return i;
        }
    }
    fprintf(outFilePtr, "error: undefined label.\n");  // Label not found
    exit(1);
}

void checkNonInteger(char*reg1, char*reg2,FILE* outFilePtr){
    if (!isNumber(reg1) || !isNumber(reg2)){
        fprintf(outFilePtr, "error: Non-integer register detected.\n");
        exit(1);
    }
}

void checkNonInteger_forR(char*reg1, char*reg2, char*destR, FILE* outFilePtr){
    if (!isNumber(reg1) || !isNumber(reg2) || !isNumber(destR)){
        fprintf(outFilePtr, "error: Non-integer register detected.\n");
        exit(1);
    }
}
void checkRegRange (int reg1, int reg2, FILE* outFilePtr){
    if (reg1 < 0 || reg1 > 7 || reg2 < 0 || reg2 > 7) {
        fprintf(outFilePtr, "Error: Register outside the range [0, 7]\n");
        exit(1);
    }
}
void checkRegRange_forR (int reg1, int reg2, int destR, FILE* outFilePtr){
    if (reg1 < 0 || reg1 > 7 || reg2 < 0 || reg2 > 7 || destR < 0 || destR > 7) {
        fprintf(outFilePtr, "Error: Register outside the range [0, 7]\n");
        exit(1);
    }
}

int
main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }


    // PASS 1
    char labels[MAXLINELENGTH][7];
    int address = 0;
    for (int i = 0; i < MAXLINELENGTH; i++) {
        labels[i][0] = '\0';
    }
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if (label[0] != '\0') {
            strcpy(labels[address], label);
        }
        address++;
    }

    // check for dulipcate labels
    for (int i = 0; i < MAXLINELENGTH; ++i){
        for (int j = i + 1; j < MAXLINELENGTH; ++j){
            if (labels[i][0] == '\0') continue;
            else if(!strcmp(labels[i], labels[j])){
                fprintf(outFilePtr, "error: duplicate label\n");
                exit(1);
            }
        }
    }


    //PASS 2
    rewind(inFilePtr);

    int machinecode = 0;
    int offset = 0;
    int programcounter = 0;
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        int reg1, reg2, destReg;
        if (!strcmp(opcode, "add")){
            checkNonInteger_forR(arg0, arg1, arg2, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            destReg = atoi(arg2);
            checkRegRange_forR(reg1, reg2, destReg, outFilePtr);
            machinecode = 0;
            machinecode = (machinecode << 3) | reg1;
            machinecode = (machinecode << 3) | reg2;
            machinecode = (machinecode << 16) | destReg;
        }
        else if (!strcmp(opcode, "nor")){
            checkNonInteger_forR(arg0, arg1, arg2, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            destReg = atoi(arg2);
            checkRegRange_forR(reg1, reg2, destReg, outFilePtr);
            machinecode = 1;
            machinecode = (machinecode << 3) | reg1;
            machinecode = (machinecode << 3) | reg2;
            machinecode = (machinecode << 16) | destReg;
        }
        else if (!strcmp(opcode, "lw")){
            checkNonInteger(arg0, arg1, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            checkRegRange(reg1, reg2, outFilePtr);
            offset = atoi(arg2);
            machinecode = 1;
            machinecode = (machinecode << 4) | reg1;
            machinecode = (machinecode << 3) | reg2;
            if (isNumber(arg2)){
                if (offset < -32768 || offset > 32767){
                    fprintf( outFilePtr, "offsetField is too big.\n");
                    exit(1);
                }
                offset = offset & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }
            else {
                offset = findLabelAddress(arg2, address, labels, outFilePtr);
                offset = offset & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }
        }
        else if (!strcmp(opcode, "sw")){
            checkNonInteger(arg0, arg1, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            checkRegRange(reg1, reg2, outFilePtr);
            offset = atoi(arg2);
            machinecode = 3;
            machinecode = (machinecode << 3) | reg1;
            machinecode = (machinecode << 3) | reg2;
            if (isNumber(arg2)){
                if (offset < -32768 || offset > 32767){
                    fprintf(outFilePtr, "offsetField is too big.\n");
                    exit(1);
                }
                offset = offset & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }
            else {
                offset = findLabelAddress(arg2, address, labels, outFilePtr);
                offset = offset & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }
        }
        else if (!strcmp(opcode, "beq")){
            checkNonInteger(arg0, arg1, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            checkRegRange(reg1, reg2, outFilePtr);
            offset = atoi(arg2);
            machinecode = 1;
            machinecode = (machinecode << 5) | reg1;
            machinecode = (machinecode << 3) | reg2;
            if (isNumber(arg2)){
                if (offset < -32768 || offset > 32767){
                    fprintf(outFilePtr, "offsetField is too big.\n");
                    exit(1);
                }
                offset = offset & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }
            else {
                int labeladdress = findLabelAddress(arg2, address, labels, outFilePtr);
                offset = (labeladdress - 1 - programcounter) & 0xFFFF;
                machinecode = (machinecode << 16) | offset;
            }

        }
        else if (!strcmp(opcode, "jalr")){
            checkNonInteger(arg0, arg1, outFilePtr);
            reg1 = atoi(arg0);
            reg2 = atoi(arg1);
            checkRegRange(reg1, reg2, outFilePtr);
            machinecode = 5;
            machinecode = (machinecode << 3) | reg1;
            machinecode = (machinecode << 3) | reg2;
            machinecode = machinecode << 16;
            
        }
        else if (!strcmp(opcode, "halt")){
            machinecode = 6;
            machinecode = machinecode << 22;
            
        }
        else if (!strcmp(opcode, "noop")){
            machinecode = 7;
            machinecode = machinecode << 22;
            
        }
        else if (!strcmp(opcode, ".fill")){
            machinecode = 0;
            if (isNumber(arg0)){
                machinecode = atoi(arg0);
            }
            else {
                machinecode = findLabelAddress(arg0, address, labels, outFilePtr);
            }
        }
        else {
            fprintf(outFilePtr, "error: you have a wrong opcode: %s\n", opcode);
            exit(1);
        }
        programcounter++;
        fprintf(outFilePtr, "%d\n", machinecode);

    }

    // /* here is an example for how to use readAndParse to read a line from
    //     inFilePtr */
    // if (! readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2) ) {
    //     /* reached end of file */
    // }

    // /* this is how to rewind the file ptr so that you start reading from the
    //     beginning of the file */
    // rewind(inFilePtr);

    // /* after doing a readAndParse, you may want to do the following to test the
    //     opcode */
    // if (!strcmp(opcode, "add")) {
    //     /* do whatever you need to do for opcode "add" */
    // }
    return(0);
}


// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line) {
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for(int line_idx=0; line_idx < strlen(line); ++line_idx) {
        int line_char_is_whitespace = 0;
        for(int whitespace_idx = 0; whitespace_idx < 4; ++ whitespace_idx) {
            if(line[line_idx] == whitespace[whitespace_idx]) {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if(!line_char_is_whitespace) {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr) {
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for(int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address) {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH-1) {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if(lineIsBlank(line)) {
            if(!blank_line_encountered) {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        } else {
            if(blank_line_encountered) { 
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}


/*
* NOTE: The code defined below is not to be modifed as it is implimented correctly.
*/

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
	/* reached end of file */
        return(0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
	printf("error: line too long\n");
	exit(1);
    }

    // Ignore blank lines at the end of the file.
    if(lineIsBlank(line)) {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
	/* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);

    return(1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return((sscanf(string, "%d%c",&num, &c)) == 1);  // 如果string = num + char， return 0， 如果只有一个match， return
}

