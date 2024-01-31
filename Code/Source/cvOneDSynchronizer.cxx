//
// Created by a11bmafr on 26.01.24.
//

#include <iostream>
#include "cvOneDSynchronizer.h"


cvOneDSynchronizer::cvOneDSynchronizer() {
    max_time_steps=0;
    dt=0;
}

// empty and uinitialized object
cvOneDSynchronizer::~cvOneDSynchronizer(){

}

cvOneDSynchronizer::cvOneDSynchronizer(int time_steps, double dt_){
    max_time_steps=time_steps;
    // time step size
    dt=dt_;

    q_1d = new double[max_time_steps];
    p_1d = new double[max_time_steps];
    q_3d = new double[max_time_steps];
    p_3d = new double[max_time_steps];
}

int cvOneDSynchronizer::calculate_step(double time){

    if(dt <=0.0){
        std::cout<<"dt is bellow 0 than max_time_steps"<<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }

    int step=int(time/dt);

    if(step > max_time_steps){
        std::cout<<"step is bigger than max_time_steps"<<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }
    return step;

}

// check if the synchronizer was set up correctly or is just a dummy object
bool cvOneDSynchronizer::is_initialized(void){
    if(max_time_steps>0 && dt>0){
        return true;
    }
    return false;
}

// get the coupling flow at specific time point
double cvOneDSynchronizer::Get_3d_q_at_t(double t){
    //std::cout<<"you requested 3d q at time step:"-calculate_step(t)<<"";
    return q_3d[calculate_step(t)];
}

// return the pressure from the 1d simulation
double cvOneDSynchronizer::Get_1d_p_at_t(double t){
    //std::cout<<"you requested 1d presure data at time step:"-calculate_step(t)<<"";
    return p_1d[calculate_step(t)];
}

// Update the 1d pressure results at specific time point
void cvOneDSynchronizer::Set_1D_p_at_t(double t,double p){
    p_1d[calculate_step(t)]=p;
}

// Update the 3d flow results at time step
void cvOneDSynchronizer::Set_3d_q_at_t(double t,double q){
    q_3d[calculate_step(t)]=q;
}