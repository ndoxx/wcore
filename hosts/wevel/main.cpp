#include <iostream>
#include <string>
#include <cassert>
#include <map>

#include "transaction.h"
#include "math3d.h"

using namespace wcore;

class Solid
{
public:
    Solid(const math::vec3& position,
          const math::vec3& velocity,
          float mass):
    position(position),
    velocity(velocity),
    mass(mass)
    {

    }

    void print()
    {
        std::cout << position << " " << velocity << " " << mass << std::endl;
    }

    math::vec3 position;
    math::vec3 velocity;
    float mass;
};

class Label
{
public:
    Label(const std::string& name,
          const std::string& description):
    name(name),
    description(description)
    {

    }

    void print()
    {
        std::cout << name << ": " << description << std::endl;
    }

    std::string name;
    std::string description;
};


class MementoSolidMove
{
public:
    MementoSolidMove(const math::vec3& oldpos,
                     const math::vec3& newpos):
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
    math::vec3 oldpos_;
    math::vec3 newpos_;
};

class CommandSolidTranslate
{
public:
    explicit CommandSolidTranslate(const math::vec3& value):
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
    math::vec3 value_;
};

class Editor
{
public:
    Editor():
    transactions_(10),
    current_solid_index_(0),
    current_label_index_(0)
    {

    }

    int add_label(const std::string& name,
                  const std::string& description)
    {
        labels_.insert(std::pair(current_label_index_, Label(name, description)));
        return current_label_index_++;
    }

    int add_solid(const math::vec3& position,
                  const math::vec3& velocity=math::vec3(0.f),
                  float mass=1.f)
    {
        solids_.insert(std::pair(current_solid_index_, Solid(position, velocity, mass)));
        return current_solid_index_++;
    }

    void move_solid(int index, const math::vec3& newpos)
    {
        Solid& solid = solids_.at(index);
        /*wevel::TransactionStore::transaction_ptr trans(new wevel::Transaction(
            solid, MementoSolidMove(solid.position, newpos)));
        transactions_.push(trans);*/
        transactions_.push(solid, MementoSolidMove(solid.position, newpos));
    }

    void translate_solid(int index, const math::vec3& value)
    {
        Solid& solid = solids_.at(index);
        /*wevel::TransactionStore::transaction_ptr trans(new wevel::Transaction(
            solid, CommandSolidTranslate(value)));
        transactions_.push(trans);*/
        transactions_.push(solid, CommandSolidTranslate(value));
    }

    inline void undo()   { transactions_.undo(); }
    inline void redo()   { transactions_.redo(); }
    inline void revert() { transactions_.revert(); }

    void print()
    {
        std::cout << "--- Solids ---" << std::endl;
        for(auto&& [key, solid]: solids_)
            solid.print();
        /*std::cout << "--- Labels ---" << std::endl;
        for(auto&& [key, label]: labels_)
            label.print();*/
    }

private:
    wevel::TransactionStore transactions_;
    std::map<int, Solid> solids_;
    std::map<int, Label> labels_;
    int current_solid_index_;
    int current_label_index_;
};

int main(int argc, char const *argv[])
{
    Editor editor;

    std::cout << "[MEMENTO TEST]" << std::endl;
    std::cout << "* init" << std::endl;
    int index = editor.add_solid(math::vec3(4,5,8));
    editor.print();

    std::cout << "* move twice" << std::endl;
    editor.move_solid(index, math::vec3(9,12,12));
    editor.print();
    editor.move_solid(index, math::vec3(15,18,-22));
    editor.print();

    std::cout << "* undo twice" << std::endl;
    editor.undo();
    editor.print();
    editor.undo();
    editor.print();

    std::cout << "* redo" << std::endl;
    editor.redo();
    editor.print();

    std::cout << "* move again" << std::endl;
    editor.move_solid(index, math::vec3(18,25,2));
    editor.print();

    std::cout << "* redo shouldn't do shit" << std::endl;
    editor.redo();
    editor.print();

    std::cout << "* revert" << std::endl;
    editor.revert();
    editor.print();

    std::cout << std::endl << "[COMMAND TEST]" << std::endl;
    editor.print();

    std::cout << "* translate twice" << std::endl;
    editor.translate_solid(index, math::vec3(1,2,1));
    editor.print();
    editor.translate_solid(index, math::vec3(-1,2,1));
    editor.print();

    std::cout << "* undo twice" << std::endl;
    editor.undo();
    editor.print();
    editor.undo();
    editor.print();

    std::cout << "* redo" << std::endl;
    editor.redo();
    editor.print();

    std::cout << "* translate again" << std::endl;
    editor.translate_solid(index, math::vec3(2,-2,-4));
    editor.print();

    std::cout << "* redo shouldn't do shit" << std::endl;
    editor.redo();
    editor.print();

    std::cout << "* revert" << std::endl;
    editor.revert();
    editor.print();

    std::cout << std::endl << "[MAX OPERATIONS TEST]" << std::endl;
    editor.print();

    std::cout << "* move 20 times (max op is 10)" << std::endl;
    for(int ii=0; ii<20; ++ii)
    {
        editor.move_solid(index, math::vec3(ii+1,ii+1,ii+1));
    }
    editor.print();
    std::cout << "* revert" << std::endl;
    editor.revert();
    editor.print();

    return 0;
}
