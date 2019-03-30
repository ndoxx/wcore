#include <map>
#include <catch2/catch.hpp>

#include "transaction.h"

// Dummy class for transaction testing
class Solid
{
public:
    Solid(float position,
          float velocity=0.f,
          float mass=1.f):
    position(position),
    velocity(velocity),
    mass(mass)
    {

    }

    float position;
    float velocity;
    float mass;
};

// Store the old state and the new state
// execute() and unexecute() just switch state
class MementoSolidMove
{
public:
    MementoSolidMove(Solid& solid, float newpos):
    solid_(solid),
    oldpos_(solid_.position),
    newpos_(newpos)
    {

    }

    void execute()
    {
        solid_.position = newpos_;
    }

    void unexecute()
    {
        solid_.position = oldpos_;
    }

private:
    Solid& solid_;
    float oldpos_;
    float newpos_;
};

// Store a modifier, execute() and unexecute() perform
// opposite operations on solid using stored modifier
class CommandSolidTranslate
{
public:
    explicit CommandSolidTranslate(Solid& solid, float value):
    solid_(solid),
    value_(value)
    {

    }

    void execute()
    {
        solid_.position -= value_;
    }

    void unexecute()
    {
        solid_.position += value_;
    }

private:
    Solid& solid_;
    float value_;
};

// Uses a transaction store of max depth = 10 to manage
// the state of a single Solid object
class TransactionFixture
{
public:
    TransactionFixture():
    transactions_(10),
    solid_(1.f)
    {

    }

    void move_solid(float newpos)
    {
        transactions_.push(MementoSolidMove(solid_, newpos));
    }

    void translate_solid(float value)
    {
        transactions_.push(CommandSolidTranslate(solid_, value));
    }

    inline void undo()   { transactions_.undo(); }
    inline void redo()   { transactions_.redo(); }
    inline void revert() { transactions_.revert(); }

protected:
    wevel::TransactionStore transactions_;
    Solid solid_;
};


TEST_CASE_METHOD(TransactionFixture, "Action is performed then undone (memento)", "[transac]")
{
    move_solid(2.f);

    undo(); // should revert back to initial state

    REQUIRE(solid_.position == 1.f);
}

TEST_CASE_METHOD(TransactionFixture, "Action is undone then redone (memento)", "[transac]")
{
    move_solid(2.f);
    undo();

    redo(); // should undo the undo()

    REQUIRE(solid_.position == 2.f);
}

TEST_CASE_METHOD(TransactionFixture, "Three actions performed, two undo calls made (memento)", "[transac]")
{
    move_solid(2.f);
    move_solid(3.f);
    move_solid(5.f);
    undo();

    undo(); // should revert back to second state

    REQUIRE(solid_.position == 2.f);
}

TEST_CASE_METHOD(TransactionFixture, "Action is undone, another action performed, then redo (memento)", "[transac]")
{
    move_solid(2.f);
    undo();
    move_solid(3.f);

    redo(); // shouldn't do shit

    REQUIRE(solid_.position == 3.f);
}

TEST_CASE_METHOD(TransactionFixture, "Action performed 5 times then revert (memento)", "[transac]")
{
    for(int ii=0; ii<5; ++ii)
        move_solid(float(ii+1));

    revert(); // should restore initial state

    REQUIRE(solid_.position == 1.f);
}

TEST_CASE_METHOD(TransactionFixture, "Action performed 20 times then revert, max depth=10 (memento)", "[transac]")
{
    for(int ii=0; ii<20; ++ii)
        move_solid(float(ii+1));

    revert(); // should roll back only 10 times, where position is 10.f

    REQUIRE(solid_.position == 10.f);
}

TEST_CASE_METHOD(TransactionFixture, "Action is performed then undone (command)", "[transac]")
{
    translate_solid(2.f);

    undo(); // should revert back to initial state

    REQUIRE(solid_.position == 1.f);
}




class CommandSolidAdd
{
public:
    explicit CommandSolidAdd(std::shared_ptr<Solid> psolid,
                             int index,
                             std::map<int, std::shared_ptr<Solid>>& container):
    psolid_(psolid),
    container_(container),
    index_(index)
    {

    }

    void execute()
    {
        container_.insert(std::pair(index_, psolid_));
    }

    void unexecute()
    {
        container_.erase(index_);
    }

private:
    std::shared_ptr<Solid> psolid_;
    std::map<int, std::shared_ptr<Solid>>& container_;
    int index_;
};

class CommandSolidRemove
{
public:
    explicit CommandSolidRemove(int index,
                                std::map<int, std::shared_ptr<Solid>>& container):
    psolid_(container.at(index)),
    container_(container),
    index_(index)
    {

    }

    void execute()
    {
        container_.erase(index_);
    }

    void unexecute()
    {
        container_.insert(std::pair(index_, psolid_));
    }

private:
    std::shared_ptr<Solid> psolid_;
    std::map<int, std::shared_ptr<Solid>>& container_;
    int index_;
};

// Uses a transaction store of max depth = 10 to manage
// the state of a collection of Solid objects
class OwningTransactionFixture
{
public:
    typedef std::shared_ptr<Solid> solid_ptr;

    OwningTransactionFixture():
    transactions_(10)
    {

    }

    void add_solid(solid_ptr psolid, int index)
    {
        transactions_.push(CommandSolidAdd(psolid, index, solids_));
    }

    void remove_solid(int index)
    {
        transactions_.push(CommandSolidRemove(index, solids_));
    }

    inline void undo()   { transactions_.undo(); }
    inline void redo()   { transactions_.redo(); }
    inline void revert() { transactions_.revert(); }

protected:
    wevel::TransactionStore transactions_;
    std::map<int, solid_ptr> solids_;
};

TEST_CASE_METHOD(OwningTransactionFixture, "Object created then undo performed", "[transac]")
{
    int index = 42;
    add_solid(std::shared_ptr<Solid>(new Solid(1.f)), index);

    REQUIRE(solids_.size() == 1);
    REQUIRE(solids_[index]->position == 1.f);

    undo(); // should clear the map

    REQUIRE(solids_.size() == 0);
}

TEST_CASE_METHOD(OwningTransactionFixture, "Object created, undo performed, then redo performed", "[transac]")
{
    int index = 42;
    add_solid(std::shared_ptr<Solid>(new Solid(1.5f)), index);
    undo();

    redo(); // should recover solid in map

    REQUIRE(solids_.size() == 1);
    REQUIRE(solids_[index]->position == 1.5f);
}

TEST_CASE_METHOD(OwningTransactionFixture, "Object created then removed, then undo called", "[transac]")
{
    int index = 42;
    add_solid(std::shared_ptr<Solid>(new Solid(2.f)), index);
    remove_solid(index);

    undo(); // should recover solid in map

    REQUIRE(solids_.size() == 1);
    REQUIRE(solids_[index]->position == 2.0f);
}
