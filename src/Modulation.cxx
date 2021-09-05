#include "Modulation.h"

ClassImp(Modulation)

// constructor
Modulation::Modulation(Int_t tw_, Int_t l_, Int_t m_,
 Int_t level_, Bool_t enablePW_, Int_t polarization_) {

  // twist, l, m, and level (where level is used if there are additional
  // modulations for a specific set of values {tw,l,m})
  tw = tw_;
  l = l_;
  m = m_;
  lev = level_;

  // if true, enables theta dependence (partial waves) in the form of
  // associated legendre polynomials; by default we leave it turned off
  // so that other programs can choose to turn it on
  enablePW = enablePW_;

  // set polarization of structure function
  polarization = polarization_;

  // build formula
  this->Initialize();
};


// alternative constructor, for BruAsymmetry formatted amplitude names
Modulation::Modulation(TString ampStr) {

  // parse amplitude name
  enablePW = ampStr.Contains("pwAmp");
  ampStr = ampStr.ReplaceAll("pw","");
  char sgn;
  sscanf(ampStr,"AmpT%dL%dM%c%dLv%dP%d",&tw,&l,&sgn,&m,&lev,&polarization);
  if(sgn=='m') m*=-1;

  // build formula
  this->Initialize();
};


// build formula (called by the constructor)
void Modulation::Initialize() {

  // validation; will likely crash if any of these errors are thrown
  if( !( tw==0 || tw==2 || tw==3 )) {
    fprintf(stderr,"ERROR: Modulation::Modulation -- bad twist (%d)\n",tw);
  };
  if(l<0 || l>LMAX) {
    fprintf(stderr,"ERROR: Modulation::Modulation -- bad L (%d)\n",l);
  };
  if(TMath::Abs(m) > l) {
    fprintf(stderr,"ERROR: Modulation::Modulation -- bad M (L=%d,M=%d)\n",l,m);
  };
  if(polarization<0 || polarization>=nPOL) {
    fprintf(stderr,"ERROR: Modulation::Modulation -- bad polarization setting\n");
  };


  // build a string for the modulation function which is used as a "base";
  // regexps are used to modify this string into a title, a formula, TF3, etc.
  // -- baseStr azimuthal dependence
  mAbs = TMath::Abs(m);
  if(polarization==kLU) {
    switch(tw) {
      case 0:
        aziStr = "1"; // constant modulation
        break;
      case 2:
        if(m==0) aziStr = "0";
        else aziStr = Form("sin(%d*phiH-%d*phiR)",mAbs,mAbs);
        if(m<0) aziStr = "-"+aziStr; // pull minus sign out front
        break;
      case 3:
        aziStr = Form("sin(%d*phiH+%d*phiR)",1-m,m);
        break;
      default: aziStr = "0";
    };
  }
  else if(polarization==kLL) {
    switch(tw) {
      case 2:
        if(m==0) aziStr = "1";
        else aziStr = Form("cos(%d*phiH-%d*phiR)",mAbs,mAbs);
        break;
      case 3:
        aziStr = Form("cos(%d*phiH+%d*phiR)",1-m,m);
        break;
      default: aziStr = "0";
    };
  }
  else if(polarization==kUU) {
    switch(tw) {
      case 0:
        aziStr = "1"; // constant modulation
        break;
      case 2:
        if(lev==0) { // transverse photon
          if(m==0) aziStr = "1";
          else aziStr = Form("cos(%d*phiH-%d*phiR)",mAbs,mAbs);
        }
        else if(lev==1) { // unpolarized photon
          aziStr = Form("cos(%d*phiH+%d*phiR)",2-m,m);
        }
        else aziStr = "0";
        break;
      case 3:
        aziStr = Form("cos(%d*phiH+%d*phiR)",1-m,m);
        break;
      default: aziStr = "0";
    };
  }
  else if(polarization==kUT) {
    switch(tw) {
      case 2:
        if(lev==0) { // transverse photon // (51)
          aziStr = Form("sin(%d*phiH-%d*phiR-phiS)",1+m,m);
        }
        else if(lev==1) { // unpolarized photon // (52)
          aziStr = Form("sin(%d*phiH+%d*phiR+phiS)",1-m,m);
        }
        else if(lev==2) { // unpolarized photon // (53)
          aziStr = Form("sin(%d*phiH+%d*phiR-phiS)",3-m,m);
        }
        else aziStr = "0";
        break;
      case 3:
        if(lev==0) { // (54), but see (26) for |m|>0 form (m=0 is sinPhiS)
          aziStr = Form("sin(%d*phiH+%d*phiR+phiS)",-m,m);
        }
        else if(lev==1) { // (55)
          aziStr = Form("sin(%d*phiH+%d*phiR-phiS)",2-m,m);
        }
        else aziStr = "0";
        break;
      default: aziStr = "0";
    };
  }
  else if(polarization==kDSIDIS) {
    if(tw==2) {
      if(lev==0)      aziStr = "sin(phiD)";
      else if(lev==1) aziStr = "sin(2*phiD)";
      else aziStr = "0";
    }
    else aziStr = "0";
  }
  else aziStr = "0";


  // -- baseStr theta dependence
  // - this is from the partial wave expansion in terms of cos(theta); this follows
  //   formulas 19-21 from arXiv:1408.5721
  // - the associated legendre polynomials are from spherical harmonics
  // - note that theta dependence of |l,m> is equal to |l,-m>
  if(enablePW) {
    if(l==0) legStr = "1";
    else if(l==1) {
      switch(mAbs) {
        case 0:
          legStr = "cos(theta)";
          break;
        case 1:
          legStr = "sin(theta)";
          break;
      };
    } else if(l==2) {
      switch(mAbs) {
        case 0:
          legStr = "0.5*(3*pow(cos(theta),2)-1)";
          break;
        case 1:
          legStr = "sin(2*theta)";
          break;
        case 2:
          legStr = "pow(sin(theta),2)";
          break;
      };
    };
  } else legStr = "1";

  // -- baseStr concatenate azimuthal and theta dependences
  if(enablePW) baseStr = "("+legStr+")*("+aziStr+")";
  else baseStr = aziStr;

  // -- clean up baseStr, to make it more human-readable
  if(aziStr=="0") baseStr="0";
  Tools::GlobalRegexp(baseStr,TRegexp("1\\*"),""); // omit 1*
  Tools::GlobalRegexp(baseStr,TRegexp("\\+0\\*phi."),""); // omit +0*phi
  Tools::GlobalRegexp(baseStr,TRegexp("-0\\*phi."),""); // omit -0*phi
  Tools::GlobalRegexp(baseStr,TRegexp("(0\\*phi."),"("); // (0*var -> (
  Tools::GlobalRegexp(baseStr,TRegexp("\\+-"),"-"); // +- -> -
  Tools::GlobalRegexp(baseStr,TRegexp("-\\+"),"-"); // -+ -> -
  Tools::GlobalRegexp(baseStr,TRegexp("--"),"+"); // -- -> +
  Tools::GlobalRegexp(baseStr,TRegexp("\\+\\+"),"+"); // ++ -> +
  Tools::GlobalRegexp(baseStr,TRegexp("(\\+"),"("); // (+ -> (

  // ----> done building baseStr


  // initialize function
  tf3name = this->ModulationName();
  tf3name.ReplaceAll("mod","modFunc");
  function = new TF3(tf3name,this->Formu(),-PI,PI,-PI,PI,0,PI);
};


