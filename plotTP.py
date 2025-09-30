# Example script to plot the Trigger Primitives in an event, grouped by MCTruth block.

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import uproot
import argparse
import mplhep as hep
hep.style.use("DUNE")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="input file name")
    parser.add_argument("index", type=int, help="Look at the ith event")
    args = parser.parse_args()
    with uproot.open(args.input) as rootfile:
        evids = np.unique(
            rootfile["triggerAnaDumpTPs/mctruths"]["Event"].array(library="numpy")
        )
        evid = evids[args.index]
        cut_expr = f"Event == {evid}"
        tps = rootfile[
            "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2;1"
        ].arrays(library="pd", cut=cut_expr)
        simides = rootfile["triggerAnaDumpTPs/simides"].arrays(
            library="pd", cut=cut_expr
        )
        mctruths = rootfile["triggerAnaDumpTPs/mctruths"].arrays(
            library="pd", cut=cut_expr
        )
        mcparticles = rootfile["triggerAnaDumpTPs/mcparticles"].arrays(
            library="pd", cut=cut_expr
        )

    plt.figure()
    hep.dune.label(rlabel=f"Run {mctruths.Run.unique().item()} Event {evid}")
    signal_tps = tps[(tps.bt_numelectrons != 0)]

    truth_blocks = dict(
        mctruths[["block_id", "generator_name"]].drop_duplicates().values
    )
    for block_id, mcparts in mcparticles.groupby("truth_block_id"):
        track_ids = mcparts.g4_track_id.unique()
        tagged_tps = signal_tps[signal_tps.bt_primary_track_id.abs().isin(track_ids)]
        label = f"{truth_blocks[block_id]}: {len(track_ids)} G4 Tracks"
        plt.scatter(tagged_tps.channel, tagged_tps.time_start, label=label, s=5)
    noise_tps = tps[(tps.bt_numelectrons == 0)]
    plt.scatter(noise_tps.channel, noise_tps.time_start, label="Noise", s=5)
    for iapa in range(12):
        plt.axvline(2560 * iapa, color="k", linestyle="--")
    plt.xlabel("ChannelID")
    plt.ylabel("DTS Ticks")
    plt.legend(loc="upper right")
    plt.show()


if __name__ == "__main__":
    main()