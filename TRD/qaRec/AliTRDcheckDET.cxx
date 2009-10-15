#include <TAxis.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH1I.h>
#include <TF1.h>
#include <TGaxis.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TMath.h>
#include <TMap.h>
#include <TObjArray.h>
#include <TObject.h>
#include <TObjString.h>

#include <TPad.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TROOT.h>

#include "AliLog.h"
#include "AliTRDcluster.h"
#include "AliESDHeader.h"
#include "AliESDRun.h"
#include "AliESDtrack.h"
#include "AliTRDgeometry.h"
#include "AliTRDpadPlane.h"
#include "AliTRDSimParam.h"
#include "AliTRDseedV1.h"
#include "AliTRDtrackV1.h"
#include "AliTRDtrackerV1.h"
#include "AliTRDReconstructor.h"
#include "AliTrackReference.h"
#include "AliTrackPointArray.h"
#include "AliTracker.h"
#include "TTreeStream.h"

#include "info/AliTRDtrackInfo.h"
#include "info/AliTRDeventInfo.h"
#include "AliTRDcheckDET.h"

#include <cstdio>
#include <iostream>

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  Reconstruction QA                                                     //
//                                                                        //
//  Task doing basic checks for tracking and detector performance         //
//                                                                        //
//  Authors:                                                              //
//    Anton Andronic <A.Andronic@gsi.de>                                  //
//    Alexandru Bercuci <A.Bercuci@gsi.de>                                //
//    Markus Fasel <M.Fasel@gsi.de>                                       //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

//_______________________________________________________
AliTRDcheckDET::AliTRDcheckDET():
  AliTRDrecoTask("checkDET", "Basic TRD data checker")
  ,fEventInfo(0x0)
  ,fTriggerNames(0x0)
  ,fReconstructor(0x0)
  ,fGeo(0x0)
{
  //
  // Default constructor
  //
  DefineInput(1,AliTRDeventInfo::Class());
  fReconstructor = new AliTRDReconstructor;
  fReconstructor->SetRecoParam(AliTRDrecoParam::GetLowFluxParam());
  fGeo = new AliTRDgeometry;
  InitFunctorList();
}

//_______________________________________________________
AliTRDcheckDET::~AliTRDcheckDET(){
  //
  // Destructor
  // 
  if(fTriggerNames) delete fTriggerNames;
  delete fReconstructor;
  delete fGeo;
}

//_______________________________________________________
void AliTRDcheckDET::ConnectInputData(Option_t *opt){
  //
  // Connect the Input data with the task
  //
  AliTRDrecoTask::ConnectInputData(opt);
  fEventInfo = dynamic_cast<AliTRDeventInfo *>(GetInputData(1));
}

//_______________________________________________________
void AliTRDcheckDET::CreateOutputObjects(){
  //
  // Create Output Objects
  //
  OpenFile(0,"RECREATE");
  fContainer = Histos();
  if(!fTriggerNames) fTriggerNames = new TMap();
}

//_______________________________________________________
void AliTRDcheckDET::Exec(Option_t *opt){
  //
  // Execution function
  // Filling TRD quality histos
  //
  if(!HasMCdata() && fEventInfo->GetEventHeader()->GetEventType() != 7) return;	// For real data we select only physical events
  AliTRDrecoTask::Exec(opt);  
  Int_t nTracks = 0;		// Count the number of tracks per event
  Int_t triggermask = fEventInfo->GetEventHeader()->GetTriggerMask();
  TString triggername =  fEventInfo->GetRunInfo()->GetFiredTriggerClasses(triggermask);
  if(fDebugLevel > 6)printf("Trigger cluster: %d, Trigger class: %s\n", triggermask, triggername.Data());
  dynamic_cast<TH1F *>(fContainer->UncheckedAt(kNeventsTrigger))->Fill(triggermask);
  for(Int_t iti = 0; iti < fTracks->GetEntriesFast(); iti++){
    if(!fTracks->UncheckedAt(iti)) continue;
    AliTRDtrackInfo *fTrackInfo = dynamic_cast<AliTRDtrackInfo *>(fTracks->UncheckedAt(iti));
    if(!fTrackInfo->GetTrack()) continue;
    nTracks++;
  }
  if(nTracks){
    dynamic_cast<TH1F *>(fContainer->UncheckedAt(kNeventsTriggerTracks))->Fill(triggermask);
    dynamic_cast<TH1F *>(fContainer->UncheckedAt(kNtracksEvent))->Fill(nTracks);
  }
  if(triggermask <= 20 && !fTriggerNames->FindObject(Form("%d", triggermask))){
    fTriggerNames->Add(new TObjString(Form("%d", triggermask)), new TObjString(triggername));
    // also set the label for both histograms
    TH1 *histo = dynamic_cast<TH1F *>(fContainer->UncheckedAt(kNeventsTriggerTracks));
    histo->GetXaxis()->SetBinLabel(histo->FindBin(triggermask), triggername);
    histo = dynamic_cast<TH1F *>(fContainer->UncheckedAt(kNeventsTrigger));
    histo->GetXaxis()->SetBinLabel(histo->FindBin(triggermask), triggername);
  }
  PostData(0, fContainer);
}

//_______________________________________________________
void AliTRDcheckDET::Terminate(Option_t *){
  //
  // Terminate function
  //
}

