#include <iostream>
#include <vector>
#include <numeric>
#include "TFile.h"
#include "TTree.h"

// Helper function to check if a vector contains only zeros
bool isAllZeros(const std::vector<int>* vec) {
    if (!vec || vec->empty()) {
        return true; // Consider null or empty vectors as "all zeros"
    }
    for (int val : *vec) {
        if (val != 0) {
            return false; // Found a non-zero value
        }
    }
    return true;
}

void check_vectors() {
    const char* filePath = "/exp/uboone/data/users/nlane/analysis/mc_inclusive_run1_fhc.root";
    const char* treeName = "nuselection/EventSelectionFilter";

    TFile *f = new TFile(filePath);
    if (!f || f->IsZombie()) {
        std::cout << "Error opening file: " << filePath << std::endl;
        return;
    }

    TTree *tree = (TTree*)f->Get(treeName);
    if (!tree) {
        std::cout << "Error getting TTree: " << treeName << std::endl;
        f->Close();
        return;
    }

    // Create variables to hold the data from the branches
    int analysis_channel;
    std::vector<int> *true_image_w = nullptr;
    std::vector<int> *true_image_u = nullptr;
    std::vector<int> *true_image_v = nullptr;

    // Link the variables to the TTree branches
    tree->SetBranchAddress("analysis_channel", &analysis_channel);
    tree->SetBranchAddress("true_image_w", &true_image_w);
    tree->SetBranchAddress("true_image_u", &true_image_u);
    tree->SetBranchAddress("true_image_v", &true_image_v);

    long long n_entries = tree->GetEntries();
    std::cout << "Scanning " << n_entries << " total events..." << std::endl;

    int signal_events_found = 0;

    // Loop over all entries in the TTree
    for (long long i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);

        // Only check events that match the signal definition
        //if (analysis_channel == 10 || analysis_channel == 11) {
        signal_events_found++;
        bool w_is_empty = isAllZeros(true_image_w);
        bool u_is_empty = isAllZeros(true_image_u);
        bool v_is_empty = isAllZeros(true_image_v);

        // Print results only for the first 10 signal events to avoid spam
        if (signal_events_found <= 10) {
            std::cout << "Signal Event (Entry " << i << "): "
                        << "w_plane_empty=" << std::boolalpha << w_is_empty << ", "
                        << "u_plane_empty=" << std::boolalpha << u_is_empty << ", "
                        << "v_plane_empty=" << std::boolalpha << v_is_empty
                        << std::endl;
        }
    }

    std::cout << "\nScan complete. Found " << signal_events_found << " signal events in total." << std::endl;
    f->Close();
}