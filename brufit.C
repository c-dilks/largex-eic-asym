R__LOAD_LIBRARY(LargexAsym)
#include "BruAsymmetry.h"

/* arguments
 * - bruDir: name of output directory; several files will be written
 * - minimizer: optimizer algorithm, choose one of the following:
 *   - minuit: maximum likelihood using Minuit MIGRAD
 *   - mcmc: sample posterior using Metropolis-Hastings walk with
 *     gaussian proposal; set hyperparameters at the end of this macro
 *   - mcmcthencov: first run mcmc as above, then use the resulting
 *     covariance matrix in the gaussian proposal function of a second,
 *     subsequent mcmc walk
 */

void brufit(TString bruDir="bruspin", TString minimizer="minuit") {

  // load macros needed for PROOF
  // - if you prefer not to use PROOF, adjust `HS::FIT::...::Go` calls in `BruAsymmetry::Fit`
  // - proof logs are sent to $PROOF_LOG; make sure it is correct, your system may be different!
  TString BRUCODE=gSystem->Getenv("BRUFIT");
  TString macpath=BRUCODE+"/macros";
  gROOT->SetMacroPath(
    Form("%s:%s",gROOT->GetMacroPath(),(macpath).Data()));

  // instantiate brufit
  BruAsymmetry * B = new BruAsymmetry(bruDir,minimizer);

  // build modulations
  // - see `Modulation::Initialize` for the full set of available modulations
  B->AddNumerMod(new Modulation(0,Modulation::kUT)); // sin(phiH-phiS)
  B->AddNumerMod(new Modulation(1,Modulation::kUT)); // sin(phiH+phiS)

  // build full PDF
  B->BuildPDF();

  // set binning scheme
  Double_t xbins[6] = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 };
  B->Bin(
      "X", // branch name in SimpleTree
      5, // number of bins
      xbins // bin boundaries (NOTE: size = number_of_bins + 1)
      );

  // load SimpleTrees
  B->LoadDataSets(
      "data/yRatio.example_5x41.root", // data to fit
      "" // unpolarized MC data for likelihood normalization / acceptance correction (not used if unspecified)
      );

  // optimizer hyperparameters (Markov Chain); not used if minimizer==minuit
  // - MCMC settings
  B->MCMC_iter        = 5000; // number of samples
  B->MCMC_burnin      = 0.1 * ((Double_t)B->MCMC_iter); // number to burn
  B->MCMC_norm        = 1.0 / 0.03; // ~ 1/stepsize
  // - 2nd MCMC settings (if using MCMCthenCov algorithm)
  B->MCMC_cov_iter    = 5000; // number of samples
  B->MCMC_cov_burnin  = 0.1 * ((Double_t)B->MCMC_iter); // number to burn
  B->MCMC_cov_norm    = 1.0 / 0.03; // ~ 1/stepSize

  // perform fit
  B->Fit();

  // print MCMC acceptance rates
  gSystem->RedirectOutput(B->GetLogName());
  gROOT->ProcessLine(".! mcmcAcceptanceRate.sh");
  gSystem->RedirectOutput(0);

  // TODO: need new draw script
  // draw
  /*
  TString cmd = Form(".x drawBru.C(\"%s\",\"%s\")",bruDir.Data(),minimizer.Data());
  printf("\nEXECUTE: %s\n\n",cmd.Data());
  gROOT->ProcessLine(cmd);
  */
};
