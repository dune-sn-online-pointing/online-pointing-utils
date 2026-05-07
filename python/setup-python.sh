export HOME_DIR=${PWD}
python -m venv $HOME_DIR/python/online-pointing-env
source $HOME_DIR/python/online-pointing-env/bin/activate
pip install -r $HOME_DIR/python/requirements.txt