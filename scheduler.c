#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFINITION_FILE "definition.txt"

typedef struct {
    char name[10];
    int executionTime;
} Instruction;

typedef struct {
    char name[10];
    int priority;
    int arrivalTime;
    char type[10];
    int programCounter;
    int nofExecution;
    int ifInCPU;
    int instructionCount;
    int readyTime;
    int finishTime;
    Instruction instructions[16];
} Process;

Instruction instructions[64];
Process processes[64];
int processCount;


// reads the instructions from the file and initializes the instruction structs
int readInstructionsFromFile() {
    FILE *file = fopen("instructions.txt", "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return -1;
    }

    char line[100];
    int numInstructions = 0;

    while (fgets(line, sizeof(line), file)) {
        Instruction instruction;
        sscanf(line, "%s %d", instruction.name, &instruction.executionTime);
        instructions[numInstructions] = instruction;
        numInstructions++;
    }
    fclose(file);
    return numInstructions;
}

// reads the definition file and initializes the process structs
int readProcessesFromFile() {
    FILE *file = fopen(DEFINITION_FILE, "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return -1;
    }

    char line[100];
    int numProcesses = 0;

    // Read the file line by line and create the process structs
    while (fgets(line, sizeof(line), file)) {
        Process process;
        memset(process.instructions, 0, sizeof(process.instructions));
        process.programCounter = 0;
        process.nofExecution = 0;
        process.ifInCPU = 0;
        process.finishTime = -1;
        sscanf(line, "%s %d %d %s", process.name, &process.priority, &process.arrivalTime, process.type);
        process.readyTime = process.arrivalTime;
        processes[numProcesses] = process;
        numProcesses++;
    }

    fclose(file);
    return numProcesses;
}

// reads the instructions of a process from its file and sets their execution times
void setInstructions(Process* process, int instructionCount) {
    char filename[100];
    sprintf(filename, "%s.txt", process->name);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return;
    }

    char line[100];
    int numInstructions = 0;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character from the end of the line
        line[strcspn(line, "\n")] = '\0';

        // Find the corresponding instruction in instructions
        for(int i = 0; i < instructionCount; i++) {
            if(strcmp(instructions[i].name, line) == 0) {
                process->instructions[numInstructions] = instructions[i];
                numInstructions++;
                break;
            }
        }
    }
    process->instructionCount = numInstructions;
    fclose(file);
}

// initiliazes the processes and instructions
void inputHandler(){
    int instructionCount = readInstructionsFromFile();
    processCount = readProcessesFromFile();
    for(int i = 0; i < processCount; i++) {
        setInstructions(&processes[i], instructionCount);
    }
}

// compares two processes according to their priority, arrival time, and name (in order)
// returns positive number if process a should be executed before process b, 
// negative number if vice versa, 0 if they are equal
int compareProcesses(const Process *a, const Process *b) {
    int aPriority = 0;
    int bPriority = 0;
    if(!strcmp(a->type,"PLATINUM")){
        aPriority++;
    }
    if(!strcmp(b->type,"PLATINUM")){
        bPriority++;
    }
    if(aPriority-bPriority != 0){
        return aPriority-bPriority;
    }
    if(a->priority-b->priority != 0){
        return a->priority-b->priority;
    }
    if(a->readyTime-b->readyTime != 0){
        return b->readyTime-a->readyTime;
    }
    return -1 * strcmp(a->name, b->name);   
}

// iterates over the processes and 
// returns the next process to be executed,
// if there is no process to be executed, returns NULL
Process* pickNextProcess(int currentTime){
    Process *nextProcess = NULL;
    for(int i = 0; i < processCount; i++) {
        if((processes[i].arrivalTime <= currentTime)&&(processes[i].finishTime == -1)){
            if(nextProcess == NULL){
                nextProcess = &processes[i];
            }
            else{
                // calls the compare function to check if the selected process has higher priority
                if(compareProcesses(&processes[i],nextProcess) > 0){
                    nextProcess = &processes[i];
                }
            }
        }
    }
    return nextProcess;
}

// compares two processes according to their priority
// returns positive number if newprocess should preempt executing process, 
// negative number or 0 if no preemption is required
int preemptiveCompareProcesses(const Process *newProcess, const Process *executingProcess) {
    if(!strcmp(executingProcess->type,"PLATINUM")){
        return -1;
    }
    else if(!strcmp(newProcess->type,"PLATINUM")){
        return 1;
    }
    else{
        return newProcess->priority-executingProcess->priority;
    }
}

// iterates over the processes and checks if there is a process can preempt the executing process
int ifPrememptionRequired(Process *executingProcess, int currentTime){
    for(int i = 0; i < processCount; i++) {
        if((processes[i].arrivalTime <= currentTime)&&(!processes[i].ifInCPU)&&(processes[i].finishTime == -1)){
            // calls the compare function to check if the selected process has higher priority
            if(preemptiveCompareProcesses(&processes[i],executingProcess) > 0){
                return 1;
            }
        }
    }
    return 0;
}

