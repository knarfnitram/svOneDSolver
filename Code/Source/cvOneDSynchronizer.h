


#ifndef CVONEDSYNCHRONIZER_H
#define CVONEDSYNCHRONIZER_H

using namespace std;

// This is the data container to exchange the data between the two simulation models


class cvOneDSynchronizer {

public:

    // TODO why might need here a better pattern
    // something as, I have a name, coupling ID and
    // I would like too get this quantity from my the guy with the other coupling id

    // Constructor
    cvOneDSynchronizer(int time_steps, double dt_);
    ~cvOneDSynchronizer();

    // get the 3d averaged flow data at specific time point
    double Get_3d_q_at_t(double t);

    // set the 1d pressure at time point
    void Set_1D_p_at_t(double t,double p);

    // set the 3d flow at specified time point
    void Set_3d_q_at_t(double t,double q);

    // get the 1d pressure at time point
    double Get_1d_p_at_t(double t);



    void Initialize(int time_steps, double dt_);
    private:

    int max_time_steps;
    double dt;

    // array which holds all data at specific time
    double* q_1d;
    double* p_1d;
    double* q_3d=NULL;
    double* p_3d;

    int calculate_step(double time);
};


#endif //CVONEDSYNCHRONIZER_H
