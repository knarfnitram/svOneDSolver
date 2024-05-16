/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
//  MthModelBase.cxx - Source for a class to handle finite element equations
//  ~~~~~~~~~~~~
//
//  SYNOPSIS...This class is the most important in a finite element
//             computation.  It handles the equation formulation, and
//             construction of the Newton-Rhapson matrix derivatives,
//             and generation of the element dense matrices.
//


# include "cvOneDGlobal.h"
# include "cvOneDMthModelBase.h"
# include "cvOneDLinearSolver.h"
# include "cvOneDUtility.h"
# include "cvOneDDenseMatrix.h"
# include "cvOneDMaterial.h"
# include "cvOneDFiniteElement.h"


// Static Declarations...
int cvOneDMthModelBase::impedIncr;

cvOneDMthModelBase::cvOneDMthModelBase(const cvOneDModel* modl){
}

cvOneDMthModelBase::cvOneDMthModelBase(const vector<cvOneDSubdomain*>& subdList,
                                       const vector<cvOneDFEAJoint*>& jtList,
                                       const vector<int>& olList){
  int i;
  int normalNodes = 0;
  int lagVariables = 0;
  for(i = 0; i < subdList.size(); i++){
    subdomainList.push_back(subdList[i]);
    normalNodes += subdList[i]->GetNumberOfNodes();
  }
  for(i = 0; i < jtList.size(); i++){
    jointList.push_back(jtList[i]);
    lagVariables += jtList[i]->getNumberOfSegments();
  }
  for(i = 0; i < olList.size(); i++){
    outletList.push_back(olList[i]);
  }
  // currently use flow rate and pressure, the equations
  // will be NumberOfSegmentsAttached - 1;
  type = nonlinear;
  numberOfEquations = normalNodes * 2 + lagVariables;
  equationNumbers = new long[numberOfEquations];
  assert( equationNumbers != 0);

  for(i = 0; i < numberOfEquations; i++){
    equationNumbers[i] = i;
  }

  flrt = NULL;
  time = NULL;
    for (int j = 0; j < 10; ++j) {
        coupling_flrt[j]=0.0;
    }

}

cvOneDMthModelBase::~cvOneDMthModelBase(){
  delete [] equationNumbers;
  if(flrt != NULL) delete [] flrt;
  if(time != NULL) delete [] time;
   // if(coupling_flrt != NULL) delete [] coupling_flrt;
}

void cvOneDMthModelBase::TimeUpdate(double pTime, double deltaT){
  previousTime = pTime;
  deltaTime = deltaT;
  currentTime = previousTime + deltaTime;
  impedIncr = 1;
}

void cvOneDMthModelBase::GetNodalEquationNumbers(long locNode, long* eqNumbers,long ithSubdomain){
  long prevID = subdomainList[ithSubdomain]->GetGlobal1stNodeID();
  // Equations are numbered consecutively following the nodal numeration
  eqNumbers[0] = 2 * (locNode + prevID);
  eqNumbers[1] = 2 * (locNode + prevID) + 1;
}

void cvOneDMthModelBase::GetEquationNumbers(long locElem, long* eqNumbers,
                                            long ithSubdomain){
  long connectivity[2];

  subdomainList[ithSubdomain]->GetConnectivity(locElem, connectivity);

  eqNumbers[0] = 2 * connectivity[0];     // S (area) node 1
  eqNumbers[1] = 2 * connectivity[0] + 1; // Q (flow rate) node 1
  eqNumbers[2] = 2 * connectivity[1];     // S (area) node 2
  eqNumbers[3] = 2 * connectivity[1] + 1; // Q (flow rate) node 2
}
double cvOneDMthModelBase::GetCouplingFlowRate(int coupling_id){
    // returns the flowrate set according to the id
    if(coupling_id>10){
        throw cvException("ERROR:Cannot handle more than 10 coupling conditions\n");
    }
    return coupling_flrt[coupling_id-1];
}

double cvOneDMthModelBase::SetCouplingFlowRate(double flow,int coupling_id){
    // returns the flowrate set according to the id
    if(coupling_id>10){
        throw cvException("ERROR:Cannot handle more than 10 coupling conditions\n");
    }
    return coupling_flrt[coupling_id-1]=flow;
}

