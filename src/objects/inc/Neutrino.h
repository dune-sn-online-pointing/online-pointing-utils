#ifndef NEUTRINO_H
#define NEUTRINO_H

#include <iostream>
#include <vector>
#include <string>

// the idea is  to associate a neutrino to each TrueParticle 
// having generator_name == "marley"

class Neutrino {
    
    public:

        int event; 
        std::string interaction;
        float x;
        float y;
        float z;
        float Px;
        float Py;
        float Pz;
        int energy;

        int truth_id {-1}; // needed for association
        // could put generator_name, but not needed for the current setup

        Neutrino(
            int event, 
            std::string interaction,
            float x,
            float y,
            float z,
            float Px,
            float Py,
            float Pz,
            float energy,
            int truth_id
        ) :
            event(event),
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
        int GetEvent() const { return event; }
        std::string GetInteraction() const { return interaction; }
        std::vector <float> GetPosition() const { return {x, y, z}; }
        std::vector <float> GetMomentum() const { return {Px, Py, Pz}; }
        float GetX() const { return x; }
        float GetY() const { return y; }
        float GetZ() const { return z; }
        float GetPx() const { return Px; }
        float GetPy() const { return Py; }
        float GetPz() const { return Pz; }
        int GetEnergy() const { return energy; }
        int GetTruthId() const { return truth_id; }

        // Setters
        void SetEvent(int e) { event = e; }
        void SetInteraction(const std::string& i) { interaction = i; }
        void SetX(float val) { x = val; }
        void SetY(float val) { y = val; }
        void SetZ(float val) { z = val; }
        void SetPx(float val) { Px = val; }
        void SetPy(float val) { Py = val; }
        void SetPz(float val) { Pz = val; }
        void SetEnergy(int e) { energy = e; }
        void SetTruthId(int id) { truth_id = id; }

};


#endif // NEUTRINO_H