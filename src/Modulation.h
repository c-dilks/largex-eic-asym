#ifndef Modulation_
#define Modulation_

// ROOT
#include "TObject.h"
#include "TString.h"
#include "TMath.h"
#include "TRegexp.h"

// dihbsa
#include "Constants.h"
#include "Tools.h"


class Modulation : public TObject
{
  public:

    enum polarization_enum { kLU, kUU, kLL, kUT, nPOL };

    Modulation(Int_t ID_, 
               Int_t polarization_ = kUT);
    ~Modulation();

    void Initialize();
    TString FormuBru();
    TString AmpName();
    TString ModulationTitle();

    Int_t GetTwist() { return twist; };
    TString GetBaseString() { return baseStr; };

  private:
    TString baseStr,formuStr;
    Int_t twist,ID;
    Int_t polarization;

  ClassDef(Modulation,1);
};

#endif