//_______________________________________________________
Bool_t AliTRDcheckDET::PostProcess(){
  //
  // Do Postprocessing (for the moment set the number of Reference histograms)
  //
  
  TH1 * h = 0x0;
  
  // Calculate of the trigger clusters purity
  h = dynamic_cast<TH1F *>(fContainer->FindObject("hEventsTrigger"));
  TH1F *h1 = dynamic_cast<TH1F *>(fContainer->FindObject("hEventsTriggerTracks"));
  h1->Divide(h);
  Float_t purities[20], val = 0;
  TString triggernames[20];
  Int_t nTriggerClasses = 0;
  for(Int_t ibin = 1; ibin <= h->GetNbinsX(); ibin++){
    if((val = h1->GetBinContent(ibin))){
      purities[nTriggerClasses] = val;
      triggernames[nTriggerClasses] = h1->GetXaxis()->GetBinLabel(ibin);
      nTriggerClasses++;
    }
  }
  h = dynamic_cast<TH1F *>(fContainer->UncheckedAt(kTriggerPurity));
  TAxis *ax = h->GetXaxis();
  for(Int_t itrg = 0; itrg < nTriggerClasses; itrg++){
    h->Fill(itrg, purities[itrg]);
    ax->SetBinLabel(itrg+1, triggernames[itrg].Data());
  }
  ax->SetRangeUser(-0.5, nTriggerClasses+.5);
  h->GetYaxis()->SetRangeUser(0,1);

  fNRefFigures = 18;

  return kTRUE;
}

