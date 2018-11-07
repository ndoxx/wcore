#include <catch2/catch.hpp>

#include "config.h"

TEST_CASE("A uint32_t is set then retrieved.", "[cfg]")
{
    uint32_t myVar = 5;
    CONFIG.set(H_("myVar"), myVar);

    uint32_t set_me = 0;

    bool success = CONFIG.get(H_("myVar"), set_me);

    REQUIRE(success);
    REQUIRE(set_me == 5);
}

TEST_CASE("An int32_t is set then retrieved.", "[cfg]")
{
    int32_t myVar = 42;
    CONFIG.set(H_("myVar"), myVar);

    int32_t set_me = 0;

    bool success = CONFIG.get(H_("myVar"), set_me);

    REQUIRE(success);
    REQUIRE(set_me == 42);
}

TEST_CASE("A float is set then retrieved.", "[cfg]")
{
    float myVar = 42.72f;
    CONFIG.set(H_("myVar"), myVar);

    float set_me = 0.0f;

    bool success = CONFIG.get(H_("myVar"), set_me);

    REQUIRE(success);
    REQUIRE(set_me == 42.72f);
}

TEST_CASE("A bool is set then retrieved.", "[cfg]")
{
    bool myVar = true;
    CONFIG.set(H_("myVar"), myVar);

    bool set_me = false;

    bool success = CONFIG.get(H_("myVar"), set_me);

    REQUIRE(success);
    REQUIRE(set_me);
}

TEST_CASE("A bool is set true then tested using is().", "[cfg]")
{
    bool myVar = true;
    CONFIG.set(H_("myVar"), myVar);

    REQUIRE(CONFIG.is(H_("myVar")));
}

TEST_CASE("A bool is set false then tested using is().", "[cfg]")
{
    bool myVar = false;
    CONFIG.set(H_("myVar"), myVar);

    REQUIRE_FALSE(CONFIG.is(H_("myVar")));
}

TEST_CASE("A non existing flag is tested using is().", "[cfg]")
{
    REQUIRE_FALSE(CONFIG.is(H_("myVar")));
}

TEST_CASE("A string is set as char* then retrieved.", "[cfg]")
{
    CONFIG.set(H_("myVar"), "plip plop");

    std::string set_me;

    bool success = CONFIG.get(H_("myVar"), set_me);

    REQUIRE(success);
    REQUIRE(!set_me.compare("plip plop"));
}

TEST_CASE("An xml file is parsed to retrieve some properties.", "[cfg]")
{
    CONFIG.load_file_xml("../source/tests/test_config.xml");
    CONFIG.debug_display_content();

    uint32_t universe_dimension = 0;
    float jet_velocity = 0.0f;
    uint32_t nqs = 0;
    CONFIG.get(H_("root.GateControlSystem.Subfluxxer.VectorFluxSubModule.numQuantumStrings"), nqs);
    CONFIG.get(H_("root.GayRadar.SubfluxxerInjectorModule.universeDimension"), universe_dimension);
    CONFIG.get(H_("root.GayRadar.SubfluxxerInjectorModule.jetVelocity"), jet_velocity);

    REQUIRE(nqs == 42);
    REQUIRE(jet_velocity == 412.58f);
    REQUIRE(universe_dimension == 26);
}
