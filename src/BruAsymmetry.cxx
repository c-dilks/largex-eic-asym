#include "BruAsymmetry.h"

ClassImp(BruAsymmetry)

BruAsymmetry::BruAsymmetry(TString outdir_, TString minimizer_)
  : outdir(outdir_)
  , minimizer(minimizer_)
{

  printf("construct BruAsymmetry\n");
  FM = new HS::FIT::FitManager();
  outdir = outdir_;
  FM->SetUp().SetOutDir(outdir); // calls mkdir automatically
  outlog = outdir+"/out."+minimizer+".log";
  gSystem->RedirectOutput(outlog,"w");
  gSystem->RedirectOutput(0);


  // variables
  FM->SetUp().LoadVariable(TString("PhiH")+Form("[%f,%f]",-PIe,PIe));
  FM->SetUp().LoadVariable(TString("PhiS")+Form("[%f,%f]",-PIe,PIe));
  FM->SetUp().LoadVariable(TString("Pol")+Form("[%f,%f]",-1.0,1.0));
  //FM->SetUp().LoadVariable(TString("Depol2")+Form("[%f,%f]",0.0,2.5)); // TODO: not yet in SimpleTree
  //FM->SetUp().LoadVariable(TString("Depol3")+Form("[%f,%f]",0.0,2.5));  // TODO: not yet in SimpleTree

  // category for spin
  FM->SetUp().LoadCategory(
    TString("Spin_idx") + Form("[SpinP=%d,SpinM=%d]",1,-1) );
  
  // unique ID variable
  FM->SetUp().SetIDBranchName("Idx");

  // default MCMC settings
  MCMC_iter = 1000;
  MCMC_burnin = 200;
  MCMC_norm = 200;
  MCMC_cov_iter = 1000;
  MCMC_cov_burnin = 200;
  MCMC_cov_norm = 200;

  // misc vars
  numerList = "";
  numerFormu = "";
  denomFormu = "";
  ampNameList = "";
  formuNameList = "";
  nDenomParams = 0;

  printf("constructed BruAsymmetry\n");

};


// add numerator modulation to PDF
void BruAsymmetry::AddNumerMod(Modulation * modu) {

  // set amplitude and modulation names
  TString ampName = modu->AmpName();
  TString formuName = ampName;
  formuName.ReplaceAll("Amp","Mod");

  // amplitude parameter
  FM->SetUp().LoadParameter(ampName+"[0.0,-1,1]");

  // determine which depolarization factor to use
  // - assumes LU, or DSIDIS twist 2
  TString depolVar;
  switch(modu->GetTwist()) {
    // TODO: not yet in SimpleTree
    //case 2: depolVar = "@Depol2[]"; break;
    //case 3: depolVar = "@Depol3[]"; break;
    default: 
      fprintf(stderr,"WARNING: unknown depolarization factor; setting to 1\n");
      depolVar = "1";
  };

  // modulation, including polarization, depolarization, and spin sign
  formu = "@Pol[]*"+depolVar+"*@Spin_idx[]*"+modu->FormuBru();
  this->PrintLog(formuName+" = "+formu);
  FM->SetUp().LoadFormula(formuName+"="+formu);

  // append to list for RooComponentsPDF
  if(numerList!="") numerList += ":";
  numerList += ampName+";"+formuName;

  // append to numerator formula string for EXPR
  if(numerFormu!="") numerFormu += "+";
  numerFormu += ampName+"*"+formuName;
  if(ampNameList!="") ampNameList += ",";
  ampNameList += ampName;
  if(formuNameList!="") formuNameList += ",";
  formuNameList += formuName;
};


// add denominator modulation to PDF
void BruAsymmetry::AddDenomMod(Modulation * modu) {

  // set amplitude and modulation names
  TString ampName = modu->AmpName();
  TString formuName = ampName;
  formuName.ReplaceAll("Amp","Mod");

  // amplitude parameter
  FM->SetUp().LoadParameter(ampName+"[0.0,-1,1]");

  // modulation
  // TODO: move UU depolarization factors to here, if denom
  // amps are possible to constrain
  formu = modu->FormuBru();
  this->PrintLog(formuName+" = "+formu);
  FM->SetUp().LoadFormula(formuName+"="+formu);

  // append to denominator formula string for EXPR
  if(denomFormu!="") denomFormu += "+";
  denomFormu += ampName+"*"+formuName;
  if(ampNameList!="") ampNameList += ",";
  ampNameList += ampName;
  if(formuNameList!="") formuNameList += ",";
  formuNameList += formuName;

  nDenomParams++;
};


