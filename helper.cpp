#include "helper.h"
#include <fstream>
#include <sstream>
#include <random>



// --- Function: getRandomTime ---
// Returns a random integer between low and high (inclusive).
int getRandomTime(int low, int high) {
    static std::default_random_engine gen(std::random_device{}());  // Random engine
    std::uniform_int_distribution<int> dist(low, high);             // Uniform distribution
    return dist(gen);  // Generate and return random number
}


// --- TA Methods ---

// ==== Statement ====
TA::TA(int initialState) : Statement(initialState) {}

// Get TA's current state (0 = sleeping, 1 = working/available)
int TA::getStatement() {
    return Statement;
}

// Set TA's state
void TA::setStatement(int s) {
    Statement = s;
}

// --- Student Methods ---

// Constructor: initialize student with ID, initial state, question time, and arrival time
Student::Student(int id_, int statement_, int q_time, int a_time)
    : id(id_), Statement(statement_), question_time(q_time), arrival_time(a_time) {}

// Change the student's arrival time.
void Student::Change_Arrival_Time(int now_time, int end_time) {
    arrival_time = getRandomTime(now_time, end_time);
}
// ==== Statement ====
// Get the current state of the student (1 = coding, 2 = waiting for help)
int Student::getStatement() const {
    return Statement;
}
// Set the student's state
void Student::setStatement(int s) {
    Statement = s;
}
// ==== ID ====
// Get student ID
int Student::getId() const {
    return id;
}

// Get duration student needs help (question time)
int Student::getQuestionTime() const {
    return question_time;
}

// Get student's scheduled arrival time
int Student::getArrivalTime() const { 
    return arrival_time;
}
// Manually set a new arrival time
void Student::setArrivalTime(int n){
    arrival_time = n;
}
// ==== Wait Time ====

// Set the time student waited before getting help
void Student::setWaitTime(int w) {
    wait_time = w;
}

// Get the student's wait time
int Student::getWaitTime() const {
    return wait_time;
}

// ====== Turnaround Time ====
// Set total time from arrival to leaving (turnaround time)
void Student::setTurnaroundTime(int t) {
    turnaround_time = t;
}
// Get the student's turnaround time
int Student::getTurnaroundTime() const {
    return turnaround_time;
}


// ==== Helped Status ====
// Set whether student received help or not
void Student::setHelped(bool h) {
    helped = h;
}

// Check if student was helped
bool Student::wasHelped() const {
    return helped;
}

