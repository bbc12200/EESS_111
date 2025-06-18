#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <iomanip>
#include <algorithm>
#include <cstring>
using namespace std;

// System constants defining the size of our banking system
constexpr int NUMBER_OF_CUSTOMERS = 5;  // Number of processes/customers in the system
constexpr int NUMBER_OF_RESOURCES = 4;  // Number of resource types

/* ──────────────────────────────────────────────────────────────────
 *  GLOBAL DATA STRUCTURES (defined exactly as in the PDF hand‑out)
 * ──────────────────────────────────────────────────────────────────*/
// Core data structures for the Banker's Algorithm
std::array<int, NUMBER_OF_RESOURCES> available{};                                    // Currently available instances of each resource type
std::array<std::array<int, NUMBER_OF_RESOURCES>, NUMBER_OF_CUSTOMERS> maximum{};     // Maximum demand of each customer for each resource
std::array<std::array<int, NUMBER_OF_RESOURCES>, NUMBER_OF_CUSTOMERS> allocation{};  // Resources currently allocated to each customer
std::array<std::array<int, NUMBER_OF_RESOURCES>, NUMBER_OF_CUSTOMERS> need{0};       // Remaining resource needs (need = maximum - allocation)


// Function to display the current state of all system matrices
void print_state(const array<int, NUMBER_OF_RESOURCES>& available,
                 const array<array<int, NUMBER_OF_RESOURCES>, NUMBER_OF_CUSTOMERS>& maximum,
                 const array<array<int, NUMBER_OF_RESOURCES>, NUMBER_OF_CUSTOMERS>& allocation) {
    // Display available resources
    cout << "Available resources: ";
    for (const auto& res : available) {
        cout << res << " ";
    }
    cout << endl;

    // Display maximum demand matrix
    cout << "Maximum matrix:" << endl;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            cout << maximum[i][j] << " ";
        }
        cout << endl;
    }

    // Display current allocation matrix
    cout << "Allocation matrix:" << endl;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            cout << allocation[i][j] << " ";
        }
        cout << endl;
    }
    
    // Display need matrix (calculated as maximum - allocation)
    cout << "Need matrix:" << endl;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            cout << need[i][j] << " ";
        }
        cout << endl;
    }
}

// Function to display the command menu
void print_prompt(){
    cout << "---------------------------------------------------" << endl;
    cout << "Enter command (type EXIT to quit):" << endl;
    cout << "   RQ <cid> <r0> <r1> ... <rn>  - Request resources" << endl;
    cout << "   RL <cid> <r0> <r1> ... <rn>  - Release resources" << endl;
    cout << "   *                            - Print state" << endl;
    cout << "   EXIT                         - Exit the program" << endl;
    cout << "---------------------------------------------------" << endl;
}
// Validate user command syntax and return command type
// Returns: 0=invalid, 1=RQ, 2=RL, 3=*, 4=EXIT
int check_command_valid(const std::string& command) {
    std::istringstream ss(command);
    std::string token;
    std::vector<std::string> tokens;

    // Tokenize the command
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return 0; // Empty command is invalid

    const std::string& cmd = tokens[0];

    // Check for print state command
    if (cmd == "*") {
        return tokens.size() == 1 ? 3 : 0;
    }

    // Check for exit command
    if (cmd == "EXIT") {
        return tokens.size() == 1 ? 4 : 0;
    }

    // Check for request/release commands
    if (cmd == "RQ" || cmd == "RL") {
        // Verify correct number of arguments
        if (tokens.size() != 2 + NUMBER_OF_RESOURCES) {
            return 0; // Wrong number of parameters
        }

        // Verify all parameters are valid integers
        for (size_t i = 1; i < tokens.size(); ++i) {
            const std::string& t = tokens[i];
            if (t.empty()) return 0;

            // Handle negative numbers
            size_t start = (t[0] == '-') ? 1 : 0;
            if (start == 1 && t.length() == 1) return 0;

            // Check all characters are digits
            for (size_t j = start; j < t.length(); ++j) {
                if (!isdigit(t[j])) return 0;
            }
        }

        return (cmd == "RQ") ? 1 : 2;
    }

    return 0; // Unknown command
}

