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
};
    
struct Snippet
{
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::string source;
};

void Print(Snippet& snip)
{
    std::cout << "== SNIPPET ==========" << std::endl;
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
            tok == "out")
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

inline std::vector<Snippet> MakeSnippets(const std::string& source)
{
    std::vector<Snippet> result;

    std::vector<unsigned> positions;
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
        }
        else if(TryMatch(source, i, fragIdentifier))
        {
            unsigned start = i;
            unsigned end = FindLineEnd(source, i);
            positions.push_back(start);
            positions.push_back(end);
        }
    }
    positions.push_back(source.size() - 1);
    positions.erase(positions.begin());
    for(unsigned i = 0; i < positions.size(); i+=2)
    {
        result.push_back(
            MakeSnippet(
                source.substr(
                    positions[i], 
                    positions[i + 1] - positions[i]
                )
            )
        );
    }
    
    return result;
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

inline void PutDecl(
    std::vector<std::string>& declStack,
    const std::string& decl
    )
{
    for(unsigned i = 0; i < declStack.size(); ++i)
    {
        if(declStack[i] == decl)
        {
            declStack.erase(declStack.begin() + i);
            break;
        }
    }
    
    declStack.push_back(decl);
}

inline void StackSnippet(
    std::vector<Snippet>& snips,
    std::vector<unsigned>& snipStack,
    std::vector<std::string>& declStack,
    Snippet& snip
    )
{
    for(unsigned i = 0; i < snip.inputs.size(); ++i)
    {
        unsigned snipIdx;
        if(!PickOutput(snips, snip.inputs[i], snipIdx))
        {
            Variable var = snip.inputs[i];
            std::string decl = var.type + " " + var.name;
            PutDecl(declStack, decl);
            continue;
        }
        PutIndex(snipStack, snipIdx);
        StackSnippet(snips, snipStack, declStack, snips[snipIdx]);
    }
}

inline std::string MakeSource(
    std::vector<Snippet>& snips,
    const std::string& source
    )
{
    std::string result;
    
    Snippet firstSnip = MakeSnippet(source);
    std::vector<unsigned> snipStack;
    std::vector<std::string> declStack;
    
    StackSnippet(snips, snipStack, declStack, firstSnip);
    
    for(unsigned i = 0; i < declStack.size(); ++i)
    {
        result += declStack[i];
        result += ";\n";
    }
    result += "void main()\n{\n";
    for(int i = snipStack.size() - 1; i >= 0; --i)
    {
        result += "    ";
        result += snips[snipStack[i]].source;
    }
    result += "    ";
    result += firstSnip.source;
    result += "}\n";
    
    return result;
}

}

int main()
{
    SM::Snippet snip = SM::MakeSnippet(
        R"(
            
        )"
    );
    
    std::vector<SM::Snippet> snips = 
        SM::MakeSnippets(
            R"(
            #vertex
                in vec3 Position;
                in mat4 MatrixModel;
                in mat4 MatrixView;
                in mat4 MatrixProjection;
                out vec3 PositionWorld;
                PositionWorld = 
                    Position * 
                    MatrixProjection * 
                    MatrixView * 
                    MatrixModel;
            #vertex 320
                
            #fragment
            #vertex
            )"
        );
        
    std::string source = MakeSource(
        snips,
        R"(
            in vec3 PositionWorld;
            gl_Position = PositionWorld;
        )"
    );
    
    std::cout << source << std::endl;
    
    /*
    for(unsigned i = 0; i < snips.size(); ++i)
    {
        SM::Print(snips[i]);
    }
    */
    std::getchar();
    
    return 0;
}
