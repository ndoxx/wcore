#include <catch2/catch.hpp>
#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "wcomponent.h"
#include "wentity.h"

// Define some dummy components
class WCfoo : public WComponent
{
public:
    WCfoo(){}
    WCfoo(float bar, const char* baz): bar_(bar), baz_(baz){}

    int get_age_of_the_captain() { return 42; }

    float bar_;
    std::string baz_;
};
REGISTER_COMPONENT(WCfoo, std::type_index(typeid(WCfoo)))

class WCbar : public WComponent
{
public:
    WCbar(){}
    WCbar(int baz, int qux): baz_(baz), qux_(qux){}

    int get_age_of_the_captain() { return 45; }

    int baz_;
    int qux_;
};
REGISTER_COMPONENT(WCbar, std::type_index(typeid(WCbar)))

TEST_CASE("A component is initialized.", "[cmp]")
{
    WCfoo foo(0.5322f, "plop");

    REQUIRE(foo.bar_ == 0.5322f);
    REQUIRE(foo.baz_ == "plop");
}

TEST_CASE("An entity is created and adds a component.", "[cmp]")
{
    WEntity entity;
    auto foo = entity.add_component<WCfoo>();
    foo->bar_ = 0.5322f;
    foo->baz_ = "plop";

    REQUIRE(entity.get_component<WCfoo>()->bar_ == 0.5322f);
    REQUIRE(entity.get_component<WCfoo>()->baz_ == "plop");
    REQUIRE(entity.get_component<WCfoo>()->get_age_of_the_captain() == 42);
}

TEST_CASE("An entity is created and adds two components.", "[cmp]")
{
    WEntity entity;
    auto foo = entity.add_component<WCfoo>();
    foo->bar_ = 0.5322f;
    foo->baz_ = "plop";

    auto bar = entity.add_component<WCbar>();
    bar->baz_ = 2;
    bar->qux_ = -1;

    REQUIRE(entity.get_component<WCfoo>()->bar_ == 0.5322f);
    REQUIRE(entity.get_component<WCfoo>()->baz_ == "plop");
    REQUIRE(entity.get_component<WCfoo>()->get_age_of_the_captain() == 42);

    REQUIRE(entity.get_component<WCbar>()->baz_ == 2);
    REQUIRE(entity.get_component<WCbar>()->qux_ == -1);
    REQUIRE(entity.get_component<WCbar>()->get_age_of_the_captain() == 45);

}

TEST_CASE("An entity creates same component twice.", "[cmp]")
{
    WEntity entity;
    auto foo = entity.add_component<WCfoo>();
    foo->bar_ = 0.5322f;

    foo = entity.add_component<WCfoo>();
    foo->baz_ = "plop";

    REQUIRE(entity.get_component<WCfoo>()->bar_ == 0.5322f);
    REQUIRE(entity.get_component<WCfoo>()->baz_ == "plop");
}

TEST_CASE("Getting a component that an entity has not returns nullptr.", "[cmp]")
{
    WEntity entity;
    entity.add_component<WCfoo>();

    //REQUIRE_THROWS_AS(entity.get_component<WCbar>(), std::out_of_range);
    REQUIRE(entity.get_component<WCbar>() == nullptr);
}

TEST_CASE("Checking if an entity has a given component.", "[cmp]")
{
    WEntity entity;
    entity.add_component<WCfoo>();

    REQUIRE(entity.has_component<WCfoo>());
    REQUIRE_FALSE(entity.has_component<WCbar>());
}
