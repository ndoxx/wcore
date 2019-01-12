#ifndef EDITOR_H
#define EDITOR_H

#include <memory>

namespace wcore
{

class Model;

class Editor
{
public:

    inline void set_model_selection(std::weak_ptr<Model> pmdl) { model_selection_ = pmdl; }
    inline std::weak_ptr<Model> get_model_selection() const    { return model_selection_; }

private:
    std::weak_ptr<Model> model_selection_;
};

} // namespace wcore

#endif // EDITOR_H
