//-*- Mode: C++ -*-
// $Id$
#ifndef ALIHLTGLOBALTRIGGERDECISION_H
#define ALIHLTGLOBALTRIGGERDECISION_H
/* This file is property of and copyright by the ALICE HLT Project        *
 * ALICE Experiment at CERN, All rights reserved.                         *
 * See cxx source for full Copyright notice                               */

/// @file   AliHLTGlobalTriggerDecision.h
/// @author Artur Szostak <artursz@iafrica.com>
/// @date   26 Nov 2008
/// @brief  Declaration of the AliHLTGlobalTriggerDecision class storing the global HLT decision.

#include "AliHLTTriggerDecision.h"
#include "TArrayL64.h"
#include "TObjArray.h"

/**
 * \class AliHLTGlobalTriggerDecision
 * The global trigger decision object is generated by the AliHLTGlobalTriggerComponent
 * class during processing of input triggers.
 *
 * Multiple input trigger components deriving from AliHLTTrigger will generate
 * AliHLTTriggerDecision objects and possibly additional summary objects. All these
 * objects are input for the global trigger component AliHLTGlobalTriggerComponent.
 * After processing the input objects based on the trigger menu encoded in AliHLTTriggerMenu,
 * the global trigger will generate and fill an AliHLTGlobalTriggerDecision object
 * based on its decision. The new object will contain all the information a normal
 * AliHLTTriggerDecision object generated by AliHLTTrigger contains. But in addition
 * all the input objects that contributed to the global decision are also stored
 * inside AliHLTGlobalTriggerDecision. The contributing trigger decisions are filled
 * in fContributingTriggers and contributing summary TObjects are filled into fInputObjects.
 * These can be accessed with the following methods:
 *  <i>NumberOfTriggerInputs</i> <i>TriggerInput</i> <i>TriggerInputs</i> for the
 *  trigger inputs;
 *  and <i>NumberOfInputObjects</i> <i>InputObject</i> <i>InputObjects</i> for the
 *  input summary objects.
 *
 * There is also an array of counters stored in the global decision. These are
 * a copy of the internal counters of the global trigger component. There is one
 * counter for every item in the trigger menu, plus a possible additional counter
 * at the end which indicated the total number of events processed by the global
 * trigger component.
 *
 * \note The counters do not necessarily correspond to the actual number of triggers
 * that are recorded in the HLT output data stream. For most simple trigger menu
 * configurations the counters will indeed correspond the the actual number of triggers
 * recorded. But for more complex menus that use non zero prescalar values this may not
 * be the case. The reason is that the counters array returned is the internal counter
 * values (state) of the global trigger component. The counters are used to make the
 * prescalars work. Thus, every time a corresponding trigger condition matches
 * (evaluates to true) the counter is incremented, but the trigger decision might
 * anyway skip the corresponding trigger in the menu since the prescalar is downscaling
 * the trigger rate for that particular trigger menu item. This means that the counter
 * values will be an upper bound.
 * The real count and rate of particular triggers should always be taken by actually
 * counting the trigger decision result.
 */
class AliHLTGlobalTriggerDecision : public AliHLTTriggerDecision
{
 public:
  
  /**
   * Default constructor.
   */
  AliHLTGlobalTriggerDecision();
  
  /**
   * Constructor specifying multiple information fields.
   * \param result  The result of the global trigger decision.
   * \param triggerDomain  The trigger domain for the global trigger decision.
   * \param description  The description of (reason for) the global trigger decision.
   */
  AliHLTGlobalTriggerDecision(
      bool result, const AliHLTTriggerDomain& triggerDomain,
      const char* description = ""
    );
  
  /**
   * Default destructor.
   */
  virtual ~AliHLTGlobalTriggerDecision();

  /**
   * Copy constructor performs a deep copy.
   */
  AliHLTGlobalTriggerDecision(const AliHLTGlobalTriggerDecision& src);

  /**
   * Assignment operator performs a deep copy.
   */
  AliHLTGlobalTriggerDecision& operator=(const AliHLTGlobalTriggerDecision& src);

  /**
   * Inherited from TObject, this prints the contents of the trigger decision.
   * \param option  Can be "short" which will print the short format or "counters"
   *    which will print only the counters or "compact" which will print only the
   *    global information but not the lists of input objects.
   */
  virtual void Print(Option_t* option = "") const;

  /**
   * Inherited from TObject. Copy this to the specified object.
   */
  virtual void Copy(TObject &object) const;
  
  /**
   * Inherited from TObject. Create a new clone.
   */
  virtual TObject *Clone(const char *newname="") const;

  /**
   * Returns the number of trigger inputs that contributed to this global trigger decision.
   */
  Int_t NumberOfTriggerInputs() const { return fContributingTriggers.GetEntriesFast(); }
  
  /**
   * Returns the i'th trigger input object in fContributingTriggers.
   */
  const AliHLTTriggerDecision* TriggerInput(Int_t i) const
  {
    return static_cast<const AliHLTTriggerDecision*>( fContributingTriggers[i] );
  }
  
  /**
   * Returns the list of trigger inputs used when making the global HLT trigger decision.
   */
  const TClonesArray& TriggerInputs() const { return fContributingTriggers; }
  
  /**
   * Adds a trigger input to the list of triggers that were considered when making
   * this global trigger decision.
   * \param decision  The trigger decision object to add.
   */
  void AddTriggerInput(const AliHLTTriggerDecision& decision)
  {
    new (fContributingTriggers[fContributingTriggers.GetEntriesFast()]) AliHLTTriggerDecision(decision);
  }
  
