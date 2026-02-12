# JSON configs

This folder contains a few tracked examples plus many local configs that are
intentionally gitignored.

Tracked examples:
- example_es_clean.json
- example_es_bg.json
- example_bg.json

How to use:
1. Copy one of the examples to a new file (for example, my_run.json).
2. Update paths (signal/bg/main folders, products prefix) and any analysis
	parameters you need.
3. Run your scripts using the new json file.

Notes:
- All other json files under json/ are ignored by git. They stay on disk but
	are not tracked.
- Keep the example files and this README tracked.