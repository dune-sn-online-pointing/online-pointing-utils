from ROOT import TFile, TTree, gSystem
import ROOT
#import PlotUtils
import sys, os

DEBUG=False
testfile = "/Users/schellma/Dropbox/newsnb/online-pointing-utils/data/es/tpstreams/prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root"
#testfile = "/Users/schellma/Dropbox/snb/online-pointing-utils/data/central/tpstreams/prodbackground_radiological_decay0_dune10kt_1x2x6_centralAPA__trg_mc_2025a_tpg__tpg_dune10kt_1x2x6__reco__v10_12_01d01__triggerana_dune10kt_1x2x6__v10_12_01d01__ntuple__CENTRAL_prod_l005000_20251126T010938_c0_tpstream.root"
testfile = "/Users/schellma/Dropbox/snb/online-pointing-utils/data/cc/tpstreams/prodmarley_nue_flat_cc_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01__ntuple__FLAT-CC_prod-pass2_v2_20251201T185316_f0.root"
if len(sys.argv) > 1:
    print (f"Command-line argument provided, using file: {sys.argv[1]}")
    inputfile = sys.argv[1]
else:
    print (f"No command-line argument provided, using test file: {testfile}")
    inputfile = testfile

theinputfile = TFile.Open(inputfile,'READONLY')

#inputfile = TFile.Open(inputfile,'READONLY')


#trees = [key.GetName() for key in theinputfile.GetListOfKeys() if key.GetClassName() == "TTree"]
trees = ["triggerAna/simides",
         "triggerAna/mctruths",
         "triggerAna/mcparticles",
         "triggerAna/event_summary",
         "triggerAna/TriggerPrimitives/tpmakerTPCsimpleThr__TPGen",
         "triggerAna/TriggerPrimitives/tpmakerTPCabsRS__TPGen"
        ]


print (f"looking at trees: {trees}")


runs = []
maxevents = {}
runtree = theinputfile.Get("triggerAna/mctruths")
for entry in runtree:
    if entry.run not in runs:
        runs.append(entry.run)
        maxevents[entry.run]=-1
    if entry.event > maxevents[entry.run]:
        maxevents[entry.run]=entry.event
    
print (f"Found runs: {runs}")
print (f"Found maxevents: {maxevents}")
skip = 500*len(runs)
for run in runs:
    for skipper in range(100,200):
        if skipper*skip > maxevents[run]: 
            print(f"Reached end of events for run {run}, max event is {maxevents[run]}, skipping rest of event ranges")
            break
        eventrange = (skipper * skip, (skipper + 1) * skip-1)
        combined = ""
        if "tpstream.root" in inputfile:
            combined = os.path.basename(inputfile).replace("tpstream.root", f"run_{run}_{eventrange[0]}-{eventrange[1]}_tpstream.root") 
        else:
            combined = os.path.basename(inputfile).replace(".root", f"run_{run}_{eventrange[0]}-{eventrange[1]}.root")
        outlist = []
    
        for tree in trees:
            treename = os.path.basename(tree)
            treepath = os.path.dirname(tree)
            if DEBUG: print(f"Processing tree: {tree}")
            tree = theinputfile.Get(tree)
            outfilename = os.path.join("tmp",  f"tmp_run_{run}_{eventrange[0]}-{eventrange[1]}_{treename}_tpstream.root")
            outfile = TFile.Open(outfilename, 'RECREATE')
            if DEBUG: outfile.ls()
            outlist.append(outfilename)
            if DEBUG: print(f"Output file: {outfilename}, tree is {tree}, treename is {treename}")
            #df = RDataFrame(tree, inputfile)
            filter = "(event>=%s && event<= %s && run==%d)" % (eventrange[0], eventrange[1],run)
            print (f"Applying filter: {filter} to tree {tree} for run {run} and event range {eventrange[0]}-{eventrange[1]} ...")
            #df = df.Filter(filter)
            # if "TriggerPrimitives" in tree:
            #     inputfile.cd("triggerAna/TriggerPrimitives")
            outfile.mkdir(treepath)
            outfile.cd(treepath)
            newtree = tree.CopyTree(filter)
            newtree.Write()
            #gSystem.ChangeDirectory(treepath)
            if DEBUG: print ("Tree %s Created directory: %s, current path is %s" % (tree,treepath, outfile.GetPath())  )
            if DEBUG:outfile.ls()
            outfile.Close()
        
        
            #if DEBUG: print(df.GetColumnNames())   
        print ("Finished processing trees for run %d event range %s-%s, now combining into %s ..." % (run,eventrange[0], eventrange[1], combined))
        os.system(f"hadd -f {combined} {' '.join(outlist)}")

theinputfile.Close()
