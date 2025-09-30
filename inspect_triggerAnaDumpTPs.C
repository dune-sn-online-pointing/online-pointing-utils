// inspect_triggerAnaDumpTPs.C
// ROOT macro to inspect the contents of the triggerAnaDumpTPs directory
void inspect_triggerAnaDumpTPs() {
    TFile *file = TFile::Open("online-pointing-utils/data/cleanES60_tpstream.root");
    if (!file || file->IsZombie()) {
        std::cout << "Cannot open file!" << std::endl;
        return;
    }
    TDirectory *dir = file->GetDirectory("triggerAnaDumpTPs");
    if (!dir) {
        std::cout << "Directory 'triggerAnaDumpTPs' not found!" << std::endl;
        return;
    }
    dir->ls();
}