void cvOneDMthModelBase::SetBoundaryConditions(){
  // This is to be called after the solution has been updated in the nonlinear
  // loop. It overwrites the components of the current solution so that
  // Dirichlet boundary conditions are always satisfied.

  long eqNumbers[2];  // two degress of freedom per node
  cvOneDSubdomain* sub;
  double InitialPressure;
  double currP, currS, resistance;

  // Set up Inlet Dirichlet boundary condition (the default is flow rate)
    // TODO Here is also one inflow assumption
  GetNodalEquationNumbers( 0, eqNumbers, 0);
  sub= subdomainList[0];
  switch(cvOneDBFSolver::inletBCtype){
    case BoundCondTypeScope::FLOW:
      (*currSolution)[eqNumbers[1]] = GetFlowRate();
      break;
    case BoundCondTypeScope::PRESSURE_WAVE:
      (*currSolution)[eqNumbers[0]] = sub->GetMaterial()->GetArea(GetFlowRate(),0);
       InitialPressure = flrt[0];
      break;
    default:

      break;
  }

  for (vector<int>::iterator it=outletList.begin(); it!=outletList.end(); it++){
    GetNodalEquationNumbers(subdomainList[*it]->GetNumberOfNodes() - 1, eqNumbers, *it);
    sub = subdomainList[*it];

    // Speciafically for RCR BC if treated as essential BC // added IV 051303
  /*    double alphaRCR, Rp, Rd, Cap, prevP;
      //double InitialQ;
      //double MemoI, MemoK, dMemoIdP, dMemoKdP;
      //double lhs_QQ, lhs_QS, rhs_Q;
      double MemoC;
      double z = sub->GetOutletZ();
  */
    switch(sub->GetBoundCondition()){
    case BoundCondTypeScope::PRESSURE:
        (*currSolution)[eqNumbers[0]] = sub->GetBoundFlowRate();
        break;
    case BoundCondTypeScope::COUPLING_3D_1D:
        // store here the equation number
        // change
        GetNodalEquationNumbers(0, eqNumbers, *it);
           // std::cout << "I have the Coupling ID "<< sub->GetCouplingID() << " and get the value: " << GetCouplingFlowRate(sub->GetCouplingID()) << std::endl;
        (*currSolution)[eqNumbers[1]] = GetCouplingFlowRate(sub->GetCouplingID());
        break;
    case BoundCondTypeScope::COUPLING_1D_3D:
        sub->SetBoundValue(coupling_pressure);
        //std::cout << "Me updating the coupling pressure: "<< coupling_pressure << " to: " << sub->GetMaterial()->GetArea(coupling_pressure,1.0) << std::endl;
        (*currSolution)[eqNumbers[0]] = sub->GetBoundFlowRate();
        break;
    case BoundCondTypeScope::FLOW:
      (*currSolution)[eqNumbers[1]] = sub->GetBoundFlowRate();
      break;

    case BoundCondTypeScope::RESISTANCE:
            if(cvOneDGlobal::CONSERVATION_FORM==0){
        currS = (*currSolution)[eqNumbers[0]];
          currP = sub->GetMaterial()->GetPressure(currS, sub->GetLength());
        resistance = sub->GetBoundResistance();
        (*currSolution)[eqNumbers[1]] = currP/resistance;
        }
      break;
    case BoundCondTypeScope::RESISTANCE_TIME:
            if(cvOneDGlobal::CONSERVATION_FORM == 0){
        currS = (*currSolution)[eqNumbers[0]];
          currP = sub->GetMaterial()->GetPressure(currS, sub->GetLength());
        resistance = sub->GetBoundResistance(currentTime);
        (*currSolution)[eqNumbers[1]] = currP/resistance;
        }
      break;

    case BoundCondTypeScope::RCR:
      break;
    default:
     break;
    }// end switch

  }// end for loop
}