//_______________________________________________________
Bool_t AliTRDcheckDET::GetRefFigure(Int_t ifig){
  //
  // Setting Reference Figures
  //
  gPad->SetLogy(0);
  gPad->SetLogx(0);
  TH1 *h = 0x0;
  switch(ifig){
  case kNclustersTrack:
    (h=(TH1F*)fContainer->FindObject("hNcls"))->Draw("pl");
    PutTrendValue("NClustersTrack", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNclustersTracklet:
    (h =(TH1F*)fContainer->FindObject("hNclTls"))->Draw("pc");
    PutTrendValue("NClustersTracklet", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNtrackletsTrack:
    h=MakePlotNTracklets();
    PutTrendValue("NTrackletsTrack", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNtrackletsCross:
    h = (TH1F*)fContainer->FindObject("hNtlsCross");
    if(!MakeBarPlot(h, kRed)) break;
    PutTrendValue("NTrackletsCross", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNtrackletsFindable:
    h = (TH1F*)fContainer->FindObject("hNtlsFindable");
    if(!MakeBarPlot(h, kGreen)) break;
    PutTrendValue("NTrackletsFindable", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNtracksEvent:
    (h = (TH1F*)fContainer->FindObject("hNtrks"))->Draw("pl");
    PutTrendValue("NTracksEvent", h->GetMean(), h->GetRMS());
    return kTRUE;
  case kNtracksSector:
    h = (TH1F*)fContainer->FindObject("hNtrksSector");
    if(!MakeBarPlot(h, kGreen)) break;
    PutTrendValue("NTracksSector", h->GetMean(2), h->GetRMS(2));
    return kTRUE;
  case kTrackStatus:
    ((TH1I *)fContainer->FindObject("hTrackStatus"))->Draw("c");
    gPad->SetLogy(1);
    return kTRUE;
  case kTrackletStatus:
    ((TH2S *)fContainer->FindObject("hTrackletStatus"))->Draw("colz");
    gPad->SetLogy(0);
    return kTRUE;
  case kChi2:
    MakePlotChi2();
    return kTRUE;
  case kPH:
    MakePlotPulseHeight();
    gPad->SetLogy(0);
    return kTRUE;
  case kChargeCluster:
    (h = (TH1F*)fContainer->FindObject("hQcl"))->Draw("c");
    gPad->SetLogy(1);
    PutTrendValue("ChargeCluster", h->GetMaximumBin(), h->GetRMS());
    return kTRUE;
  case kChargeTracklet:
    (h=(TH1F*)fContainer->FindObject("hQtrklt"))->Draw("c");
    PutTrendValue("ChargeTracklet", h->GetMaximumBin(), h->GetRMS());
    return kTRUE;
  case kNeventsTrigger:
    ((TH1F*)fContainer->FindObject("hEventsTrigger"))->Draw("");
    return kTRUE;
  case kNeventsTriggerTracks:
    ((TH1F*)fContainer->FindObject("hEventsTriggerTracks"))->Draw("");
    return kTRUE;
  case kTriggerPurity: 
    if(!MakeBarPlot((TH1F*)fContainer->FindObject("hTriggerPurity"), kGreen)) break;
    break;
  default:
    break;
  }
  AliInfo(Form("Reference plot [%d] missing result", ifig));
  return kFALSE;
}

//_______________________________________________________
TObjArray *AliTRDcheckDET::Histos(){
  //
  // Create QA histograms
  //
  if(fContainer) return fContainer;
  
  fContainer = new TObjArray(20);
  //fContainer->SetOwner(kTRUE);

  // Register Histograms
  TH1 * h = NULL;
  TAxis *axis = NULL;
  if(!(h = (TH1F *)gROOT->FindObject("hNcls"))){
    h = new TH1F("hNcls", "N_{clusters} / track", 181, -0.5, 180.5);
    h->GetXaxis()->SetTitle("N_{clusters}");
    h->GetYaxis()->SetTitle("Entries");
  } else h->Reset();
  fContainer->AddAt(h, kNclustersTrack);

  if(!(h = (TH1F *)gROOT->FindObject("hNclTls"))){
    h = new TH1F("hNclTls","N_{clusters} / tracklet", 51, -0.5, 50.5);
    h->GetXaxis()->SetTitle("N_{clusters}");
    h->GetYaxis()->SetTitle("Entries");
  } else h->Reset();
  fContainer->AddAt(h, kNclustersTracklet);

  if(!(h = (TH1F *)gROOT->FindObject("hNtls"))){
    h = new TH1F("hNtls", "N_{tracklets} / track", AliTRDgeometry::kNlayer, 0.5, 6.5);
    h->GetXaxis()->SetTitle("N^{tracklet}");
    h->GetYaxis()->SetTitle("freq. [%]");
  } else h->Reset();
  fContainer->AddAt(h, kNtrackletsTrack);

  if(!(h = (TH1F *)gROOT->FindObject("htlsSTA"))){
    h = new TH1F("hNtlsSTA", "N_{tracklets} / track (Stand Alone)", AliTRDgeometry::kNlayer, 0.5, 6.5);
    h->GetXaxis()->SetTitle("N^{tracklet}");
    h->GetYaxis()->SetTitle("freq. [%]");
  }
  fContainer->AddAt(h, kNtrackletsSTA);

  if(!(h = (TH1F *)gROOT->FindObject("htlsBAR"))){
    h = new TH1F("hNtlsBAR", "N_{tracklets} / track (Barrel)", AliTRDgeometry::kNlayer, 0.5, 6.5);
    h->GetXaxis()->SetTitle("N^{tracklet}");
    h->GetYaxis()->SetTitle("freq. [%]");
  }
  fContainer->AddAt(h, kNtrackletsBAR);

  // 
  if(!(h = (TH1F *)gROOT->FindObject("hNtlsCross"))){
    h = new TH1F("hNtlsCross", "N_{tracklets}^{cross} / track", 7, -0.5, 6.5);
    h->GetXaxis()->SetTitle("n_{row cross}");
    h->GetYaxis()->SetTitle("freq. [%]");
  } else h->Reset();
  fContainer->AddAt(h, kNtrackletsCross);

  if(!(h = (TH1F *)gROOT->FindObject("hNtlsFindable"))){
    h = new TH1F("hNtlsFindable", "Found/Findable Tracklets" , 101, -0.005, 1.005);
    h->GetXaxis()->SetTitle("r [a.u]");
    h->GetYaxis()->SetTitle("Entries");
  } else h->Reset();
  fContainer->AddAt(h, kNtrackletsFindable);

  if(!(h = (TH1F *)gROOT->FindObject("hNtrks"))){
    h = new TH1F("hNtrks", "N_{tracks} / event", 100, 0, 100);
    h->GetXaxis()->SetTitle("N_{tracks}");
    h->GetYaxis()->SetTitle("Entries");
  } else h->Reset();
  fContainer->AddAt(h, kNtracksEvent);

  if(!(h = (TH1F *)gROOT->FindObject("hNtrksSector"))){
    h = new TH1F("hNtrksSector", "N_{tracks} / sector", AliTRDgeometry::kNsector, -0.5, 17.5);
    h->GetXaxis()->SetTitle("sector");
    h->GetYaxis()->SetTitle("freq. [%]");
  } else h->Reset();
  fContainer->AddAt(h, kNtracksSector);

  if(!(h = (TH1I *)gROOT->FindObject("hTrackStatus"))){
    h = new TH1I("hTrackStatus", "Track Status", 7,0,7);
    axis = h->GetXaxis();
    axis->SetBinLabel(axis->GetFirst() + 0, "OK");
    axis->SetBinLabel(axis->GetFirst() + 1, "PROL");
    axis->SetBinLabel(axis->GetFirst() + 2, "PROP");
    axis->SetBinLabel(axis->GetFirst() + 3, "ADJ");
    axis->SetBinLabel(axis->GetFirst() + 4, "SNP");
    axis->SetBinLabel(axis->GetFirst() + 5, "TLIN");
    axis->SetBinLabel(axis->GetFirst() + 6, "UP");
  }
  fContainer->AddAt(h, kTrackStatus);

  if(!(h = (TH2S *)gROOT->FindObject("hTrackletStatus"))){
    h = new TH2S("hTrackletStatus", "Tracklet status", 6,0,6,8,0,8);
    axis = h->GetYaxis();
    axis->SetBinLabel(1, "OK");
    axis->SetBinLabel(2, "Geom");
    axis->SetBinLabel(3, "Boun");
    axis->SetBinLabel(4, "NoCl");
    axis->SetBinLabel(5, "NoAttach");
    axis->SetBinLabel(6, "NoClTr");
    axis->SetBinLabel(7, "NoFit");
    axis->SetBinLabel(8, "Chi2");
    h->GetXaxis()->SetTitle("layer");
  }
  fContainer->AddAt(h, kTrackletStatus);

  // <PH> histos
  TObjArray *arr = new TObjArray(2);
  arr->SetOwner(kTRUE);  arr->SetName("<PH>");
  fContainer->AddAt(arr, kPH);
  if(!(h = (TH1F *)gROOT->FindObject("hPHt"))){
    h = new TProfile("hPHt", "<PH>", 31, -0.5, 30.5);
    h->GetXaxis()->SetTitle("Time / 100ns");
    h->GetYaxis()->SetTitle("<PH> [a.u]");
  } else h->Reset();
  arr->AddAt(h, 0);
  if(!(h = (TH1F *)gROOT->FindObject("hPHx")))
    h = new TProfile("hPHx", "<PH>", 31, -0.08, 4.88);
  else h->Reset();
  arr->AddAt(h, 1);

  // Chi2 histos
  if(!(h = (TH2S*)gROOT->FindObject("hChi2"))){
    h = new TH2S("hChi2", "#chi^{2} per track", AliTRDgeometry::kNlayer, .5, AliTRDgeometry::kNlayer+.5, 100, 0, 50);
    h->SetXTitle("ndf");
    h->SetYTitle("#chi^{2}/ndf");
    h->SetZTitle("entries");
  } else h->Reset();
  fContainer->AddAt(h, kChi2);

  if(!(h = (TH1F *)gROOT->FindObject("hQcl"))){
    h = new TH1F("hQcl", "Q_{cluster}", 200, 0, 1200);
    h->GetXaxis()->SetTitle("Q_{cluster} [a.u.]");
    h->GetYaxis()->SetTitle("Entries");
  }else h->Reset();
  fContainer->AddAt(h, kChargeCluster);

  if(!(h = (TH1F *)gROOT->FindObject("hQtrklt"))){
    h = new TH1F("hQtrklt", "Q_{tracklet}", 6000, 0, 6000);
    h->GetXaxis()->SetTitle("Q_{tracklet} [a.u.]");
    h->GetYaxis()->SetTitle("Entries");
  }else h->Reset();
  fContainer->AddAt(h, kChargeTracklet);


  if(!(h = (TH1F *)gROOT->FindObject("hEventsTrigger")))
    h = new TH1F("hEventsTrigger", "Trigger Class", 100, 0, 100);
  else h->Reset();
  fContainer->AddAt(h, kNeventsTrigger);

  if(!(h = (TH1F *)gROOT->FindObject("hEventsTriggerTracks")))
    h = new TH1F("hEventsTriggerTracks", "Trigger Class (Tracks)", 100, 0, 100);
  else h->Reset();
  fContainer->AddAt(h, kNeventsTriggerTracks);

  if(!(h = (TH1F *)gROOT->FindObject("hTriggerPurity"))){
    h = new TH1F("hTriggerPurity", "Trigger Purity", 10, -0.5, 9.5);
    h->GetXaxis()->SetTitle("Trigger Cluster");
    h->GetYaxis()->SetTitle("freq.");
  } else h->Reset();
  fContainer->AddAt(h, kTriggerPurity);

  return fContainer;
}

/*
* Plotting Functions
*/

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotTrackStatus(const AliTRDtrackV1 *track){
  //
  // Plot the track status
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1I *>(fContainer->At(kTrackStatus)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  h->Fill(fTrack->GetStatusTRD());
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotTrackletStatus(const AliTRDtrackV1 *track){
  //
  // Plot the track status
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH2S *>(fContainer->At(kTrackletStatus)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  UChar_t status = 0;
  for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
    status = fTrack->GetStatusTRD(il);
    h->Fill(il, status);
  }
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotNClustersTracklet(const AliTRDtrackV1 *track){
  //
  // Plot the mean number of clusters per tracklet
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNclustersTracklet)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  AliTRDseedV1 *tracklet = 0x0;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK()) continue;
    h->Fill(tracklet->GetN2());
  }
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotNClustersTrack(const AliTRDtrackV1 *track){
  //
  // Plot the number of clusters in one track
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNclustersTrack)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  
  Int_t nclusters = 0;
  AliTRDseedV1 *tracklet = 0x0;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK()) continue;
    nclusters += tracklet->GetN();
    if(fDebugLevel > 2){
      Int_t crossing = Int_t(tracklet->IsRowCross());
      Int_t detector = tracklet->GetDetector();
      Float_t theta = TMath::ATan(tracklet->GetZref(1));
      Float_t phi = TMath::ATan(tracklet->GetYref(1));
      Float_t momentum = 0.;
      Int_t pdg = 0;
      Int_t kinkIndex = fESD ? fESD->GetKinkIndex() : 0;
      UShort_t TPCncls = fESD ? fESD->GetTPCncls() : 0;
      if(fMC){
        if(fMC->GetTrackRef()) momentum = fMC->GetTrackRef()->P();
        pdg = fMC->GetPDG();
      }
      (*fDebugStream) << "NClustersTrack"
        << "Detector="  << detector
        << "crossing="  << crossing
        << "momentum="	<< momentum
        << "pdg="				<< pdg
        << "theta="			<< theta
        << "phi="				<< phi
        << "kinkIndex="	<< kinkIndex
        << "TPCncls="		<< TPCncls
        << "nclusters=" << nclusters
        << "\n";
    }
  }
  h->Fill(nclusters);
  return h;
}


//_______________________________________________________
TH1 *AliTRDcheckDET::PlotNTrackletsTrack(const AliTRDtrackV1 *track){
  //
  // Plot the number of tracklets
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0, *hMethod = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNtrackletsTrack)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  Int_t status = fESD->GetStatus();
/*  printf("in/out/refit/pid: TRD[%d|%d|%d|%d]\n", status &AliESDtrack::kTRDin ? 1 : 0, status &AliESDtrack::kTRDout ? 1 : 0, status &AliESDtrack::kTRDrefit ? 1 : 0, status &AliESDtrack::kTRDpid ? 1 : 0);*/
  if((status & AliESDtrack::kTRDin) != 0){
    // Full BarrelTrack
    if(!(hMethod = dynamic_cast<TH1F *>(fContainer->At(kNtrackletsBAR))))
      AliWarning("Method: Barrel.  Histogram not processed!");
  } else {
    // Stand alone Track
    if(!(hMethod = dynamic_cast<TH1F *>(fContainer->At(kNtrackletsSTA))))
      AliWarning("Method: StandAlone.  Histogram not processed!");
  }
  Int_t nTracklets = fTrack->GetNumberOfTracklets();
  h->Fill(nTracklets);
  hMethod->Fill(nTracklets);
  if(fDebugLevel > 3){
    if(nTracklets == 1){
      // If we have one Tracklet, check in which layer this happens
      Int_t layer = -1;
      AliTRDseedV1 *tracklet = 0x0;
      for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
        if((tracklet = fTrack->GetTracklet(il)) && tracklet->IsOK()){layer =  il; break;}
      }
      (*fDebugStream) << "NTrackletsTrack"
        << "Layer=" << layer
        << "\n";
    }
  }
  return h;
}


//_______________________________________________________
TH1 *AliTRDcheckDET::PlotNTrackletsRowCross(const AliTRDtrackV1 *track){
  //
  // Plot the number of tracklets
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNtrackletsCross)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }

  Int_t ncross = 0;
  AliTRDseedV1 *tracklet = 0x0;
  for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
    if(!(tracklet = fTrack->GetTracklet(il)) || !tracklet->IsOK()) continue;

    if(tracklet->IsRowCross()) ncross++;
  }
  h->Fill(ncross);
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotFindableTracklets(const AliTRDtrackV1 *track){
  //
  // Plots the ratio of number of tracklets vs.
  // number of findable tracklets
  //
  // Findable tracklets are defined as track prolongation
  // to layer i does not hit the dead area +- epsilon
  //
  // In order to check whether tracklet hist active area in Layer i, 
  // the track is refitted and the fitted position + an uncertainty 
  // range is compared to the chamber border (also with a different
  // uncertainty)
  //
  // For the track fit two cases are distinguished:
  // If the track is a stand alone track (defined by the status bit 
  // encoding, then the track is fitted with the tilted Rieman model
  // Otherwise the track is fitted with the Kalman fitter in two steps:
  // Since the track parameters are give at the outer point, we first 
  // fit in direction inwards. Afterwards we fit again in direction outwards
  // to extrapolate the track to layers which are not reached by the track
  // For the Kalman model, the radial track points have to be shifted by
  // a distance epsilon in the direction that we want to fit
  //
  const Float_t epsilon = 0.01;   // dead area tolerance
  const Float_t epsilon_R = 1;    // shift in radial direction of the anode wire position (Kalman filter only)
  const Float_t delta_y = 0.7;    // Tolerance in the track position in y-direction
  const Float_t delta_z = 7.0;    // Tolerance in the track position in z-direction (Padlength)
  Double_t x_anode[AliTRDgeometry::kNlayer] = {300.2, 312.8, 325.4, 338.0, 350.6, 363.2}; // Take the default X0
 
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNtrackletsFindable)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  Int_t nFound = 0, nFindable = 0;
  Int_t stack = -1;
  Double_t ymin = 0., ymax = 0., zmin = 0., zmax = 0.;
  Double_t y = 0., z = 0.;
  AliTRDseedV1 *tracklet = 0x0;
  AliTRDpadPlane *pp;  
  for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
    if((tracklet = fTrack->GetTracklet(il)) && tracklet->IsOK()){
      tracklet->SetReconstructor(fReconstructor);
      nFound++;
    }
  }
  // 2 Different cases:
  // 1st stand alone: here we cannot propagate, but be can do a Tilted Rieman Fit
  // 2nd barrel track: here we propagate the track to the layers
  AliTrackPoint points[6];
  Float_t xyz[3];
  memset(xyz, 0, sizeof(Float_t) * 3);
  if(((fESD->GetStatus() & AliESDtrack::kTRDout) > 0) && !((fESD->GetStatus() & AliESDtrack::kTRDin) > 0)){
    // stand alone track
    for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
      xyz[0] = x_anode[il];
      points[il].SetXYZ(xyz);
    }
    AliTRDtrackerV1::FitRiemanTilt(const_cast<AliTRDtrackV1 *>(fTrack), 0x0, kTRUE, 6, points);
  } else {
    // barrel track
    //
    // 2 Steps:
    // -> Kalman inwards
    // -> Kalman outwards
    AliTRDtrackV1 copy_track(*fTrack);  // Do Kalman on a (non-constant) copy of the track
    AliTrackPoint points_inward[6], points_outward[6];
    for(Int_t il = AliTRDgeometry::kNlayer; il--;){
      // In order to avoid complications in the Kalman filter if the track points have the same radial
      // position like the tracklets, we have to shift the radial postion of the anode wire by epsilon
      // in the direction we want to go
      // The track points have to be in reverse order for the Kalman Filter inwards
      xyz[0] = x_anode[AliTRDgeometry::kNlayer - il - 1] - epsilon_R;
      points_inward[il].SetXYZ(xyz);
      xyz[0] = x_anode[il] + epsilon_R;
      points_outward[il].SetXYZ(xyz);
    }
    /*for(Int_t ipt = 0; ipt < AliTRDgeometry::kNlayer; ipt++)
      printf("%d. X = %f\n", ipt, points[ipt].GetX());*/
    // Kalman inwards
    AliTRDtrackerV1::FitKalman(&copy_track, 0x0, kFALSE, 6, points_inward);
    memcpy(points, points_inward, sizeof(AliTrackPoint) * 6); // Preliminary store the inward results in the Array points
    // Kalman outwards
    AliTRDtrackerV1::FitKalman(&copy_track, 0x0, kTRUE, 6, points_inward);
    memcpy(points, points_outward, sizeof(AliTrackPoint) * AliTRDgeometry::kNlayer);
  }
  for(Int_t il = 0; il < AliTRDgeometry::kNlayer; il++){
    y = points[il].GetY();
    z = points[il].GetZ();
    if((stack = fGeo->GetStack(z, il)) < 0) continue; // Not findable
    pp = fGeo->GetPadPlane(il, stack);
    ymin = pp->GetCol0() + epsilon;
    ymax = pp->GetColEnd() - epsilon; 
    zmin = pp->GetRowEnd() + epsilon; 
    zmax = pp->GetRow0() - epsilon;
    // ignore y-crossing (material)
    if((z + delta_z > zmin && z - delta_z < zmax) && (y + delta_y > ymin && y - delta_y < ymax)) nFindable++;
      if(fDebugLevel > 3){
        Double_t pos_tracklet[2] = {tracklet ? tracklet->GetYfit(0) : 0, tracklet ? tracklet->GetZfit(0) : 0};
        Int_t hasTracklet = tracklet ? 1 : 0;
        (*fDebugStream)   << "FindableTracklets"
          << "layer="     << il
          << "ytracklet=" << pos_tracklet[0]
          << "ytrack="    << y
          << "ztracklet=" << pos_tracklet[1]
          << "ztrack="    << z
          << "tracklet="  << hasTracklet
          << "\n";
      }
  }
  
  h->Fill(nFindable > 0 ? TMath::Min(nFound/static_cast<Double_t>(nFindable), 1.) : 1);
  if(fDebugLevel > 2) AliInfo(Form("Findable[Found]: %d[%d|%f]", nFindable, nFound, nFound/static_cast<Float_t>(nFindable > 0 ? nFindable : 1)));
  return h;
}


