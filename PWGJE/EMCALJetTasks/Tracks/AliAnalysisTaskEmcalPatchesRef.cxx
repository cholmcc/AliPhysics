/**************************************************************************
 * Copyright(c) 1998-2015, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
#include <array>
#include <iostream>
#include <map>
#include <vector>

#include <TClonesArray.h>
#include <TGrid.h>
#include <THistManager.h>
#include <THashList.h>
#include <TLinearBinning.h>
#include <TObjArray.h>
#include <TParameter.h>

#include "AliAnalysisUtils.h"
#include "AliESDEvent.h"
#include "AliEMCALTriggerPatchInfo.h"
#include "AliEmcalTriggerOfflineSelection.h"
#include "AliInputEventHandler.h"
#include "AliLog.h"
#include "AliMultSelection.h"
#include "AliMultEstimator.h"
#include "AliOADBContainer.h"

#include "AliAnalysisTaskEmcalPatchesRef.h"

/// \cond CLASSIMP
ClassImp(EMCalTriggerPtAnalysis::AliAnalysisTaskEmcalPatchesRef)
/// \endcond

namespace EMCalTriggerPtAnalysis {

/**
 * Dummy (I/O) onstructor
 */
AliAnalysisTaskEmcalPatchesRef::AliAnalysisTaskEmcalPatchesRef() :
    AliAnalysisTaskEmcal(),
    fTriggerSelection(nullptr),
    fAcceptTriggers(),
    fHistos(nullptr),
    fRequestAnalysisUtil(kTRUE),
    fTriggerStringFromPatches(kFALSE),
    fCentralityRange(-999., 999.),
    fVertexRange(-999., 999.),
    fRequestCentrality(false),
    fNameDownscaleOADB(""),
    fDownscaleOADB(nullptr),
    fDownscaleFactors(nullptr)
{
  SetCaloTriggerPatchInfoName("EmcalTriggers");
}

/**
 * Named constructor
 * @param name Name of the task
 */
AliAnalysisTaskEmcalPatchesRef::AliAnalysisTaskEmcalPatchesRef(const char *name):
    AliAnalysisTaskEmcal(name, kTRUE),
    fTriggerSelection(nullptr),
    fAcceptTriggers(),
    fHistos(nullptr),
    fRequestAnalysisUtil(kTRUE),
    fTriggerStringFromPatches(kFALSE),
    fCentralityRange(-999., 999.),
    fVertexRange(-999., 999.),
    fRequestCentrality(false),
    fNameDownscaleOADB(""),
    fDownscaleOADB(nullptr),
    fDownscaleFactors(nullptr)
{
  SetCaloTriggerPatchInfoName("EmcalTriggers");
}

/**
 * Destructor
 */
AliAnalysisTaskEmcalPatchesRef::~AliAnalysisTaskEmcalPatchesRef() {
}

/**
 * Creating output histograms:
 * + Patch (calibrated) energy spectrum - separated by patch type - for different trigger classes
 * + Patch eta-phi map - separated by patch type - for different trigger classes and different min. energies
 */
