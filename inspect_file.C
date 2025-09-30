// inspect_file.C
// ROOT macro to inspect the structure of a ROOT file
void inspect_file() {
    TFile *file = TFile::Open("online-pointing-utils/data/cleanES60_tpstream.root");
    if (!file || file->IsZombie()) {
        std::cout << "Cannot open file!" << std::endl;
        return;
    }
    file->ls();
}
