#pragma once
#ifndef LI_DecayRangeSIREN_H
#define LI_DecayRangeSIREN_H

#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>                                  // for uint32_t
#include <stdexcept>                                // for runtime_error

#include <cereal/cereal.hpp>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/utility.hpp>

#include "SIREN/interactions/InteractionCollection.h"
#include "SIREN/interactions/CrossSection.h"
#include "SIREN/interactions/Decay.h"
#include "SIREN/detector/DetectorModel.h"
#include "SIREN/distributions/primary/vertex/DecayRangeFunction.h"
#include "SIREN/distributions/primary/vertex/DecayRangePositionDistribution.h"
#include "SIREN/injection/Injector.h"  // for Injector

namespace siren { namespace dataclasses { class InteractionRecord; } }
namespace siren { namespace injection { class PrimaryInjectionProcess; } }
namespace siren { namespace math { class Vector3D; } }
namespace siren { namespace utilities { class LI_random; } }

namespace siren {
namespace injection {

class DecayRangeSIREN : public Injector {
friend cereal::access;
protected:
    std::shared_ptr<siren::distributions::DecayRangeFunction> range_func;
    double disk_radius;
    double endcap_length;
    std::shared_ptr<siren::distributions::DecayRangePositionDistribution> position_distribution;
    std::shared_ptr<siren::interactions::InteractionCollection> interactions;
    DecayRangeSIREN();
public:
    DecayRangeSIREN(unsigned int events_to_inject, std::shared_ptr<siren::detector::DetectorModel> detector_model, std::shared_ptr<injection::PrimaryInjectionProcess> primary_process, std::vector<std::shared_ptr<injection::SecondaryInjectionProcess>> secondary_processes, std::shared_ptr<siren::utilities::LI_random> random, std::shared_ptr<siren::distributions::DecayRangeFunction> range_func, double disk_radius, double endcap_length);
    std::string Name() const override;
    virtual std::tuple<siren::math::Vector3D, siren::math::Vector3D> PrimaryInjectionBounds(siren::dataclasses::InteractionRecord const & interaction) const override;
    template<typename Archive>
    void save(Archive & archive, std::uint32_t const version) const {
        if(version == 0) {
            archive(::cereal::make_nvp("RangeFunction", range_func));
            archive(::cereal::make_nvp("DiskRadius", disk_radius));
            archive(::cereal::make_nvp("EndcapLength", endcap_length));
            archive(::cereal::make_nvp("PositionDistribution", position_distribution));
            archive(cereal::virtual_base_class<Injector>(this));
        } else {
            throw std::runtime_error("DecayRangeSIREN only supports version <= 0!");
        }
    }

    template<typename Archive>
    void load(Archive & archive, std::uint32_t const version) {
        if(version == 0) {
            archive(::cereal::make_nvp("RangeFunction", range_func));
            archive(::cereal::make_nvp("DiskRadius", disk_radius));
            archive(::cereal::make_nvp("EndcapLength", endcap_length));
            archive(::cereal::make_nvp("PositionDistribution", position_distribution));
            archive(cereal::virtual_base_class<Injector>(this));
        } else {
            throw std::runtime_error("DecayRangeSIREN only supports version <= 0!");
        }
    }
};

} // namespace injection
} // namespace siren

CEREAL_CLASS_VERSION(siren::injection::DecayRangeSIREN, 0);
CEREAL_REGISTER_TYPE(siren::injection::DecayRangeSIREN);
CEREAL_REGISTER_POLYMORPHIC_RELATION(siren::injection::Injector, siren::injection::DecayRangeSIREN);

#endif // LI_DecayRangeSIREN_H