void AliAnalysisTaskEmcalPatchesRef::UserCreateOutputObjects(){
  AliInfoStream() <<  "Creating histograms for task " << GetName() << std::endl;

  if(fRequestAnalysisUtil && ! fAliAnalysisUtils) fAliAnalysisUtils = new AliAnalysisUtils;

  EnergyBinning energybinning;
  TLinearBinning etabinning(100, -0.7, 0.7);
  fHistos = new THistManager("Ref");
  std::array<TString, 21> triggers = {"MB", "EMC7", "DMC7",
      "EJ1", "EJ2", "EG1", "EG2", "DJ1", "DJ2", "DG1", "DG2",
      "EMC7excl", "DMC7excl", "EG2excl", "EJ2excl", "DG2excl", "DJ2excl",
      "EG1excl", "EJ1excl", "DG1excl", "DJ1excl"
  };
  std::array<TString, 10> patchtypes = {"EG1", "EG2", "EJ1", "EJ2", "EMC7", "DG1", "DG2", "DJ1", "DJ2", "DMC7"};
  Double_t encuts[5] = {1., 2., 5., 10., 20.};
  for(auto trg : triggers){
    fHistos->CreateTH1(Form("hEventCount%s", trg.Data()), Form("Event count for trigger class %s", trg.Data()), 1, 0.5, 1.5);
    fHistos->CreateTH1(Form("hEventCentrality%s", trg.Data()), Form("Event centrality for trigger class %s", trg.Data()), 103, -2., 101.);
    fHistos->CreateTH1(Form("hVertexZ%s", trg.Data()), Form("z-position of the primary vertex for trigger class %s", trg.Data()), 200, -40., 40.);
    for(auto patch : patchtypes){
      fHistos->CreateTH1(Form("h%sPatchEnergy%s", patch.Data(), trg.Data()), Form("%s-patch energy for trigger class %s", patch.Data(), trg.Data()), energybinning);
      fHistos->CreateTH1(Form("h%sPatchET%s", patch.Data(), trg.Data()), Form("%s-patch transverse energy for trigger class %s", patch.Data(), trg.Data()), energybinning);
      fHistos->CreateTH2(Form("h%sPatchEnergyEta%s", patch.Data(), trg.Data()), Form("%s-patch energy for trigger class %s", patch.Data(), trg.Data()), energybinning, etabinning);
      fHistos->CreateTH2(Form("h%sPatchETEta%s", patch.Data(), trg.Data()), Form("%s-patch transverse energy for trigger class %s", patch.Data(), trg.Data()), energybinning, etabinning);
      for(int ien = 0; ien < 5; ien++){
        fHistos->CreateTH2(Form("h%sEtaPhi%dG%s", patch.Data(), static_cast<int>(encuts[ien]), trg.Data()), Form("%s-patch #eta-#phi map for patches with energy larger than %f GeV/c for trigger class %s", patch.Data(), encuts[ien], trg.Data()), 100, -0.7, 0.7, 200, 0, TMath::TwoPi());
        fHistos->CreateTH2(Form("h%sColRow%dG%s", patch.Data(), static_cast<int>(encuts[ien]), trg.Data()), Form("%s-patch col-row map for patches with energy larger than %f GeV/c for trigger class %s", patch.Data(), encuts[ien], trg.Data()), 48, -0.5, 47.5, 104, -0.5, 103.5);
      }
    }
  }
  for(auto h : *(fHistos->GetListOfHistograms())) fOutput->Add(h);
  PostData(1, fOutput);
  AliDebugStream(1) << "Histograms done" << std::endl;
}

