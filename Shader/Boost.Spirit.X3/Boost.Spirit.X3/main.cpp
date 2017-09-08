#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4456)
#pragma warning(disable: 4521)
#pragma warning(disable: 4702)
#pragma warning(disable: 4819)
#define BOOST_SPIRIT_X3_DEBUG
#define BOOST_SPIRIT_UNICODE 
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/io.hpp>
#pragma warning(pop)

namespace client {
namespace fusion = boost::fusion;
namespace x3 = boost::spirit::x3;

namespace ast 
{
    struct sampler_desc;
    struct blend_desc;
    struct depth_stencil_desc;
    struct rasterizer_desc;
    struct statement : x3::variant<
          x3::forward_ast<sampler_desc>
        , x3::forward_ast<blend_desc>
        , x3::forward_ast<depth_stencil_desc>
        , x3::forward_ast<rasterizer_desc>
    >
    {
        using base_type::base_type;
        using base_type::operator=;
    };

    typedef std::pair<std::string, std::string> sampler_key_value;
    typedef std::map<std::string, std::string> sampler_map;
    struct sampler_desc
    {
        std::string name;
        uint32_t slot;
        sampler_map entries;
    };

    typedef std::pair<std::string, uint32_t> blend_key;
    typedef std::pair<blend_key, std::string> blend_key_value;
    typedef std::map<blend_key, std::string> blend_map;
    struct blend_desc
    {
        std::string name;
        blend_map entries;
    };

    typedef std::pair<std::string, std::string> depth_key_value;
    typedef std::map<std::string, std::string> depth_map;
    struct depth_stencil_desc
    {
        std::string name;
        depth_map entries;
    };

    typedef std::pair<std::string, std::string> raster_key_value;
    typedef std::map<std::string, std::string> raster_map;
    struct rasterizer_desc
    {
        std::string name;
        raster_map entries;
    };

    struct program
    {
        std::list<statement> states;
    };
}
}

// We need to tell fusion about our rexpr struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::sampler_desc,
    (std::string, name)
    (uint32_t, slot)
    (client::ast::sampler_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::blend_desc,
    (std::string, name)
    (client::ast::blend_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::depth_stencil_desc,
    (std::string, name)
    (client::ast::depth_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::rasterizer_desc,
    (std::string, name)
    (client::ast::raster_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::program,
    (std::list<client::ast::statement>, states)
)

namespace client { namespace parser {
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;

    using x3::lit;
    using x3::lexeme;

    using ascii::char_;
    using ascii::string;

    x3::rule<class block> const
        block = "block";

    auto const identifier = x3::lexeme[+(x3::alnum | '_')];

    auto const comment_line = "//" >> *(char_ -  x3::eol) >> x3::eol;
    auto const comment_block = "/*" >> *(char_ - "*/") >> "*/";
    auto const include = "#include" >> *(char_ - x3::eol) >> x3::eol;
    auto const texture = x3::no_case["texture"] >> *(char_ - ';') >> ";";
    auto const structure = 
           "struct"
        >> +(char_ - "}")
        >> "}"
        >> ";"
        ;
    auto const not_block = +(char_ - "{" - "}");
    auto const block_def =
           "{"
        >> +(block | not_block)
        >> "}"
        ;
    auto const function = 
           identifier
        >> x3::space
        >> identifier
        >> "(" 
        >> +(char_ - ")")
        >> ")"
        >> *(char_ - "{")
        >> block
        ;
    auto const space_comment = x3::space | comment_line | comment_block;
    auto const skip = 
           space_comment
        |  include
        |  texture
        |  structure
        |  function 
        ;
    auto const slot = 
           x3::lit(":") 
        >> "register"
        >> "("
        >> x3::omit[x3::alpha]
        >> x3::uint32 
        >> ")";

    x3::rule<class sampler_state, ast::sampler_desc> const 
        sampler_state = "sampler_state";
    x3::rule<class sampler_key_value, ast::sampler_key_value> const
        sampler_key_value = "sampler_key_value";
    auto const sampler_key_value_def =
        lexeme[+(char_ - x3::space)] >> '=' >> lexeme[+(char_ - x3::space - ';')] >> lit(";");
    auto const sampler_state_def = 
           "SamplerState"
        >> identifier
        >> slot
        >> '{' 
        >> *sampler_key_value 
        >> '}'
        >> ";"
        ;

    x3::rule<class blend_state, ast::blend_desc> const 
        blend_state = "blend_state";
    x3::rule<class blend_key, ast::blend_key> const
        blend_key = "blend_key";
    x3::rule<class blend_key_value, ast::blend_key_value> const
        blend_key_value = "blend_key_value";
    auto const blend_key_def =
           lexeme[+(x3::alpha - '[')]
        >> '['
        >> x3::uint32
        >> ']'
        ;
    auto const blend_key_value_def =
           blend_key
        >> '='
        >> lexeme[+(x3::alpha | '_')]
        >> ';'
        ;
    auto const blend_state_def =
           "BlendState"
        >> identifier
        >> '{'
        >> *blend_key_value
        >> '}'
        >> ';'
        ;

    x3::rule<class depth_stencil_state, ast::depth_stencil_desc> const 
        depth_stencil_state = "depth_stencil_state";
    x3::rule<class depth_key_value, ast::depth_key_value> const
        depth_key_value = "depth_key_value";
    auto const depth_key_value_def =
        lexeme[+(char_ - x3::space)] >> '=' >> lexeme[+(char_ - x3::space - ';')] >> lit(";");
    auto const depth_stencil_state_def = 
           "DepthStencilState"
        >> identifier
        >> '{' 
        >> *depth_key_value 
        >> '}'
        >> ";"
        ;

    x3::rule<class rasterizer_state, ast::rasterizer_desc> const 
        rasterizer_state = "rasterizer_state";
    x3::rule<class raster_key_value, ast::raster_key_value> const
        raster_key_value = "raster_key_value";
    auto const raster_key_value_def =
        lexeme[+(char_ - x3::space)] >> '=' >> lexeme[+(char_ - x3::space - ';')] >> lit(";");
    auto const rasterizer_state_def = 
           "RasterizerState"
        >> identifier
        >> '{' 
        >> *raster_key_value
        >> '}'
        >> ";"
        ;
    
    x3::rule<class statement, ast::statement> const 
        statement = "statement";
    auto const statement_def = 
        (
              blend_state
            | sampler_state
            | depth_stencil_state
        );
    x3::rule<class program, ast::program> const 
        program = "program";
    auto const program_def = +statement;

    BOOST_SPIRIT_DEFINE(
        sampler_state, sampler_key_value,
        blend_state, blend_key, blend_key_value,
        depth_stencil_state, depth_key_value,
        program,
        statement,
        block);

    struct expression_class
    {
        //  Our error handler
        template <typename Iterator, typename Exception, typename Context>
        x3::error_handler_result
            on_error( Iterator&, Iterator const& last, Exception const& x, Context const& context )
        {
            std::cout
                << "Error! Expecting: "
                << x.which()
                << " here: \""
                << std::string( x.where(), last )
                << "\""
                << std::endl
                ;
            return x3::error_handler_result::fail;
        }
    };
}
}

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
    std::string::const_iterator iter = k1.begin();
    std::string::const_iterator end = k1.end();
    namespace x3 = boost::spirit::x3;
    using namespace client;

    ast::program program;
    while (x3::phrase_parse(iter, end, parser::program, parser::skip, program))
        std::cout << "parse sucess:\n";
    return 0;
}
