


#ifndef CVONEDSYNCHRONIZER_H
#define CVONEDSYNCHRONIZER_H


using namespace std;
#ifdef __cplusplus
extern "C" {
#endif
// This is the data container to exchange the data between the two simulation models


class cvOneDSynchronizer {

public:

    // TODO why might need here a better pattern
    // something as, I have a name, coupling ID and
    // I would like too get this quantity from my the guy with the other coupling id

    // Constructor
    cvOneDSynchronizer(int time_steps, double dt,int max_ids, int coupling_types);

    // uninitialized default Constructor
    cvOneDSynchronizer();
    ~cvOneDSynchronizer();

    // get the 3d averaged flow data at specific time point of the id
    double Get_3d_q_at_t(double t,int id);

    // get the 3d averaged pressure data at specific time point of the id
    double Get_3d_p_at_t(double t,int id);

    // set the 1d pressure at time point of the id
    void Set_1D_p_at_t(double t,double p,int id);

    void Set_1D_q_at_t(double t,double p,int id);

    double Get_1d_q_at_t(double t,int id);

    // set the 3d flow at specified time point of the id
    void Set_3d_q_at_t(double t,double q,int id);

    // get the 1d pressure at time point of the id
    double Get_1d_p_at_t(double t,int id);

    // set the 3d pressure at specified time point
    void Set_3d_p_at_t(double t,double p,int id);

    // check if the object was set up correctly
    bool is_initialized();

    bool Get_coupling_3d_1d_info(){ return coupling_3d_1d_;}

    bool Get_coupling_1d_3d_info(){ return coupling_1d_3d_;}


    void Initialize(int time_steps, double dt_);

    double* get_q_1d(void){ return q_1d;};

    double* get_t(void){return time_array;};

    void Print(void);

    void Print_id(int id);

    int get_max_time_steps(void){return max_time_steps;};

    // Print item with ID
    void Print_Item(string name, double * value, int id);

    // Print all items
    void Print_Item(string name, double * value);

    // Print l2 norm between entries
    void Print_Norm_of_Item(string name, double * value1, double * value2);


private:

    // maximum time steps
    int max_time_steps;

    // time step size
    double dt_;

    // flow array
    double* q_1d;
    double* q_3d;

    // pressure array
    double* p_3d;
    double* p_1d;

    // time array
    double* time_array;

    // max_id starts from 1 up to ...
    int max_ids_;

    // return the step size according to the given time
    int Calculate_Step(double time);

    // return the step size accoriding to the given time and
    int Calculate_Step(double time, const int id);

    // indicate if a 3d domain is used as a inflow condition for the artery
    bool coupling_3d_1d_;

    // indicate if a 3d domain is coupled at one outlet
    bool coupling_1d_3d_;


};

#ifdef __cplusplus
}
#endif

#endif //CVONEDSYNCHRONIZER_H

