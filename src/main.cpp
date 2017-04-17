#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>

#include "glsltok.h"

namespace SM
{
    
struct Variable
{
    std::string type;
    std::string name;
    
    bool operator==(const Variable& other) const
    {
        return name == other.name;
    }
    
    void Print()
    {
        std::cout << type << " " << name << ";\n";
    }
};
    
struct Snippet
{
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::vector<Variable> other; // uniforms
    std::string source;
    
    void AddInput(const Variable& var)
    {
        for(unsigned i = 0; i < inputs.size(); ++i)
        {
            if(inputs[i] == var)
                return;
        }
        
        inputs.push_back(var);
    }
    
    void RemoveInput(const Variable& var)
    {
        for(unsigned i = 0; i < inputs.size(); ++i)
        {
            if(inputs[i] == var)
            {
                inputs.erase(inputs.begin() + i);
                return;
            }
        }
    }
    
    void AddOutput(const Variable& var)
    {
        for(unsigned i = 0; i < outputs.size(); ++i)
        {
            if(outputs[i] == var)
                return;
        }
        
        outputs.push_back(var);
    }
    
    void AddOther(const Variable& var)
    {
        for(unsigned i = 0; i < other.size(); ++i)
        {
            if(other[i] == var)
                return;
        }
        other.push_back(var);
    }
};

void Print(Snippet& snip)
{
    std::cout << "== SNIPPET ==========" << std::endl;
    std::cout << "-- INPUTS -----------" << std::endl;
    for(unsigned i = 0; i < snip.inputs.size(); ++i)
        snip.inputs[i].Print();
    std::cout << "-- OUTPUTS ----------" << std::endl;
    for(unsigned i = 0; i < snip.outputs.size(); ++i)
        snip.outputs[i].Print();
    std::cout << "-- SOURCE -----------" << std::endl;
    std::cout << snip.source << std::endl;
    std::cout << std::endl;
}

inline bool GetNextToken(
    std::vector<GLSLTok::Token>& tokens, 
    unsigned& cursor, 
    GLSLTok::Token& tok
    )
{
    for(unsigned i = cursor; i < tokens.size(); ++i)
    {
        if(tokens[i].type != GLSLTok::Token::WHITESPACE)
        {
            tok = tokens[i];
            cursor = i + 1;
            return true;
        }
    }
    
    return false;
}

inline Snippet MakeSnippet(const std::string& source)
{
    Snippet snip;
    std::vector<GLSLTok::Token> tokens =
        GLSLTok::Tokenize(source);
    
    GLSLTok::Token tok;
    unsigned cursor = 0;
    while(GetNextToken(tokens, cursor, tok))
    {
        if(tok == "in" ||
            tok == "out" ||
            tok == "uniform")
        {
            GLSLTok::Token specTok = tok;
            Variable var;
            unsigned cursorOld = cursor;
            while(GetNextToken(tokens, cursor, tok) &&
                tok.type != GLSLTok::Token::SEMICOLON) {}
                
            if(cursor - cursorOld < 2)
                continue;
            
            cursor = cursorOld;
            
            GetNextToken(tokens, cursor, tok);
            var.type = tok;
            unsigned varNameCursor = cursor;
            GetNextToken(tokens, cursor, tok);
            var.name = tok;

            if(specTok == "in")
                snip.inputs.push_back(var);
            else if(specTok == "out")
                snip.outputs.push_back(var);
            else if(specTok == "uniform")
                snip.other.push_back(var);
            
            GetNextToken(tokens, cursor, tok);
            if(tok.type == GLSLTok::Token::OPERATOR)
            {
                cursor = varNameCursor;
                continue;
            }
            
        }
        else
        {
            snip.source += tok;
            snip.source += " ";
            if(tok.type == GLSLTok::Token::SEMICOLON)
                snip.source += "\n";
        }
    }
    
    return snip;
}

inline bool TryMatch(
    const std::string& source, 
    unsigned from, 
    std::string& token
    )
{
    if(source.size() - from < token.size())
        return false;
    
    for(unsigned i = 0; i < token.size(); ++i)
    {
        if(token[i] != source[from + i])
            return false;
    }

    return true;
}

inline unsigned FindLineEnd(
    const std::string& source,
    unsigned& from
    )
{
    for(unsigned i = from; i < source.size(); ++i)
    {
        if(source[i] == '\n')
            return i;
    }
    return from;
}

inline void MakeSnippets(
    const std::string& source,
    std::vector<Snippet>& vSnips,
    std::vector<Snippet>& fSnips
    )
{
    std::vector<unsigned> positions;
    std::vector<char> types;
    std::string vertIdentifier = "#vertex";
    std::string fragIdentifier = "#fragment";
    for(unsigned i = 0; i < source.size(); ++i)
    {
        if(TryMatch(source, i, vertIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
            types.push_back(0);
        }
        else if(TryMatch(source, i, fragIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
            types.push_back(1);
        }
    }
    positions.push_back(source.size() - 1);
    positions.erase(positions.begin());
    for(unsigned i = 0; i < positions.size(); i+=2)
    {
        char type = types[i/2];
        std::vector<Snippet>* v;
        if(type == 0)
            v = &vSnips;
        else
            v = &fSnips;
        v->push_back(
            MakeSnippet(
                source.substr(
                    positions[i], 
                    positions[i + 1] - positions[i]
                )
            )
        );
    }
}

inline bool PickOutput(
    std::vector<Snippet>& snips,
    Variable& input,
    unsigned& index
    )
{
    for(unsigned i = 0; i < snips.size(); ++i)
    {
        for(unsigned j = 0; j < snips[i].outputs.size(); ++j)
        {
            Variable& output = snips[i].outputs[j];
            if(input.name == output.name)
            {
                index = i;
                return true;
            }
        }
    }
    
    return false;
}

inline void PutIndex(
    std::vector<unsigned>& snipStack,
    unsigned index
    )
{
    for(unsigned i = 0; i < snipStack.size(); ++i)
    {
        if(snipStack[i] == index)
        {
            snipStack.erase(snipStack.begin() + i);
            break;
        }
    }
    
    snipStack.push_back(index);
}

inline void StackSnippet(
    std::vector<Snippet>& snips,
    std::vector<unsigned>& snipStack,
    Snippet& snip
    )
{    
    for(unsigned i = 0; i < snip.inputs.size(); ++i)
    {
        unsigned snipIdx;
        if(!PickOutput(snips, snip.inputs[i], snipIdx))
            continue;
        PutIndex(snipStack, snipIdx);
        StackSnippet(
            snips, 
            snipStack, 
            snips[snipIdx]
        );
    }
}

inline std::vector<Snippet> ArrangeSnippets(
    std::vector<Snippet>& snips,
    Snippet& first
    )
{
    std::vector<Snippet> result;
    std::vector<unsigned> snipStack;
    
    StackSnippet(
        snips, 
        snipStack, 
        first
    );
    
    for(unsigned i = 0; i < snipStack.size(); ++i)
        result.push_back(snips[snipStack[i]]);
    
    return result;
}

inline void MergeSnippets(Snippet& low, Snippet& top)
{
    low.source = "{\n" + top.source + "}" + "\n" + low.source;
    
    for(unsigned i = 0; i < top.outputs.size(); ++i)
    {
        low.AddOutput(top.outputs[i]);
        low.RemoveInput(top.outputs[i]);
    }
    
    for(unsigned i = 0; i < top.inputs.size(); ++i)
        low.AddInput(top.inputs[i]);
    for(unsigned i = 0; i < top.other.size(); ++i)
        low.AddOther(top.other[i]);
}

inline Snippet AssembleSnippet(
    std::vector<Snippet>& snips,
    const std::string& source
    )
{
    Snippet result = MakeSnippet(source);
    
    std::vector<Snippet> dependencies = 
        ArrangeSnippets(snips, result);
    
    for(unsigned i = 0; i < dependencies.size(); ++i)
    {
        MergeSnippets(result, dependencies[i]);
    }
    
    return result;
}

inline void LinkSnippets(
    Snippet& snip, 
    Snippet& next, 
    std::vector<Snippet> extSnips
    )
{
    extSnips.insert(extSnips.begin(), snip);
    
    std::vector<Snippet> snips = 
        ArrangeSnippets(extSnips, next);
        
    extSnips.erase(extSnips.begin());
        
    for(unsigned i = 0; i < snips.size(); ++i)
    {
        MergeSnippets(snip, snips[i]);
    }
}

inline std::string Finalize(Snippet& snip)
{
    std::string result;
    
    for(unsigned i = 0; i < snip.inputs.size(); ++i)
    {
        Variable& var = snip.inputs[i];
        result += "in " + var.type + " " + var.name + ";\n";
    }
    result += "\n";
    for(unsigned i = 0; i < snip.outputs.size(); ++i)
    {
        Variable& var = snip.outputs[i];
        result += "out " + var.type + " " + var.name + ";\n";
    }
    result += "\n";
    for(unsigned i = 0; i < snip.other.size(); ++i)
    {
        Variable& var = snip.other[i];
        result += "uniform " + var.type + " " + var.name + ";\n";
    }
    
    result += "\nvoid main()\n{\n";
    result += snip.source;
    result += "}\n";
    
    return result;
}

}

int main()
{
    std::vector<SM::Snippet> vertSnips;
    std::vector<SM::Snippet> fragSnips;
    SM::MakeSnippets(
        R"(
        #vertex
            in vec3 Position;
            uniform mat4 MatrixModel;
            uniform mat4 MatrixView;
            uniform mat4 MatrixProjection;
            out vec4 PositionWorld;
            PositionWorld =  
                MatrixProjection * 
                MatrixView * 
                MatrixModel *
                vec4(Position, 1.0);
        #vertex
            in vec3 Normal;
            uniform mat4 MatrixModel;
            out vec3 NormalModel;
            NormalModel = (MatrixModel * vec4(Normal, 0.0)).xyz;
        #vertex
            in vec3 Position;
            uniform mat4 MatrixModel;
            out vec3 FragPosWorld;
            FragPosWorld = vec3(MatrixModel * vec4(Position, 1.0));
        #fragment
            uniform vec3 LightPosition;
            in vec3 FragPosWorld;
            out vec3 LightDirection;
            LightDirection = normalize(LightPosition - FragPosWorld);
        #fragment
            in vec3 NormalModel;
            uniform vec3 LightRGB;
            uniform vec3 LightPosition;
            in vec3 FragPosWorld;
            in vec3 LightDirection;
            out vec3 LightOmniLambert;
            float diff = max(dot(NormalModel, LightDirection), 0.0);
            float dist = distance(LightPosition, FragPosWorld);
            LightOmniLambert = 
                LightRGB * 
                diff *
                (1.0 / (1.0 + 0.5 * dist + 3.0 * dist * dist));
        )",
        vertSnips,
        fragSnips
    );
    
    SM::Snippet vSnip = AssembleSnippet(
        vertSnips,
        R"(
            in vec3 PositionWorld;
            gl_Position = PositionWorld;
        )"
    );
        
    SM::Snippet fSnip = AssembleSnippet(
        fragSnips,
        R"(
            uniform vec3 AmbientColor;
            in vec3 LightOmniLambert;
            out vec4 fragColor;
            fragColor = vec4(AmbientColor + LightOmniLambert, 1.0);
        )"
    );
    
    LinkSnippets(vSnip, fSnip, vertSnips);
    
    std::string vshader = SM::Finalize(vSnip);
    std::string fshader = SM::Finalize(fSnip);
    
    std::cout << "== VERTEX =========" << std::endl;
    std::cout << vshader << std::endl;
    std::cout << "== FRAGMENT =======" << std::endl;
    std::cout << fshader << std::endl;
    
    /*
    for(unsigned i = 0; i < snips.size(); ++i)
    {
        SM::Print(snips[i]);
    }
    */
    std::getchar();
    
    return 0;
}