//_______________________________________________________
TH1 *AliTRDcheckDET::PlotChi2(const AliTRDtrackV1 *track){
  //
  // Plot the chi2 of the track
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH2S*>(fContainer->At(kChi2)))) {
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  Int_t n = fTrack->GetNumberOfTracklets();
  if(!n) return 0x0;

  h->Fill(n, fTrack->GetChi2()/n);
  return h;
}


//_______________________________________________________
TH1 *AliTRDcheckDET::PlotPHt(const AliTRDtrackV1 *track){
  //
  // Plot the average pulse height
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TProfile *h = 0x0;
  if(!(h = dynamic_cast<TProfile *>(((TObjArray*)(fContainer->At(kPH)))->At(0)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  AliTRDseedV1 *tracklet = 0x0;
  AliTRDcluster *c = 0x0;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK())continue;
    Int_t crossing = Int_t(tracklet->IsRowCross());
    Int_t detector = tracklet->GetDetector();
    tracklet->ResetClusterIter();
    while((c = tracklet->NextCluster())){
      if(!c->IsInChamber()) continue;
      Int_t localtime        = c->GetLocalTimeBin();
      Double_t absolute_charge = TMath::Abs(c->GetQ());
      h->Fill(localtime, absolute_charge);
      if(fDebugLevel > 3){
        Double_t distance[2];
        GetDistanceToTracklet(distance, tracklet, c);
        Float_t theta = TMath::ATan(tracklet->GetZref(1));
        Float_t phi = TMath::ATan(tracklet->GetYref(1));
        Float_t momentum = 0.;
        Int_t pdg = 0;
        Int_t kinkIndex = fESD ? fESD->GetKinkIndex() : 0;
        UShort_t TPCncls = fESD ? fESD->GetTPCncls() : 0;
        if(fMC){
          if(fMC->GetTrackRef()) momentum = fMC->GetTrackRef()->P();
          pdg = fMC->GetPDG();
        }
        (*fDebugStream) << "PHt"
          << "Detector="	<< detector
          << "crossing="	<< crossing
          << "Timebin="		<< localtime
          << "Charge="		<< absolute_charge
          << "momentum="	<< momentum
          << "pdg="				<< pdg
          << "theta="			<< theta
          << "phi="				<< phi
          << "kinkIndex="	<< kinkIndex
          << "TPCncls="		<< TPCncls
          << "dy="        << distance[0]
          << "dz="        << distance[1]
          << "c.="        << c
          << "\n";
      }
    }
  }
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotPHx(const AliTRDtrackV1 *track){
  //
  // Plots the average pulse height vs the distance from the anode wire
  // (plus const anode wire offset)
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TProfile *h = 0x0;
  if(!(h = dynamic_cast<TProfile *>(((TObjArray*)(fContainer->At(kPH)))->At(1)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  Float_t offset = .5*AliTRDgeometry::CamHght();
  AliTRDseedV1 *tracklet = 0x0;
  AliTRDcluster *c = 0x0;
  Double_t distance = 0;
  Double_t x, y;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !(tracklet->IsOK())) continue;
    tracklet->ResetClusterIter();
    while((c = tracklet->NextCluster())){
      if(!c->IsInChamber()) continue;
      x = c->GetX()-AliTRDcluster::GetXcorr(c->GetLocalTimeBin());
      y = c->GetY()-AliTRDcluster::GetYcorr(AliTRDgeometry::GetLayer(c->GetDetector()), c->GetCenter());

      distance = tracklet->GetX0() - (c->GetX() + 0.3) + offset;
      h->Fill(distance, TMath::Abs(c->GetQ()));
    }
  }  
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotChargeCluster(const AliTRDtrackV1 *track){
  //
  // Plot the cluster charge
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kChargeCluster)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  AliTRDseedV1 *tracklet = 0x0;
  AliTRDcluster *c = 0x0;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK())continue;
    for(Int_t itime = 0; itime < AliTRDtrackerV1::GetNTimeBins(); itime++){
      if(!(c = tracklet->GetClusters(itime))) continue;
      h->Fill(c->GetQ());
    }
  }
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotChargeTracklet(const AliTRDtrackV1 *track){
  //
  // Plot the charge deposit per chamber
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kChargeTracklet)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }
  AliTRDseedV1 *tracklet = 0x0;
  AliTRDcluster *c = 0x0;
  Double_t Qtot = 0;
  Int_t nTracklets =fTrack->GetNumberOfTracklets();
  for(Int_t itl = 0x0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK()) continue;
    Qtot = 0.;
    for(Int_t ic = AliTRDseedV1::kNclusters; ic--;){
      if(!(c = tracklet->GetClusters(ic))) continue;
      Qtot += TMath::Abs(c->GetQ());
    }
    h->Fill(Qtot);
    if(fDebugLevel > 3){
      Int_t crossing = (Int_t)tracklet->IsRowCross();
      Int_t detector = tracklet->GetDetector();
      Float_t theta = TMath::ATan(tracklet->GetZfit(1));
      Float_t phi = TMath::ATan(tracklet->GetYfit(1));
      Float_t momentum = 0.;
      Int_t pdg = 0;
      Int_t kinkIndex = fESD ? fESD->GetKinkIndex() : 0;
      UShort_t TPCncls = fESD ? fESD->GetTPCncls() : 0;
      if(fMC){
	      if(fMC->GetTrackRef()) momentum = fMC->GetTrackRef()->P();
        pdg = fMC->GetPDG();
      }
      (*fDebugStream) << "ChargeTracklet"
        << "Detector="  << detector
        << "crossing="  << crossing
        << "momentum="	<< momentum
        << "nTracklets="<< nTracklets
        << "pdg="				<< pdg
        << "theta="			<< theta
        << "phi="				<< phi
        << "kinkIndex="	<< kinkIndex
        << "TPCncls="		<< TPCncls
        << "QT="        << Qtot
        << "\n";
    }
  }
  return h;
}