// build full PDF
void BruAsymmetry::BuildPDF() {

  // build PDFstr
  fprintf(stderr,"WARNING: depolarization factors not included in obsList (todo when depolarization is included)\n");
  //TString obsList = "PhiH,PhiS,Pol,Depol2,Depol3,Spin_idx"; // TODO depolarization not yet in SimpleTree
  TString obsList = "PhiH,PhiS,Pol,Spin_idx";
  if(nDenomParams==0) {
    // if PDF has numerator amplitudes only, we can use RooComponentsPDF
    PDFstr = "RooComponentsPDF::PWfit(1,"; // PDF class::name ("+1" term ,
    PDFstr += "{"+obsList+"},"; // observables list
    PDFstr += "=" + numerList + ")"; // sum_i { pol *depol*spin * amp_i * mod_i }
    // alternatively, use EXPR
    //PDFstr = "EXPR::PWfit('1+"+numerFormu+"'";
    //PDFstr += ","+obsList+","+ampNameList+","+formuNameList+")";
  } else {
    // if PDF has denominator amplitudes, must use EXPR
    PDFstr = "EXPR::PWfit('1+("+numerFormu+")/(1+"+denomFormu+")'";
    PDFstr += ","+obsList+","+ampNameList+","+formuNameList+")";
  };


  // construct the extended likelihood
  this->PrintLog("construct PDF "+PDFstr);
  FM->SetUp().FactoryPDF(PDFstr);
  FM->SetUp().LoadSpeciesPDF("PWfit",1); /* second arg is lower bound of
                                          * yield parameter */

  // print PDF
  this->PrintFitter();
};


// load data and MC events from SimpleTree into FitManager
void BruAsymmetry::LoadDataSets(
  TString dataFileN, TString mcFileN, TString treename
) {

  // read input trees
  enum dataEnum {dt,mc}; // data, MC
  infile[dt] = new TFile(dataFileN,"READ");
  if(mcFileN=="") useMCint=false;
  else {
    useMCint=true;
    infile[mc] = new TFile(mcFileN,"READ");
  };

  TString outfileN[2];
  int ffMax = useMCint ? 2:1;
  for(int ff=0; ff<ffMax; ff++) {

    // create output tree, initially cloned from input trees
    intr[ff] = (TTree*) infile[ff]->Get(treename);
    switch(ff) {
      case dt: outfileN[ff] = outdir+"/treeData.root"; break;
      case mc: outfileN[ff] = outdir+"/treeMC.root"; break;
    };
    outfile[ff] = new TFile(outfileN[ff],"RECREATE");
    outtr[ff] = intr[ff]->CloneTree();

    // add unique ID branch to output trees, write and close
    Idx[ff] = 0;
    IdxBr[ff] = outtr[ff]->Branch("Idx",&(Idx[ff]),"Idx/D");
    for(Long64_t i=0; i<outtr[ff]->GetEntries(); i++) {
      IdxBr[ff]->Fill();
      Idx[ff] += 1;
    };
    outtr[ff]->Write(treename);
    outfile[ff]->Close();
    infile[ff]->Close();
  };

  // load trees into FitManager
  FM->LoadData("tree",outfileN[dt]);
  if(useMCint) FM->LoadSimulated("tree",outfileN[mc],"PWfit");
  
  if(useMCint)
    this->PrintLog(Form("MC INTEGRATION ENABLED, using %s",mcFileN.Data()));
  else
    this->PrintLog("MC INTEGRATION DISABLED");
};


// bin the data (and MC) according to specified binning scheme
void BruAsymmetry::Bin(TString varName, Int_t nBins, Double_t *binsArray) {
  FM->Bins().LoadBinVar(varName,nBins,binsArray);
};