// Eval Mass Balance
double cvOneDMthModelBase::CheckMassBalance(){

  long eqNumbers[2];  // Two degress of freedom per node
  double inletFlow = GetFlowRate();

  if(cvOneDBFSolver::inletBCtype == BoundCondTypeScope::FLOW or cvOneDBFSolver::inletBCtype ==BoundCondTypeScope::COUPLING_3D_1D ) {
      inletFlow = GetFlowRate();
  }else {
   GetNodalEquationNumbers( 0, eqNumbers, 0);
   inletFlow = (*currSolution)[eqNumbers[1]];
  }

  double outletFlow = 0;
  for (vector<int>::iterator it=outletList.begin(); it!=outletList.end(); it++){
    GetNodalEquationNumbers(subdomainList[*it]->GetNumberOfNodes() - 1, eqNumbers, *it);
    outletFlow += (*currSolution)[eqNumbers[1]];
  }
  if(cvOneDGlobal::debugMode){
    printf("(Debug) Inlet Flow: %e\n",inletFlow);
    printf("(Debug) Outlet Flow: %e\n",outletFlow);
  }
  return (inletFlow-outletFlow);
}

void cvOneDMthModelBase::ApplyBoundaryConditions(){
  char propName[256];
  // Make sure this is called after you have performed the assembly of the
  // global matrix and global vectors.
  //

  long eqNumbers[2];  // two degrees of freedom per node
  double value, currS, k_m, currP, prevP, currQ;
  double prevS;//added IV 080603
  int num_timesteps,j;
  double *h,*press;
  double rhsBC;//changed rhs to rhsBC to avoid confusion with other "rhs" IV 052003

  // Brooke's BC implementation
  if(cvOneDGlobal::CONSERVATION_FORM == 0){
    // Set up the inlet Dirichlet boundary condition (flow rate)
    // RHS corresponding to imposed Essential BC
    value = 0.0;
    // TODO: Here is also the implict assumption, that we add the dirichlet condtion only at 1
    if(cvOneDBFSolver::inletBCtype == BoundCondTypeScope::FLOW){
      GetNodalEquationNumbers(0, eqNumbers, 0);
      cvOneDGlobal::solver->SetSolution(eqNumbers[1], value);
    }else if (cvOneDBFSolver::inletBCtype == BoundCondTypeScope::PRESSURE_WAVE){
      GetNodalEquationNumbers(0, eqNumbers, 0);
      cvOneDGlobal::solver->SetSolution(eqNumbers[0], value);
    }

    // Set up the correct outlet boundary condition
    cvOneDSubdomain* sub;
    for (vector<int>::iterator it=outletList.begin(); it!=outletList.end(); it++){

      sub = subdomainList[*it];
      GetNodalEquationNumbers(sub->GetNumberOfNodes() - 1, eqNumbers, *it);

        // Speciafically for RCR BC - added IV 050803
      double alphaRCR, Rp, Rd, Cap;
      double DpDS;
      double lhs_QQ, lhs_QS, rhs_Q, MemoC; // For essential implementation
      double z = sub->GetOutletZ(); // Checked IV 02-03-03


      value = 0.0;  // RHS corresponding to imposed Essential BC

      switch(sub->GetBoundCondition()){
        case BoundCondTypeScope::COUPLING_1D_3D:
        case BoundCondTypeScope::PRESSURE:
        case BoundCondTypeScope::PRESSURE_WAVE:
          cvOneDGlobal::solver->SetSolution( eqNumbers[0], value);
          break;
          //case BoundCondTypeScope::FLOW:
          //cvOneDGlobal::solver->SetSolution( eqNumbers[1], value);
          //break;
        case BoundCondTypeScope::COUPLING_3D_1D:
            GetNodalEquationNumbers(0, eqNumbers, *it);
          cvOneDGlobal::solver->SetSolution( eqNumbers[1], value);
          break;

        case BoundCondTypeScope::RESISTANCE:
          currS = (*currSolution)[eqNumbers[0]];// dpds shouldn't be affected by p1
          k_m = sub->GetMaterial()->GetDpDS(currS, sub->GetLength())/ sub->GetBoundResistance();
          cvOneDGlobal::solver->Minus1dof(eqNumbers[1], k_m);
          break;

        case BoundCondTypeScope::RESISTANCE_TIME:
          currS = (*currSolution)[eqNumbers[0]];
          k_m = sub->GetMaterial()->GetDpDS(currS, sub->GetLength())/ sub->GetBoundResistance(currentTime);
          cvOneDGlobal::solver->Minus1dof(eqNumbers[1], k_m);
          break;

        //added by IV 051403
        case BoundCondTypeScope::RCR:
          currS = (*currSolution)[eqNumbers[0]];
          currP = sub->GetMaterial()->GetPressure(currS, sub->GetLength());
          Rp  = sub -> GetRp();
          Rd  = sub -> GetRd();
          Cap = sub -> GetCap();
          alphaRCR = sub -> GetAlphaRCR();
          prevP = sub->GetMaterial()->GetPressure(prevSolution->Get(eqNumbers[0]),z);
          DpDS= sub->GetMaterial()->GetDpDS(currS, z);
          //essential implementation tried like resistance and resistance_other->not ok
          MemoC = sub->MemC(currP, prevP, deltaTime, currentTime);//need MemC to be public
          currQ = (*currSolution)[eqNumbers[1]];
          lhs_QQ = 1;
          lhs_QS = DpDS*((1-exp(-alphaRCR*deltaTime))/(alphaRCR*Cap*Rp*Rp)-1/Rp);
          rhs_Q = -currQ + MemoC*exp(-alphaRCR*deltaTime) + currP/(Rp+Rd);
          cvOneDGlobal::solver->DirectAppResistanceBC(eqNumbers[1], -lhs_QQ, lhs_QS, rhs_Q);//minus because of current impl of function "try"
          break;

        default:

          cout<<"ERROR: boundary condition type not handled in ApplyBC"<<endl;
          exit(-1);
          break;
        }//end switch outletBC
      }//end outlet list loop
    }//end Brooke's BC


    //IV BC implementation, to specialize Inlet&Outlet fluxes
    //+ treat Dirichlet BC for pressure and flow rate BC
    //IV 03-20-03
    if(cvOneDGlobal::CONSERVATION_FORM == 1){

      // set up the inlet Dirichlet boundary condition (flow rate)
      // for these BC the Inlet term doesn't have to be specialized
      // so same treatment as regular Essential BC like in Brooke's
      value = 0.0;  // RHS corresponding to imposed Essential BC
        // TODO Here is also one inflow assumption
      if(cvOneDBFSolver::inletBCtype == BoundCondTypeScope::FLOW){
        GetNodalEquationNumbers( 0, eqNumbers, 0);
        cvOneDGlobal::solver->SetSolution( eqNumbers[1], value);
      }else if (cvOneDBFSolver::inletBCtype == BoundCondTypeScope::PRESSURE_WAVE){
        GetNodalEquationNumbers( 0, eqNumbers, 0);
        cvOneDGlobal::solver->SetSolution( eqNumbers[0], value);
      }


      // Set up the correct outlet boundary condition
      cvOneDSubdomain* sub;
      for (vector<int>::iterator it=outletList.begin(); it!=outletList.end(); it++){

        sub = subdomainList[*it];
        GetNodalEquationNumbers(sub->GetNumberOfNodes()- 1, eqNumbers, *it);
        cvOneDMaterial* material = sub->GetMaterial();
        strcpy(propName,"density");
        double density = material->GetProperty(propName);
        strcpy(propName,"delta");
        double delta = material->GetProperty(propName);
        strcpy(propName,"kinematic viscosity");
        double kinViscosity = material->GetProperty(propName);

        double OutletLHS[4];//OutletLHS[4]is an array that stores the element matrix for the Outlet term Outlet11,12,21,22
        double OutletRHS[2];//idem for OutletRHS[2]
        double z = sub->GetOutletZ();//checked IV 02-03-03

        currS = (*currSolution)[eqNumbers[0]];
        currP = material->GetPressure(currS, z);
        double DpDS = material->GetDpDS( currS, z);
        double IntegralpS = material->GetIntegralpS( currS, z);
        double So_= material->GetArea(material->GetReferencePressure(),z);

        // specifically for resistance BC
        double Resistance;
        // double Cp;
        double pd; //add distal pressure wgyang 2019/4
        double rhsQ;//outlet flow rate given by RCR solution Q(t) is expressed in terms of  pressure, wgyang 2019/4
        double convP; // convolution int(P(t')*exp(-alpharcr(t-t'))dt; wgyang/2019/4
        double convS; // convolution int(S(t')^(-3/2)*exp(-alpharcr(t-t'))dt; wgyang/2019/4
        double DQDS; // wgyang 2019/4
        double DQDp;
        double EHR=material->GetEHR(z); // 4*Eh/(3r0)
        double Eh=EHR*3.0/4.0*sqrt(So_/3.14);


        // for viscosity term in Resistance fluxes-have to be checked
        // double currS_BeginElem, DSDz, D2pDz, dpdz;

        // Specifically for RCR and wave BC// added IV 050803, modified 080603
        double alphaRCR, Rp, Rd, Cap;
        double InitialQ;
        double MemoQ, MemoK, dMemoIdP, dMemoKdP, dMemoIdS, dMemoKdS;
        double MemoI;//for RCR and possibly wave
        // double MemoH;//for waveBC
        // double lhs_QQ, lhs_QS, rhs_Q, MemoC;//for essential implementation

        //added for coronary boundary conditions kimhj 09022005
        double Ra1, Ra2, Ca, Cc, Rv1, P_v;
        double expo1COR, expo2COR, detCOR, CoefR;
        double p0COR, p1COR, p2COR, q0COR, q1COR, q2COR, b0COR, b1COR;
        double InitialdQdT, CurrentlvP, InitiallvP;
        double InitCOR1, InitCOR2;
        double MemoI1, MemoI2, dMemoI1dP, dMemoI2dP;

        value = 0.0;  // RHS corresponding to imposed Essential BC
        switch(sub->GetBoundCondition()){
          // set up the inlet Dirichlet boundary condition (flow rate)
          // for these BC the Inlet term doesn't have to be specialized
          // so same treatment as regular Essential BC like in Brooke's
          case BoundCondTypeScope::PRESSURE:
              case BoundCondTypeScope::COUPLING_1D_3D:
          case BoundCondTypeScope::PRESSURE_WAVE:
            cvOneDGlobal::solver->SetSolution( eqNumbers[0], value);
            break;
          case BoundCondTypeScope::FLOW:
            cvOneDGlobal::solver->SetSolution( eqNumbers[1], value);
            break;
            case BoundCondTypeScope::COUPLING_3D_1D:
                GetNodalEquationNumbers(0, eqNumbers, *it);
                cvOneDGlobal::solver->SetSolution( eqNumbers[1], value);
                break;


          case BoundCondTypeScope::RESISTANCE:

            // Cp = material->GetLinCompliance(z);
            // double Cp = material->GetnonLinCompliance(currS, z);//tried 02-13-03 worse results

            //Resistance = sub->GetBoundResistance();
            Resistance=  sub->GetResistanceR(); //wgyang get resistance and Pd values;
            pd=sub->GetResistancePd();

            // for Resistance with P-P1=Q*R add to Resistance part
            // currP= material->GetPressure( currS, z)-material->p1;//for P-P1=Q*R

            // for viscosity term in fluxes-have to be checked
            /*  double currS_BeginElem = (*currSolution)[eqNumbers[0]-2];
            double DSDz  = (currS-currS_BeginElem)/(sub->GetLength()/sub->GetNumberOfElements());
            double D2pDz = material->GetDpDz( currS, z);//zero of straight tube
            double dpdz  = DpDS*DSDz+ D2pDz;
            //*/
            OutletLHS[0] = -deltaTime*DpDS/Resistance;
            OutletLHS[1] = 0.0;
            //OutletLHS[2] = -deltaTime*(currS/density/Cp);//linear downstream domain-Hughes
            //OutletLHS[2] = -deltaTime*(currS/density/Cp-(1+delta)*currP*currP/pow(Resistance*currS,2)
            //  +2*(1+delta)*DpDS*currP/currS/pow(Resistance,2));//linearized IntegralpS
            OutletLHS[2] = -deltaTime*(DpDS*currS/density-(1+delta)*(currP-pd)*(currP-pd)/pow(Resistance*currS,2)
              +2*(1+delta)*DpDS*(currP-pd)/currS/pow(Resistance,2));//without viscosity term
            //OutletLHS[2] = -deltaTime*(DpDS*currS/density);//without adv term
            //OutletLHS[2] = 0.0;//no M2 h2
            OutletLHS[3] = 0.0;

            //finiteElement->Evaluate( z, shape, DxShape, &jacobian);//careful: shape is in the natural coord system (xi)

            OutletRHS[0] = deltaTime*(currP-pd)/Resistance;
            //OutletRHS[1] = deltaTime*(currS*currS/(2.0*density*Cp) - So_*So_/(2*density*Cp));//linear downstream domain-Hughes
            //OutletRHS[1] = deltaTime*((1+delta)*pow(currP/Resistance,2)/currS+currS*currS/(2.0*density*Cp) - So_*So_/(2*density*Cp));
            //OutletRHS[1] = deltaTime*((1+delta)*pow(currP/Resistance,2)/currS+So_*(currS-So_)/Cp/density);//linearized IntegralpS
            OutletRHS[1] = deltaTime*((1.0+delta)*(currP-pd)*(currP-pd)/currS/pow(Resistance,2)+IntegralpS/density);//without viscosity term

              //-kinViscosity*dpdz/Resistance);
            //OutletRHS[1] = deltaTime*(IntegralpS/density);//without advective term
            //OutletRHS[1] =0;//no M2 h2
            //  cout<<(So_*(currS-So_)/Cp-IntegralpS)/IntegralpS*100<<" "<<endl;
            //cout<<((1.0+delta)*currP*currP/currS/pow(Resistance,2))/IntegralpS*density<<endl;

            cvOneDGlobal::solver->AddFlux( eqNumbers[1],&(OutletLHS[0]),&(OutletRHS[0]));//specialize the Outlet flux term in LHS and RHS

            // Essential way of treating resistance BC- as in Brooke's
            // k_m = sub->GetMaterial()->GetDpDS(currS, sub->GetLength())/ sub->GetBoundResistance();
            // LinearSolver::Minus1dof(eqNumbers[1], k_m);

            // Natural way of treating resistance BC
            break;

          case BoundCondTypeScope::RESISTANCE_TIME://natural way of treating BC, very similar to Resistance BC
            Resistance = sub->GetBoundResistance(currentTime);
            OutletLHS[0] = -deltaTime*DpDS/Resistance;
            OutletLHS[1] = 0.0;
            OutletLHS[2] = -deltaTime*(DpDS*currS/density-(1+delta)*currP*currP/pow(Resistance*currS,2)
                           +2*(1+delta)*DpDS*currP/currS/pow(Resistance,2));
            OutletLHS[3] = 0.0;

            OutletRHS[0] = deltaTime*currP/Resistance;
            OutletRHS[1] = deltaTime*((1.0+delta)*currP*currP/currS/pow(Resistance,2)+IntegralpS/density);
            //-kinViscosity*dpdz/Resistance);

            cvOneDGlobal::solver->AddFlux( eqNumbers[1],&(OutletLHS[0]),&(OutletRHS[0]));//specialize the Outlet flux term in LHS and RHS
            break;

          case BoundCondTypeScope::RCR:
            Rp  = sub -> GetRp();
            Rd  = sub -> GetRd();
            Cap = sub -> GetCap();
            alphaRCR = sub -> GetAlphaRCR();
            InitialQ = sub -> GetInitialFlow();
            prevP = material->GetPressure(prevSolution->Get(eqNumbers[0]),z);
/*
           //Irene's implementation, RCR without Pd
            MemoI = sub->MemIntRCR(currP, prevP, deltaTime, currentTime);
            MemoK = sub->MemAdvRCR(currP, prevP, deltaTime, currentTime);
            dMemoIdP = sub->dMemIntRCRdP(deltaTime);
            dMemoKdP = sub->dMemAdvRCRdP(currP, prevP, deltaTime, currentTime);

            //Neumann implementation
           OutletLHS[0] = -deltaTime*DpDS/Rp+DpDS/(Rp*Rp*Cap)*dMemoIdP;
            OutletLHS[1] = 0.0;
            //OutletLHS[2] = -deltaTime*(DpDS*currS/density);//without adv term
            OutletLHS[2] = -deltaTime*(DpDS*currS/density)+(1+delta)*MemoK/(currS*currS)-(1+delta)*DpDS/currS*dMemoKdP;//with adv term
           OutletLHS[3] = 0.0;

           OutletRHS[0] = deltaTime*currP/Rp +(InitialQ-material->GetReferencePressure()/Rp)*(exp(alphaRCR*deltaTime)-1)*exp(-alphaRCR*currentTime)/alphaRCR - MemoI/(Rp*Rp*Cap);
           OutletRHS[1] = deltaTime*IntegralpS/density + (1+delta)/currS*MemoK;
            //viscosity term ignored;
*/
/////////////////////////////////////////////////////////////////////////////
          //RCR with Pd  wgyang 2019/4
            pd=sub->GetResistancePd();
            convP=sub->MemIntPexp(currP, deltaTime, currentTime);
            rhsQ=currP/Rp-pd/(Rp+Rd) +(InitialQ-material->GetReferencePressure()/Rp+pd/(Rp+Rd))*exp(-alphaRCR*currentTime) - convP/(Rp*Rp*Cap);
             // convP(t)=int(P(t')exp(-alphaRCR*(t-t'))dt= convP(t-dt)*exp(-alphaRCR*dt)+int^t_(t-dt) (P(t')exp(-alphaRCR(t-t'))dt,
             // assumming constant P in dt, the second term = P/alpha *(1-exp(-alphaRCR*dt)), dconvP(t-dt)/dS=0 dconvP(t)dS=dconvP/dp*DpDs=(1-exp(-alphaRCR*dt))/alpha*DpDS
            DQDS=1.0/Rp*DpDS-(1-exp(-alphaRCR*deltaTime))/alphaRCR*DpDS/(Rp*Rp*Cap);

            OutletRHS[0]=deltaTime*rhsQ;
            OutletRHS[1]=deltaTime*((1.0+delta)*rhsQ*rhsQ/currS+IntegralpS/density);
            OutletLHS[0] = -deltaTime*DQDS;
            OutletLHS[1] = 0.0;
            OutletLHS[2] = -deltaTime*(DpDS*currS/density)+deltaTime*(1+delta)*(rhsQ*rhsQ)/(currS*currS)-2.0*(1+delta)*rhsQ/currS*DQDS*deltaTime;
            OutletLHS[3] = 0.0;
/////////////////////////////////////////////////////////////////////////
            cvOneDGlobal::solver->AddFlux( eqNumbers[1],&(OutletLHS[0]),&(OutletRHS[0]));//specialize the Outlet flux term in LHS and RHS

            /* //essential implementation tried like resistance and resistance_other->not ok
            MemoC = sub->MemC(currP, prevP, deltaTime, currentTime);//need MemC to be public
            currQ = (*currSolution)[eqNumbers[1]];
            //currP = currP - sub->GetMaterial()->p1; // from Brooke
            lhs_QQ = 1;
            lhs_QS = DpDS*((1-exp(-alphaRCR*deltaTime))/(alphaRCR*Cap*Rp*Rp)-1/Rp);
            rhs_Q = -currQ + MemoC*exp(-alphaRCR*deltaTime) + currP/(Rp+Rd);
            LinearSolver::DirectAppResistanceBC(eqNumbers[1], -lhs_QQ, lhs_QS, rhs_Q);//minus because of current impl of function "try"
            //LinearSolver::Minus1dof(eqNumbers[1], lhs_QS);*/
            break;


        // added Jongmin Seo & Hyunjin Kim 04062020
          case BoundCondTypeScope::CORONARY:
            Ra1=sub -> GetRa1(); //5*1333.27;
            Ra2=sub -> GetRa2(); //5*1333.27;
            Ca=sub -> GetCa(); //0.015/21333.27;
            Cc=sub -> GetCc(); //0.04/1333.27;
            Rv1=sub -> GetRv1();
            P_v=sub -> GetP_v();
            p0COR=1;
            p1COR=Ra2*Ca+(Rv1)*(Ca+Cc);
            p2COR=Ca*Cc*Ra2*(Rv1);
            q0COR=Ra1+Ra2+Rv1;
            q1COR=Ra1*Ca*(Ra2+Rv1)+Cc*(Rv1)*(Ra1+Ra2);
            q2COR=Ca*Cc*Ra1*Ra2*(Rv1);
            b0COR=0;
            b1COR=Cc*(Rv1);
            detCOR=sqrt(q1COR*q1COR-4*q0COR*q2COR);
            expo2COR=-(q1COR+detCOR)/2/q2COR;
            expo1COR=q0COR/q2COR/expo2COR;
            CoefR=p2COR/q2COR;

            CurrentlvP = sub->getBoundCoronaryValues(currentTime);
            InitiallvP = sub->getBoundCoronaryValues(0);

            prevP = material->GetPressure(prevSolution->Get(eqNumbers[0]),z);

            currP = currP + P_v;
            prevP = prevP + P_v;
            CurrentlvP = CurrentlvP + P_v;
            InitiallvP = InitiallvP + P_v;

            MemoI1 = sub->MemIntCoronary(currP, prevP, deltaTime, currentTime, expo1COR);
            MemoI2 = sub->MemIntCoronary(currP, prevP, deltaTime, currentTime, expo2COR);
            MemoK = sub->MemAdvCoronary(currP, prevP, deltaTime, currentTime);
            dMemoI1dP = sub->dMemIntCoronarydP(deltaTime, expo1COR);
            dMemoI2dP = sub->dMemIntCoronarydP(deltaTime, expo2COR);
            dMemoKdP = sub->dMemAdvCoronarydP(currP, prevP, deltaTime, currentTime);
            InitCOR1 = sub->CORic1();
            InitCOR2 = sub->CORic2();

            //Neumann implementation
            OutletLHS[0] = -DpDS*(CoefR*deltaTime+dMemoI1dP-dMemoI2dP);
            OutletLHS[1] = 0.0;
            OutletLHS[2] = -deltaTime*(DpDS*currS/density)+(1+delta)*MemoK/(currS*currS)-(1+delta)*DpDS/currS*dMemoKdP;//with adv term
            OutletLHS[3] = 0.0;

            OutletRHS[0] = InitCOR1/expo1COR*exp(expo1COR*currentTime)*(1-exp(-expo1COR*deltaTime))
                          -InitCOR2/expo2COR*exp(expo2COR*currentTime)*(1-exp(-expo2COR*deltaTime))
                          +CoefR*currP*deltaTime+MemoI1-MemoI2;
            OutletRHS[1] = deltaTime*IntegralpS/density + (1+delta)/currS*MemoK;

            //viscosity term ignored;

            cvOneDGlobal::solver->AddFlux( eqNumbers[1],&(OutletLHS[0]),&(OutletRHS[0]));//specialize the Outlet flux term in LHS and RHS
            break;


          case BoundCondTypeScope::NOBOUND:

          default:
            cout<<"ERROR:boundary condition type not handled in ApplyBC"<<endl;
            exit(-1);
              break;
        }//end switch BC
      }//end loop over outlets
  }//end Irene's BC
}//end ApplyBC


