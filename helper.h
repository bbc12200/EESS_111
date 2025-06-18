#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <vector>
#include <iostream>

int getRandomTime(int min, int max);


class TA {
    private:
        // 1: TA is available
        // 2: TA is sleeping
        int Statement;
    public:
        TA(int initialState = 1);
        int getStatement();
        void setStatement(int s);
};

class Student {
    private:
        int Statement;
        // 1: Codeing
        // 2: Seeking Help
        int id;
        int question_time;
        int arrival_time;
        
        int wait_time = 0;
        int turnaround_time = 0;
        bool helped = false;

    public:
        Student(int id_, int statement_, int q_time, int a_time);
        void Change_Arrival_Time(int now_time, int end_time) ;
        void setArrivalTime(int n);

        int getStatement() const;
        int getId() const;
        int getQuestionTime() const;
        int getArrivalTime() const;

        void setStatement(int s);

        // static setter/getter
        void setWaitTime(int w);
        int getWaitTime() const;

        void setTurnaroundTime(int t);
        int getTurnaroundTime() const;

        void setHelped(bool h);
        bool wasHelped() const;

};

#endif // HELPER_H
