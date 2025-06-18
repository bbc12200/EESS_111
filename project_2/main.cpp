#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <queue>
#include "helper.h"   // TA, Student, and helper functions

using namespace std;

/* ========= Global State ========= */
int Total_minutes = 0;
int current_time  = 0;
bool simulation_running = true;

/* ========= Synchronization Primitives ========= */
pthread_mutex_t time_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  tick_cond   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cout_mutex  = PTHREAD_MUTEX_INITIALIZER;
sem_t ta_sleeping;

/* ========= Data Structures ========= */
TA TA_object(1);
vector<Student> students_vector;

int chairs = 0;
queue<int> chairs_queue;
queue<int> office_chair;

int total_ta_nap_time = 0;
int nap_start_time    = -1;
int help_start_time   = -1;

/* ========= Printting Detail ========= */
void print＿time_detail() {
    pthread_mutex_lock(&queue_mutex);
    queue<int> hall = chairs_queue;
    queue<int> off  = office_chair;
    int ta_state = TA_object.getStatement();
    pthread_mutex_unlock(&queue_mutex);

    pthread_mutex_lock(&cout_mutex);
    cout << "=============================\n";
    cout << "Time: ";
    cout << "TA state: ";
    if (ta_state == 0)
        cout << "0 (sleeping)\n";
    else {
        if (!off.empty()) cout << "1 (helping)\n";
        else              cout << "1 (available)\n";
    }

    cout << "Current Office Chair: ";
    if (off.empty()) cout << "[ ]\n";
    else             cout << "[S" << off.front() << "]\n";

    int filled = hall.size();
    int empty  = chairs - filled;

    cout << "Current Hallway Chairs: ";
    while (!hall.empty()) {
        cout << "[S" << hall.front() << "]";
        hall.pop();
    }
    for (int i = 0; i < empty; ++i) cout << "[  ]";
    cout << '\n';
    pthread_mutex_unlock(&cout_mutex);
}