// Parse command string to extract customer number and resource amounts
void command_to_array(const std::string& command, int& customer_num, int request[]) {
    std::istringstream ss(command);
    std::string token;
    ss >> token; // Skip command token (RQ/RL)
    ss >> customer_num;

    // Extract resource amounts
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        if (!(ss >> request[i])) {
            request[i] = 0; // Default to 0 if not provided
        }
    }
}


// ----------------------------------------------------------------
// Initialize maximum and allocation matrices from CSV files
// ----------------------------------------------------------------
bool initialize_matrices(const string& max_file, const string& alloc_file) {
    // Read maximum resource requirements from file
    ifstream infile_max(max_file);
    if (!infile_max.is_open()) {
        cerr << "Failed to open " << max_file << endl;
        return false;
    }

    // Parse CSV file for maximum matrix
    string line;
    int customer = 0;
    while (getline(infile_max, line) && customer < NUMBER_OF_CUSTOMERS) {
        stringstream ss(line);
        string token;
        int res_index = 0;
        // Split by comma and store values
        while (getline(ss, token, ',') && res_index < NUMBER_OF_RESOURCES) {
            maximum[customer][res_index] = stoi(token);
            res_index++;
        }
        customer++;
    }
    infile_max.close();

    // Read current allocation from file
    ifstream infile_alloc(alloc_file);
    if (!infile_alloc.is_open()) {
        cerr << "Failed to open " << alloc_file << endl;
        return false;
    }

    // Parse CSV file for allocation matrix
    customer = 0;
    while (getline(infile_alloc, line) && customer < NUMBER_OF_CUSTOMERS) {
        stringstream ss(line);
        string token;
        int res_index = 0;
        // Split by comma and store values
        while (getline(ss, token, ',') && res_index < NUMBER_OF_RESOURCES) {
            allocation[customer][res_index] = stoi(token);
            res_index++;
        }
        customer++;
    }
    infile_alloc.close();

    return true;
}

// Implementation of the Banker's Safety Algorithm
// Returns true if system is in safe state, false otherwise
bool safe_algorithm() {
    // Step 1: Initialize work vector and finish array
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS] = {false};
    int safe_sequence[NUMBER_OF_CUSTOMERS];  // Store the safe execution sequence
    int sequence_index = 0;                  // Index for safe sequence

    // Initialize work = available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }

    // Step 2: Find a process that can execute
    bool progress_made;
    do {
        progress_made = false;
        
        // Check each customer/process
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {  // If process not yet finished
                bool can_run = true;
                
                // Check if need[i] <= work for all resources
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        can_run = false;
                        break;
                    }
                }

                // If process can run, simulate its completion
                if (can_run) {
                    // Release allocated resources back to work
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    safe_sequence[sequence_index++] = i;  // Record in safe sequence
                    progress_made = true;
                    
                    // Print progress (optional debugging output)
                    printf("Customer %d completed. Available resources: ", i);
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                        printf("%d ", work[k]);
                    }
                    printf("\n");
                }
            }
        }
    } while (progress_made);  // Continue while making progress

    // Step 3: Check if all processes completed
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            // System is unsafe - deadlock possible
            printf("System is in UNSAFE state!\n");
            printf("==== Finish Status ====\n");
            for (int k = 0; k < NUMBER_OF_CUSTOMERS; k++) {
                if (finish[k]) {
                    printf("Customer %d: completed\n", k);
                } else {
                    printf("Customer %d: not completed\n", k);
                }
            }
            return false; // Unsafe state
        }
    }
    
    // All processes completed - system is safe
    printf("System is in SAFE state!\n");
    printf("Safe sequence: <");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("T%d", safe_sequence[i]);
        if (i < NUMBER_OF_CUSTOMERS - 1) {
            printf(", ");
        }
    }
    printf(">\n");
    
    return true; // Safe state
}

/* ------------------------------------------------------------------
 * REQUEST — Handle resource request using Banker's algorithm
 * Tentatively allocate, run safety test, rollback if unsafe
 * ----------------------------------------------------------------*/
