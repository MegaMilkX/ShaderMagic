#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>

#include "glsltok.h"

namespace Au{
namespace GLSLStitch{
    
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
    std::string name;
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::vector<Variable> other; // uniforms
    std::vector<Variable> locals;
    std::string source;
    
    void RenameInput(const std::string& from, const std::string& to)
    {
        for(unsigned i = 0; i < inputs.size(); ++i)
            if(inputs[i].name == from)
            { inputs[i].name = to; break; }
        RenameSourceTokens(from, to);
    }
    
    void RenameOutput(const std::string& from, const std::string& to)
    {
        for(unsigned i = 0; i < outputs.size(); ++i)
            if(outputs[i].name == from)
            { outputs[i].name = to; break; }
        RenameSourceTokens(from, to);
    }
    
    void RenameOther(const std::string& from, const std::string& to)
    {
        for(unsigned i = 0; i < other.size(); ++i)
            if(other[i].name == from)
            { other[i].name = to; break; }
        RenameSourceTokens(from, to);
    }
    
    void RenameLocal(const std::string& from, const std::string& to)
    {
        for(unsigned i = 0; i < locals.size(); ++i)
            if(locals[i].name == from)
            { locals[i].name = to; break; }
        RenameSourceTokens(from, to);
    }
    
    void RenameSourceTokens(const std::string& from, const std::string& to)
    {
        std::vector<GLSLTok::Token> tokens =
            GLSLTok::Tokenize(source);
        source.clear();
        for(unsigned i = 0; i < tokens.size(); ++i)
        {
            if(tokens[i].data == from)
                tokens[i].data = to;
            source += tokens[i].data;
        }
    }
    
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
    
    void RemoveLocal(const Variable& var)
    {
        for(unsigned i = 0; i < locals.size(); ++i)
        {
            if(locals[i] == var)
            {
                locals.erase(locals.begin() + i);
                return;
            }
        }
    }
};

inline void Print(Snippet& snip)
{
    std::cout << "== SNIPPET ==========" << std::endl;
    std::cout << snip.name << std::endl;
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

inline std::string ParseSnippetName(const std::string& str)
{
    std::string name;
    std::istringstream iss(str);
    std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    
    if(tokens.size() > 1)
        name = tokens[1];
    
    return name;
}

inline void MakeSnippets(
    const std::string& source,
    std::vector<Snippet>& vSnips,
    std::vector<Snippet>& fSnips,
    std::vector<Snippet>& gSnips
    )
{
    std::vector<unsigned> positions;
    std::vector<char> types;
    std::vector<std::string> names;
    std::string vertIdentifier = "#vertex";
    std::string fragIdentifier = "#fragment";
    std::string genericIdentifier = "#generic";
    for(unsigned i = 0; i < source.size(); ++i)
    {
        if(TryMatch(source, i, vertIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
            types.push_back(0);
            names.push_back(ParseSnippetName(source.substr(start, end - start)));
        }
        else if(TryMatch(source, i, fragIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
            types.push_back(1);
            names.push_back(ParseSnippetName(source.substr(start, end - start)));
        }
        else if(TryMatch(source, i, genericIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
            types.push_back(2);
            names.push_back(ParseSnippetName(source.substr(start, end - start)));
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
        else if(type == 1)
            v = &fSnips;
        else
            v = &gSnips;
        v->push_back(
            MakeSnippet(
                source.substr(
                    positions[i], 
                    positions[i + 1] - positions[i]
                )
            )
        );
        v->back().name = names[i/2];
        Print(v->back());
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
    Snippet source
    )
{
    std::vector<Snippet> dependencies = 
        ArrangeSnippets(snips, source);
    
    for(unsigned i = 0; i < dependencies.size(); ++i)
    {
        MergeSnippets(source, dependencies[i]);
    }
    
    return source;
}

inline Snippet AssembleSnippet(
    std::vector<Snippet>& snips,
    const std::string& source
    )
{
    Snippet snip = MakeSnippet(source);
    return AssembleSnippet(snips, snip);    
}

inline void CleanupOutputs(Snippet& snip, Snippet& next)
{
    snip.locals = snip.outputs;
    snip.outputs.clear();
    for(unsigned i = 0; i < next.inputs.size(); ++i)
    {
        Variable& in = next.inputs[i];
        for(unsigned j = 0; j < snip.locals.size(); ++j)
        {
            Variable& local = snip.locals[j];
            if(in == local)
            {
                snip.AddOutput(local);
                snip.RemoveLocal(local);
            }
        }
    }
}

inline void LinkSnippets(
    Snippet& snip, 
    Snippet& next, 
    std::vector<Snippet> extSnips = std::vector<Snippet>()
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
    
    CleanupOutputs(snip, next);
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
    for(unsigned i = 0; i < snip.locals.size(); ++i)
    {
        Variable& var = snip.locals[i];
        result += var.type + " " + var.name + ";\n";
    }
    result += snip.source;
    result += "}\n";
    
    return result;
}

}
}

int main()
{
    std::vector<Au::GLSLStitch::Snippet> vertSnips;
    std::vector<Au::GLSLStitch::Snippet> fragSnips;
    std::vector<Au::GLSLStitch::Snippet> genericSnips;
    Au::GLSLStitch::MakeSnippets(
        R"(
        #vertex PositionWorld
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
        #vertex NormalModel
            in vec3 Normal;
            uniform mat4 MatrixModel;
            out vec3 NormalModel;
            NormalModel = (MatrixModel * vec4(Normal, 0.0)).xyz;
        #vertex FragPosWorld
            in vec3 Position;
            uniform mat4 MatrixModel;
            out vec3 FragPosWorld;
            FragPosWorld = vec3(MatrixModel * vec4(Position, 1.0));
        #fragment LightDirection
            uniform vec3 LightPosition;
            in vec3 FragPosWorld;
            out vec3 LightDirection;
            LightDirection = normalize(LightPosition - FragPosWorld);
        #fragment LightOmniLambert
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
        #generic Multiply
            in vec4 first;
            in vec4 second;
            out vec4 result = first * second;
        )",
        vertSnips,
        fragSnips,
        genericSnips
    );
    
    Au::GLSLStitch::Snippet vSnip = Au::GLSLStitch::AssembleSnippet(
        vertSnips,
        R"(
            in vec3 PositionWorld;
            gl_Position = PositionWorld;
        )"
    );

    Au::GLSLStitch::Snippet fSnip = AssembleSnippet(
        fragSnips,
        R"(
            uniform vec3 AmbientColor;
            in vec3 LightOmniLambert;
            out vec4 fragColor;
            fragColor = vec4(AmbientColor + LightOmniLambert, 1.0);
        )"
    );
    
    LinkSnippets(vSnip, fSnip, vertSnips);
    CleanupOutputs(fSnip, Au::GLSLStitch::MakeSnippet("in vec4 fragColor;"));
    
    std::string vshader = Au::GLSLStitch::Finalize(vSnip);
    std::string fshader = Au::GLSLStitch::Finalize(fSnip);
    
    std::cout << "== VERTEX =========" << std::endl;
    std::cout << vshader << std::endl;
    std::cout << "== FRAGMENT =======" << std::endl;
    std::cout << fshader << std::endl;
    
    /*
    for(unsigned i = 0; i < snips.size(); ++i)
    {
        Au::GLSLStitch::Print(snips[i]);
    }
    */
    std::getchar();
    
    return 0;
}