// returns the time quantum of a process
int timeQuantum(Process *process){
    if(!strcmp(process->type,"SILVER")){
        return 80;
    }
    else if(!strcmp(process->type,"GOLD")){
        return 120;
    }
    else if(!strcmp(process->type,"PLATINUM")){
        return 999999999;
    }
}

// no preemption
int executePlatinumProcess(Process *process, int currentTime){
    int executedTime = 0;
    do{
        //printf("%5d\t%3s\t  %7s\t %8s\n", currentTime + executedTime,  process->name, process->instructions[process->programCounter].name, process->type);
        executedTime = executedTime + process->instructions[process->programCounter].executionTime;
        process->programCounter++;
    } while (process->programCounter < process->instructionCount);

    process->finishTime = currentTime + executedTime;
    process->readyTime = currentTime + executedTime;
    return currentTime + executedTime;
}

// executes a process by iterating over its instructions
// checks if preemption is required after each instruction
// returns the time when the process finishes
int executeProcess(Process *process, int beginningTime){
    //if(!strcmp(process->type,"PLATINUM")){
    //    return executePlatinumProcess(process, beginningTime);
    //}
    process->nofExecution++;
    int quantum = timeQuantum(process);
    int executedTime = 0;
    do
    {
        printf("%5d\t%3s\t  %7s\t %8s\n", beginningTime + executedTime,  process->name, process->instructions[process->programCounter].name, process->type);
        executedTime = executedTime + process->instructions[process->programCounter].executionTime;
        process->programCounter++;
        if(process->programCounter >= process->instructionCount){
            process->finishTime = beginningTime + executedTime;
            break;
        }
    } while ((!ifPrememptionRequired(process, beginningTime + executedTime) && (executedTime < quantum)));
    
    process->readyTime = beginningTime + executedTime;
    return beginningTime + executedTime;
}

// updates the type of a process if necessary
void updateType(Process *process){
    if(!strcmp(process->type,"SILVER")){
        if(process->nofExecution >= 3){
            strcpy(process->type,"GOLD");
            process->nofExecution = 0;
        }
    }
    else if(!strcmp(process->type,"GOLD")){
        if(process->nofExecution >= 5){
            strcpy(process->type,"PLATINUM");
            process->nofExecution = 0;
        }
    }
}

// main control function for the scheduler. it is responsible to iterate over the time.
// handles context switches 
// finds the next process to be executed by calling pickNextProcess
// executes processes by calling executeProcess
void scheduler(){
    int currentTime = 0;
    int terminatedProcesses = 0;
    Process *previousProcess = NULL;
    while(terminatedProcesses < processCount){
        // pickNextProcess returns the process with the highest priority 
        // or null if there is no process to be executed
        Process *nextProcess = pickNextProcess(currentTime);
        // if there is a process to be executed, execute it
        if(nextProcess != NULL){

            //context switch
            if(previousProcess != NULL){
                // if the process to be executed is different than the previous one
                // context switch is required
                if(strcmp(previousProcess->name,nextProcess->name)){
                    previousProcess->ifInCPU = 0;
                    nextProcess->ifInCPU = 1;
                    currentTime = currentTime + 10;
                }
            }
            else{ // context switch for the first process
                nextProcess->ifInCPU = 1;
                currentTime = currentTime + 10;
            }
            // takes the time when the process finishes
            currentTime = executeProcess(nextProcess,currentTime);
            if(nextProcess->finishTime != -1){
                terminatedProcesses++;
            }
            updateType(nextProcess);
            previousProcess = nextProcess;
        }
        // if there is no process to be executed, increase the time
        else{
            currentTime++;
        }
    }
}

// calculates and returns turnaround time of a process
double turnAroundTime(Process *process){
    return process->finishTime - process->arrivalTime;
}

// calculates and returns burst time of a process
int burstTime(Process *process){
    int burstTime = 0;
    for(int i = 0; i < process->instructionCount; i++) {
        burstTime = burstTime + process->instructions[i].executionTime;
    }
    return burstTime;
}

// calculates and returns waiting time of a process
double waitingTime(Process *process){
    return turnAroundTime(process) - burstTime(process);
}

// calculates and print average waiting and turnaoound times
void outputHandler(){
    double averageTurnAroundTime = 0;
    double averageWaitingTime = 0;
    for (size_t i = 0; i < processCount; i++){
        averageTurnAroundTime = averageTurnAroundTime + turnAroundTime(&processes[i]);
        averageWaitingTime = averageWaitingTime + waitingTime(&processes[i]);
    }
    averageTurnAroundTime = averageTurnAroundTime / processCount;
    averageWaitingTime = averageWaitingTime / processCount;

    if (averageWaitingTime == (int)averageWaitingTime) {
        printf("%d\n", (int)averageWaitingTime);
    } else {
        printf("%.1f\n", averageWaitingTime);
    }
    
    if (averageTurnAroundTime == (int)averageTurnAroundTime) {
        printf("%d\n", (int)averageTurnAroundTime);
    } else {
        printf("%.1f\n", averageTurnAroundTime);
    }
    
}

int main() {
    inputHandler();
    scheduler();
    outputHandler();
    return 0;
}