./scripts/backtrack.sh -j json/backtrack/es_myproc_test.json --override
./scripts/make_clusters.sh -j json/make_clusters/es_myproc_test.json
./scripts/analyze_clusters.sh -j json/analyze_clusters/es_myproc.json -i /eos/user/e/evilla/dune/sn-tps/production_es//es_myproc_test_tick3_ch2_min2_tot1_clusters.root
