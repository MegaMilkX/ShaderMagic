#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>

#include "glsltok.h"

class ShaderMagic
{
public:
    void Parts(const std::string& source)
    {
        
    }
    std::string Make()
    {
        
    }
    void Save(const std::string& filename)
    {
        std::ofstream file;
        file.open(filename.c_str(), std::ios::out);
        file << 
        R"(
            out vec4 fragment;
            void main()
            {
                vec4 diffuse = vec4(1.0, 1.0, 1.0, 1.0);
                {{PRECOG}}
                {{LAYERS}}
                fragment = diffuse;
            }
        )";
        file.close();
    }
};

int main()
{
    ShaderMagic shaderMagic;
    shaderMagic.Parts(
        R"(
        0.3, .4lf, 3.14;
        0xfff, 0xffffu, -1u, 30000, 0XA01, 023;
        #vertex
        #define SOME_STUPID_MACRO 56.0
            in vec3 Position;
            in mat4 MatrixModel;
            in mat4 MatrixView;
            in mat4 MatrixProjection;
            out vec3 PositionWorld;
            PositionWorld = MatrixProjection * MatrixView * MatrixModel * Position;
        #vertex
            in vec3 Normal;
            in mat4 MatrixModel;
            out vec3 NormalModel = (MatrixModel * vec4(Normal, 0.0)).xyz;
        #fragment
            // TODO: Some things
            /* Multi
                line
                comment */
            out vec4 fragColor;
            int main()
            {
                fragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )"
    );
    std::string shaderMagic.Make(
        R"(
            #vertex
            in vec3 PositionWorld;
            in vec3 PositionModel;
            in vec3 NormalModel;
            out vec3 NormalFrag;
            out vec3 FragmentPos;
            
            FragmentPos = PositionModel;
            NormalFrag = NormalWorld;
            gl_Position = PositionWorld;
            
            #fragment
            out vec4 FragmentColor;
            FragmentColor = vec4(1.0, 0.0, 0.0, 1.0);
        )"
    );
    
    shaderMagic.Save("test.glsl");
    
    std::getchar();
    
    return 0;
}
