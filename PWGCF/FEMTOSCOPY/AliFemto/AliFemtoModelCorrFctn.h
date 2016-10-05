////////////////////////////////////////////////////////////////////////////////
///                                                                          ///
/// AliFemtoModelCorrFctn - the base class for correlation function which    ///
/// uses the model framework and weight generation                           ///
///                                                                          ///
////////////////////////////////////////////////////////////////////////////////
#ifndef ALIFEMTOMODELCORRFCTN_H
#define ALIFEMTOMODELCORRFCTN_H

#include "AliFemtoCorrFctn.h"
class AliFemtoPair;
class AliFemtoModelManager;
class TH1D;
class TH2D;

class AliFemtoModelCorrFctn: public AliFemtoCorrFctn {

public:
  AliFemtoModelCorrFctn();
  AliFemtoModelCorrFctn(const char *title, Int_t aNbins, Double_t aQinvLo, Double_t aQinvHi);
  AliFemtoModelCorrFctn(const AliFemtoModelCorrFctn& aCorrFctn);
  virtual ~AliFemtoModelCorrFctn();

  AliFemtoModelCorrFctn& operator=(const AliFemtoModelCorrFctn& aCorrFctn);

  virtual void ConnectToManager(AliFemtoModelManager *aManager);

  virtual AliFemtoString Report();

  virtual void AddRealPair(AliFemtoPair* aPair);
  virtual void AddMixedPair(AliFemtoPair* aPair);

  virtual void EventBegin(const AliFemtoEvent* aEvent);
  virtual void EventEnd(const AliFemtoEvent* aEvent);
  virtual void Finish();

  virtual TList* GetOutputList();
  virtual void Write();

  virtual AliFemtoModelCorrFctn* Clone();

  Double_t GetQinvTrue(AliFemtoPair*);

protected:
  AliFemtoModelManager *fManager; // Link back to the manager to get the weights

  TH1D *fNumeratorTrue; // Numerator made with pairs from the same event
  TH1D *fNumeratorFake; // Numerator made with pairs from different events (mixed pairs)
  TH1D *fDenominator;   // Denominator made with mixed pairs

  TH1D *fNumeratorTrueIdeal; // Numerator made with pairs (true qinv) from the same event
  TH1D *fNumeratorFakeIdeal; // Numerator made with pairs (true qinv) from different events (mixed pairs)
  TH1D *fDenominatorIdeal;   // Denominator made with mixed pairs (true qinv)

  TH2D *fQgenQrec; // Qinv true (generated) vs. Qinv reconstructed


private:

#ifdef __ROOT__
  /// \cond CLASSIMP
  ClassDef(AliFemtoModelCorrFctn, 1);
  /// \endcond
#endif
};

#endif