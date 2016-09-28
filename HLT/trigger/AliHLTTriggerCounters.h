//-*- Mode: C++ -*-
// $Id: $
#ifndef AliHLTTRIGGERCOUNTERS_H
#define AliHLTTRIGGERCOUNTERS_H
/* This file is property of and copyright by the ALICE HLT Project        *
 * ALICE Experiment at CERN, All rights reserved.                         *
 * See cxx source for full Copyright notice                               */

///  @file   AliHLTTriggerCounters.h
///  @author Artur Szostak <artursz@iafrica.com>
///  @date   4 Sep 2010
///  @brief  Declares the global trigger counters class for the HLT.

#include "AliHLTScalars.h"
#include "TNamed.h"
#include "TTimeStamp.h"
#include "TClonesArray.h"
#include "THashTable.h"

/**
 * @class AliHLTTriggerCounters
 * @brief HLT global trigger counters.
 *
 * This counters class contains summary information about the number of times a
 * trigger was fired in the HLT and its current rate estimate. Objects of this
 * class are generated by the AliHLTTriggerCountersMakerComponent to be used for
 * monitoring purposes.
 *
 * \ingroup alihlt_trigger_components
 */
class AliHLTTriggerCounters : public AliHLTScalars
{
public:
	/**
	 * This class stores all the required information for a HLT global trigger counter.
	 */
	class AliCounter : public AliHLTScalars::AliScalar
	{
	public:
		/// Default constructor
		AliCounter() : AliHLTScalars::AliScalar(), fCounter(0) {}
		
		/// Constructor to set some initial values.
		AliCounter(const char* name, const char* description, ULong64_t value, Double_t rate) :
			AliHLTScalars::AliScalar(name, description, rate), fCounter(value)
		{}
		
		/// Default destructor
		virtual ~AliCounter() {}
		
		/// Resets the counter value.
		virtual void Clear(Option_t* /*option*/ = "") { fCounter = 0; Rate(0); }
	
		/// Inherited from TObject. Performs a deep copy.
		virtual void Copy(TObject& object) const;
		
		/// Returns the value of the counter.
		ULong64_t Counter() const { return fCounter; }

		/// Sets a new value for the counter.
		void Counter(ULong64_t value) { fCounter = value; }

		/**
		 * Increments the counter by a value of 'count'.
		 * \param count  The number to increment the counter by. The default is 1.
		 * \note The rate is not updated.
		 */
		void Increment(UInt_t count = 1) { fCounter += count; }

		/// Returns the rate of the counter (Hz).
		Double_t Rate() const { return Value(); }
		
		/**
		 * Sets a new value for the counter's rate.
		 * \param value  The new value to use (Hz). If negative then 0 is used instead.
		 */
		void Rate(Double_t value) { Value(value >= 0 ? value : 0); }
		
		/// Checks if two counter objects are identical.
		bool operator == (const AliCounter& x) const
		{
			return AliHLTScalars::AliScalar::operator == (x)
			       and fCounter == x.fCounter;
		}
		
		/// Checks if two counter objects are not identical.
		bool operator != (const AliCounter& x) const
		{
			return not (this->operator == (x));
		}
		
	private:
		ULong64_t fCounter; // The counter's value.
		
		ClassDef(AliCounter, 1);  // HLT trigger counter value.
	};
	
	/// Default constructor.
	AliHLTTriggerCounters();
	
	/// The copy constructor performs a deep copy.
	AliHLTTriggerCounters(const AliHLTTriggerCounters& obj);
	
	/// Default destructor.
	virtual ~AliHLTTriggerCounters();
	
	/**
	 * Adds a new counter to the end of the counters list.
	 * If the counter already exists then its values are updated instead.
	 * \param name  The name of the counter value.
	 * \param description  A short description of the counter value.
	 * \param rate  The rate of the new counter.
	 * \param value  The value of the new counter.
	 * \returns true if the counter already exists and false otherwise.
	 */
	bool Add(const char* name, const char* description, Double_t rate, ULong64_t value);
	
	using AliHLTScalars::Add;

