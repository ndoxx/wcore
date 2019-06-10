#include "vertex_format.h"
#include "logger.h"

namespace wcore
{

BufferLayoutElement::BufferLayoutElement()
{

}

static uint32_t data_type_to_size(ShaderDataType type)
{
    switch(type)
    {
        case ShaderDataType::Float: return sizeof(float);
        case ShaderDataType::Vec2:  return sizeof(float) * 2;
        case ShaderDataType::Vec3:  return sizeof(float) * 3;
        case ShaderDataType::Vec4:  return sizeof(float) * 4;
        case ShaderDataType::Mat3:  return sizeof(float) * 3 * 3;
        case ShaderDataType::Mat4:  return sizeof(float) * 4 * 4;
        case ShaderDataType::Int:   return sizeof(int);
        case ShaderDataType::IVec2: return sizeof(int) * 2;
        case ShaderDataType::IVec3: return sizeof(int) * 3;
        case ShaderDataType::IVec4: return sizeof(int) * 4;
    }

    DLOGF("Unknown ShaderDataType", "batch");
    return 0;
}

BufferLayoutElement::BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized):
name(name),
type(type),
size(data_type_to_size(type)),
offset(0),
normalized(normalized)
{

}

uint32_t BufferLayoutElement::get_component_count() const
{
    switch(type)
    {
        case ShaderDataType::Float: return 1;
        case ShaderDataType::Vec2:  return 2;
        case ShaderDataType::Vec3:  return 3;
        case ShaderDataType::Vec4:  return 4;
        case ShaderDataType::Mat3:  return 3 * 3;
        case ShaderDataType::Mat4:  return 4 * 4;
        case ShaderDataType::Int:   return 1;
        case ShaderDataType::IVec2: return 2;
        case ShaderDataType::IVec3: return 3;
        case ShaderDataType::IVec4: return 4;
    }

    DLOGF("Unknown ShaderDataType", "batch");
    return 0;
}

BufferLayout::BufferLayout(const std::initializer_list<BufferLayoutElement>& elements):
elements_(elements),
stride_(0)
{
    compute_offset_and_stride();
}

void BufferLayout::compute_offset_and_stride()
{
    uint32_t offset = 0;
    stride_ = 0;
    for(auto&& element: elements_)
    {
        element.offset = offset;
        offset += element.size;
        stride_ += element.size;
    }
}

BufferLayout Vertex3P3N2U4C::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_normal"_h,   ShaderDataType::Vec3},
    {"a_texCoord"_h, ShaderDataType::Vec2},
    {"a_color"_h,    ShaderDataType::Vec4}
};

BufferLayout Vertex3P3N3T2U::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_normal"_h,   ShaderDataType::Vec3},
    {"a_tangent"_h,  ShaderDataType::Vec3},
    {"a_texCoord"_h, ShaderDataType::Vec2}
};

BufferLayout VertexAnim::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_normal"_h,   ShaderDataType::Vec3},
    {"a_tangent"_h,  ShaderDataType::Vec3},
    {"a_texCoord"_h, ShaderDataType::Vec2},
    {"a_weights"_h,  ShaderDataType::Vec4},
    {"a_boneIDs"_h,  ShaderDataType::IVec4},
};

BufferLayout Vertex3P3N2U::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_normal"_h,   ShaderDataType::Vec3},
    {"a_texCoord"_h, ShaderDataType::Vec2}
};

BufferLayout Vertex3P3N::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_normal"_h,   ShaderDataType::Vec3}
};

BufferLayout Vertex3P2U::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_texCoord"_h, ShaderDataType::Vec2}
};

BufferLayout Vertex3P::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3}
};

BufferLayout Vertex3P3C::Layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_color"_h,    ShaderDataType::Vec3}
};

BufferLayout Vertex2P2U::Layout =
{
    {"a_position"_h, ShaderDataType::Vec2},
    {"a_texCoord"_h, ShaderDataType::Vec2}
};



} // namespace wcore
