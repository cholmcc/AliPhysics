#ifndef AliAnalysisTaskSEHFv2_H
#define AliAnalysisTaskSEHFv2_H

/* Copyright(c) 1998-2010, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

//*************************************************************************
// AliAnalysisTaskSEHFv2 gives the needed tools for the D
// mesons v2 analysis
// Authors: Chiara Bianchin, cbianchi@pd.infn.it,
//          Robert Grajcarek, grajcarek@physi.uni-heidelberg.de
//          Giacomo Ortona, ortona@to.infn.it,
//          Carlos Perez Lara, carlos.eugenio.perez.lara@cern.ch
//          Francesco Prino, prino@to.infn.it
//
//*************************************************************************

/* $Id$ */

#include "AliAnalysisTaskSE.h"
#include "AliAnalysisVertexingHF.h"
#include "AliHFAfterBurner.h"
#include "AliQnCorrectionsQnVector.h"

class TH1F;
class TH2F;
class TH2D;
class AliMultiDimVector;
class AliRDHFCuts;
class TVector2;

class AliAnalysisTaskSEHFv2 : public AliAnalysisTaskSE
{
    
public:
    
    enum DecChannel{kDplustoKpipi,kD0toKpi,kDstartoKpipi,kDstoKKpi}; //more particles can be added
    enum EventPlaneMeth{kTPC,kTPCVZERO,kVZERO,kVZEROA,kVZEROC}; //Event plane to be calculated in the task
    //  enum SubEvents{kFullTPC,kPosTPC,kNegTPC,kSingleV0Side}; //Sub-events for V0 EP
    
    AliAnalysisTaskSEHFv2();
    AliAnalysisTaskSEHFv2(const char *name, AliRDHFCuts *rdCuts, Int_t decaychannel);
    
    virtual ~AliAnalysisTaskSEHFv2();

    void SetAODMismatchProtection(Int_t opt=1) {fAODProtection=opt;}
    void SetReadMC(Bool_t readMC=kTRUE){fReadMC=readMC;}
    void SetMassLimits(Float_t range,Int_t pdg);
    void SetMassLimits(Float_t lowlimit, Float_t uplimit);
    void SetNMassBins(Int_t nbins){fNMassBins=nbins;}
    void SetV0EventPlaneOrder(Int_t n){fV0EPorder=n;}
    void SetMinCentrality(Int_t mincentr){fMinCentr=mincentr;}
    void SetMaxCentrality(Int_t maxcentr){fMaxCentr=maxcentr;}
    void SetUseAfterBurner(Bool_t ab){fUseAfterBurner=ab;}
    void SetAfterBurner(AliHFAfterBurner *ab){fAfterBurner=ab;}
    void SetEtaGapFeatureForEventplaneFromTracks (Bool_t etaGap) {fEtaGap = etaGap;}
    void SetCentralityBinWidthPerMil(Int_t w){fCentBinSizePerMil=w;}
    
    
    void SetNTPCSubEvents(Int_t nsub);
    void Set2TPCEPSubEvents(){SetNTPCSubEvents(2);}
    void Set3TPCEPSubEvents(){SetNTPCSubEvents(3);}
    void SetEventPlaneMethod(Int_t epmethod);
    void SetTPCEPOnly(){SetEventPlaneMethod(kTPC);}
    void SetVZEROEP(){SetEventPlaneMethod(kVZERO);}
    void SetVZEROAEP(){SetEventPlaneMethod(kVZEROA);}
    void SetVZEROCEP(){SetEventPlaneMethod(kVZEROC);}
    void SetTPCEP(){SetEventPlaneMethod(kTPCVZERO);}
    void SetEventPlanesCompatibility(Float_t comp) {fEventPlanesComp=comp;}
    void SetUseNewQnCorrFw(Bool_t flag) {fUseNewQnCorrFw=flag;}
    //void SetSubEvents(Int_t subev){if(subev>=kFullTPC&&subev<=kSingleV0Side)fSubEvents=subev;}
    
    Int_t GetEventPlaneMethod()const {return fEventPlaneMeth;}
    Int_t GetNTPCSubEvents()const {return fSubEvents;}
    Float_t GetEventPlanesCompatibility()const {return fEventPlanesComp;}
    Float_t GetUpperMassLimit()const {return fUpmasslimit;}
    Float_t GetLowerMassLimit()const {return fLowmasslimit;}
    Int_t GetNMassBins()const {return fNMassBins;}
    //Float_t GetPhi02Pi(Float_t phi);
    Float_t GetPhi0Pi(Float_t phi);
    AliHFAfterBurner *GetAfterBurner()const {return fAfterBurner;}
    const AliQnCorrectionsQnVector *GetQnVectorFromList( const TList *list,
                                                         const char *subdetector,
                                                         const char *expectedstep,
                                                         const char *altstep);