//_______________________________________________________
TH1 *AliTRDcheckDET::PlotNTracksSector(const AliTRDtrackV1 *track){
  //
  // Plot the number of tracks per Sector
  //
  if(track) fTrack = track;
  if(!fTrack){
    AliWarning("No Track defined.");
    return 0x0;
  }
  TH1 *h = 0x0;
  if(!(h = dynamic_cast<TH1F *>(fContainer->At(kNtracksSector)))){
    AliWarning("No Histogram defined.");
    return 0x0;
  }

  // TODO we should compare with
  // sector = Int_t(track->GetAlpha() / AliTRDgeometry::GetAlpha());

  AliTRDseedV1 *tracklet = 0x0;
  Int_t sector = -1;
  for(Int_t itl = 0; itl < AliTRDgeometry::kNlayer; itl++){
    if(!(tracklet = fTrack->GetTracklet(itl)) || !tracklet->IsOK()) continue;
    sector = static_cast<Int_t>(tracklet->GetDetector()/AliTRDgeometry::kNdets);
    break;
  }
  h->Fill(sector);
  return h;
}


//________________________________________________________
void AliTRDcheckDET::SetRecoParam(AliTRDrecoParam *r)
{

  fReconstructor->SetRecoParam(r);
}

//________________________________________________________
void AliTRDcheckDET::GetDistanceToTracklet(Double_t *dist, AliTRDseedV1 *tracklet, AliTRDcluster *c)
{
  Float_t x = c->GetX();
  dist[0] = c->GetY() - tracklet->GetYat(x);
  dist[1] = c->GetZ() - tracklet->GetZat(x);
}


