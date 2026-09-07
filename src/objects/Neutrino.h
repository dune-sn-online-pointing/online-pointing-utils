#ifndef NEUTRINO_H
#define NEUTRINO_H

#include "root.h"

// the idea is  to associate a neutrino to each TrueParticle 
// having generator_name == "marley"

class Neutrino {
    
    public:

        UInt_t event; 
        UInt_t run;
        std::string interaction;
        Double_t x;
        Double_t y;
        Double_t z;
        Double_t Px;
        Double_t Py;
        Double_t Pz;
        int energy;

        int truth_id {-1}; // needed for association
        // could put generator_name, but not needed for the current setup

        Neutrino(
            UInt_t event, 
            UInt_t run,
            std::string interaction,
            Double_t x,
            Double_t y,
            Double_t z,
            Double_t Px,
            Double_t Py,
            Double_t Pz,
            Double_t energy,
            int truth_id
        ) :
            event(event),
            run(run),
            interaction(interaction),
            x(x),
            y(y),
            z(z),
            Px(Px),
            Py(Py),
            Pz(Pz),
            energy(energy),
            truth_id(truth_id)
        {}

        Neutrino() :
            event(-1),
            run(-1),
            interaction(""),
            x(0),
            y(0),
            z(0),
            Px(0),
            Py(0),
            Pz(0),
            energy(0),
            truth_id(-1)
        {}  

        // Getters
        UInt_t GetEvent() const { return event; }
        UInt_t GetRun() const { return run; }
        std::string GetInteraction() const { return interaction; }
        std::vector <Double_t> GetPosition() const { return {x, y, z}; }
        std::vector <Double_t> GetMomentum() const { return {Px, Py, Pz}; }
        Double_t GetX() const { return x; }
        Double_t GetY() const { return y; }
        Double_t GetZ() const { return z; }
        Double_t GetPx() const { return Px; }
        Double_t GetPy() const { return Py; }
        Double_t GetPz() const { return Pz; }
        double_t GetEnergy() const { 
            //return energy; 
            return std::sqrt(Px*Px + Py*Py + Pz*Pz); // compute energy from momentum
        }
        int GetTruthId() const { return truth_id; }

        // Setters
        void SetEvent(UInt_t e) { event = e; }
        void SetRun(UInt_t r) { event = r; }
        void SetInteraction(const std::string& i) { interaction = i; }
        void SetX(Double_t val) { x = val; }
        void SetY(Double_t val) { y = val; }
        void SetZ(Double_t val) { z = val; }
        void SetPx(Double_t val) { Px = val; }
        void SetPy(Double_t val) { Py = val; }
        void SetPz(Double_t val) { Pz = val; }
        void SetEnergy(int e) { energy = e; }
        void SetTruthId(int id) { truth_id = id; }

};


#endif // NEUTRINO_H