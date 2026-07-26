#ifndef TRUEPARTICLE_H
#define TRUEPARTICLE_H

#include <Neutrino.h>
#include "global.h"

class TrueParticle {
    
    public:
        // constructor
        TrueParticle(
            UInt_t event,
            UInt_t run, 
            Double_t x,
            Double_t y,
            Double_t z,
            Double_t Px,
            Double_t Py,
            Double_t Pz,
            Double_t energy,
            std::string generator_name,
            int pdg,
            std::string process,
            int track_id = -1,
            int truth_id = -1
        ) :
            event(event),
            run(run),
            x(x),
            y(y),
            z(z),
            Px(Px),
            Py(Py),
            Pz(Pz),
            energy(energy),
            generator_name(generator_name),
            pdg(pdg),
            process(process),
            track_id(track_id),
            truth_id(truth_id)
        {};

        // default constructor
        TrueParticle() : event(-1), run(-1), x(0), y(0), z(0), Px(0), Py(0), Pz(0), energy(0), generator_name(""), pdg(-1), process(""),
            track_id(-1), truth_id(-1) {};

        // special constructor for when reading the files
        TrueParticle( 
            UInt_t event, 
            UInt_t run,
            std::string generator_name, 
            int block_id
        ) : 
            event(event),
            run(run),
            generator_name(generator_name),
            truth_id(block_id)
        {
            // set the rest to 0
            x = 0;
            y = 0;
            z = 0;
            Px = 0;
            Py = 0;
            Pz = 0;
            energy = 0;
            process = "";
        };
        
        // destructor
        ~TrueParticle() {};

        // Setters
        void SetEvent(UInt_t event) { this->event = event; }
        void SetRun(UInt_t run) { this->run = run; }
        void SetX(Double_t x) { this->x = x; }
        void SetY(Double_t y) { this->y = y; }
        void SetZ(Double_t z) { this->z = z; }
        void SetPx(Double_t Px) { this->Px = Px; }
        void SetPy(Double_t Py) { this->Py = Py; }
        void SetPz(Double_t Pz) { this->Pz = Pz; }
        void SetEnergy(Double_t energy) { this->energy = energy; }
        void SetGeneratorName(std::string generator_name) { this->generator_name = generator_name; }
        void SetPdg(int pdg) { this->pdg = pdg; }
        void SetProcess(std::string process) { this->process = process; }
        void SetTrackId(int track_id) { this->track_id = track_id; }
        void SetTruthId(int truth_id) { this->truth_id = truth_id; }
        void SetNeutrino(const Neutrino *neutrino) { this->neutrino = neutrino; }
        void SetTimeStart(double time_start) { this->time_start = time_start; }
        void SetTimeEnd(double time_end) { this->time_end = time_end; }

        // Getters
        int GetEvent() const { return event; }
        int GetRun() const { return run; }
        std::vector <Double_t> GetPosition() const { return {x, y, z}; }
        std::vector <Double_t> GetMomentum() const { return {Px, Py, Pz}; }
        Double_t GetX() const { return x; }
        Double_t GetY() const { return y; }
        Double_t GetZ() const { return z; }
        Double_t GetPx() const { return Px; }
        Double_t GetPy() const { return Py; }
        Double_t GetPz() const { return Pz; }
        Double_t GetEnergy() const { return energy; }
        std::string GetGeneratorName() const { return generator_name; }
        int GetPdg() const { return pdg; }
        std::string GetProcess() const { return process; }
        int GetTrackId() const { return track_id; }
        int GetTruthId() const { return truth_id; }
        const Neutrino* GetNeutrino() const {
            if (neutrino == nullptr) {
                // LogWarning << "No neutrino associated to this TrueParticle" << std::endl;
            }
            return neutrino;
        }
        double GetTimeStart() const { return time_start; }
        double GetTimeEnd() const { return time_end; }
        std::unordered_set<int> GetChannels() const { return channels; }

        void AddChannel(int channel){
            channels.insert(channel);
        };

        void Print() const {
            LogInfo << "TrueParticle: " << std::endl;
            LogInfo << "Event: " << event << std::endl;
            LogInfo << "Run: " << run << std::endl;
            LogInfo << "X: " << x << std::endl;
            LogInfo << "Y: " << y << std::endl;
            LogInfo << "Z: " << z << std::endl;
            LogInfo << "Px: " << Px << std::endl;
            LogInfo << "Py: " << Py << std::endl;
            LogInfo << "Pz: " << Pz << std::endl;
            LogInfo << "Energy: " << energy << std::endl;
            LogInfo << "Generator name: " << generator_name << std::endl;
            LogInfo << "Pdg: " << pdg << std::endl;
            LogInfo << "Process: " << process << std::endl;
        };

    private:
        UInt_t event {0};
        UInt_t run {0};
        float x {0.0f};
        float y {0.0f};
        float z {0.0f};
        float Px {0.0f};
        float Py {0.0f};
        float Pz {0.0f};
        float energy {0.0f};
        std::string generator_name {"UNKNOWN"};
        int pdg {0};
        std::string process {"UNKNOWN"};
        
        int track_id {-1}; // needed for association
        int truth_id {-1}; // needed for association
        const Neutrino * neutrino = nullptr; // this is the neutrino that generated the particle, if any. For bkg, it will stay empty

        // these are needed to make associations
        double time_start {INT_MAX};
        double time_end {0};
        std::unordered_set<int> channels = {};

};

#endif // TRUEPARTICLE_H
