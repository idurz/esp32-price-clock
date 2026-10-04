#include "appliance_profiles.h"

namespace Appliance {

const Profile kWasmachine{
    "Wasmachine",
    {
        {0, 1.35f},
        {10, 0.15f},
        {195, 0.45f},
        {205, 0.15f},
        {265, 1.20f},
    },
    280,
};

const Profile kDroger{
    "Droger",
    {
        {0, 0.05f},
        {5, 1.10f},
        {35, 0.05f},
    },
    40,
};

} // namespace Appliance