void cvOneDMthModelBase::SetInflowRate(double *t, double *flow, int size, double cycleT){
  int i;
  time = new double[size];
  flrt = new double[size];
  for(i = 0; i < size; i++){
  time[i] = t[i];
  flrt[i] = flow[i];
  }
  nFlowPts=size;
  cycleTime = cycleT;
  //nFlowPts = size; //added by bnsteel
}

void cvOneDMthModelBase::UpdateCouplingPressure(double pressure){
    coupling_pressure=pressure;
}

int cvOneDMthModelBase::Get_Pressure_Position(){
    if(inletpressure_List.size()>1){
        std::cout<<"Congratulations, currently we only support 1 coupling Condition. Feel free to add some more."<< std::endl;
        throw "FAIL";
    }
    if( inletpressure_List.empty()){
        return 0;
    }

    return inletpressure_List.front();
}

void cvOneDMthModelBase::UpdateInflowRate(double flow,double search_time) {
    //int step=0;
    int step=nFlowPts;
    for (int i = 0; i < nFlowPts; ++i) {
        if(abs(time[i]-search_time) < abs( time[step] - search_time)){
            step= i;
            //break;
        }
    }

    if(step >nFlowPts || step<0){
        printf("%d exceeds the array %d size of inflow rates ",step, nFlowPts);
        throw ("no segfault.");
    }

    flrt[step] = flow;
}

double cvOneDMthModelBase::GetFlowRate(){

  // Check if flow or time are defined
  if(time == NULL || flrt == NULL){
    cout << "ERROR: inflow information is not prescribed!"<< endl;
    cout<<"Time and flow rate: " << time << ", " << flrt << endl;
    exit(1);
  }

  // Flow rate is assumed to be periodic
  double correctedTime = currentTime - static_cast<long>(currentTime / cycleTime) * cycleTime;
  //printf("Corrected Time: %e\n",correctedTime);

  int ptr = 0;
  bool wasFound = false;
  while( !wasFound){
    if( correctedTime >= time[ptr] && correctedTime <= time[ptr+1])
      wasFound = true;
    else
      ptr++;
  }

  // linear interpolation between values
  double xi = (correctedTime - time[ptr]) / (time[ptr+1] - time[ptr]);
  // Return
  double result = flrt[ptr] + xi * (flrt[ptr+1] - flrt[ptr]);
  return result;

}