// perform the fit
void BruAsymmetry::Fit() {

  // number of parallel threads
  nThreads = (Int_t) std::thread::hardware_concurrency();
  if(nThreads<1) nThreads=1;
  if(nThreads>6) nThreads=6; // max (to not clobber shared nodes)
  nWorkers = TMath::Min(nThreads,this->GetNbins()); // for PROOF
  printf("---- fit with %d parallel threads\n",nWorkers);


  // set minimizer algorithm
  if(minimizer.CompareTo("mcmc",TString::kIgnoreCase)==0) {
    FM->SetMinimiser( new HS::FIT::RooMcmcSeq(
      MCMC_iter, MCMC_burnin, MCMC_norm ) );
    this->PrintLog("");
    this->PrintLog("OPTIMIZER: MCMC");
    this->PrintLog(
      Form("MCMC iter,burnin,stepsize = %d, %d, %f",
            MCMC_iter,MCMC_burnin,1.0/MCMC_norm));
  } else if(minimizer.CompareTo("mcmcthencov",TString::kIgnoreCase)==0) {
    FM->SetMinimiser( new HS::FIT::RooMcmcSeqThenCov(
      MCMC_iter,     MCMC_burnin,     MCMC_norm,
      MCMC_cov_iter, MCMC_cov_burnin, MCMC_cov_norm ) );
    this->PrintLog("");
    this->PrintLog("OPTIMIZER: MCMCthenCov");
    this->PrintLog(
      Form("MCMC chain 1 iter,burnin,stepsize = %d, %d, %f",
            MCMC_iter,MCMC_burnin,1.0/MCMC_norm));
    this->PrintLog(
      Form("MCMC chain 2 iter,burnin,stepsize = %d, %d, %f",
            MCMC_cov_iter,MCMC_cov_burnin,1.0/MCMC_cov_norm));
  } else if(minimizer.CompareTo("minuit",TString::kIgnoreCase)==0) {
    this->PrintLog("");
    this->PrintLog("OPTIMIZER: Minuit");
    FM->SetMinimiser(new HS::FIT::Minuit2());
  } else {
    fprintf(stderr,"ERROR: unknown minimizer in BruAsymmetry::Fit()\n");
    return;
  };


  // fit settings:
  // -optimize: calculate and cache formulas; suitable for RooComponentsPDF
  // -number of parellel threads
  FM->SetUp().AddFitOption(RooFit::Optimize(1));
  FM->SetUp().AddFitOption(RooFit::NumCPU(nWorkers));


  // MINOS uncertainty estimation
  //FM->SetUp().AddFitOption(RooFit::Minos(kTRUE));


  // perform the fit
  //HS::FIT::PROCESS::Here::Go(FM); // do not use PROOF
  HS::FIT::PROCESS::Proof::Go(FM,nWorkers); // use PROOF
};


// get number of bins and dimensions configured in FitManager
Int_t BruAsymmetry::GetNbins() {
  Int_t nb = 1;
  for(auto axis : FM->Bins().GetBins().GetVarAxis())
    nb *= axis.GetNbins();
  return nb;
};
Int_t BruAsymmetry::GetNdim() {
  return FM->Bins().GetBins().GetNAxis();
};

// print bin information
void BruAsymmetry::PrintBinScheme() {
  printf("\nBin Scheme defined in BruAsymmetry FM:\n");
  Tools::PrintSeparator(50,"=");
  printf("- nBins = %d\n- nDims = %d\n",
    this->GetNbins(), this->GetNdim() );
  for(auto axis : FM->Bins().GetBins().GetVarAxis()) {
    Tools::PrintSeparator(50,"-");
    printf("%s axis has %d bins:\n",
      axis.GetName(), axis.GetNbins() );
    for(int ib=1; ib<=axis.GetNbins(); ib++) {
      printf("  bin %d:  %.3f to %.3f\n",
        ib, axis.GetBinLowEdge(ib), axis.GetBinUpEdge(ib) );
    };
  };
  Tools::PrintSeparator(50,"=");
};

BruAsymmetry::~BruAsymmetry() {
};

