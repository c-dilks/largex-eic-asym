# largex-eic-asym

## Building

- clone with submodules 
  `git clone --recurse-submodules <this-repo's-clone-address>`
- build BruFit
  - `source env.sh`
  - `pushd deps/brufit`
  - `mkdir build`
  - `cd build`
  - `cmake ../`
  - `make install` (probably will have warnings, since on dev)
  - `cmake ../` (again, for pcm file)
  - `make install`
  - `popd`
- build Largex-eic-asym
  - `make` (make sure `env.sh` has been sourced)

## Usage

- `source env.sh`
- put a `root` file with a `SimpleTree` TTree in `data/` (or anywhere you want)
- edit `brufit.C` and run
  - specify a directory name for output files
  - use `quantiles.C` to help define binning, if you want quantile binning
  - specify which modulations you want to simultaneously fit for; see `Modulation`
    class for more information
  - there are three minimizers available:
    - `minuit` uses Minuit MIGRAD to search for the maximum likelihood
    - `mcmc` uses Metropolis-Hastings Markov Chain Monte Carlo to sample the posterior
      near the likelihood
    - `mcmcthencov` uses the covariance from one MCMC as guidance for a second
  - uses PROOF for multithreading (one bin = one job); see `BruAsymmetry::Fit`
    to disable PROOF and run single-threaded
- run `errorCheck.sh` to grep for errors in PROOF log files
  - if you don't use PROOF, check `stdout` for errors
  - don't get confused by uncertainty reports
  - some errors are benign
  - make sure `$PROOF_LOG` is correctly set for your system in `env.sh`
- run `draw.C`
  - a new root file will appear in the output directory, containing the fit
    results, along with several other plots