bool AliAnalysisTaskEmcalPatchesRef::IsEventSelected(){
  fAcceptTriggers.clear();
  TString triggerstring = "";
  if(fTriggerStringFromPatches){
    triggerstring = GetFiredTriggerClassesFromPatches(fTriggerPatchInfo);
  } else {
    triggerstring = fInputEvent->GetFiredTriggerClasses();
  }
  UInt_t selectionstatus = fInputHandler->IsEventSelected();
  Bool_t isMinBias = selectionstatus & AliVEvent::kINT7,
      isEMC7 = (selectionstatus & AliVEvent::kEMC7) && triggerstring.Contains("EMC7"),
      isEJ1 = (selectionstatus & AliVEvent::kEMCEJE) && triggerstring.Contains("EJ1"),
      isEJ2 = (selectionstatus & AliVEvent::kEMCEJE) && triggerstring.Contains("EJ2"),
      isEG1 = (selectionstatus & AliVEvent::kEMCEGA) && triggerstring.Contains("EG1"),
      isEG2 = (selectionstatus & AliVEvent::kEMCEGA) && triggerstring.Contains("EG2"),
      isDMC7 = (selectionstatus & AliVEvent::kEMC7) && triggerstring.Contains("DMC7"),
      isDJ1 = (selectionstatus & AliVEvent::kEMCEJE) && triggerstring.Contains("DJ1"),
      isDJ2 = (selectionstatus & AliVEvent::kEMCEJE) && triggerstring.Contains("DJ2"),
      isDG1 = (selectionstatus & AliVEvent::kEMCEGA) && triggerstring.Contains("DG1"),
      isDG2 = (selectionstatus & AliVEvent::kEMCEGA) && triggerstring.Contains("DG2");
  if(fTriggerPatchInfo && fTriggerSelection){
      isEMC7 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgEL0, fTriggerPatchInfo);
      isEJ1 &=  fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgEJ1, fTriggerPatchInfo);
      isEJ2 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgEJ2, fTriggerPatchInfo);
      isEG1 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgEG1, fTriggerPatchInfo);
      isEG2 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgEG2, fTriggerPatchInfo);
      isDMC7 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgDL0, fTriggerPatchInfo);
      isDJ1 &=  fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgDJ1, fTriggerPatchInfo);
      isDJ2 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgDJ2, fTriggerPatchInfo);
      isDG1 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgDG1, fTriggerPatchInfo);
      isDG2 &= fTriggerSelection->IsOfflineSelected(AliEmcalTriggerOfflineSelection::kTrgDG2, fTriggerPatchInfo);

  }
  if(!(isMinBias || isEMC7 || isEG1 || isEG2 || isEJ1 || isEJ2 || isDMC7 || isDG1 || isDG2 || isDJ1 || isDJ2)){
    AliDebugStream(1) << GetName() << ": Reject trigger" << std::endl;
    return false;
  }
  AliDebugStream(1) << "Trigger selected" << std::endl;
  // In case a centrality estimator is used, event selection,
  // otherwise ignore event selection from multiplicity task
  double centrality = -1;
  if(fRequestCentrality){
    AliMultSelection *mult = dynamic_cast<AliMultSelection *>(InputEvent()->FindListObject("MultSelection"));
    if(!mult){
      AliErrorStream() << GetName() << ": Centrality selection enabled but no centrality estimator available" << std::endl;
      return false;
    }
    if(!mult->IsEventSelected()) return false;
    centrality =  mult->GetEstimator("V0M")->GetPercentile();
    AliDebugStream(1) << GetName()  << ": Centrality " << centrality << std::endl;
    if(!fCentralityRange.IsInRange(centrality)){
      AliDebugStream(1) << GetName() << ": reject centrality: " << centrality << std::endl;
      return false;
    } else {
      AliDebugStream(1) << GetName() << ": select centrality " << centrality << std::endl;
    }
  } else {
    AliDebugStream(1) << GetName() << ": No centrality selection required" << std::endl;
  }
  const AliVVertex *vtx = fInputEvent->GetPrimaryVertex();
  if(!vtx) vtx = fInputEvent->GetPrimaryVertexSPD();
  //if(!fInputEvent->IsPileupFromSPD(3, 0.8, 3., 2., 5.)) return;         // reject pileup event
  if(vtx->GetNContributors() < 1){
    AliDebug(1, Form("%s: Reject contributor\n", GetName()));
    return false;
  }
  // Fill reference distribution for the primary vertex before any z-cut
  if(fRequestAnalysisUtil){
    AliDebugStream(1) << GetName() << ": Reject analysis util" << std::endl;
    if(fInputEvent->IsA() == AliESDEvent::Class() && fAliAnalysisUtils->IsFirstEventInChunk(fInputEvent)) return false;
    if(!fAliAnalysisUtils->IsVertexSelected2013pA(fInputEvent)) return false;       // Apply new vertex cut
    if(fAliAnalysisUtils->IsPileUpEvent(fInputEvent)) return false;       // Apply new vertex cut
  }
  // Apply vertex z cut
  if(!fVertexRange.IsInRange(vtx->GetZ())){
    AliDebugStream(1) <<  GetName() << ": Reject Z(" << vtx->GetZ() << ")" << std::endl;
    return false;
  }
  AliDebugStream(1) << GetName() << ": Event Selected" << std::endl;

  // Store trigger decision
  if(isMinBias) fAcceptTriggers.push_back("MB");
  if(isEMC7){
    fAcceptTriggers.push_back("EMC7");
    if(!isMinBias) fAcceptTriggers.push_back("EMC7excl");
  }
  if(isDMC7){
    fAcceptTriggers.push_back("DMC7");
    if(!isMinBias) fAcceptTriggers.push_back("DMC7excl");
  }
  if(isEG2){
    fAcceptTriggers.push_back("EG2");
    if(!(isMinBias || isEMC7)) fAcceptTriggers.push_back("EG2excl");
  }
  if(isEG1){
    fAcceptTriggers.push_back("EG1");
    if(!(isMinBias || isEMC7 || isEG2)) fAcceptTriggers.push_back("EG1excl");
  }
  if(isDG2){
    fAcceptTriggers.push_back("DG2");
    if(!(isMinBias || isDMC7)) fAcceptTriggers.push_back("DG2excl");
  }
  if(isDG1){
    fAcceptTriggers.push_back("DG1");
    if(!(isMinBias || isDMC7 || isDG2)) fAcceptTriggers.push_back("DG1excl");
  }
  if(isEJ2){
    fAcceptTriggers.push_back("EJ2");
    if(!(isMinBias || isEMC7)) fAcceptTriggers.push_back("EJ2excl");
  }
  if(isEJ1){
    fAcceptTriggers.push_back("EJ1");
    if(!(isMinBias || isEMC7 || isEJ2)) fAcceptTriggers.push_back("EJ1excl");
  }
  if(isDJ2){
    fAcceptTriggers.push_back("DJ2");
    if(!(isMinBias || isDMC7)) fAcceptTriggers.push_back("DJ2excl");
  }
  if(isDJ1){
    fAcceptTriggers.push_back("DJ1");
    if(!(isMinBias || isDMC7 || isDJ2)) fAcceptTriggers.push_back("DJ1excl");
  }

  return true;
}

