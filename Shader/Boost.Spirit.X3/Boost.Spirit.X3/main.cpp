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

namespace ast 
{
    namespace fusion = boost::fusion;
    namespace x3 = boost::spirit::x3;

    struct sampler_desc;
    struct blend_desc;
    struct statement : x3::variant<
          x3::forward_ast<sampler_desc>
        , x3::forward_ast<blend_desc>
    >
    {
        using base_type::base_type;
        using base_type::operator=;
    };

    typedef std::map<std::string, std::string> sampler_map;
    typedef std::pair<std::string, std::string> sampler_key_value;
    struct sampler_desc
    {
        std::string name;
        uint32_t slot;
        sampler_map entries;
    };

    typedef std::pair<uint32_t, std::string> blend_key;
    typedef std::pair<blend_key, std::string> blend_key_value;
    typedef std::map<blend_key, std::string> blend_map;
    struct blend_desc
    {
        std::string name;
        blend_map entries;
    };

    struct program
    {
        std::list<statement> states;
    };
}

// We need to tell fusion about our rexpr struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    ast::sampler_desc,
    (std::string, name)
    (uint32_t, slot)
    (ast::sampler_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    ast::blend_desc,
    (std::string, name)
    (ast::blend_map, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
    ast::program,
    (std::list<ast::statement>, states)
)

namespace parser {
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
        >> x3::lit("register") 
        >> x3::lit("(") 
        >> x3::omit[x3::alpha]
        >> x3::uint32 
        >> x3::lit(")");

    x3::rule<class sampler_state, ast::sampler_desc> const 
        sampler_state = "sampler_state";
    x3::rule<class sampler_key_value, ast::sampler_key_value> const
        sampler_key_value = "sampler_key_value";
    auto const sampler_key_value_def =
        lexeme[+(char_ - x3::space)] >> '=' >> lexeme[+(char_ - x3::space - ';')] >> lit(";");
    auto const sampler_state_def = 
           x3::lit("SamplerState")
        >> identifier
        >> slot
        >> '{' 
        >> *sampler_key_value 
        >> '}'
        >> ";"
        ;

    x3::rule<class blend_state, ast::blend_desc> const 
        blend_state = "blend_state";
    x3::rule<class blend_key_value, ast::blend_key_value> const
        blend_key_value = "blend_key_value";
    auto const blend_key_value_def =
           lexeme[+(x3::alpha - '[')]
        >> x3::lit('[')
        >> x3::uint32
        >> x3::lit(']')
        >> x3::lit('=')
        >> lexeme[+(x3::alpha | '_')]
        >> x3::lit(';')
        ;
    auto const blend_state_def =
           x3::lit("BlendState")
        >> identifier
        >> x3::lit('{')
        >> *blend_key_value
        >> x3::lit('}')
        >> x3::lit(';')
        ;

    x3::rule<class statement, ast::statement> const 
        statement = "statement";
    auto const statement_def = 
        (
              blend_state
            | sampler_state
        );
    x3::rule<class program, ast::program> const 
        program = "program";
    auto const program_def = *statement;

    BOOST_SPIRIT_DEFINE(
        sampler_state, sampler_key_value,
        blend_state, blend_key_value,
        // program,
        // statement,
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
    // auto&& k = readFile( "pmx.fx" );
    // auto&& k3 = readFileEnc( L"pmx.fx", "utf-8" );
    // auto&& k2 = readFileEnc( L"mikudayo-3_6.fx", "shift-jis" );
    auto&& k1 = simpleRead( "pmx.fx" );
    std::string::const_iterator iter = k1.begin();
    std::string::const_iterator end = k1.end();
    namespace x3 = boost::spirit::x3;

    ast::sampler_desc desc;
    if (x3::phrase_parse(iter, end, parser::sampler_state, parser::skip, desc))
        std::cout << "parse sucess:\n";
#if 0
    ast::program program;
    while (x3::phrase_parse(iter, end, parser::program, parser::skip, program))
        std::cout << "parse sucess:\n";
#else
    ast::blend_desc blend_desc;
    while (x3::phrase_parse(iter, end, parser::blend_state, parser::skip, blend_desc))
        std::cout << "parse sucess:\n";
#endif
    return 0;
}