//_______________________________________________________
TH1* AliTRDcheckDET::MakePlotChi2()
{
// Plot chi2/track normalized to number of degree of freedom 
// (tracklets) and compare with the theoretical distribution.
// 
// Alex Bercuci <A.Bercuci@gsi.de>

  TH2S *h2 = (TH2S*)fContainer->At(kChi2);
  TF1 f("fChi2", "[0]*pow(x, [1]-1)*exp(-0.5*x)", 0., 50.);
  TLegend *leg = new TLegend(.7,.7,.95,.95);
  leg->SetBorderSize(1); leg->SetHeader("Tracklets per Track");
  TH1D *h1 = 0x0;
  Bool_t kFIRST = kTRUE;
  for(Int_t il=1; il<=h2->GetNbinsX(); il++){
    h1 = h2->ProjectionY(Form("pyChi2%d", il), il, il);
    if(h1->Integral()<50) continue;
    h1->Scale(1./h1->Integral());
    h1->SetMarkerStyle(7);h1->SetMarkerColor(il);
    h1->SetLineColor(il);h1->SetLineStyle(2);
    f.SetParameter(1, .5*il);f.SetLineColor(il);
    h1->Fit(&f, "QW+", kFIRST ? "pc": "pcsame");
    leg->AddEntry(h1, Form("%d", il), "l");
    if(kFIRST){
      h1->GetXaxis()->SetRangeUser(0., 25.);
    }
    kFIRST = kFALSE;
  }
  leg->Draw();
  gPad->SetLogy();
  return h1;
}


