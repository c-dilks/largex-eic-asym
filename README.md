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
