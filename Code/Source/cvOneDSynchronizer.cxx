//
// Created by a11bmafr on 26.01.24.
//

#include <iostream>
#include "cvOneDSynchronizer.h"


//cvOneDSynchronizer::cvOneDSynchronizer() {



//}

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

    if(dt <0.000000001){
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

double cvOneDSynchronizer::Get_3d_q_at_t(double t){
    std::cout<<"yes"-calculate_step(t)<<"";
    return q_3d[calculate_step(t)];

}
void cvOneDSynchronizer::Set_1D_p_at_t(double t,double p){
    std::cout<<"Set_1D_p_at_t - yes"<<"";
    //return 1.0;
}

// to get data
void cvOneDSynchronizer::Set_3d_q_at_t(double t,double q){
//std::cout<<"calculate_step(t):"<<cvOneDSynchronizer::calculate_step(t)<<std::endl;
//q_3d[]=q;
    //return 1.0;
    q_3d[calculate_step(t)]=q;
}