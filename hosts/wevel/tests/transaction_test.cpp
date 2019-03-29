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
// undo() and redo() just switch state
class MementoSolidMove
{
public:
    MementoSolidMove(float oldpos,
                     float newpos):
    oldpos_(oldpos),
    newpos_(newpos)
    {

    }

    void undo(Solid& solid)
    {
        solid.position = oldpos_;
    }

    void redo(Solid& solid)
    {
        solid.position = newpos_;
    }

private:
    float oldpos_;
    float newpos_;
};

// Store a modifier, undo() and redo() perform
// opposite operations on solid using stored modifier
class CommandSolidTranslate
{
public:
    explicit CommandSolidTranslate(float value):
    value_(value)
    {

    }

    void undo(Solid& solid)
    {
        solid.position += value_;
    }

    void redo(Solid& solid)
    {
        solid.position -= value_;
    }

private:
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
        transactions_.push(solid_, MementoSolidMove(solid_.position, newpos));
    }

    void translate_solid(float value)
    {
        transactions_.push(solid_, CommandSolidTranslate(value));
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
