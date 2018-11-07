#include "logger.h"
#include "config.h"

int main(int argc, char const *argv[])
{
    CONFIG.load_file_xml("../source/tests/test_config.xml");

    CONFIG.debug_display_content();

    std::cout << std::endl << std::endl;

    uint32_t universe_dimension = 0;
    float jet_velocity = 0.0f;
    uint32_t nqs = 0;
    CONFIG.get(H_("root.GateControlSystem.Subfluxxer.VectorFluxSubModule.numQuantumStrings"), nqs);
    CONFIG.get(H_("root.GayRadar.SubfluxxerInjectorModule.universeDimension"), universe_dimension);
    CONFIG.get(H_("root.GayRadar.SubfluxxerInjectorModule.jetVelocity"), jet_velocity);

    std::cout << "#quantum strings: " << nqs << std::endl;
    std::cout << "universe dimension: " << universe_dimension << std::endl;
    std::cout << "jet velocity: " << jet_velocity << std::endl;

    return 0;
}