	/**
	 * Fetches the specified counter object.
	 * \param name  The name of the counter object.
	 * \returns the found counter object, otherwise an empty sentinel object with
	 *    zeros. One can tell it is a sentinel because the name will be empty.
	 */
	const AliCounter& GetCounter(const char* name) const { return static_cast<const AliCounter&>(GetScalar(name)); }

	/**
	 * Fetches the specified counter object for editing.
	 * \param name  The name of the counter object.
	 * \returns the found counter object. If the counter does not already
	 *     exist then a new one is created and returned.
	 */
	AliCounter& GetCounter(const char* name) { return static_cast<AliCounter&>(GetScalar(name)); }
	
	/// Returns the number of counter values.
	UInt_t NumberOfCounters() const { return NumberOfScalars(); }

	// Note: the following GetCounterN methods do not use the same name as
	// GetCounter above because the parameter type would unfortunately be
	// ambiguous to an ISO c++ compiler.
	
	/**
	 * Fetches the n'th counter object.
	 * \param n  The number of the counter object.
	 * \returns the found counter object, otherwise an empty sentinel object with
	 *    zeros. One can tell it is a sentinel because the name will be empty.
	 */
	const AliCounter& GetCounterN(UInt_t n) const { return static_cast<const AliCounter&>(GetScalarN(n)); }

	/**
	 * Fetches the n'th counter object for editing.
	 * \param n  The number of the counter object.
	 * \returns the found counter object. If the counter does not already
	 *     exist then a new one is created and returned.
	 */
	AliCounter& GetCounterN(UInt_t n) { return static_cast<AliCounter&>(GetScalarN(n)); }

	/// Returns the timestamp for the counters.
	const TTimeStamp& TimeStamp() const { return fTimeStamp; }

	/// Updates the timestamp to the current time.
	void UpdateTimeStamp() { fTimeStamp = TTimeStamp(); }
	
	/// Resets all counter values and rates to zero.
	virtual void Reset();
	
	/// Inherited form TObject. Performs a deep copy.
	virtual void Copy(TObject& object) const;
	
	/**
	 * Inherited from TObject, this prints the contents of this summary object.
	 * \param option  Can be "compact", which will just print all the values on one line.
	 */
	virtual void Print(Option_t* option = "") const;
	
	/**
	 * The assignment operator performs a deep copy.
	 */
	AliHLTTriggerCounters& operator = (const AliHLTTriggerCounters& obj);
	
	/// Returns the n'th counter or a zero sentinel if n is out of range.
	const AliCounter& operator [] (UInt_t n) const { return GetCounterN(n); }

	/// Returns the n'th counter for editing. A new counter is created if n is out of range.
	AliCounter& operator [] (UInt_t n) { return GetCounterN(n); }

	/// Returns the named counter or a zero sentinel if no such counter is found.
	const AliCounter& operator [] (const char* name) const { return GetCounter(name); }

	/// Returns the named counter for editing. A new counter is created if n is out of range.
	AliCounter& operator [] (const char* name) { return GetCounter(name); }
	
	/**
	 * Comparison operator to check if two sets of counters have the same values
	 * and time stamp.
	 * \note The description strings are not checked so they could be different
	 *   and the order of the counters does not matter either.
	 */
	bool operator == (const AliHLTTriggerCounters& obj) const;
	
	/**
	 * Comparison operator to check if two sets of counters are different.
	 * \note The description strings are not checked, only the values and rates are.
	 *   In addition, the order of the counters does not matter.
	 */
	bool operator != (const AliHLTTriggerCounters& obj) const
	{
		return not (this->operator == (obj));
	}

protected:
	
	// The following is inherited from AliHLTScalars:
	virtual AliScalar* NewScalar(UInt_t i, const char* name, const char* description, Double_t value);
	virtual const AliScalar& Sentinel() const;
	
private:
	
	TTimeStamp fTimeStamp;   // The time stamp for the counters.
	
	ClassDef(AliHLTTriggerCounters, 1);  // Set of HLT global trigger counters.
};

#endif // AliHLTTRIGGERCOUNTERS_H