  /**
   * Returns the number of other input objects that contributed to this global trigger decision.
   */
  Int_t NumberOfInputObjects() const { return fInputObjects.GetEntriesFast(); }
  
  /**
   * Returns the i'th input object in fInputObjects.
   */
  const TObject* InputObject(Int_t i) const { return fInputObjects[i]; }
  
  /**
   * Returns the list of other input objects used when making the global HLT trigger decision.
   */
  const TObjArray& InputObjects() const { return fInputObjects; }
  
  /**
   * Adds an input object to the list of input objects that were considered when
   * making this global trigger decision.
   * \param object  The input object to add.
   * \note  A copy of the object is made with TObject::Clone() and added.
   */
  void AddInputObject(const TObject* object);
  
  /**
   * Adds an input object to the list of input objects that were considered when
   * making this global trigger decision.
   * \param object  The input object to add.
   * \param own  If true then the global trigger decision takes ownership of the
   *   object and will delete it when destroyed. The caller must not delete the
   *   object after this method call. The default is false (ownership is not taken).
   * \note Unlike AddInputObject, the object pointer is added directly without creating
   *   a deep copy of the object. This means that the added object can only be deleted
   *   after this global trigger object is no longer using the object, unless <i>own</i>
   *   is true. If <i>own</i> is true then the object must not be deleted by the caller.
   * \note The kCanDelete bit of the object is modified by this method call and is
   *   used to track who the object belongs to. This bit should not be modified for
   *   the object after a call to this method, until the decision object is cleared
   *   or destroyed.
   */
  void AddInputObjectRef(TObject* object, bool own = false);
  
  /**
   * Sets the counter array.
   * If the number of events is specified, an additional counter is added at the end
   * and filled with <i>eventCount</i> which indicates the number of events that have been counted.
   * \param  counters  The array of new counter values that the internal counters should be set to.
   * \param  eventCount  This should be the total number of events processed. If it is
   *     a positive number >= 0 then the extra counter is added to the array and filled
   *     with the value of <i>eventCount</i>.
   */
  void SetCounters(const TArrayL64& counters, Long64_t eventCount = -1);
  
  /**
   * Returns the event trigger counters associated with the global trigger classes.
   * There is one counter for every trigger menu item that the global trigger component
   * was configured with. Thus the first counter will correspond to the first menu item
   * added to the trigger menu, the second counter for the second item added and so on.
   * If the total number of events processed counter is pressent it will be at the
   * end of the array in position N-1 where N is the number of items in the counter
   * array (also this will correspond to N-1 trigger menu items in the global trigger menu).
   *
   * \note The counters do not necessarily correspond to the actual number of trigger
   * that are recorded in the HLT output data stream. For most simple trigger menu
   * configurations the counters will indeed correspond the the actual number of triggers
   * recorded. But for more complex menus which use prescalar values this may not be
   * the case. The reason is that the counters array returned is the internal counter
   * values (state) of the global trigger component. The counters are used to make the
   * prescalars work. Thus every time a corresponding trigger condition matches
   * (evaluates to true) the counter is incremented, but the trigger decision might
   * anyhow skip the corresponding trigger in the menu since the prescalar is downscaling
   * the trigger rate for that particular trigger menu item.
   */
  const TArrayL64& Counters() const { return fCounters; }
  
  /**
   * This method removes clears the trigger domain, sets the decision result to false
   * and clears the input object arrays and counters.
   * \param  option  This is passed onto the internal array clear methods.
   * The method is inherited from TObject.
   */
  virtual void Clear(Option_t* option = "C");
  
  /**
   * Finds a named input object from the list of contributing triggers and other input objects.
   * i.e. Both the arrays returned by TriggerInputs() and InputObjects() will be searched,
   * but the contributing triggers will be searched first.
   * \param  name  The name of the object to match as returned by the objects GetName() method.
   * \returns a pointer to the object found or NULL if none was found.
   * The method is inherited from TObject.
   */
  virtual /*const*/ TObject* FindObject(const char* name) const;
  
  /**
   * Finds a matching object from the list of contributing triggers and other input objects
   * by comparing the given object to objects in the lists with the IsEqual() method.
   * i.e. Both the arrays returned by TriggerInputs() and InputObjects() will be searched,
   * but the contributing triggers will be searched first.
   * \param  obj  The object to match to with the IsEqual() method.
   * \returns a pointer to the object found or NULL if none was found.
   * The method is inherited from TObject.
   */
  virtual /*const*/ TObject* FindObject(const TObject* obj) const;
  
  /**
   * This method is called in the streamer to mark the entries in
   * fInputObjects as owned and deletable.  MUST be public for
   * streamer evolution to work.
   */
  void MarkInputObjectsAsOwned();
 private:
  
  /**
   * Deletes all the input objects in fInputObjects that are marked with kCanDelete
   * and empties the whole array.
   */
  void DeleteInputObjects();
  
  
  TClonesArray fContributingTriggers;  /// The list of contributing trigger decisions from all AliHLTTrigger components that were considered.
  TObjArray fInputObjects;  /// The list of other input objects.
  TArrayL64 fCounters;  /// Event trigger counters. One counter for each trigger class in the global trigger.
  
  ClassDef(AliHLTGlobalTriggerDecision, 1) // Contains the HLT global trigger decision and information contributing to the decision.
};

#endif // ALIHLTGLOBALTRIGGERDECISION_H
