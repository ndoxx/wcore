#include <catch2/catch.hpp>
#include <iostream>
#include <vector>
#include <string>

#define __DEBUG__
#include "informer.h"
#include "listener.h"
#include "logger.h"

class MessageIntValue: public WData
{
public:
    explicit MessageIntValue(int val): new_value(val){}
    virtual std::string to_string() const
    {
        return std::string("[MessageIntValue] new_value=") + std::to_string(new_value);
    }
    int new_value;
};

class MessageStrs: public WData
{
public:
    explicit MessageStrs(const std::string& name,
                         const std::string& desc): newname(name), newdesc(desc){}
    virtual std::string to_string() const
    {
        return std::string("[MessageStrs] newname='" + newname + "' newdesc='" + newdesc + "'");
    }
    std::string newname;
    std::string newdesc;
};

class InformerA: public Informer
{
public:
    void sendValueUpdate(int value)
    {
        post(H_("updateVal"), MessageIntValue(value));
    }
    void sendNameDescUpdate(const std::string& name, const std::string& desc)
    {
        post(H_("updateStrs"), MessageStrs(name, desc));
    }
};

class ListenerA: public Listener
{
public:
    ListenerA(): value_(0), name_("default"), description_("Default description."){};

    void onUpdateVal(const WData& data)
    {
        const MessageIntValue& MIV = static_cast<const MessageIntValue&>(data);
        value_ = MIV.new_value;
    }

    void onUpdateNameDesc(const WData& data)
    {
        const MessageStrs& MS = static_cast<const MessageStrs&>(data);
        name_ = MS.newname;
        description_ = MS.newdesc;
    }

    inline int get_value() const {return value_;}
    inline const std::string& get_name() const {return name_;}
    inline const std::string& get_desc() const {return description_;}

private:
    int value_;
    std::string name_;
    std::string description_;
};

TEST_CASE("Debug log functions.", "[dbg]")
{
    using namespace dbg;

    std::cout << "Testing debug logging." << std::endl;
    LOG.print_reference();
    std::cout << "Done." << std::endl;

    SUCCEED();
}

class OneToOneMessagingFixture
{
public:
    OneToOneMessagingFixture(){}

protected:
    InformerA infA;
    ListenerA lisA;
};

TEST_CASE_METHOD(OneToOneMessagingFixture, "Listener subscribes to informer.", "[mes]")
{
    lisA.subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);

    auto delegates = infA.get_delegates();
    auto firstDelegate = delegates.begin();

    REQUIRE(firstDelegate->first == H_("updateVal"));
}

TEST_CASE_METHOD(OneToOneMessagingFixture, "Listener subscribes to informer twice.", "[mes]")
{
    lisA.subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);
    lisA.subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);

    auto delegates = infA.get_delegates();

    REQUIRE(delegates.size() == 1);
}

TEST_CASE_METHOD(OneToOneMessagingFixture, "Informer sends message to listener.", "[mes]")
{
    lisA.subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);

    infA.sendValueUpdate(42);

    REQUIRE(lisA.get_value() == 42);
}

TEST_CASE_METHOD(OneToOneMessagingFixture, "Informer sends message to listener then unsubs.", "[mes]")
{
    lisA.subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);

    infA.sendValueUpdate(42);
    lisA.unsubscribe(H_("updateVal"), infA);
    infA.sendValueUpdate(-21);

    REQUIRE(lisA.get_value() == 42);
}


class OneToManyMessagingFixture
{
public:
    OneToManyMessagingFixture()
    {
        for(unsigned ii = 0; ii < 5; ++ii) {
            listeners.push_back(new ListenerA);
        }
    }

    ~OneToManyMessagingFixture()
    {
        for(auto lp: listeners) {
            delete lp;
        }
    }

protected:
    InformerA infA;
    std::vector<Listener*> listeners;
};

TEST_CASE_METHOD(OneToManyMessagingFixture, "Informer sends to many listeners, not all of whom have subscribed.", "[mes]")
{
    // subscribe listeners 1 2 3 4 to updateVal
    // subscribe listeners 1 3 to updateVal
    for(unsigned ii = 0; ii < listeners.size(); ++ii) {
        if(ii>0)
            listeners[ii]->subscribe(H_("updateVal"), infA, &ListenerA::onUpdateVal);
        if(ii%2)
            listeners[ii]->subscribe(H_("updateStrs"), infA, &ListenerA::onUpdateNameDesc);
    }
    dbg::LOG.track(H_("updateVal"), infA);
    dbg::LOG.track(H_("updateStrs"), infA);

    infA.sendValueUpdate(45);
    infA.sendNameDescUpdate("bob", "This is just bob.");

    dbg::LOG.untrack(infA);

    for(unsigned ii = 0; ii < listeners.size(); ++ii) {
        ListenerA* laptr = dynamic_cast<ListenerA*>(listeners[ii]);
        if(ii>0)
            REQUIRE(laptr->get_value() == 45);
        else
            REQUIRE(laptr->get_value() == 0);
        if(ii%2)
        {
            REQUIRE(laptr->get_name() == "bob");
            REQUIRE(laptr->get_desc() == "This is just bob.");
        }
        else
        {
            REQUIRE(laptr->get_name() == "default");
            REQUIRE(laptr->get_desc() == "Default description.");
        }
    }
}
