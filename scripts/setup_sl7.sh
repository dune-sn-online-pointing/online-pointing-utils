source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
echo "got here"
echo " Setting up necessary packages..."
# versions are chosen for compatibility with the pyenv
setup root v6_26_06a -q e20:p3913:prof
setup nlohmann_json v3_10_4_1 -q e26:prof
setup cmake  v3_27_4
setup valgrind v3_10_1
setup gcc v9_3_0