//________________________________________________________
TH1* AliTRDcheckDET::MakePlotNTracklets(){
  //
  // Make nice bar plot of the number of tracklets in each method
  //
  TH1F *hBAR = (TH1F *)fContainer->FindObject("hNtlsBAR");
  TH1F *hSTA = (TH1F *)fContainer->FindObject("hNtlsSTA");
  TH1F *hCON = (TH1F *)fContainer->FindObject("hNtls");
  TLegend *leg = new TLegend(0.13, 0.75, 0.39, 0.89);
  leg->SetBorderSize(1);
  leg->SetFillColor(0);

  Float_t scale = hCON->Integral();
  hCON->Scale(100./scale);
  hCON->SetFillColor(kRed);hCON->SetLineColor(kRed);
  hCON->SetBarWidth(0.2);
  hCON->SetBarOffset(0.6);
  hCON->SetStats(kFALSE);
  hCON->GetYaxis()->SetRangeUser(0.,40.);
  hCON->GetYaxis()->SetTitleOffset(1.2);
  hCON->Draw("bar1"); leg->AddEntry(hCON, "Total", "f");
  hCON->SetMaximum(55.);

  hBAR->Scale(100./scale);
  hBAR->SetFillColor(kGreen);hBAR->SetLineColor(kGreen);
  hBAR->SetBarWidth(0.2);
  hBAR->SetBarOffset(0.2);
  hBAR->SetTitle("");
  hBAR->SetStats(kFALSE);
  hBAR->GetYaxis()->SetRangeUser(0.,40.);
  hBAR->GetYaxis()->SetTitleOffset(1.2);
  hBAR->Draw("bar1same"); leg->AddEntry(hBAR, "Barrel", "f");

  hSTA->Scale(100./scale);
  hSTA->SetFillColor(kBlue);hSTA->SetLineColor(kBlue);
  hSTA->SetBarWidth(0.2);
  hSTA->SetBarOffset(0.4);
  hSTA->SetTitle("");
  hSTA->SetStats(kFALSE);
  hSTA->GetYaxis()->SetRangeUser(0.,40.);
  hSTA->GetYaxis()->SetTitleOffset(1.2);
  hSTA->Draw("bar1same"); leg->AddEntry(hSTA, "Stand Alone", "f");
  leg->Draw();
  gPad->Update();
  return hCON;
}

