//
// Created by a11bmafr on 26.01.24.
//

#include <iostream>
#include "cvOneDSynchronizer.h"
#include <string.h>

cvOneDSynchronizer::cvOneDSynchronizer() {
    max_time_steps=0;
    dt_=0;
    max_ids_=0;
}

// empty and uinitialized object
cvOneDSynchronizer::~cvOneDSynchronizer(){
    coupling_3d_1d_=false;
    coupling_1d_3d_=false;
}

cvOneDSynchronizer::cvOneDSynchronizer(int time_steps, double dt, int max_ids, int coupling_types){
    max_ids_=max_ids;
    max_time_steps=time_steps;
    coupling_3d_1d_=false;
    coupling_1d_3d_=false;

    // time step size
    dt_=dt;

    q_1d = new double[max_time_steps*max_ids_];
    p_1d = new double[max_time_steps*max_ids_];
    q_3d = new double[max_time_steps*max_ids_];
    p_3d = new double[max_time_steps*max_ids_];
    time_array = new double[max_time_steps+1];
    for (int i =0;i <max_time_steps; i++){
        time_array[i]=i*dt_;
    }
    // make sure every variable is initialized correctly
    for (int id =0;id <max_ids_; id++){
        for (int i =0;i <max_time_steps; i++){
            q_1d[i+max_time_steps*id]=0.0;
            p_1d[i+max_time_steps*id]=0.0;
            q_3d[i+max_time_steps*id]=0.0;
            p_3d[i+max_time_steps*id]=0.0;
        }
    }



    switch (coupling_types) {
        case 1:
            coupling_3d_1d_=true;
            break;
        case 2:
            coupling_1d_3d_=true;
            break;
        case 3:
            coupling_3d_1d_=true;
            coupling_1d_3d_=true;
            break;


    }

    std::cout<<"Coupling values choosen are: coupling_3d_1d_: " << coupling_3d_1d_ << " - coupling_1d_3d_:" << coupling_1d_3d_ << std::endl;


}

int cvOneDSynchronizer::Calculate_Step(double time){

    if(dt_ <=0.0){
        std::cout<<"dt is bellow 0 than max_time_steps"<<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }

    int step=int(time/dt_);

    if(step > max_time_steps){
        std::cout<<"step is bigger than max_time_steps"<<std::endl;
        std::cout<<step<<" "<<max_time_steps<<std::endl;
        std::cout<<"time"<<time <<dt_ <<std::endl;
        throw "currently the synchronizer does not support stepsizes bigger than:";
    }
    return step;

}

int cvOneDSynchronizer::Calculate_Step(double time, const int id){
    int step=Calculate_Step(time);
    return step+max_time_steps*id;
}


// check if the synchronizer was set up correctly or is just a dummy object
bool cvOneDSynchronizer::is_initialized(void){
    if(max_time_steps>0 && dt_>0 && max_ids_>0){
        return true;
    }
    return false;
}

// get the coupling flow at specific time point
double cvOneDSynchronizer::Get_3d_p_at_t(double t,int id){
    return p_3d[Calculate_Step(t,id-1)];
}

// get the coupling flow at specific time point
double cvOneDSynchronizer::Get_3d_q_at_t(double t,int id){

    return q_3d[Calculate_Step(t,id-1)];
}

// return the pressure from the 1d simulation
double cvOneDSynchronizer::Get_1d_p_at_t(double t,int id){

    return p_1d[Calculate_Step(t,id-1)];
}

double cvOneDSynchronizer::Get_1d_q_at_t(double t,int id){

    return q_1d[Calculate_Step(t,id-1)];
}


// Update the 1d pressure results at specific time point
void cvOneDSynchronizer::Set_1D_p_at_t(double t,double p,int id){
    p_1d[Calculate_Step(t,id-1)]=p;
}

// Update the 1d pressure results at specific time point
void cvOneDSynchronizer::Set_1D_q_at_t(double t,double p,int id){
    q_1d[Calculate_Step(t,id-1)]=p;
}

// Update the 3d flow results at time step
void cvOneDSynchronizer::Set_3d_q_at_t(double t,double q,int id){
    q_3d[Calculate_Step(t,id-1)]=q;
}

// Update the 3d flow results at time step
void cvOneDSynchronizer::Set_3d_p_at_t(double t,double p,int id){
    p_3d[Calculate_Step(t,id-1)]=p;
}

void cvOneDSynchronizer::Print_Item(string name, double * value, int id){
    std::cout << name << ": ";
    for (int i = 0; i < max_time_steps; ++i) {
        std::cout << value[i+max_time_steps*(id-1)] << " ";
    }
    std::cout<< " " << std::endl;
}


void cvOneDSynchronizer::Print_Item(string name, double * value){
    std::cout << name << ": " <<std::endl;
    for (int id =1; id<=max_ids_;id++){
        std::cout << id <<": ";

        for (int i = 0; i < max_time_steps; ++i) {
            std::cout << value[i+max_time_steps*(id-1)] << " ";
        }
        std::cout<< " " << std::endl;
    }
}

void cvOneDSynchronizer::Print() {


    Print_Item("q_1d",q_1d);
    Print_Item("q_3d",q_3d);
    Print_Item("p_1d",p_1d);
    Print_Item("p_3d",p_3d);
    Print_Norm_of_Item("q",q_1d,q_3d);
    Print_Norm_of_Item("p",p_1d,p_3d);
}

void cvOneDSynchronizer::Print_Norm_of_Item(string name, double * value1, double * value2){
    std::cout <<"Print_Norm_of_Item "<< name << ": " <<std::endl;
    for (int id =1; id<=max_ids_;id++){
        std::cout << id <<": ";

        for (int i = 0; i < max_time_steps; ++i) {
            const double disp=abs( abs( value1[i+max_time_steps*(id-1)]) - abs(value2[i+max_time_steps*(id-1)]));
            std::cout << disp << " ";
        }
        std::cout<< " " << std::endl;
    }
}