// evaluate the modulation for specified values of phiR, phiH, theta
Double_t Modulation::Evaluate(Float_t phiR, Float_t phiH, Float_t theta) {
  if(polarization==kUT) {
    fprintf(stderr,"ERROR: Modulation::Evaluate not yet functional for UT\n");
    return UNDEF; // TODO
  };
  return function->Eval(phiR,phiH,theta);
};


// build formula string for TF3
TString Modulation::Formu() {
  if(polarization==kUT || polarization==kDSIDIS) {
    return "1"; // need 4D function // TODO
  };
  formuStr = baseStr;
  Tools::GlobalRegexp(formuStr,TRegexp("sin"),"TMath::Sin");
  Tools::GlobalRegexp(formuStr,TRegexp("cos"),"TMath::Cos");
  Tools::GlobalRegexp(formuStr,TRegexp("pow"),"TMath::Power");
  Tools::GlobalRegexp(formuStr,TRegexp("phiR"),"x");
  Tools::GlobalRegexp(formuStr,TRegexp("phiH"),"y");
  Tools::GlobalRegexp(formuStr,TRegexp("theta"),"z");
  return formuStr;
};


// build formula string for RooFit
TString Modulation::FormuRF() {
  formuStr = baseStr;
  Tools::GlobalRegexp(formuStr,TRegexp("sin"),"TMath::Sin");
  Tools::GlobalRegexp(formuStr,TRegexp("cos"),"TMath::Cos");
  Tools::GlobalRegexp(formuStr,TRegexp("pow"),"TMath::Power");
  Tools::GlobalRegexp(formuStr,TRegexp("phiH"),"PhiH");
  Tools::GlobalRegexp(formuStr,TRegexp("phiR"),"PhiR");
  Tools::GlobalRegexp(formuStr,TRegexp("phiS"),"PhiS");
  Tools::GlobalRegexp(formuStr,TRegexp("phiD"),"PhiD");
  Tools::GlobalRegexp(formuStr,TRegexp("theta"),"Theta");
  return formuStr;
};