int request_resources(int customer_num, int request[]){

    // Step 1: Validate request doesn't exceed customer's declared need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] < 0 || request[i] > need[customer_num][i]) {
            cout << "Invalid request: Customer " << customer_num << " requests more than needed or negative resources." << endl;
            return -1; // Invalid request
        }
    }
    
    // Step 2: Check if sufficient resources are available
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            cout << "Request cannot be satisfied: Not enough available resources." << endl;
            return -1; // Insufficient resources
        }
    }
    
    // Step 3: Tentatively allocate resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    // Step 4: Run safety algorithm to check if state remains safe
    if (safe_algorithm()) {
        std::cout << "Request granted to customer " << customer_num << ".\n";
        return 0; // Success - keep the allocation
    }

    // Step 5: Unsafe state - rollback the allocation
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        available[i] += request[i];
        allocation[customer_num][i] -= request[i];
        need[customer_num][i] += request[i];
    }

    std::cout << "Request denied — unsafe state.\n";
    return -1;
}

/* ------------------------------------------------------------------
 * RELEASE — Return allocated resources to available pool
 * No safety check needed as releasing resources cannot cause deadlock
 * ----------------------------------------------------------------*/
void release_resources(int customer_num, int release[]) {
    // Validate release doesn't exceed allocated resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        if (release[i] > allocation[customer_num][i]) {
            std::cout << "Error: Customer " << customer_num
                       << " cannot release more than allocated resources for type " << i << std::endl;
            return;
        }
    }
    
    // Perform the release operation
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        allocation[customer_num][i] -= release[i];  // Reduce customer's allocation
        available[i]                += release[i];  // Return resources to available pool
        need[customer_num][i]       += release[i];  // Increase need (since need = max - allocation)
    }
    
    std::cout << "Resources released by customer " << customer_num << "." << std::endl;
}


/* ------------------------------------------------------------------
 *  MAIN — Program entry point
 *  Parse command-line arguments, initialize system, run command loop
 * ----------------------------------------------------------------*/
int main(int argc, char* argv[])
{   
    // ----------------------------------------------------------------
    // Parse command-line arguments for initial available resources
    // ----------------------------------------------------------------
    if (argc != NUMBER_OF_RESOURCES+1 ){
       cout << "Please enter the right number of arguments: " << NUMBER_OF_RESOURCES << endl;
       return 0;
    }
    else{
        // Store command-line arguments as initial available resources
        for(int i = 1; i < argc; i++){
            available.at(i-1) = atoi(argv[i]); // Convert string to integer
        }
    }
    
    // ---------------------------------------------------------------
    // Initialize system state from configuration files
    // ---------------------------------------------------------------
    initialize_matrices("max.txt", "allocation.txt");
    
    // Calculate initial need matrix (need = maximum - allocation)
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i){
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j){
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }
    
    // Display initial state and check safety
    print_state(available, maximum, allocation); 
    if (safe_algorithm()){
        cout << "Initial state is safe." << endl;
    } else {
        cout << "Initial state is unsafe." << endl;
    }
    
    // ---------------------------------------------------------------
    // Main command processing loop
    // ---------------------------------------------------------------
    cout << "============ ZotBank Resource Manager =============" << endl;
    string command;
    bool Loop_state = true;
    int command_valid_and_type = 0;
    
    while (Loop_state){
        // Display menu and get user input
        print_prompt();
        cout << "Enter command: ";
        getline(std::cin, command);
        cout << "Command entered: " << command << endl; 
        
        // Validate command syntax
        command_valid_and_type = check_command_valid(command);

        // Process command based on type
        if(command_valid_and_type==0){
            // Invalid command
            cout << "Invalid command. Please try again." << endl;
            continue;
        }
        else if (command_valid_and_type == 1){
            // RQ - Request resources
            int customer_num;
            int request[NUMBER_OF_RESOURCES] = {0}; 
            command_to_array(command, customer_num, request);
            request_resources(customer_num, request);
        }
        else if (command_valid_and_type == 2){
            // RL - Release resources
            int customer_num;
            int request[NUMBER_OF_RESOURCES] = {0}; 
            command_to_array(command, customer_num, request);
            release_resources(customer_num, request);
        }
        else if(command_valid_and_type == 3){
            // * - Print current system state
            cout << "Current state of the system:" << endl;
            print_state(available, maximum, allocation);
            if (safe_algorithm()){
                cout << "The system is in a safe state." << endl;
            } else {
                cout << "The system is in an unsafe state." << endl;
            }
        }
        else if(command_valid_and_type == 4){
            // EXIT - Terminate program
            cout << "Exiting program." << endl;
            break;
        }
    }
    
    return 0;
}