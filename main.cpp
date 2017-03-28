#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include <windows.h>

inline std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;
    std::string s = str;
    
    size_t start = 0U;
    size_t end = s.find(delim);
    while(end != std::string::npos)
    {
        result.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
        if(end == std::string::npos)
            result.push_back(s.substr(start, end));
    }
    
    return result;
}

class ShaderMagic
{
public:
    void AddLayer(const std::string& source,
        const std::string& method = "")
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
    
    void ParseParts(const std::string& source)
    {
        std::istringstream buffer(source);
        
        std::vector<std::string> ret(
            (std::istream_iterator<std::string>(buffer)),
            std::istream_iterator<std::string>()
        );
        
        while(ParsePart(ret));
    }
    
    bool ParsePart(std::vector<std::string>& source)
    {
        if(source.empty())
            return false;
        if(source[0] != "#vertex" &&
            source[0] != "#fragment")
            return false;
            
        std::string partType = source[0];
        source.erase(source.begin());
        
        std::cout << partType << std::endl;
        
        while(!source.empty() &&
            source[0] != "#vertex" &&
            source[0] != "#fragment")
        {
            ParseStatement(source);
        }
        
        return true;
    }
    
    void ParseStatement(std::vector<std::string>& source)
    {
        std::string statement = source[0];
        source.erase(source.begin());
        while(!source.empty() &&
            statement.back() != ';')
        {
            statement.push_back(' ');
            statement += source[0];
            source.erase(source.begin());
        }
        
        std::cout << statement << std::endl;
    }
};

int main()
{
    ShaderMagic shaderMagic;
    shaderMagic.AddLayer(
        R"(
            in sampler2D Texture2D;
            in vec2 UV;
            out vec4 textureColor2D = texture2D(Texture2D, UV);
        )"
    );
    shaderMagic.AddLayer(
        R"(
            in vec3 lightOmniPos0;
            in vec3 fragPos;
            out float lightOmniDist0 = distance(lightOmniPos0, fragPos);
        )"
    );
    shaderMagic.AddLayer(
        R"(
            in sampler2D Texture2D[];
            in vec2 UV[];
            return texture2D(Texture2D[0], UV[0]);
        )"
    );
    shaderMagic.AddLayer(
        R"(
            in vec3 fragPos;
            in vec4 diffuse;
            in vec3 normal;
            
            vec3 result = diffuse.xyz;
            for(int i = 0; i < lightOmniPos.length(); ++i)
            {
                vec3 lightDir = normalize(lightOmniPos[i] - fragPos);
                float diff = max(dot(normal, lightDir), 0.0);
                float dist = distance(lightOmniPos[i], fragPos);
                result += lightOmniRGB[i] * diff * (1.0 / (1.0 + 0.5 * dist + 3.0 * dist * dist));
            }
            
            return result;
        )"
    );
    /*
    shaderMagic.AddComponents(
        R"(
            #vertex
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
            out vec3 lambertOmni16
        )"
    );
    
    shaderMagic.AddLayer("lambertOmni16", ShaderMagic::ADD);
    shaderMagic.AddLayer("lambertOmniSpecular16", ShaderMagic::ADD);
    shaderMagic.AddLayer("lambertDirection", ShaderMagic::ADD);
    shaderMagic.AddLayer("ambientColor", ShaderMagic::SET);
    shaderMagic.AddLayer("skin32");
    
    shaderMagic.Compose(
        R"(
            #vertex
            in vec3 PositionWorld;
            gl_Position = PositionWorld;
            
            #fragment
            in vec3 NormalWorld;
            out vec4 fragmentColor = 
                vec4(NormalWorld, 1.0);
        )"
    );
    */
    shaderMagic.ParseParts(
        R"(
            #vertex
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
            out vec3 lambertOmni16
        )"
    );
    
    shaderMagic.Save("test.glsl");
    
    std::getchar();
    
    return 0;
}
