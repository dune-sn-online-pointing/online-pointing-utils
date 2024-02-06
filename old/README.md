# online-pointing-utils
Collection of tools used to read the data from online or offline frameworks.

Code is mainly python, to run apps use the scripts. 
Mind that different apps need different environments. 
To generate images, you can have your own venv with matplotlib (seems like the easiest option).

As of this commit, you can use `convert-hdf5-to-text.sh` and `create-images-from-tps.sh`. 
They can be executed selecting just input file and output folder from command line. 
Improvements will come.

`generate-images-from-hdf5.sh` doesn't work yet, you have to run the two steps separately.

## Example of usage
To make it simple, I work in lxplus in a shell with python 3.8, with `scl enable rh-python38 bash`.
Then these steps work for me:

```
./convert-hdf5-to-text.sh -i /eos/home-e/evilla/dune/tps/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5 -o /eos/home-e/evilla/dune/tps/;
source ../venv/bin/activate; # my venv containing matplotlib
./create-images-from-tps.sh -i /eos/home-e/evilla/dune/tps/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.txt -o /eos/home-e/evilla/dune/tps/; # output will be npy "images"
deactivate;
```


