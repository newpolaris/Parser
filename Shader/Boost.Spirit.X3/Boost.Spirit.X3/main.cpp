#include "stdafx.h"
#include "parse.h"
#include <iostream>

std::string simpleRead( const std::string& FileName )
{
    std::ifstream in( FileName, std::ios_base::in );
    in.unsetf(std::ios::skipws); // No white space skipping!
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::wstring readFile(const char* filename)
{
    std::wifstream wif(filename);
    wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    std::wstringstream wss;
    wss << wif.rdbuf();
    return wss.str();
}

std::wstring readFileEnc( std::wstring FileName, std::string Enc )
{
    std::ifstream in( FileName, std::ios_base::in );
    in.unsetf(std::ios::skipws); // No white space skipping!
    std::stringstream buffer;
    buffer << in.rdbuf();
    return boost::locale::conv::to_utf<wchar_t>( buffer.str(), Enc );
}

int main() {
    auto&& k1 = simpleRead( "pmx.fx" );
    // auto&& k0 = readFile( "pmx.fx" );
    // auto&& k2 = readFileEnc( L"mikudayo-3_6.fx", "shift-jis" );
    // auto&& k3 = readFileEnc( L"pmx.fx", "utf-8" );
    client::ast::program program;
    FxParse( k1, program);
    return 0;
}
