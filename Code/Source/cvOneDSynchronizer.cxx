//
// Created by a11bmafr on 26.01.24.
//

#include <iostream>
#include "cvOneDSynchronizer.h"


cvOneDSynchronizer::cvOneDSynchronizer() {
    max_time_steps=0;
    dt_=0;
}

// empty and uinitialized object
cvOneDSynchronizer::~cvOneDSynchronizer(){

}

cvOneDSynchronizer::cvOneDSynchronizer(int time_steps, double dt){
    max_time_steps=time_steps;
    // time step size
    dt_=dt;

    q_1d = new double[max_time_steps+1];
    p_1d = new double[max_time_steps+1];
    q_3d = new double[max_time_steps+1];
    p_3d = new double[max_time_steps+1];
    time_array = new double[max_time_steps+1];
    for (int i =0;i <max_time_steps; i++){
        time_array[i]=i*dt_;
    }
}

int cvOneDSynchronizer::calculate_step(double time){

    if(dt_ <=0.0){
        std::cout<<"dt is bellow 0 than max_time_steps"<<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }

    int step=int(time/dt_);

    if(step > max_time_steps){
        std::cout<<"step is bigger than max_time_steps"<<std::endl;
        std::cout<<step<<" "<<max_time_steps<<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }
    return step;

}

// check if the synchronizer was set up correctly or is just a dummy object
bool cvOneDSynchronizer::is_initialized(void){
    if(max_time_steps>0 && dt_>0){
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
    std::cout<<"you requested 1d presure data at time step:"-calculate_step(t)<<"";
    return p_1d[calculate_step(t)];
}

double cvOneDSynchronizer::Get_1d_q_at_t(double t){
    std::cout<<"you requested 1d q data at time step:"-calculate_step(t)<<"";
    return q_1d[calculate_step(t)];
}


// Update the 1d pressure results at specific time point
void cvOneDSynchronizer::Set_1D_p_at_t(double t,double p){
    p_1d[calculate_step(t)]=p;
}

// Update the 1d pressure results at specific time point
void cvOneDSynchronizer::Set_1D_q_at_t(double t,double p){
    q_1d[calculate_step(t)]=p;
}

// Update the 3d flow results at time step
void cvOneDSynchronizer::Set_3d_q_at_t(double t,double q){
    q_3d[calculate_step(t)]=q;
}

// Update the 3d flow results at time step
void cvOneDSynchronizer::Set_3d_p_at_t(double t,double p){
    p_3d[calculate_step(t)]=p;
}

void cvOneDSynchronizer::Print_Item(string name, double * value){
    std::cout << name << ": ";
    for (int i = 0; i < max_time_steps; ++i) {
        std::cout << value[i] << " ";
    }
    std::cout<< " " << std::endl;
}

void cvOneDSynchronizer::Print() {

    Print_Item("q_1d",q_1d);
    Print_Item("q_3d",q_3d);
    Print_Item("p_1d",p_1d);
    Print_Item("p_3d",p_3d);

}
