#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <ctime>

#include "position_calculator.h"
#include "group_to_root_libs.h"
#include "group.h"
#include "superimpose_root_files_libs.h"

int main(int argc, char* argv[]) {
    std::string sig_group_filename;
    std::string bkg_group_filename;
    std::string outfolder;
    float radius = 1.0;
    
    if (argc < 5) {
        std::cout << "Usage: superimpose_root_files <sig_group_filename> <bkg_group_filename> <outfolder> <radius>" << std::endl;
        return 1;        
    } else {
        sig_group_filename = argv[1];
        bkg_group_filename = argv[2];
        outfolder = argv[3];
        radius = std::stof(argv[4]);
    }

    // start the clock
    std::clock_t start;

    // read the groups from the root files
    std::vector<group> sig_groups = read_groups_from_root(sig_group_filename);
    std::cout << "Number of sig groups: " << sig_groups.size() << std::endl;
    std::vector<group> bkg_groups = read_groups_from_root(bkg_group_filename);
    std::cout << "Number of bkg groups: " << bkg_groups.size() << std::endl;
    // create a map connecting the event number to the background groups
    std::map<int, std::vector<group>> bkg_event_mapping = create_event_mapping(bkg_groups);
    std::cout << "Bkg event mapping created" << std::endl;
    std::vector<int> bkg_list_of_event_numbers;
    for (auto const& x : bkg_event_mapping) {
        bkg_list_of_event_numbers.push_back(x.first);
    }
    std::cout << "Number of bkg events: " << bkg_list_of_event_numbers.size() << std::endl;

    // create a map connecting the event number to the signal groups
    std::map<int, std::vector<group>> sig_event_mapping = create_event_mapping(sig_groups);
    std::cout << "Sig event mapping created" << std::endl;
    std::vector<int> sig_list_of_event_numbers;
    for (auto const& x : sig_event_mapping) {
        sig_list_of_event_numbers.push_back(x.first);
    }
    std::cout << "Number of sig events: " << sig_list_of_event_numbers.size() << std::endl;

    // superimpose a random background event to a signal event
    std::vector<group> superimposed_groups;
    std::vector<int> test_appo_vec;

    for (int i = 0; i < sig_list_of_event_numbers.size(); i++) {
    // for (int i = 0; i < sig_list_of_event_numbers.size(); i++) {
        if (i%1000 ==0){
            std::cout<<"Event number: "<< i << std::endl;
        }
        int sig_event_number = sig_list_of_event_numbers[i];
        std::vector<group> sig_event_groups = sig_event_mapping[sig_event_number];
        // get a random background event
        int bkg_event_number = bkg_list_of_event_numbers[rand() % bkg_list_of_event_numbers.size()];
        std::vector<group> bkg_event_groups = bkg_event_mapping[bkg_event_number];
        while (int(bkg_event_groups.size()) < 3000){
            std::cout << "WARNING: For group " << i << " Background event " << bkg_event_number << " has " << bkg_event_groups.size() << " groups" << std::endl;
            test_appo_vec.push_back(bkg_event_number);
            int bkg_event_number = bkg_list_of_event_numbers[rand() % bkg_list_of_event_numbers.size()];
            bkg_event_groups = bkg_event_mapping[bkg_event_number];
        }
 
        // superimpose the two events by connecting the groups        
        std::vector<group> superimposed_gp = sig_event_groups;
        superimposed_gp.insert(superimposed_gp.end(), bkg_event_groups.begin(), bkg_event_groups.end());
        // filter the superimposed groups within a radius
        // std::cout << "Index: " << i << " Superimposed group size: " << superimposed_gp.size() << std::endl;
        group filtered_superimposed_group = filter_groups_within_radius(superimposed_gp, radius);
        if (filtered_superimposed_group.get_min_distance_from_true_pos()>10){
            std::cout<<"WARNING: Min distance from true pos: "<< filtered_superimposed_group.get_min_distance_from_true_pos() << std::endl;
            for (auto g: sig_event_groups){
                std::cout<<"Sig group min distance from true pos: "<< g.get_min_distance_from_true_pos() << std::endl;
                for (auto tp: g.get_tps()){
                    for (auto var: tp){
                        std::cout<<var<<" ";
                    }
                    std::cout<<calculate_position(tp)[0] << " " << calculate_position(tp)[1] << " " << calculate_position(tp)[2] << std::endl;
                }
            }
            continue;
        }
        // if (filtered_superimposed_group.get_supernova_tp_fraction() >0.9){
        //     test_appo_vec.push_back(bkg_event_number);
        //     // print the group
        //     std::cout<<"Filtered superimposed group size: "<< filtered_superimposed_group.get_tps().size() << std::endl;
        //     for (auto const& tp : filtered_superimposed_group.get_tps()){
        //         std::cout<<"TP: ";
        //         for (auto const& var : tp){
        //             std::cout<<var<<" ";
        //         }
        //         std::cout<<std::endl;
        //     }
        //     std::cout<<"Sig pos: " << filtered_superimposed_group.get_reco_pos()[0] << " " << filtered_superimposed_group.get_reco_pos()[1] << " " << filtered_superimposed_group.get_reco_pos()[2] << std::endl;
        //     std::vector<int> reco_pos = {0, 0, 0};
        //     for (auto tp: filtered_superimposed_group.get_tps()){
        //         std::cout<<"TP: "<< tp[0] << " " << tp[1] << " " << tp[2] << " " << tp[3] << " " << calculate_position(tp)[0] << " " << calculate_position(tp)[1] << " " << calculate_position(tp)[2] << std::endl;
        //         reco_pos[0] += calculate_position(tp)[0];
        //         reco_pos[1] += calculate_position(tp)[1];
        //         reco_pos[2] += calculate_position(tp)[2];
        //     }
        //     reco_pos[0] /= int(filtered_superimposed_group.get_tps().size());            
        //     reco_pos[1] /= int(filtered_superimposed_group.get_tps().size());       
        //     reco_pos[2] /= int(filtered_superimposed_group.get_tps().size());
        //     std::cout<<"Reco pos: " << reco_pos[0] << " " << reco_pos[1] << " " << reco_pos[2] << std::endl;

        //     // break;
        //     }



        superimposed_groups.push_back(filtered_superimposed_group);
    }
    
    // print unique bkg event numbers with counts
    std::map<int, int> bkg_event_counts;
    for (auto const& x : test_appo_vec){
        bkg_event_counts[x]++;
    }
    for (auto const& x : bkg_event_counts){
        std::cout<<"Bkg event number: "<< x.first << " Count: " << x.second << " N events: " << bkg_event_mapping[x.first].size() << std::endl;
    }


    // write the superimposed groups to a root file
    std::cout<<"Writing "<< superimposed_groups.size() <<" events to root" << std::endl;
    std::string sig_name = sig_group_filename.substr(sig_group_filename.find_last_of("/")+1);
    std::string bkg_name = bkg_group_filename.substr(bkg_group_filename.find_last_of("/")+1);
    std::string superimposed_group_filename = outfolder + "/superimposed_radius_" + std::to_string(radius) + ".root";
    write_groups_to_root(superimposed_groups, superimposed_group_filename);
    // stop the clock
    std::clock_t end;
    double elapsed_time = double(end - start) / CLOCKS_PER_SEC;
    std::cout << "Elapsed time: " << elapsed_time << " seconds" << std::endl;

    return 0;
}