/**
 * Run event loop
 * @param Not used
 */
bool AliAnalysisTaskEmcalPatchesRef::Run(){
  AliDebugStream(1) << GetName() << ": Start function" << std::endl;

  double centrality = -1;
  AliMultSelection *mult = dynamic_cast<AliMultSelection *>(InputEvent()->FindListObject("MultSelection"));
  if(mult && mult->IsEventSelected()) centrality =  mult->GetEstimator("V0M")->GetPercentile();

  // Fill Event counter and reference vertex distributions for the different trigger classes
  for(const auto &trg : fAcceptTriggers) FillEventHistograms(TString(trg.c_str()), centrality, fVertex[2]);


  if(!fTriggerPatchInfo){
    AliErrorStream() << GetName() << ": Trigger patch container not available" << std::endl;
    return false;
  }

  AliDebugStream(1) << GetName() << ": Number of trigger patches " << fTriggerPatchInfo->GetEntries() << std::endl;

  Double_t vertexpos[3];
  fInputEvent->GetPrimaryVertex()->GetXYZ(vertexpos);

  Double_t energy, eta, phi, et;
  Int_t col, row;
  for(auto patchIter : *fTriggerPatchInfo){
    if(!IsOfflineSimplePatch(patchIter)) continue;
    AliEMCALTriggerPatchInfo *patch = static_cast<AliEMCALTriggerPatchInfo *>(patchIter);

    bool isDCAL         = SelectDCALPatch(patchIter),
        isSingleShower  = SelectSingleShowerPatch(patchIter),
        isJetPatch      = SelectJetPatch(patchIter);

    std::vector<TString> patchnames;
    if(isJetPatch){
      if(isDCAL){
        patchnames.push_back("DJ1");
        patchnames.push_back("DJ2");
      } else {
        patchnames.push_back("EJ1");
        patchnames.push_back("EJ2");
      }
    }
    if(isSingleShower){
      if(isDCAL){
        patchnames.push_back("DMC7");
        patchnames.push_back("DG1");
        patchnames.push_back("DG2");
      } else {
        patchnames.push_back("EMC7");
        patchnames.push_back("EG1");
        patchnames.push_back("EG2");
      }
    }
    if(!patchnames.size()){
      // Undefined patch type - ignore
      continue;
    }

    TLorentzVector posvec;
    energy = patch->GetPatchE();
    eta = patch->GetEtaGeo();
    phi = patch->GetPhiGeo();
    col = patch->GetColStart();
    row = patch->GetRowStart();
    et = patch->GetLorentzVectorCenterGeo().Et();

    // fill histograms allEta
    for(const auto &nameit : patchnames){
      for(const auto &trg : fAcceptTriggers){
        FillPatchHistograms("MB", nameit, energy, et, eta, phi, col, row);
      }
    }
  }
}

void AliAnalysisTaskEmcalPatchesRef::ExecOnce(){
  AliAnalysisTaskEmcal::ExecOnce();
  if(!fLocalInitialized) return;

  // Handle OADB container with downscaling factors
  if(fNameDownscaleOADB.Length()){
    if(fNameDownscaleOADB.Contains("alien://") && ! gGrid) TGrid::Connect("alien://");
    fDownscaleOADB = new AliOADBContainer("AliEmcalDownscaleFactors");
    fDownscaleOADB->InitFromFile(fNameDownscaleOADB.Data(), "AliEmcalDownscaleFactors");
  }
}

