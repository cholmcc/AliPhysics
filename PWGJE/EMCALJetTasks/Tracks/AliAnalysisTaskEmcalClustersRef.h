#ifndef ALIANALYSISTASKEMCALCLUSTERSREF_H
#define ALIANALYSISTASKEMCALCLUSTERSREF_H
/* Copyright(c) 1998-2015, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

#include <string>
#include <vector>

#include "AliAnalysisTaskEmcal.h"
#include "AliCutValueRange.h"
#include <TCustomBinning.h>
#include <TString.h>

class AliOADBContainer;

class TArrayD;
class TClonesArray;
class THistManager;
class TList;
class TObjArray;
class TString;

namespace EMCalTriggerPtAnalysis {

class AliEmcalTriggerOfflineSelection;

class AliAnalysisTaskEmcalClustersRef : public AliAnalysisTaskEmcal {
public:
  AliAnalysisTaskEmcalClustersRef();
  AliAnalysisTaskEmcalClustersRef(const char *name);
  virtual ~AliAnalysisTaskEmcalClustersRef();

  void SetOfflineTriggerSelection(AliEmcalTriggerOfflineSelection *sel) { fTriggerSelection = sel; }
  void SetClusterContainer(TString clustercontname) { fNameClusterContainer = clustercontname; }
  void SetCreateTriggerStringFromPatches(Bool_t doUsePatches) { fTriggerStringFromPatches = doUsePatches; }

  void SetRequestAnalysisUtil(Bool_t doRequest) { fRequestAnalysisUtil = doRequest; }
  void SetCentralityRange(double min, double max) { fCentralityRange.SetLimits(min, max); fRequestCentrality = true; }
  void SetVertexRange(double min, double max) { fVertexRange.SetLimits(min, max); }
  void SetDownscaleOADB(TString oadbname) { fNameDownscaleOADB = oadbname; }

protected:

  virtual void UserCreateOutputObjects();
  virtual bool Run();
  virtual bool IsEventSelected();
  virtual void ExecOnce();
  virtual void RunChanged(Int_t runnumber);

  Double_t GetTriggerWeight(const TString &triggerclass) const;
  void GetPatchBoundaries(TObject *o, Double_t *boundaries) const;
  bool IsOfflineSimplePatch(TObject *o) const;
  bool SelectDCALPatch(TObject *o) const;
  bool SelectSingleShowerPatch(TObject *o) const;
  bool SelectJetPatch(TObject *o) const;
  double GetPatchEnergy(TObject *o) const;

  void FillClusterHistograms(const TString &triggerclass, double energy, double transversenergy, double eta, double phi, TList *triggerpatches);
  void FillEventHistograms(const TString &triggerclass, double centrality, double vertexz);
  TString GetFiredTriggerClassesFromPatches(const TClonesArray* triggerpatches) const;
  void FindPatchesForTrigger(TString triggerclass, const TClonesArray * triggerpatches, TList &foundpatches) const;
  Bool_t CorrelateToTrigger(Double_t etaclust, Double_t phiclust, TList *triggerpatches) const;

  THistManager                        *fHistos;                   //!<! Histogram handler
  AliEmcalTriggerOfflineSelection     *fTriggerSelection;         ///< EMCAL offline trigger selection tool
  std::vector<std::string>            fAcceptTriggers;            //!<! Temporary container with list of selected triggers
  TString                             fNameClusterContainer;      ///< Name of the cluster container in the event

  Bool_t                              fRequestAnalysisUtil;       ///< Switch on request for event selection using analysis utils
  Bool_t                              fTriggerStringFromPatches;  ///< Build trigger string from trigger patches
  AliCutValueRange<double>            fCentralityRange;           ///< Selected centrality range
  AliCutValueRange<double>            fVertexRange;               ///< Selected vertex range
  Bool_t                              fRequestCentrality;         ///< Swich on request for centrality range

  TString                             fNameDownscaleOADB;         ///< Name of the downscale OADB container
  AliOADBContainer                    *fDownscaleOADB;            //!<! Container with downscale factors for different triggers
  TObjArray                           *fDownscaleFactors;         //!<! Downscalfactors for given run

private:

  class EnergyBinning : public TCustomBinning {
  public:
    EnergyBinning();
    virtual ~EnergyBinning() {}
  };

  AliAnalysisTaskEmcalClustersRef(const AliAnalysisTaskEmcalClustersRef &);
  AliAnalysisTaskEmcalClustersRef &operator=(const AliAnalysisTaskEmcalClustersRef &);

  /// \cond CLASSIMP
  ClassDef(AliAnalysisTaskEmcalClustersRef, 1);
  /// \endcond
};

} /* namespace EMCalTriggerPtAnalysis */

#endif /* ALIANALYSISTASKEMCALCLUSTERSREF_H */