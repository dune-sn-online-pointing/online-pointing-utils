# Production_Sequence.md

this documents production sequence from reco files to images.

## setup for using LArSoft to produce tpstream format tuples

This is done using the merge-utils package

Log into a gpvm and get the merge-utils repo

``` bash
export DUNEDATA=/exp/dune/data/users/$USER
mkdir $DUNEDATA/snb
cd snb
git clone http://github.com/DUNE/merge-utils.git
```

Make a setup script

```bash
export DUNEDATA=/exp/dune/data/users/$USER
export DUNE_VERSION=v10_12_01d01 ## you can modify as needed
export DUNE_QUALIFIER=e26:prof ## you can modify as needed
cd $DUNEDATA/snb/merge-utils
source setup_fnal.sh
cd campaigns
source setup_campaign.sh trigprim-2026-03
cd trigprim-2026-03
```

You can then follow the instructions for [merge-utils campaigns](https://dune.github.io/merge-utils/Campaigns_holder.html) to generate tagged campaigns.

### current files

Current files (April 13,2026) were produced from the following files:

```
trigprim-2026-03.csv
trigprim-2026-03_jobs.csv
triggerana_tree_1x2x2_simpleThr_production.yaml
triggerana_tree_1x2x6_simpleThr_production.fcl
triggerana_tree_1x2x6_simpleThr_production.yaml
triggerana_tree_1x2x2_simpleThr_production.fcl
```

using commands

```bash
python -m make_pass1 TRGSIM_CENTRAL_v4
python -m make_pass1 TRGSIM_CC_v9
python -m make_pass1 TRGSIM_ES_v9
```

which made merge command files:

```
TRGSIM_CENTRAL_v4.sh
TRGSIM_ES_v9.sh
TRGSIM_CC_v9.sh
```

Only the first few commands in those files were run.

One can find the outputs by metacat queries

~~~
metacat query -s "files where merge.tag=TRGSIM_CENTRAL_v4 and dune.output_status=confirmed"
Files:        20
Total size:   25900358325 (25.900 GB)
~~~

## Now run the backtracker and the rest of the pipeline

This runs fine under AL9

log onto another gpvm 

First time around

``` bash
export DUNEDATA=/exp/dune/data/users/$USER
cd $DUNEDATA/snb
git clone https://github.com/dune-sn-online-pointing/online-pointing-utils.git
git checkout OSU-v2.1.1 # need this branch
cd $DUNEDATA/snb/online-pointing-utils
export HOME_DIR=$PWD
source python/setup-python.sh # make the local python environment
$HOME_DIR/scripts/manage-submodules.sh --up # set up the submodules you need
mkdir data
```


make a file called `setup_snb9.sh` to run every time you log in 



``` bash
source /cvmfs/dune.opensciencegrid.org/spack/v1.1/share/spack/setup-env.sh
echo "Activate dune-prototype"
spack env activate dune-prototype
echo "load GCC and CMAKE so don't use system"
echo "GCC"
spack load root@6_28_12
spack load gcc@12.5.0 arch=linux-almalinux9-x86_64_v2 
cd $DUNEDATA/snb/online-pointing-utils
export HOME_DIR=$PWD

```

You can then edit/use the json configurations to run the backtracker

### getting lists of file locations for a given sample

~~~
justin get-token
./scripts/generatelist.sh <merge tag>
~~~

will produce `lists/<merge-tag>.txt`

You need to backtrack the "CENTRAL" background sample first. 

Look at the json file to see what it does.


~~~
./scripts/sequence.sh -j json/gpvmlist_bg_only.json -bt
~~~

I found this took about 50 minutes/file to produce 100 events. 

This makes files in `$HOME_DIR/data/output/bg_backtracker`

once those are done

``` bash
ls $HOME_DIR/data/output/bg_backtracker  | grep <merge-tag> >  lists/bg_tps_list.txt
```

NOTE: should be able to use the folder method but have not tested it. 

## Now you can run the whole chain for the ES and CC samples

``` bash
./scripts/sequence.sh -j json/gpvmlist_cc_bg.json --all
./scripts/sequence.sh -j json/gpvmlist_es_bg.json --all
```

This should give you file structure like this for `cc` and `es`

```
data/cc/output/sig
data/cc/output/clusters/es_bg_cluster_images_tick3_ch2_min2_tot3_e2p0/U
data/cc/output/clusters/es_bg_cluster_images_tick3_ch2_min2_tot3_e2p0/V
data/cc/output/clusters/es_bg_cluster_images_tick3_ch2_min2_tot3_e2p0/X
data/cc/output/clusters/es_bg_cluster_images_tick3_ch2_min2_tot3_e2p0
data/cc/output/clusters
data/cc/output/matched_clusters
```

## looking at the results

You can then do fun things like run the `cluster_display.py` and `view_volume_quick.py` programs.

`view_volume_quick.py` is a real memory user. 


















