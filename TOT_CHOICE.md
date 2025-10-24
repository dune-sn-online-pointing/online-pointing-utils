tot_cut=2 vs tot_cut=3 Comparison
Key Metrics at 0 MeV (baseline - no energy cut):
Sample	Metric	tot_cut=2	tot_cut=3	Change
CC	MARLEY clusters	686	655	-4.5%
CC	Background	21,556	17,328	-19.6%
CC	S/B ratio	0.032	0.038	+19% better
ES	MARLEY clusters	353	340	-3.7%
ES	Background	20,780	16,864	-18.8%
ES	S/B ratio	0.017	0.020	+18% better
At 2.5 MeV (optimal operating point):
Sample	Metric	tot_cut=2	tot_cut=3	Change
CC	MARLEY retained	381 (55.5%)	376 (57.4%)	Similar
CC	Background removed	99.2%	98.9%	Similar
CC	S/B ratio	2.08	2.05	Similar
ES	MARLEY retained	265 (75.1%)	262 (77.1%)	Similar
ES	Background removed	99.6%	99.5%	Similar
ES	S/B ratio	2.94	2.98	Similar
At 3.0 MeV:
Sample	Metric	tot_cut=2	tot_cut=3	Change
CC	S/B ratio	8.78	8.73	Similar
ES	S/B ratio	14.71	15.44	Similar
Recommendation: tot_cut=3 is BETTER
Why tot_cut=3 wins:
Better baseline purity: Right from the start (0 MeV), tot_cut=3 gives you ~19% better signal-to-background ratio by filtering out low-quality TPs with short time-over-threshold.

Similar signal efficiency: You only lose ~4% of MARLEY signal upfront, which is a small price to pay for the cleaner sample.

Cleaner background rejection: tot_cut=3 removes ~4,000 background clusters in CC and ~4,000 in ES before any energy cut is applied, with minimal signal loss.

Similar final performance: At optimal operating points (2-3 MeV), both configurations perform nearly identically in terms of S/B ratios and signal retention percentages.

More robust: The stricter ToT requirement means you're working with higher-quality hits that are more likely to represent real energy depositions, not just noise or electronic artifacts.

Bottom line:
Use tot_cut=3. You get better background rejection "for free" (just by requiring slightly longer pulses) without sacrificing much signal, and the final discrimination performance is virtually identical. The cleaner starting sample will likely make downstream analysis more reliable.