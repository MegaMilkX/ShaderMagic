#include <string>
#include <fstream>


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
    
    shaderMagic.Save("test.glsl");
    
    return 0;
}