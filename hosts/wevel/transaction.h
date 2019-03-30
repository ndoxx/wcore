#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <memory>
#include <deque>

namespace wevel
{

// Interface for all transactions
class AbstractTransaction
{
public:
    virtual void execute() = 0;
    virtual void unexecute() = 0;
};

// Concrete transaction on object of type T, using action policy defined
// by the type ActionImplementerT
template <typename ActionImplementerT>
class Transaction: public AbstractTransaction
{
public:
    Transaction(ActionImplementerT&& impl):
    impl_(std::move(impl))
    {

    }

    virtual ~Transaction() {}

    virtual void execute() override   { impl_.execute(); }
    virtual void unexecute() override { impl_.unexecute(); }

private:
    ActionImplementerT impl_;
};

// Transaction container which maintains two double ended queues, one for
// undo operations, one for redo operations
class TransactionStore
{
public:
    typedef std::shared_ptr<AbstractTransaction> transaction_ptr;

    TransactionStore(int max_operations=30):
    max_op_(max_operations)
    {

    }

    // Execute transaction and push it into the undo stack
    // Will clear the redo stack
    void push(transaction_ptr trans)
    {
        trans->execute();
        undo_stack_.push_front(trans);
        redo_stack_.clear();
        // Limit number of transactions
        if(undo_stack_.size()>max_op_)
            undo_stack_.pop_back();
    }

    // Construct transaction in place given its arguments, then push it
    template <typename ActionImplementerT>
    void push(ActionImplementerT&& implementer)
    {
        push(transaction_ptr(new Transaction(std::forward<ActionImplementerT>(implementer))));
    }

    // Roll back to previous state
    void undo()
    {
        if(!undo_stack_.empty())
        {
            undo_stack_.front()->unexecute();
            redo_stack_.push_front(undo_stack_.front());
            undo_stack_.pop_front();
        }
    }

    // Roll forward to previously undone state
    void redo()
    {
        if(!redo_stack_.empty())
        {
            redo_stack_.front()->execute();
            undo_stack_.push_front(redo_stack_.front());
            redo_stack_.pop_front();
        }
    }

    // Revert back to first known state
    void revert()
    {
        while(!undo_stack_.empty())
            undo();
    }

    // Clear transaction queues
    void clear()
    {
        undo_stack_.clear();
        redo_stack_.clear();
    }

    // Change max number of operations
    inline void set_max_operations(std::size_t value) { max_op_ = value; }

private:
    std::deque<transaction_ptr> undo_stack_;
    std::deque<transaction_ptr> redo_stack_;
    std::size_t max_op_;
};

} // namespace wevel

#endif // TRANSACTION_H
