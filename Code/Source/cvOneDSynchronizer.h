


#ifndef CVONEDSYNCHRONIZER_H
#define CVONEDSYNCHRONIZER_H

using namespace std;

class cvOneDSynchronizer {
public:
    cvOneDSynchronizer(int time_steps, double dt_);
    ~cvOneDSynchronizer();
    // to send data away
    double Get_3d_q_at_t(double t);
    void Set_1D_p_at_t(double t,double p);

    // to get data
    void Set_3d_q_at_t(double t,double q);
    //void et_3d_q_at_t(double t,double q);



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
