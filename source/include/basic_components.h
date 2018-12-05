#ifndef WCOMPONENTS_H
#define WCOMPONENTS_H

#include "wcomponent.h"
#include "transformation.h"

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

} // namespace wcore::component

REGISTER_COMPONENT(WCTransform);

#endif // WCOMPONENTS_H