    // Implementation of interface methods
    virtual void UserCreateOutputObjects();
    virtual void LocalInit();// {Init();}
    virtual void UserExec(Option_t *option);
    virtual void Terminate(Option_t *option);
    
private:
    
    AliAnalysisTaskSEHFv2(const AliAnalysisTaskSEHFv2 &source);
    AliAnalysisTaskSEHFv2& operator=(const AliAnalysisTaskSEHFv2& source);
    
    void CalculateInvMasses(AliAODRecoDecayHF* d,Float_t* &masses,Int_t& nmasses);
    
    void FillDplus(AliAODRecoDecayHF* d,TClonesArray *arrayMC,Int_t ptbin, Float_t dphi, const Float_t* masses,Int_t isSel,Int_t icentr, Double_t phiD);
    void FillD02p(AliAODRecoDecayHF* d,TClonesArray *arrayMC,Int_t ptbin, Float_t dphi, const Float_t* masses, Int_t isSel,Int_t icentr, Double_t phiD);
    void FillDstar(AliAODRecoDecayHF* d,TClonesArray *arrayMC,Int_t ptbin, Float_t dphi, const Float_t* masses,Int_t isSel,Int_t icentr, Double_t phiD);
    void FillDs(AliAODRecoDecayHF* d,TClonesArray *arrayMC,Int_t ptbin, Float_t dphi, const Float_t* masses,Int_t isSel,Int_t icentr, Double_t phiD);
    Float_t GetEventPlaneForCandidate(AliAODRecoDecayHF* d, const TVector2* q,AliEventplane *pl,const TVector2* qsub1,const TVector2* qsub2);
    //  Float_t GetEventPlaneFromV0(AliAODEvent *aodEvent);
    
    TH1F* hEvPlaneQncorrTPC;      //! histogram for EP
    TH1F* hEvPlaneQncorrVZEROA;   //! histogram for EP
    TH1F* hEvPlaneQncorrVZEROC;   //! histogram for EP
    TH1F* hEvPlaneQncorrResol;   //! histogram for EP
    TH1F* fhEventsInfo;           //! histogram send on output slot 1
    TH1F *fHistCentrality[2];//!<!hist. for cent distr (all,sel ev, )
    TH2F *fHistCentralityMult[2];//!<!hist. for cent distr vs mult (all,sel ev, )
    TList   *fOutput;             //! list send on output slot 2
    AliRDHFCuts *fRDCuts;         //cut values (saved in slot 3)
    Float_t fLowmasslimit;        //lower inv mass limit for histos
    Float_t fUpmasslimit;         //upper inv mass limit for histos
    Int_t fNPtBins;               //number of pt bins
    Int_t fNMassBins;             //number of bins in the mass histograms
    Bool_t fReadMC;               //flag for access to MC
    Bool_t fUseAfterBurner;      //enable afterburning
    Int_t fDecChannel;            //decay channel identifier
    AliHFAfterBurner *fAfterBurner;//Afterburner options
    Int_t fEventPlaneMeth;         //flag to select EP method
    Float_t fEventPlanesComp;     // Maximum distance between TPC/VZERO event planes
    Int_t  fV0EPorder;            //harmonic for VZERO event plane
    Int_t fMinCentr;              //minimum centrality
    Int_t fMaxCentr;              //maximum centrality
    Bool_t fEtaGap;               // Eta gap feature for Eventplane from tracks; be careful that you do the correct settings in AddTaskEventPlane.C !!!!
    Int_t fSubEvents;             //Sub-events definition for TPC EP
    Int_t fCentBinSizePerMil;     // width of centrality bins
    Int_t fAODProtection;         /// flag to activate protection against AOD-dAOD mismatch.
                                  /// -1: no protection,  0: check AOD/dAOD nEvents only,  1: check AOD/dAOD nEvents + TProcessID names
    Bool_t fSetName;              //to set name of histos
    Bool_t fUseNewQnCorrFw;       //flag tu use the new Qn correction framework
    
    ClassDef(AliAnalysisTaskSEHFv2,3); // AliAnalysisTaskSE for the HF v2 analysis
};

#endif