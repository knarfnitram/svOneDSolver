#include "OneDSolverInterface.h"

// ==============
// RUN ONEDSOLVER
// ==============
void runOneDSolver(string inputFile){

    OneDSolverInterface myOneDSolver=OneDSolverInterface();
    // Create Options
    cvOneDOptions* opts = new cvOneDOptions();

    // Read Model From File
    myOneDSolver.readModel(inputFile,opts);

    // Model Checking
    opts->check();

    // Print Input Data Echo
    string fileName("echo.out");
    opts->printToFile(fileName);

    // Create Model and Run Simulation
    myOneDSolver.createAndRunModel(opts);

    // Delete Options
    delete opts;
}

// ====================
// WRITE PROGRAM HEADER
// ====================
void WriteHeader(){
    printf("---------------------------------\n");
    printf(" oneDSolver 2 \n");
    printf(" 1D Finite Element Hemodynamics  \n");
    printf("---------------------------------\n");
}

// =============
// MAIN FUNCTION
// =============
int main(int argc, char** argv){

    // Write Program Header
    WriteHeader();

    try{

        // Run Simulation
        string inputFile(argv[1]);
        runOneDSolver(inputFile);

    }catch(exception& e){
        // Print Exception Message
        printf("%s\n",e.what());
        // Execution Terminated
        printf("Terminated.\n");
        return 1;
    }
    printf("Completed!\n");
    return 0;

}