#define BOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT
// #define BOOST_SPIRIT_LEXERTL_DEBUG
// #define BOOST_VARIANT_MINIMIZE_SIZE
// #define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include "mqo.h"

inline std::string 
read_from_file(char const* infile)
{
    std::ifstream instream(infile);
    if (!instream.is_open()) {
        std::cerr << "Couldn't open file: " << infile << std::endl;
        exit(-1);
    }
    instream.unsetf(std::ios::skipws);      // No white space skipping!
    return std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                       std::istreambuf_iterator<char>());
}

BOOST_FUSION_ADAPT_STRUCT(
    client::scene,
    (glm::vec3, pos)
    (glm::vec3, lookat)
    (glm::float32, head)
    (glm::float32, pitch)
    (glm::float32, zoom2)
    (glm::int32, orthogonal)
    (glm::vec3, ambient)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::object,
    (std::string, name)
    (glm::int32, visible)
    (glm::int32, locking)
    (glm::int32, shading)
    (glm::int32, color_type)
    (glm::float32, facet)
    (glm::vec3, color)
    (std::vector<glm::vec3>, vertex)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::mqo,
    (client::scene, scene_)
    (std::vector<client::object>, objects_)
)

BOOST_FUSION_ADAPT_STRUCT(
    glm::vec3,
    (float, x)
    (float, y)
    (float, z)
)