void AliAnalysisTaskEmcalPatchesRef::RunChanged(Int_t runnumber){
 if(fDownscaleOADB){
    fDownscaleFactors = static_cast<TObjArray *>(fDownscaleOADB->GetObject(runnumber));
  }
}

/**
 * Get a trigger class dependent event weight. The weight
 * is defined as 1/downscalefactor. The downscale factor
 * is taken from the OADB. For triggers which are not downscaled
 * the weight is always 1.
 * @param[in] triggerclass Class for which to obtain the trigger.
 * @return Downscale facror for the trigger class (1 if trigger is not downscaled or no OADB container is available)
 */
Double_t AliAnalysisTaskEmcalPatchesRef::GetTriggerWeight(const TString &triggerclass) const {
  if(fDownscaleFactors){
    TParameter<double> *result(nullptr);
    // Downscaling only done on MB, L0 and the low threshold triggers
    if(triggerclass.Contains("MB")) result = static_cast<TParameter<double> *>(fDownscaleFactors->FindObject("INT7"));
    else if(triggerclass.Contains("EMC7")) result = static_cast<TParameter<double> *>(fDownscaleFactors->FindObject("EMC7"));
    else if(triggerclass.Contains("EJ2")) result = static_cast<TParameter<double> *>(fDownscaleFactors->FindObject("EJ2"));
    else if(triggerclass.Contains("EG2")) result = static_cast<TParameter<double> *>(fDownscaleFactors->FindObject("EG2"));
    if(result) return 1./result->GetVal();
  }
  return 1.;
}


/**
 * Filling patch related histogram. In case a downscaling correction is
 * available it is applied to the histograms as weight
 * @param[in] triggerclass Name of the trigger class firing the event
 * @param[in] patchname Name of the patchtype
 * @param[in] energy Calibrated energy of the patch
 * @param[in] eta Patch eta at the geometrical center
 * @param[in] phi Patch phi at the geometrical center
 */
void AliAnalysisTaskEmcalPatchesRef::FillPatchHistograms(TString triggerclass, TString patchname, double energy, double transverseenergy, double eta, double phi, int col, int row){
  Double_t weight = GetTriggerWeight(triggerclass);
  AliDebugStream(1) << GetName() << ": Using weight " << weight << " for trigger " << triggerclass << " in patch histograms." << std::endl;
  fHistos->FillTH1(Form("h%sPatchEnergy%s", patchname.Data(), triggerclass.Data()), energy, weight);
  fHistos->FillTH1(Form("h%sPatchET%s", patchname.Data(), triggerclass.Data()), transverseenergy, weight);
  fHistos->FillTH2(Form("h%sPatchEnergyEta%s", patchname.Data(), triggerclass.Data()), energy, eta, weight);
  fHistos->FillTH2(Form("h%sPatchETEta%s", patchname.Data(), triggerclass.Data()), transverseenergy, eta, weight);
  Double_t encuts[5] = {1., 2., 5., 10., 20.};
  for(int ien = 0; ien < 5; ien++){
    if(energy > encuts[ien]){
      fHistos->FillTH2(Form("h%sEtaPhi%dG%s", patchname.Data(), static_cast<int>(encuts[ien]), triggerclass.Data()), eta, phi, weight);
      fHistos->FillTH2(Form("h%sColRow%dG%s", patchname.Data(), static_cast<int>(encuts[ien]), triggerclass.Data()), col, row, weight);
    }
  }
}

/**
 * Fill event-based histograms. Monitored are
 * - Number of events
 * - Centrality percentile (if available)
 * - z-position of the primary vertex
 * In case a downscaling correction is avaiable it is applied to all
 * histograms as a weight.
 * @param[in] triggerclass Name of the trigger class
 * @param[in] centrality Centrality percentile of the event
 * @param[in] vertexz z-position of the
 */
void AliAnalysisTaskEmcalPatchesRef::FillEventHistograms(const TString &triggerclass, double centrality, double vertexz){
  Double_t weight = GetTriggerWeight(triggerclass);
  AliDebugStream(1) << GetName() << ": Using weight " << weight << " for trigger " << triggerclass << " in event histograms." << std::endl;
  fHistos->FillTH1(Form("hEventCount%s", triggerclass.Data()), 1, weight);
  fHistos->FillTH1(Form("hEventCentrality%s", triggerclass.Data()), centrality, weight);
  fHistos->FillTH1(Form("hVertexZ%s", triggerclass.Data()), vertexz, weight);
}