//________________________________________________________
TH1* AliTRDcheckDET::MakePlotPulseHeight(){
  //
  // Create Plot of the Pluse Height Spectrum
  //
  TH1 *h, *h1, *h2;
  TObjArray *arr = (TObjArray*)fContainer->FindObject("<PH>");
  h = (TH1F*)arr->At(0);
  h->SetMarkerStyle(24);
  h->SetMarkerColor(kBlack);
  h->SetLineColor(kBlack);
  h->Draw("e1");
//   copy the second histogram in a new one with the same x-dimension as the phs with respect to time
  h1 = (TH1F *)arr->At(1);
  h2 = new TH1F("hphs1","Average PH", 31, -0.5, 30.5);
  for(Int_t ibin = h1->GetXaxis()->GetFirst(); ibin < h1->GetNbinsX(); ibin++) 
    h2->SetBinContent(ibin, h1->GetBinContent(ibin));
  h2->SetMarkerStyle(22);
  h2->SetMarkerColor(kBlue);
  h2->SetLineColor(kBlue);
  h2->Draw("e1same");
  gPad->Update();
//   create axis according to the histogram dimensions of the original second histogram
  TGaxis *axis = new TGaxis(gPad->GetUxmin(),
                    gPad->GetUymax(),
                    gPad->GetUxmax(),
                    gPad->GetUymax(),
                    -0.08, 4.88, 510,"-L");
  axis->SetLineColor(kBlue);
  axis->SetLabelColor(kBlue);
  axis->SetTextColor(kBlue);
  axis->SetTitle("x_{0}-x_{c} [cm]");
  axis->Draw();
  return h1;
}

//________________________________________________________
Bool_t AliTRDcheckDET::MakeBarPlot(TH1 *histo, Int_t color){
  //
  // Draw nice bar plots
  //
  if(!histo->GetEntries()) return kFALSE;
  histo->Scale(100./histo->Integral());
  histo->SetFillColor(color);
  histo->SetBarOffset(.2);
  histo->SetBarWidth(.6);
  histo->Draw("bar1");
  return kTRUE;
}