namespace client
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;
    using boost::spirit::ascii::space;
    namespace qi = boost::spirit::qi;
    using qi::on_error;
    using qi::fail;
    using qi::double_;
    using qi::_1;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
    using phoenix::construct;
    using phoenix::val;
    using phoenix::at_c;
    using phoenix::push_back;
    using phoenix::ref;

    enum token_ids {
        kScene = 1000,
        kObject,
        kMaterial,
    };

    template <typename Iterator>
    struct scene_grammer 
        : qi::grammar<Iterator, client::scene(), ascii::space_type>
    {
        scene_grammer() : scene_grammer::base_type(program)
        {
            program = 
                qi::omit["{"] > 
                *(
                        pos_    [at_c<0>(_val) = _1]
                    |   lookat_ [at_c<1>(_val) = _1]
                    |   head_   [at_c<2>(_val) = _1]
                    |   pich_   [at_c<3>(_val) = _1]
                    |   zoom2_  [at_c<4>(_val) = _1]
                    |   ortho_  [at_c<5>(_val) = _1]
                    |   amb_    [at_c<6>(_val) = _1]
                )
                > qi::omit["}"]
                ;

            pos_ = 
                    "pos"
                >   vec3_   [_val = _1]
                ;

            lookat_ =
                    "lookat"
                >   vec3_   [_val = _1]
                ;

            head_ =
                    "head"
                >   qi::float_  [_val = _1]
                ;

            pich_ = 
                    "pich"
                >   qi::float_  [_val = _1]
                ;

            ortho_ = 
                    "ortho"
                >   qi::int_    [_val = _1]
                ;

            zoom2_ = 
                    "zoom2"
                >   qi::float_  [_val = _1]
                ;

            amb_ =
                    "amb"
                >   vec3_   [_val = _1]
                ;

            vec3_ = 
                    qi::float_ [at_c<0>(_val) = _1]
                >>  qi::float_ [at_c<1>(_val) = _1]
                >>  qi::float_ [at_c<2>(_val) = _1]
                ;

            on_error<fail>
            (
                program,
                std::cout
                << val( "Error! Expecting " )
                << _4                               // what failed?
                << val( " here: \"" )
                << construct<std::string>( _3, _2 )   // iterators to error-pos, end
                << val( "\"" )
                << std::endl
            );
        }

        qi::rule<Iterator, glm::vec3(), ascii::space_type> vec3_, pos_, lookat_, amb_;
        qi::rule<Iterator, float, ascii::space_type> head_, pich_, zoom2_;
        qi::rule<Iterator, int, ascii::space_type> ortho_;
        qi::rule<Iterator, client::scene(), ascii::space_type> program;
    };

    template <typename Iterator>
    struct object_grammer
        : qi::grammar<Iterator, client::object(), ascii::space_type>
    {
        object_grammer() : object_grammer::base_type(program)
        {

            program =
                quoted_string[at_c<0>(_val) = _1] 
                >  qi::omit["{"] >
                *(
                        visible_    [at_c<1>(_val) = _1]
                    |   locking_    [at_c<2>(_val) = _1]
                    |   shading_    [at_c<3>(_val) = _1]
                    |   color_type_ [at_c<4>(_val) = _1]
                    |   facet_      [at_c<5>(_val) = _1]
                    |   color_      [at_c<6>(_val) = _1]
                    |   vertex_     [at_c<7>(_val) = _1]
                ) 
                >> qi::omit["}"]
                ;

            quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];
            visible_ =
                    "visible"
                >   qi::int_    [_val = _1]
                ;

            locking_ =
                    "locking"
                >   qi::int_    [_val = _1]
                ;

            shading_ = 
                    "shading"
                >   qi::int_    [_val = _1]
                ;

            facet_ =
                    "facet"
                >   qi::float_  [_val = _1]
                ;

            color_ = 
                    "color"
                >   vec3_       [_val = _1]
                ;

            color_type_ =
                    "color_type"
                >   qi::int_    [_val = _1]
                ;

            vertex_ =
                    "vertex"
                >   qi::int_
                >   "{"
                >   *vec3_[push_back( _val, _1 )]
                >   "}"
                ;

            vec3_ = 
                    qi::float_ [at_c<0>(_val) = _1]
                >>  qi::float_ [at_c<1>(_val) = _1]
                >>  qi::float_ [at_c<2>(_val) = _1]
                ;

            on_error<fail>
            (
                program,
                std::cout
                << val( "Error! Expecting " )
                << _4                               // what failed?
                << val( " here: \"" )
                << construct<std::string>( _3, _2 )   // iterators to error-pos, end
                << val( "\"" )
                << std::endl
            );
        }

        qi::rule<Iterator, int, ascii::space_type> visible_, locking_, shading_, color_type_;
        qi::rule<Iterator, float, ascii::space_type> facet_;
        qi::rule<Iterator, glm::vec3, ascii::space_type> vec3_, color_;
        qi::rule<Iterator, client::object(), ascii::space_type> program;
        qi::rule<Iterator, std::vector<glm::vec3>, ascii::space_type> vertex_;
        qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
    };

    struct chunk_symbol_ : qi::symbols<char, token_ids>
    {
        chunk_symbol_()
        {
            add
            ("Scene", kScene)
            ("Material", kMaterial)
            ("Object", kObject);
        }
    } chunk_symbol;

    bool parse_mqo( const std::string& file_name, client::mqo& mqo)
    {
        std::string str = read_from_file( file_name.c_str() );

        typedef std::string::const_iterator iterator_type;
        iterator_type iter = str.begin();
        iterator_type end = str.end();
        scene_grammer<iterator_type> scene_;
        object_grammer<iterator_type> object_;

        double version = 0.0;
        if (!qi::phrase_parse( iter, end, "Metasequoia Document", qi::space))
            return false;
        if (!qi::phrase_parse( iter, end, "Format Text Ver " >> qi::double_, qi::space, version))
            return false;

        while (true) {
            if (qi::phrase_parse( iter, end, "Eof", qi::space))
                break;

            token_ids token;
            if (!qi::phrase_parse( iter, end, chunk_symbol, qi::space, token))
                return false;

            switch (token)
            {
            case kScene:
                if (!qi::phrase_parse( iter, end, scene_, space, mqo.scene_))
                    return false;
                break;
            case kObject:
                if (!qi::phrase_parse( iter, end, object_, space, mqo.objects_))
                    return false;
                break;
            default:
                return false;
            }
        }
        return 0;
    }
};