/* ========= TA Thread ========= */
void* ta_function(void*){
    while (true) {
        // Lock the time mutex to safely access the simulation state and time
        pthread_mutex_lock(&time_mutex);
        if (!simulation_running) {
            pthread_mutex_unlock(&time_mutex);
            break;  // Exit the thread if simulation is no longer running
        }
        // Wait for a tick signal (time update) before proceeding
        pthread_cond_wait(&tick_cond, &time_mutex);
        int now = current_time;  // Store the current simulation time
        pthread_mutex_unlock(&time_mutex);

        // Lock the queue mutex to safely manage the student queues
        pthread_mutex_lock(&queue_mutex);
        if (TA_object.getStatement() == 1) {  // Check if TA is in active state
            if (office_chair.empty()) {  // No student currently being helped
                if (chairs_queue.empty()) {  // No students waiting in the hallway
                    TA_object.setStatement(0);  // TA goes to sleep
                    nap_start_time = now;  // Record when TA starts napping

                    // Print message that TA is sleeping
                    pthread_mutex_lock(&cout_mutex);
                    cout << "[T=" << now << "] TA is sleeping.\n";
                    pthread_mutex_unlock(&cout_mutex);
                    pthread_mutex_unlock(&queue_mutex);
                    sem_wait(&ta_sleeping);  // Block until a student wakes up the TA
                    // Get the current simulation state and time after waking up
                    pthread_mutex_lock(&time_mutex);
                    bool running2 = simulation_running;
                    int  wake_t   = current_time;
                    pthread_mutex_unlock(&time_mutex);
                    total_ta_nap_time += wake_t - nap_start_time;  // Track total nap time
                    if (!running2) break;  // Exit if simulation ended while sleeping
                    TA_object.setStatement(1);  // TA is now active
                    pthread_mutex_lock(&queue_mutex);
                    // Print message that TA woke up
                    pthread_mutex_lock(&cout_mutex);
                    cout << "[T=" << wake_t << "] TA wakes up.\n";
                    pthread_mutex_unlock(&cout_mutex);
                    if (!chairs_queue.empty()) {  // If students are waiting after TA wakes up
                        int sid = chairs_queue.front();  // Get first waiting student
                        chairs_queue.pop();
                        office_chair.push(sid);  // Move student to the office
                        int arrive = students_vector[sid].getArrivalTime();
                        students_vector[sid].setWaitTime(wake_t - arrive);  // Calculate wait time
                        help_start_time = wake_t;  // Record when help started
                        // Print message that TA starts helping a student
                        pthread_mutex_lock(&cout_mutex);
                        cout << "[T=" << wake_t << "] TA starts helping S" << sid << '\n';
                        pthread_mutex_unlock(&cout_mutex);
                    }
                    pthread_mutex_unlock(&queue_mutex);
                    continue;  // Go back to the start of the loop
                }

                // At least one student is waiting in the hallway
                int sid = chairs_queue.front();  // Get first waiting student
                chairs_queue.pop();
                office_chair.push(sid);  // Move student to the office
                int arrive = students_vector[sid].getArrivalTime();
                students_vector[sid].setWaitTime(now - arrive);  // Calculate wait time
                help_start_time = now;  // Record when help started

                // Print message that TA starts helping a student
                pthread_mutex_lock(&cout_mutex);
                cout << "[T=" << now << "] TA starts helping S" << sid << '\n';
                pthread_mutex_unlock(&cout_mutex);
            }
            else {
                // TA is currently helping a student, check if they are finished
                int sid   = office_chair.front();
                int qtime = students_vector[sid].getQuestionTime();

                // Check if the help session is complete
                if (help_start_time != -1 && now - help_start_time >= qtime) {
                    office_chair.pop();  // Student leaves the office
                    students_vector[sid].setHelped(true);  // Mark student as helped
                    int arrive = students_vector[sid].getArrivalTime();
                    students_vector[sid].setTurnaroundTime(now - arrive);  // Calculate total time in system
                    help_start_time = -1;  // Reset help start time

                    // Print message that student finished and leaves
                    pthread_mutex_lock(&cout_mutex);
                    cout << "[T=" << now << "] Student S" << sid << " finished and leaves.\n";
                    pthread_mutex_unlock(&cout_mutex);

                    // If more students are waiting, help the next one
                    if (!chairs_queue.empty()) {
                        int next_id = chairs_queue.front();
                        chairs_queue.pop();
                        office_chair.push(next_id);  // Move next student to the office
                        int arrive2 = students_vector[next_id].getArrivalTime();
                        students_vector[next_id].setWaitTime(now - arrive2);  // Calculate wait time
                        help_start_time = now;  // Record when help started

                        // Print message that TA starts helping next student
                        pthread_mutex_lock(&cout_mutex);
                        cout << "[T=" << now << "] TA starts helping S" << next_id << '\n';
                        pthread_mutex_unlock(&cout_mutex);
                    }
                }
            }
        }

        pthread_mutex_unlock(&queue_mutex);
    }
    return nullptr;  // Thread terminates
}

