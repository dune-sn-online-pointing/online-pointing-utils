#include <vector>

void plot_var(){

    // std::vector<TriggerPrimitive> tps;
    // std::vector<TrueParticle> true_particles;
    // std::vector<Neutrino> neutrinos;
    // int supernova_option = 0;
    // int event_number = 0;
    
    // std::string filename = "/exp/dune/app/users/emvilla/online-pointing-utils/data/ES100clean/clusters_tick1_ch1_min1.root";
    std::string filename = "/exp/dune/app/users/emvilla/online-pointing-utils/data/clusters_tick1_ch1_min1.root";

    // open the file, read the tree, plot tp_adc_peak (a branch containing vectors, plot all values)
    TFile *file = TFile::Open(filename.c_str());
    if (!file || file->IsZombie()) {
        std::cerr << " Error opening file: " << filename << std::endl;
        return 0;
    }
    // open the file, read the three trees for the three planes
    TTree *tree_X = (TTree*)file->Get("clusters_tree_X");
    TTree *tree_U = (TTree*)file->Get("clusters_tree_U");
    TTree *tree_V = (TTree*)file->Get("clusters_tree_V");
    if (!tree_X || !tree_U || !tree_V) {
        std::cerr << "One or more trees not found in file: " << filename << std::endl;
        return 0;
    }
    // get the number of entries in the tree
    int nentries_X = tree_X->GetEntries();
    int nentries_U = tree_U->GetEntries();
    int nentries_V = tree_V->GetEntries();
    // Use nentries_X for loops over tree_X, nentries_U for tree_U, etc.
    int nentries = nentries_X; // For backward compatibility with existing code below
    std::cout << "Number of entries in tree: " << nentries << std::endl;
    // get the branches
    std::vector<int>* tp_adc_peak = nullptr;
    std::vector<int>* tp_samples_over_threshold = nullptr;
    int n_tps;
    double total_charge;
    float true_particle_energy;

    tree_X->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
    tree_X->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold);
    tree_X->SetBranchAddress("n_tps", &n_tps);
    tree_X->SetBranchAddress("total_charge", &total_charge);
    tree_X->SetBranchAddress("true_particle_energy", &true_particle_energy);

    // Set branch addresses for U and V planes
    std::vector<int>* tp_adc_peak_U = nullptr;
    std::vector<int>* tp_samples_over_threshold_U = nullptr;
    int n_tps_U;
    double total_charge_U;
    float true_particle_energy_U;
    tree_U->SetBranchAddress("tp_adc_peak", &tp_adc_peak_U);
    tree_U->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold_U);
    tree_U->SetBranchAddress("n_tps", &n_tps_U);
    tree_U->SetBranchAddress("total_charge", &total_charge_U);
    tree_U->SetBranchAddress("true_particle_energy", &true_particle_energy_U);

    std::vector<int>* tp_adc_peak_V = nullptr;
    std::vector<int>* tp_samples_over_threshold_V = nullptr;
    int n_tps_V;
    double total_charge_V;
    float true_particle_energy_V;
    tree_V->SetBranchAddress("tp_adc_peak", &tp_adc_peak_V);
    tree_V->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold_V);
    tree_V->SetBranchAddress("n_tps", &n_tps_V);
    tree_V->SetBranchAddress("total_charge", &total_charge_V);
    tree_V->SetBranchAddress("true_particle_energy", &true_particle_energy_V);


    // create hists for all and for each plane
    TH1F *h_peak_all = new TH1F("h_peak_all", "ADC Peak (All Planes)", 100, 54.5, 1500.5);
    TH1F *h_peak_X = new TH1F("h_peak_X", "ADC Peak (Plane X)", 200, 54.5, 1500.5);
    TH1F *h_peak_U = new TH1F("h_peak_U", "ADC Peak (Plane U)", 200, 54.5, 1500.5);
    TH1F *h_peak_V = new TH1F("h_peak_V", "ADC Peak (Plane V)", 200, 54.5, 1500.5);

    int tot_cut = 0;
    // Fill X plane
    for (int i = 0; i < nentries_X; i++) {
        tree_X->GetEntry(i);
        for (size_t j = 0; j < tp_adc_peak->size(); j++) {
            if ((*tp_samples_over_threshold)[j] > tot_cut) {
                h_peak_X->Fill((*tp_adc_peak)[j]);
                h_peak_all->Fill((*tp_adc_peak)[j]);
            }
        }
    }
    // Fill U plane
    for (int i = 0; i < nentries_U; i++) {
        tree_U->GetEntry(i);
        for (size_t j = 0; j < tp_adc_peak_U->size(); j++) {
            if ((*tp_samples_over_threshold_U)[j] > tot_cut) {
                h_peak_U->Fill((*tp_adc_peak_U)[j]);
                h_peak_all->Fill((*tp_adc_peak_U)[j]);
            }
        }
    }
    // Fill V plane
    for (int i = 0; i < nentries_V; i++) {
        tree_V->GetEntry(i);
        for (size_t j = 0; j < tp_adc_peak_V->size(); j++) {
            if ((*tp_samples_over_threshold_V)[j] > tot_cut) {
                h_peak_V->Fill((*tp_adc_peak_V)[j]);
                h_peak_all->Fill((*tp_adc_peak_V)[j]);
            }
        }
    }

    // graph to plot true_particle energy vs total_charge
    TGraph *g = new TGraph();
    g->SetTitle("True Particle Energy vs Total Charge");
    g->SetMarkerStyle(20);
    g->SetMarkerColor(kBlue);
    g->SetMarkerSize(0.5);
    g->SetLineColor(kBlue);
    g->SetLineWidth(2);
    
    // loop over the entries
    for (int i = 0; i < nentries; i++) {
        tree_X->GetEntry(i);
        // loop over the values in the vector
        for (size_t j = 0; j < tp_adc_peak->size(); j++) {
            if ((*tp_samples_over_threshold)[j] > tot_cut) 
                h_peak_X->Fill((*tp_adc_peak)[j]);

            if (n_tps > 5){
                if (true_particle_energy > 0 &&  true_particle_energy < 50 && total_charge > 0){
                    g->SetPoint(g->GetN(), total_charge, true_particle_energy);
                    // std::cout << "Total charge: " << total_charge << ", True particle energy: " << true_particle_energy << std::endl;
                }
            }
        }
    }

    gStyle->SetOptStat(111111);

    // show overflow in legend
    h_peak_all->SetOption("HIST");
    h_peak_all->SetXTitle("ADC Peak");
    h_peak_all->SetYTitle("Entries");
    h_peak_all->SetTitle("ADC Peak Histogram");
    h_peak_all->SetLineColor(kBlack);
    h_peak_all->SetFillColorAlpha(kBlack, 0.15);

    h_peak_X->SetOption("HIST");
    h_peak_X->SetXTitle("ADC Peak");
    h_peak_X->SetYTitle("Entries");
    h_peak_X->SetTitle("ADC Peak Histogram");
    h_peak_X->SetLineColor(kBlue);
    h_peak_X->SetFillColorAlpha(kBlue, 0.2);

    h_peak_U->SetOption("HIST");
    h_peak_U->SetXTitle("ADC Peak");
    h_peak_U->SetYTitle("Entries");
    h_peak_U->SetTitle("ADC Peak Histogram");
    h_peak_U->SetLineColor(kRed);
    h_peak_U->SetFillColorAlpha(kRed, 0.2);

    h_peak_V->SetOption("HIST");
    h_peak_V->SetXTitle("ADC Peak");
    h_peak_V->SetYTitle("Entries");
    h_peak_V->SetTitle("ADC Peak Histogram");
    h_peak_V->SetLineColor(kGreen+2);
    h_peak_V->SetFillColorAlpha(kGreen+2, 0.2);

    // create a canvas divided in 2x2 pads to show the 4 histograms separately
    TCanvas *c1 = new TCanvas("c1", "ADC Peak by Plane", 1000, 800);
    c1->Divide(2,2);

    c1->cd(1);
    gPad->SetLogy();
    h_peak_all->Draw("HIST");
    TLegend *leg_all = new TLegend(0.7, 0.8, 0.9, 0.9);
    leg_all->AddEntry(h_peak_all, "All Planes", "f");
    leg_all->Draw();

    c1->cd(2);
    gPad->SetLogy();
    h_peak_X->Draw("HIST");
    TLegend *leg_X = new TLegend(0.7, 0.8, 0.9, 0.9);
    leg_X->AddEntry(h_peak_X, "Plane X", "f");
    leg_X->Draw();

    c1->cd(3);
    gPad->SetLogy();
    h_peak_U->Draw("HIST");
    TLegend *leg_U = new TLegend(0.7, 0.8, 0.9, 0.9);
    leg_U->AddEntry(h_peak_U, "Plane U", "f");
    leg_U->Draw();

    c1->cd(4);
    gPad->SetLogy();
    h_peak_V->Draw("HIST");
    TLegend *leg_V = new TLegend(0.7, 0.8, 0.9, 0.9);
    leg_V->AddEntry(h_peak_V, "Plane V", "f");
    leg_V->Draw();

    c1->SaveAs("adc_peak_planes.png");
    // create a canvas to plot the graph
    TCanvas *c2 = new TCanvas("c2", "True Particle Energy vs Total Charge", 800, 600);
    c2->SetGrid();
    g->GetXaxis()->SetTitle("Total Charge");
    g->GetYaxis()->SetTitle("True Particle Energy");
    //g->GetXaxis()->SetTitleSize(0.05);
    //g->GetYaxis()->SetTitleSize(0.05);
    //g->GetXaxis()->SetLabelSize(0.05);
    //g->GetYaxis()->SetLabelSize(0.05);
    //g->GetXaxis()->SetTitleOffset(1.2);
    //g->GetYaxis()->SetTitleOffset(1.2);
    // larger graph marker
    g->SetMarkerSize(1.5);
    g->SetMarkerStyle(20);

    // apply a linear fit on the graph only if it has points
    TLegend *leg = new TLegend(0.1, 0.7, 0.3, 0.9);
    leg->AddEntry(g, "Data", "p");
    
    if (g->GetN() > 1) {
        g->Fit("pol1", "Q");
        TF1 *fit_func = g->GetFunction("pol1");
        if (fit_func) {
            fit_func->SetLineColor(kRed);
            fit_func->SetLineWidth(2);
            fit_func->SetLineStyle(2);
            fit_func->SetTitle("Linear Fit");
            
            leg->AddEntry(fit_func, "Linear Fit", "l");
            leg->AddEntry((TObject*)0, Form("Slope: %.2f", fit_func->GetParameter(1)), "");
            leg->AddEntry((TObject*)0, Form("Intercept: %.2f", fit_func->GetParameter(0)), "");
            
            std::cout << "Slope: " << fit_func->GetParameter(1) << std::endl;
        } else {
            std::cout << "Fit failed - no function available" << std::endl;
        }
    } else {
        std::cout << "Graph has " << g->GetN() << " points - not enough for fitting" << std::endl;
    }
    
    leg->Draw();

    g->Draw("AP");
    c2->SaveAs("true_particle_energy_vs_total_charge.png");
    std::cout << "True Particle Energy vs Total Charge graph saved as true_particle_energy_vs_total_charge.png" << std::endl;

    //close
    // file->Close();

}