/**
 * Apply trigger selection using offline patches and trigger thresholds based on offline ADC Amplitude
 * @param triggerpatches Trigger patches found by the trigger maker
 * @return String with EMCAL trigger decision
 */
TString AliAnalysisTaskEmcalPatchesRef::GetFiredTriggerClassesFromPatches(const TClonesArray* triggerpatches) const {
  TString triggerstring = "";
  if(!triggerpatches) return triggerstring;
  Int_t nEJ1 = 0, nEJ2 = 0, nEG1 = 0, nEG2 = 0, nDJ1 = 0, nDJ2 = 0, nDG1 = 0, nDG2 = 0;
  double  minADC_J1 = 260.,
          minADC_J2 = 127.,
          minADC_G1 = 140.,
          minADC_G2 = 89.;
  for(TIter patchIter = TIter(triggerpatches).Begin(); patchIter != TIter::End(); ++patchIter){
    AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(*patchIter);
    if(!patch->IsOfflineSimple()) continue;
    if(patch->IsJetHighSimple() && patch->GetADCOfflineAmp() > minADC_J1){
      if(patch->IsDCalPHOS()) nDJ1++;
      else nEJ1++;
    }
    if(patch->IsJetLowSimple() && patch->GetADCOfflineAmp() > minADC_J2){
      if(patch->IsDCalPHOS()) nDJ2++;
      else nEJ2++;
    }
    if(patch->IsGammaHighSimple() && patch->GetADCOfflineAmp() > minADC_G1){
      if(patch->IsDCalPHOS()) nDG1++;
      else nEG1++;
    }
    if(patch->IsGammaLowSimple() && patch->GetADCOfflineAmp() > minADC_G2){
      if(patch->IsDCalPHOS()) nDG2++;
      else nEG2++;
    }
  }
  if(nEJ1) triggerstring += "EJ1";
  if(nEJ2){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "EJ2";
  }
  if(nEG1){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "EG1";
  }
  if(nEG2){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "EG2";
  }
  if(nDJ1){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "DJ1";
  }
  if(nEJ2){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "DJ2";
  }
  if(nDG1){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "DG1";
  }
  if(nDG2){
    if(triggerstring.Length()) triggerstring += ",";
    triggerstring += "DG2";
  }
  return triggerstring;
}

void AliAnalysisTaskEmcalPatchesRef::GetPatchBoundaries(TObject *o, Double_t *boundaries) const {
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  boundaries[0] = patch->GetEtaMin();
  boundaries[1] = patch->GetEtaMax();
  boundaries[2] = patch->GetPhiMin();
  boundaries[3] = patch->GetPhiMax();
}

bool AliAnalysisTaskEmcalPatchesRef::IsOfflineSimplePatch(TObject *o) const {
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  return patch->IsOfflineSimple();
}

bool AliAnalysisTaskEmcalPatchesRef::SelectDCALPatch(TObject *o) const {
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  return patch->GetRowStart() >= 64;
}

bool AliAnalysisTaskEmcalPatchesRef::SelectSingleShowerPatch(TObject *o) const{
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  if(!patch->IsOfflineSimple()) return false;
  return patch->IsGammaLowSimple();
}

bool AliAnalysisTaskEmcalPatchesRef::SelectJetPatch(TObject *o) const{
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  if(!patch->IsOfflineSimple()) return false;
  return patch->IsJetLowSimple();
}

double AliAnalysisTaskEmcalPatchesRef::GetPatchEnergy(TObject *o) const {
  double energy = 0.;
  AliEMCALTriggerPatchInfo *patch = dynamic_cast<AliEMCALTriggerPatchInfo *>(o);
  energy = patch->GetPatchE();
  return energy;
}

AliAnalysisTaskEmcalPatchesRef::EnergyBinning::EnergyBinning():
    TCustomBinning()
{
  this->SetMinimum(0.);
  this->AddStep(1., 0.05);
  this->AddStep(2., 0.1);
  this->AddStep(4, 0.2);
  this->AddStep(7, 0.5);
  this->AddStep(16, 1);
  this->AddStep(32, 2);
  this->AddStep(40, 4);
  this->AddStep(50, 5);
  this->AddStep(100, 10);
  this->AddStep(200, 20);
}


} /* namespace EMCalTriggerPtAnalysis */