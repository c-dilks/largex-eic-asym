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
- edit `brufit.C` and run; output will appear in specified `bruDir`
- run `errorPrintProof.sh` to quickly check log files (though
  it may confuse uncertainty reports for errors...)
- TODO: asymmetry plotting code
