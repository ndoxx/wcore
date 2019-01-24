#ifndef WCOMPONENTS_H
#define WCOMPONENTS_H

#include "wcomponent.h"
#include "transformation.h"
#include "model.h"

namespace wcore::component
{

class WCTransform: public WComponent, public Transformation
{
public:
    WCTransform(): Transformation() {}
    WCTransform(const math::vec3& position,
                const math::quat& orientation,
                float scale=1.0f): Transformation(position, orientation, scale) {}
    WCTransform(math::vec3&& position,
                math::quat&& orientation,
                float scale=1.0f): Transformation(position, orientation, scale) {}
};

class WCModel: public WComponent
{
public:
    std::shared_ptr<Model> model;
};

} // namespace wcore::component

REGISTER_COMPONENT(WCTransform);
REGISTER_COMPONENT(WCModel);

#endif // WCOMPONENTS_H