// build formula string for BruFit
TString Modulation::FormuBru() {
  formuStr = baseStr;
  Tools::GlobalRegexp(formuStr,TRegexp("phiH"),"@PhiH[]");
  Tools::GlobalRegexp(formuStr,TRegexp("phiR"),"@PhiR[]");
  Tools::GlobalRegexp(formuStr,TRegexp("phiS"),"@PhiS[]");
  Tools::GlobalRegexp(formuStr,TRegexp("phiD"),"@PhiD[]");
  Tools::GlobalRegexp(formuStr,TRegexp("theta"),"@Theta[]");
  return formuStr;
};

// amplitude name for parameter name
TString Modulation::AmpName() {
  TString retstr = Form("AmpT%dL%dM%s%dLv%dP%d",
    tw,
    l,
    m<0?"m":"p",
    TMath::Abs(m),
    lev,
    polarization
  );
  if(enablePW) retstr = "pw" + retstr;
  return retstr;
};


// modulation formula for ROOT title
TString Modulation::ModulationTitle() {
  TString retstr = baseStr;
  retstr.ReplaceAll("phi","#phi");
  retstr.ReplaceAll("phiD","Delta#phi");
  retstr.ReplaceAll("theta","#theta");
  return retstr;
};
// modulation formula for ROOT name
TString Modulation::ModulationName() {
  TString retstr = Form("mod_t%d_l%d_m%s%d_lev%d",
    tw,
    l,
    m<0?"N":"",
    TMath::Abs(m),
    lev
  );
  return retstr;
};

TString Modulation::PolarizationTitle() {
  switch(polarization) {
    case kLU:
      return "LU";
      break;
    case kLL:
      return "LL";
      break;
    case kUU: 
      if(tw==2 && lev==0) return "UU,T";
      else return "UU";
      break;
    case kUT:
      if(tw==2) {
        if(lev==0)      return "UT,T";
        else if(lev==1) return "UT";
        else if(lev==2) return "UT";
        else return "unknown";
      } else if(tw==3) {
        if(lev==0)      return "UT";
        else if(lev==1) return "UT";
        else return "unknown";
      };
      break;
    case kDSIDIS:
      return "LU";
      break;
  };
  return "unknown";
};

TString Modulation::StateTitle() {

  TString retstr,lStr;
  TString polStr = this->PolarizationTitle();

  if(polarization==kDSIDIS) return "DSIDIS";
  if(tw==0) return "const";

  lStr = enablePW ? Form("%d",l) : "L";
  
  retstr = Form("|%s,%d>^{tw%d}_{%s}",lStr.Data(),m,tw,polStr.Data());
  return retstr;
};

TString Modulation::AsymmetryTitle() {
  TString retstr = "A_{"+this->PolarizationTitle()+"}["+
    this->ModulationTitle()+"]";
  return retstr;
};


Modulation::~Modulation() {};