/* ========= Student Thread ========= */
void* student_function(void* arg) {
    // Cast the argument to a Student object pointer
    Student* stu = static_cast<Student*>(arg);

    while (true) {
        // Lock the time mutex to safely access simulation state and time
        pthread_mutex_lock(&time_mutex);
        if (!simulation_running) {
            pthread_mutex_unlock(&time_mutex);
            return nullptr;  // Exit the thread if simulation is no longer running
        }
        // Wait for a tick signal (time update) before proceeding
        pthread_cond_wait(&tick_cond, &time_mutex);
        int now = current_time;  // Store the current simulation time
        pthread_mutex_unlock(&time_mutex);

        // Check if it's time for this student to arrive
        if (stu->getArrivalTime() == now) {
            // Lock the queue mutex to safely manage the waiting chairs
            pthread_mutex_lock(&queue_mutex);
            // Check if there are available chairs in the hallway
            if ((int)chairs_queue.size() < chairs) {
                chairs_queue.push(stu->getId());  // Student takes a seat in the hallway
                stu->setStatement(2);  // Update student's state to waiting

                // Print message that student arrives and takes a seat
                pthread_mutex_lock(&cout_mutex);
                cout << "[T=" << now << "] Student S" << stu->getId() << " arrives and takes a seat.\n";
                pthread_mutex_unlock(&cout_mutex);

                // If TA is sleeping, wake them up
                if (TA_object.getStatement() == 0) {
                    pthread_mutex_lock(&cout_mutex);
                    cout << "[T=" << now << "] Student S" << stu->getId() << " wakes up TA.\n";
                    pthread_mutex_unlock(&cout_mutex);
                    sem_post(&ta_sleeping);  // Signal the TA semaphore to wake the TA
                }
            }
            else {
                // No seats available in the hallway, student will try again later
                // Calculate a new arrival time for the student to return
                int new_time = (now + 1 > Total_minutes) ?
                               Total_minutes + 1 : getRandomTime(now + 1, Total_minutes);
                stu->setArrivalTime(new_time);  // Update student's arrival time

                pthread_mutex_lock(&cout_mutex);
                if (new_time <= Total_minutes) {
                    // Student will try again later within simulation time
                    cout << "[T=" << now << "] Student S" << stu->getId()
                         << " arrives but hall is full, will retry at " << new_time << " min.\n";
                } else {
                    // Not enough time left in simulation for student to return
                    cout << "[T=" << now << "] Student S" << stu->getId()
                         << " arrives but hall is full but time is not enough.( Just Leave )\n";
                }
                pthread_mutex_unlock(&cout_mutex);

                // If new arrival time is after simulation ends, terminate student thread
                if (new_time > Total_minutes) {
                    pthread_mutex_unlock(&queue_mutex);
                    return nullptr;
                }
            }
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    return nullptr;  // Thread terminates
}

/* ========= Main Function ========= */
int main() {
    int student_count = 0;
    char show;
    bool Print_or_Not = false;
    // +========== Data Setting =========
    cout << "Enter number of chairs: " << endl;
    cin >> chairs;
    cout << "Enter number of students: " << endl;
    cin >> student_count;
    cout << "Enter simulation time (minutes): " << endl;
    cin >> Total_minutes;
    cout << "Do you want to print every second statements (Y/N)?" << endl;
    cin >> show;

    Print_or_Not = (show == 'Y');

    for (int i = 0; i < student_count; i++) {
        int qtime = getRandomTime(1, 5);
        int atime = getRandomTime(0, Total_minutes);
        students_vector.emplace_back(i, 1, qtime, atime);

        cout << "Student S" << i
             << "  Arrival: "  << atime << " min"
             << "  Question: " << qtime << " min\n";

    }

    cout << "----------------------------------------\n";

    sem_init(&ta_sleeping, 0, 0);
    pthread_t ta_thread;
    pthread_create(&ta_thread, nullptr, ta_function, nullptr);
    vector<pthread_t> student_threads(student_count);

    for (int i = 0; i < student_count; ++i){
        pthread_create(&student_threads[i], nullptr, student_function, &students_vector[i]);
    }
    for (int tick = 0; tick < Total_minutes; tick++) {
        usleep(200);  // 0.02 s → simulate 1 min
        pthread_mutex_lock(&time_mutex);
        current_time = tick + 1;
        pthread_cond_broadcast(&tick_cond);
        pthread_mutex_unlock(&time_mutex);
        if(Print_or_Not){
            print＿time_detail();
        }
    }
    pthread_mutex_lock(&time_mutex);
    simulation_running = false;
    pthread_cond_broadcast(&tick_cond);
    pthread_mutex_unlock(&time_mutex);
    sem_post(&ta_sleeping);
    cout << "Simulation End\n";
    pthread_join(ta_thread, nullptr);
    for (auto& th : student_threads) pthread_join(th, nullptr);



    int total_helped = 0, total_wait = 0, total_turn = 0, total_qtime = 0;
    for (const auto& s : students_vector) {
        if (s.wasHelped()) {
            ++total_helped;
            total_wait  += s.getWaitTime();
            total_turn  += s.getTurnaroundTime();
            total_qtime += s.getQuestionTime();
        }
    }


    cout << "\n========= Simulation Summary =========\n";
    cout << "Total Students       : " << students_vector.size()                << '\n';
    cout << "Students Helped      : " << total_helped                          << '\n';
    cout << "Students Not Helped  : " << students_vector.size() - total_helped << '\n';
    if (total_helped) {
        cout << "Average Question Time: " << (int)total_qtime / total_helped << " Minutes" << endl;
        cout << "Average Wait Time    : " << (int)total_wait / total_helped << " Minutes" << endl;
        cout << "Average Turnaround   : " << (int)total_turn / total_helped << " Minutes" << endl;
    }
    cout << "TA Total Nap Time    : " << total_ta_nap_time << "Minutes" << endl;
    cout << "=============================\n";


    return 0;
}
