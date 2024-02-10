#pragma once
#ifndef LI_Particle_H
#define LI_Particle_H

// Used to define the Particle class
// Partiles have a type, energy, position, and direction

#include <set>
#include <string>
#include <utility>
#include <stdint.h>

#include "LeptonInjector/dataclasses/ParticleID.h"
#include "LeptonInjector/dataclasses/ParticleType.h"

#include <cereal/cereal.hpp>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/details/helpers.hpp>

#include "LeptonInjector/serialization/array.h"

namespace LI { namespace dataclasses { class Particle; } }

std::ostream & operator<<(std::ostream & os, LI::dataclasses::Particle const & p);

namespace LI {
namespace dataclasses {

// simple data structure for particles
class Particle {
public:
    typedef LI::dataclasses::ParticleType ParticleType;

    Particle() = default;
    Particle(Particle const & other) = default;
    Particle(ParticleID id, ParticleType type, double mass, std::array<double, 4> momentum, std::array<double, 3> position, double length, double helicity);
    Particle(ParticleType type, double mass, std::array<double, 4> momentum, std::array<double, 3> position, double length, double helicity);

    ParticleID id;
    ParticleType type = ParticleType::unknown;
    double mass = 0;
    std::array<double, 4> momentum = {0, 0, 0, 0};
    std::array<double, 3> position = {0, 0, 0};
    double length = 0;
    double helicity = 0;

    ParticleID & GenerateID();

    friend std::ostream & ::operator<<(std::ostream & os, LI::dataclasses::Particle const & p);

    template<typename Archive>
    void serialize(Archive & archive, std::uint32_t const version) {
        if(version == 0) {
            archive(::cereal::make_nvp("ID", id));
            archive(::cereal::make_nvp("Type", type));
            archive(::cereal::make_nvp("Mass", mass));
            archive(::cereal::make_nvp("Momentum", momentum));
            archive(::cereal::make_nvp("Position", position));
            archive(::cereal::make_nvp("Length", length));
            archive(::cereal::make_nvp("helicity", helicity));
        } else {
            throw std::runtime_error("Particle only supports version <= 0!");
        }
    }
};

// prototype some of the particle helper functions

bool isLepton(Particle::ParticleType p);
bool isCharged(Particle::ParticleType p);
bool isNeutrino(Particle::ParticleType p);

} // namespace dataclasses
} // namespace LI

CEREAL_CLASS_VERSION(LI::dataclasses::Particle, 0);

#endif // LI_Particle_H
