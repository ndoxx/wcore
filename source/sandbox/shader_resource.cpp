#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "utils.h"

struct ShaderResource
{
public:
    std::string vertex_shader;
    std::string geometry_shader;
    std::string fragment_shader;
    std::vector<std::string> flags;

    static const std::string SHADER_PATH;
    static constexpr hashstr_t VS = H_("vert");
    static constexpr hashstr_t GS = H_("geom");
    static constexpr hashstr_t FS = H_("frag");

    ShaderResource(std::string&& resource_str,
                   std::string&& flags_str = "")
    {
        // Tokenize resource string
        std::vector<std::string> resources;
        split_string(resource_str, resources, ';');

        // For each token, determine shader type and initialize corresponding field
        for(uint32_t ii=0; ii<resources.size(); ++ii)
        {
            std::vector<std::string> name_extension;
            split_string(resources[ii], name_extension, '.');

            hashstr_t shader_type = H_(name_extension[1].c_str());

            switch(shader_type)
            {
                case VS:
                    vertex_shader   = SHADER_PATH + resources[ii];
                    break;
                case GS:
                    geometry_shader = SHADER_PATH + resources[ii];
                    break;
                case FS:
                    fragment_shader = SHADER_PATH + resources[ii];
                    break;
                default:
                    std::cout << "ERROR" << std::endl;
            }

        }

        // Parse flags (for shader variants)
        if(flags_str.size())
            split_string(flags_str, flags, ';');

        std::cout << vertex_shader << " " << geometry_shader << " "
                  << fragment_shader << std::endl;

        if(flags.size())
        {
            std::cout << "FLAGS:" << std::endl;
            for(uint32_t ii=0; ii<flags.size(); ++ii)
                std::cout << flags[ii] << std::endl;
        }
    }
};

const std::string ShaderResource::SHADER_PATH("../res/shaders/");

int main(int argc, char const *argv[])
{

    ShaderResource sres("gpass.vert;gpass.geom;gpass.frag");
    ShaderResource sres2("lpass_exp.vert;lpass_exp.frag");
    ShaderResource sres3("lpass_exp.vert;lpass_exp.frag", "VARIANT_DIRECTIONAL_LIGHT");
    ShaderResource sres4("lpass_exp.vert;lpass_exp.frag", "VARIANT_POINT_LIGHT;PLOP");

    return 0;
}
