from ROOT import TFile, TTree
import os
import sys

DEBUG = False
COUNT = 10000

testfile = "/Users/schellma/Dropbox/newsnb/online-pointing-utils/data/es/tpstreams/prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root"
if len(sys.argv) > 1:
    print (f"Debug: Command-line argument provided, using file: {sys.argv[1]}")
    filename = sys.argv[1]
else:
    filename = testfile
basename = os.path.basename(filename).replace(".root","_").replace("__","_")

print(f"Opening file: {filename}")
file = TFile.Open(filename,"READONLY")
if not file or file.IsZombie():
    print(f"Error opening file: {filename}")
    sys.exit(1) 

paths = ["triggerAna/simides","triggerAna/mctruths","triggerAna/mcparticles","triggerAna/TriggerPrimitives/tpmakerTPCsimpleThr__TPGen",
         "triggerAna/TriggerPrimitives/tpmakerTPCabsRS__TPGen"]

tp_map_lo = {}
tp_map_hi = {}
counter = {}
for path in paths:
    tp_tree = TTree()
    tp_tree = file.Get(path)
    if not tp_tree:
        print(f"Error: Tree not found at path: {path}")
        continue
    tp_map_lo[path] ={}
    tp_map_hi[path] ={}
    counter[path] = {}
    newevent = -1
    count = 0
    for index in range(tp_tree.GetEntries()):
        tp_tree.GetEntry(index)
        event = (tp_tree.run,tp_tree.event)
        if DEBUG: print ("event",path,count,event,index)
        tp_map_hi[path][event] = index
        if event != newevent:
            if DEBUG:print ("new event",path,count,event,index)
            
            newevent = event
            
            count += 1
            if count > COUNT:
                print(f"Debug: Reached {COUNT} events for path {path}, stopping early.")
                break
            tp_map_lo[path][event] = index
            counter[path][event] = count
        
        
#file.close()


for path in paths:
    f = open(f"{basename}_{path.replace('/','_')}.txt","w")
    for event in tp_map_lo[path].keys():
        diff = tp_map_hi[path][event] - tp_map_lo[path][event]+1
        out = f" {event[0]} {event[1]} {counter[path][event]} {tp_map_lo[path][event]} {tp_map_hi[path][event]} {diff}"
        f.write(out + "\n")
        print(out)
        #print(f"{path} event {event} low {tp_map_lo[path][event]} high {tp_map_hi[path][event]} diff {diff}", file=f)
    f.close()
