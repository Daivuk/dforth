#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

#define FORTH_IMPLEMENT
#include <forth/forth.h>

#include "eval_check.h"

TEST_CASE("ForthContext", "[ForthContext]")
{
    SECTION("Not enough memory for standard WORDs")
    {
        ForthContext* ctx = forth_createContext(100);
        REQUIRE_FALSE(ctx);
    }

    SECTION("Dictionnary too small for standard WORDs")
    {
        ForthContext* ctx = forth_createContext(-1, -1, 100);
        REQUIRE_FALSE(ctx);
    }

    SECTION("Stack overflow")
    {
        ForthContext* ctx = forth_createContext(-1, 1);

        evalTestSection(ctx, "1", FORTH_SUCCESS, {1});
        evalTestSection(ctx, "1 2", FORTH_FAILURE, {1}, "Stack overflow\n");

        forth_destroyContext(ctx);
    }
}

// Testing examples and exercices from the book titled "Starting FORTH"
TEST_CASE("Starting FORTH", "[StartingForth]")
{
    ForthContext* ctx = forth_createContext();

    // Chapter 1
    evalTest(ctx, "15 SPACES", FORTH_SUCCESS, {}, "               ");
    evalTest(ctx, "42 EMIT", FORTH_SUCCESS, {}, "*");
    evalTest(ctx, "15 SPACES  42 EMIT  42 EMIT", FORTH_SUCCESS, {}, "               **");
    evalTest(ctx, ": STAR   42 EMIT ; STAR", FORTH_SUCCESS, {}, "*");
    evalTest(ctx, "CR", FORTH_SUCCESS, {}, "\n");
    evalTest(ctx, "CR STAR CR STAR CR STAR", FORTH_SUCCESS, {}, "\n*\n*\n*");
    evalTest(ctx, ": MARGIN   CR 30 SPACES ;", FORTH_SUCCESS, {});
    evalTest(ctx, "MARGIN STAR MARGIN STAR MARGIN STAR", FORTH_SUCCESS, {}, "\n                              *\n                              *\n                              *");
    evalTest(ctx, ": BLIP   MARGIN STAR ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": STARS   0 DO STAR LOOP ;", FORTH_SUCCESS, {});
    evalTest(ctx, "5 STARS", FORTH_SUCCESS, {}, "*****");
    evalTest(ctx, "35 STARS", FORTH_SUCCESS, {}, "***********************************");
    evalTest(ctx, ": BAR   MARGIN  5 STARS ;", FORTH_SUCCESS, {});
    evalTest(ctx, "BAR", FORTH_SUCCESS, {}, "\n                              *****");
    evalTest(ctx, "BAR BLIP BAR BLIP BLIP  CR", FORTH_SUCCESS, {},
             "\n                              *****"
             "\n                              *"
             "\n                              *****"
             "\n                              *"
             "\n                              *\n");
    evalTest(ctx, ": F   BAR BLIP BAR BLIP BLIP  CR ;", FORTH_SUCCESS, {});
    evalTest(ctx, "F", FORTH_SUCCESS, {},
             "\n                              *****"
             "\n                              *"
             "\n                              *****"
             "\n                              *"
             "\n                              *\n");
    evalTest(ctx, ": GREET   .\" HELLO, I SPEAK FORTH \" ;", FORTH_SUCCESS, {});
    evalTest(ctx, "GREET", FORTH_SUCCESS, {}, "HELLO, I SPEAK FORTH ");
    evalTest(ctx, "3 4 + .", FORTH_SUCCESS, {}, "7 ");
    evalTest(ctx, ": FOUR-MORE   4 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, "3 FOUR-MORE .", FORTH_SUCCESS, {}, "7 ");
    evalTest(ctx, "-10 FOUR-MORE .", FORTH_SUCCESS, {}, "-6 ");
    evalTest(ctx, "2 4 6 8 . . . .", FORTH_SUCCESS, {}, "8 6 4 2 ");
    evalTest(ctx, "10 20 30 . . . .", FORTH_FAILURE, {}, "30 20 10 Stack underflow\n");
    // ( 1.)
    evalTest(ctx, ": GIFT   .\" BOOKENDS\" ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": GIVER   .\" STEPHANIE\" ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": THANKS   .\" DEAR \" GIVER 44 EMIT CR 4 SPACES .\" THANKS FOR THE \" GIFT 46 EMIT ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "THANKS", FORTH_SUCCESS, {}, "DEAR STEPHANIE,\n    THANKS FOR THE BOOKENDS.");
    // ( 2.)
    evalTest(ctx, ": TEN.LESS ( n -- n-10 ) -10 + ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "7 TEN.LESS .", FORTH_SUCCESS, {}, "-3 ");
    evalTest(ctx, "12 TEN.LESS .", FORTH_SUCCESS, {}, "2 ");
    // ( 3.)
    evalTest(ctx, ": GIVER   .\" JOHN\" ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "THANKS", FORTH_SUCCESS, {}, "DEAR STEPHANIE,\n    THANKS FOR THE BOOKENDS.");

    forth_destroyContext(ctx);
}

TEST_CASE("store", "[store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("number_sign", "[number_sign]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "#", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("number_sign_greater", "[number_sign_greater]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "#>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("number_sign_s", "[number_sign_s]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "#S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("tick", "[tick]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "'", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("paren", "[paren]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "(", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( )", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment )", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment)", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment) 4 5 +", FORTH_SUCCESS, {9});
    evalTestSection(ctx, "2 3 + ( Comment) 4 5 +", FORTH_SUCCESS, {5, 9});
    evalTestSection(ctx, ": foo ( n --  ) . ; 5 foo", FORTH_SUCCESS, {}, "5 ");

    forth_destroyContext(ctx);
}

TEST_CASE("paren_local_paren", "[paren_local_paren]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "(LOCAL)", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("star", "[star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "*", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 *", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 *", FORTH_SUCCESS, {1, 6});
    evalTestSection(ctx, "0 0 *", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0 *", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 *", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "2 1 *", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "3 3 *", FORTH_SUCCESS, {9});
    evalTestSection(ctx, "-3 3 *", FORTH_SUCCESS, {-9});
    evalTestSection(ctx, "3 -3 *", FORTH_SUCCESS, {-9});
    evalTestSection(ctx, "-3 -3 *", FORTH_SUCCESS, {9});

    forth_destroyContext(ctx);
}

TEST_CASE("star_slash", "[star_slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "*/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("star_slash_mod", "[star_slash_mod]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "*/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("plus", "[plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "+", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 +", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 +", FORTH_SUCCESS, {1, 5});
    evalTestSection(ctx, "0 5 +", FORTH_SUCCESS, {5});
    evalTestSection(ctx, "5 0 +", FORTH_SUCCESS, {5});
    evalTestSection(ctx, "0 -5 +", FORTH_SUCCESS, {-5});
    evalTestSection(ctx, "-5 0 +", FORTH_SUCCESS, {-5});
    evalTestSection(ctx, "1 2 +", FORTH_SUCCESS, {3});
    evalTestSection(ctx, "1 -2 +", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 2 +", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-1 -2 +", FORTH_SUCCESS, {-3});
    evalTestSection(ctx, "-1 1 +", FORTH_SUCCESS, {0});

    forth_destroyContext(ctx);
}

TEST_CASE("plus_store", "[plus_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "+!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("plus_field", "[plus_field]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "+FIELD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("plus_loop", "[plus_loop]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "+LOOP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("plus_x_string", "[plus_x_string]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "+X/STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("comma", "[comma]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ",", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("minus", "[minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "-", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 -", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 -", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 5 -", FORTH_SUCCESS, {-5});
    evalTestSection(ctx, "5 0 -", FORTH_SUCCESS, {5});
    evalTestSection(ctx, "0 -5 -", FORTH_SUCCESS, {5});
    evalTestSection(ctx, "-5 0 -", FORTH_SUCCESS, {-5});
    evalTestSection(ctx, "1 2 -", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 -2 -", FORTH_SUCCESS, {3});
    evalTestSection(ctx, "-1 2 -", FORTH_SUCCESS, {-3});
    evalTestSection(ctx, "-1 -2 -", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "0 1 -", FORTH_SUCCESS, {-1});

    forth_destroyContext(ctx);
}

TEST_CASE("dash_trailing", "[dash_trailing]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "-TRAILING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("dash_trailing_garbage", "[dash_trailing_garbage]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "-TRAILING-GARBAGE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("dot", "[dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ".", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 .", FORTH_SUCCESS, {}, "1 ");
    evalTestSection(ctx, "1 2 .", FORTH_SUCCESS, {1}, "2 ");
    evalTestSection(ctx, "1 . 2 . 3 . 4 5 6 . . .", FORTH_SUCCESS, {}, "1 2 3 6 5 4 ");

    forth_destroyContext(ctx);
}

TEST_CASE("dot_quote", "[dot_quote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "\"", FORTH_FAILURE, {}, "Undefined word\n");
    evalTestSection(ctx, ".\"", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, ".\" Text", FORTH_SUCCESS, {}, "Text");
    evalTestSection(ctx, ".\" Text\"", FORTH_SUCCESS, {}, "Text");
    evalTestSection(ctx, "   .\" Text with spaces\"   ", FORTH_SUCCESS, {}, "Text with spaces");
    evalTestSection(ctx, ".\" Text\" CR", FORTH_SUCCESS, {}, "Text\n");
    evalTestSection(ctx, ".\" Text\"CR", FORTH_SUCCESS, {}, "Text\n");
    evalTestSection(ctx, "CR .\" You should see 2345: \".\" 2345\"", FORTH_SUCCESS, {}, "\nYou should see 2345: 2345");
    evalTestSection(ctx, ": pb1 CR .\" You should see 2345: \".\" 2345\"; pb1", FORTH_SUCCESS, {}, "\nYou should see 2345: 2345");
    evalTestSection(ctx, ": print-stack-top  CR DUP .\" The top of the stack is \" . CR .\" which looks like '\" DUP EMIT .\" ' in ascii  \" ; 48 print-stack-top", FORTH_SUCCESS, {48}, "\nThe top of the stack is 48 \nwhich looks like '0' in ascii  ");

    forth_destroyContext(ctx);
}

TEST_CASE("dot_paren", "[dot_paren]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ".(", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("dot_r", "[dot_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ".R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("dot_s", "[dot_s]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ".S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("slash", "[slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "/", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 /", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 /", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 1 /", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 /", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "2 1 /", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "-1 1 /", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-2 1 /", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "0 -1 /", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 -1 /", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "2 -1 /", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "-1 -1 /", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-2 -1 /", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "2 2 /", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-1 -1 /", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-2 -2 /", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "7 3 /", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "7 -3 /", FORTH_SUCCESS, {-3});
    evalTestSection(ctx, "-7 3 /", FORTH_SUCCESS, {-3});
    evalTestSection(ctx, "-7 -3 /", FORTH_SUCCESS, {2});

    forth_destroyContext(ctx);
}

TEST_CASE("slash_mod", "[slash_mod]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("slash_string", "[slash_string]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "/STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("zero_less", "[zero_less]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "0<", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0<", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 0<", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 0<", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0<", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 0<", FORTH_SUCCESS, {0});

    forth_destroyContext(ctx);
}

TEST_CASE("zero_not_equals", "[zero_not_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "0<>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0<>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0<>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0<>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0<>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 0<>", FORTH_SUCCESS, {-1});

    forth_destroyContext(ctx);
}

TEST_CASE("zero_equals", "[zero_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "0>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});

    forth_destroyContext(ctx);
}

TEST_CASE("zero_greater", "[zero_greater]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "0>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});

    forth_destroyContext(ctx);
}

TEST_CASE("one_plus", "[one_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "1+", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 1+", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "1 2 1+", FORTH_SUCCESS, {1, 3});
    evalTestSection(ctx, "0 1+", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-1 1+", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1+", FORTH_SUCCESS, {2});

    forth_destroyContext(ctx);
}

TEST_CASE("one_minus", "[one_minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "1-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_store", "[two_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_star", "[two_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_slash", "[two_slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_to_r", "[two_to_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2>R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_fetch", "[two_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_constant", "[two_constant]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2CONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_drop", "[two_drop]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2DROP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_dupe", "[two_dupe]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2DUP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_literal", "[two_literal]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2LITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_over", "[two_over]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2OVER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_r_from", "[two_r_from]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2R>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_r_fetch", "[two_r_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2R@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_rote", "[two_rote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2ROT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_swap", "[two_swap]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2SWAP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_value", "[two_value]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2VALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("two_variable", "[two_variable]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "2VARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("colon", "[colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ": foo 100 + ; 1000 foo", FORTH_SUCCESS, {1100});
    evalTestSection(ctx, ": foo : bar ; ;", FORTH_FAILURE, {}, "Undefined word\n");
    evalTestSection(ctx, "foo foo1 foo foo2", FORTH_FAILURE, {}, "Undefined word\n");
    evalTestSection(ctx, ": GDX 123 ; : GDX GDX 234 ; GDX", FORTH_SUCCESS, {123, 234});

    //    forth::eval(ctx, ": print-stack-top  cr dup .\" The top of the stack is \" . cr .\" which looks like '\" dup emit .\" ' in ascii  \" ;");
    //forth::eval(ctx, "48 print-stack-top");

    //REQUIRE(LogCapturer::log == "\nThe top of the stack is 48 \nwhich looks like '0' in ascii  ");

    forth_destroyContext(ctx);
}

TEST_CASE("colon_no_name", "[colon_no_name]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ":NONAME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("semicolon", "[semicolon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ";", FORTH_FAILURE, {}, "Unexpected ';'\n");

    forth_destroyContext(ctx);
}

TEST_CASE("semicolon_code", "[semicolon_code]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ";CODE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("less_than", "[less_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "<", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 <", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 <", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 3 <", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 1 <", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 <", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 0 <", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 1 <", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "0 0 <", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 <", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0 <", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "2 1 <", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 -1 <", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 -1 <", FORTH_SUCCESS, {0});

    forth_destroyContext(ctx);
}

TEST_CASE("less_number_sign", "[less_number_sign]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "<#", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("not_equals", "[not_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "<>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 <>", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 <>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 3 <>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "1 1 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "-1 -1 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "1 0 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "-1 0 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "0 1 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "0 -1 <>", FORTH_SUCCESS, {-1}, "");

    forth_destroyContext(ctx);
}

TEST_CASE("equals", "[equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "=", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 =", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 =", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 3 =", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 0 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "1 1 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "-1 -1 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "1 0 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "-1 0 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "0 1 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "0 -1 =", FORTH_SUCCESS, {0}, "");

    forth_destroyContext(ctx);
}

TEST_CASE("greater_than", "[greater_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 >", FORTH_FAILURE, {1}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 3 >", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 1 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 1 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 0 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 >", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0 >", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "2 1 >", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "0 -1 >", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 -1 >", FORTH_SUCCESS, {-1});

    forth_destroyContext(ctx);
}

TEST_CASE("to_body", "[to_body]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">BODY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("to_float", "[to_float]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">FLOAT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("to_in", "[to_in]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">IN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("to_number", "[to_number]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">NUMBER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("to_r", "[to_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, ">R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("question", "[question]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("question_do", "[question_do]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "?DO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("question_dupe", "[question_dupe]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "?DUP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("fetch", "[fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ABORT", "[ABORT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ABORT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("abort_quote", "[abort_quote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ABORT\"", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("abs", "[abs]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ABS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ACCEPT", "[ACCEPT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ACCEPT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ACTION_OF", "[ACTION_OF]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ACTION-OF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("AGAIN", "[AGAIN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "AGAIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("AHEAD", "[AHEAD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "AHEAD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ALIGN", "[ALIGN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ALIGNED", "[ALIGNED]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ALLOCATE", "[ALLOCATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ALLOCATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ALLOT", "[ALLOT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ALLOT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ALSO", "[ALSO]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ALSO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("AND", "[AND]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "AND", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ASSEMBLER", "[ASSEMBLER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ASSEMBLER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("at_x_y", "[at_x_y]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "AT-XY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BASE", "[BASE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BEGIN", "[BEGIN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BEGIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BEGIN_STRUCTURE", "[BEGIN_STRUCTURE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BEGIN-STRUCTURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BIN", "[BIN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("b_l", "[b_l]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BLANK", "[BLANK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BLANK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("b_l_k", "[b_l_k]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BLK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BLOCK", "[BLOCK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BLOCK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BUFFER", "[BUFFER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BUFFER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("buffer_colon", "[buffer_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BUFFER:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("BYE", "[BYE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "BYE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_store", "[c_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "C!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_quote", "[c_quote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "C\"", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_comma", "[c_comma]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "C,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_fetch", "[c_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "C@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CASE", "[CASE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CATCH", "[CATCH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CATCH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("cell_plus", "[cell_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CELL+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CELLS", "[CELLS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CELLS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_field_colon", "[c_field_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("char", "[char]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("char_plus", "[char_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CHAR+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("chars", "[chars]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CHARS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CLOSE_FILE", "[CLOSE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CLOSE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_move", "[c_move]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CMOVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_move_up", "[c_move_up]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CMOVE>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CODE", "[CODE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CODE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("COMPARE", "[COMPARE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "COMPARE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("compile_comma", "[compile_comma]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "COMPILE,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CONSTANT", "[CONSTANT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("COUNT", "[COUNT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "COUNT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_r", "[c_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CR", FORTH_SUCCESS, {}, "\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CREATE", "[CREATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CREATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("CREATE_FILE", "[CREATE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CREATE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_s_pick", "[c_s_pick]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CS-PICK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("c_s_roll", "[c_s_roll]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "CS-ROLL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_plus", "[d_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_minus", "[d_minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_dot", "[d_dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_dot_r", "[d_dot_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D.R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_zero_less", "[d_zero_less]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D0<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_zero_equals", "[d_zero_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D0=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_two_star", "[d_two_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D2*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_two_slash", "[d_two_slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D2/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_less_than", "[d_less_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_equals", "[d_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_to_f", "[d_to_f]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D>F", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_to_s", "[d_to_s]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "D>S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_abs", "[d_abs]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DABS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DECIMAL", "[DECIMAL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DECIMAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DEFER", "[DEFER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DEFER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("defer_store", "[defer_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DEFER!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("defer_fetch", "[defer_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DEFER@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DEFINITIONS", "[DEFINITIONS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DEFINITIONS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DELETE_FILE", "[DELETE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DELETE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DEPTH", "[DEPTH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DEPTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_f_store", "[d_f_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DF!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_f_fetch", "[d_f_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DF@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_f_align", "[d_f_align]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DFALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_f_aligned", "[d_f_aligned]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DFALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_f_field_colon", "[d_f_field_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DFFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_float_plus", "[d_float_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DFLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_floats", "[d_floats]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DFLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_max", "[d_max]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DMAX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_min", "[d_min]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DMIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_negate", "[d_negate]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DNEGATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DO", "[DO]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DO", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP ; 5 STARS", FORTH_SUCCESS, {}, "*****");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP ; 5 STARS .\"  <- should see 5 stars\"", FORTH_SUCCESS, {}, "***** <- should see 5 stars");
    evalTestSection(ctx, ": STARS 0 DO 4 2 DO 42 EMIT LOOP LOOP ; 3 STARS", FORTH_SUCCESS, {}, "******");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP .\" Carrots\"; 5 STARS", FORTH_SUCCESS, {}, "*****Carrots");

    forth_destroyContext(ctx);
}

TEST_CASE("does", "[does]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DOES>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DROP", "[DROP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DROP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("d_u_less", "[d_u_less]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DU<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("DUMP", "[DUMP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DUMP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("dupe", "[dupe]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "DUP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 DUP", FORTH_SUCCESS, {1, 1});
    evalTestSection(ctx, "1 2 DUP", FORTH_SUCCESS, {1, 2, 2});

    forth_destroyContext(ctx);
}

TEST_CASE("EDITOR", "[EDITOR]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EDITOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("e_key", "[e_key]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("e_key_to_char", "[e_key_to_char]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EKEY>CHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("e_key_to_f_key", "[e_key_to_f_key]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EKEY>FKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("e_key_to_x_char", "[e_key_to_x_char]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EKEY>XCHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("e_key_question", "[e_key_question]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EKEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ELSE", "[ELSE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ELSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("EMIT", "[EMIT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EMIT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "65 EMIT EMIT", FORTH_FAILURE, {}, "AStack underflow\n");
    evalTestSection(ctx, "65 EMIT", FORTH_SUCCESS, {}, "A");
    evalTestSection(ctx, "66 65 EMIT", FORTH_SUCCESS, {66}, "A");
    evalTestSection(ctx, "33 119 111 87 EMIT EMIT EMIT EMIT", FORTH_SUCCESS, {}, "Wow!");
    evalTestSection(ctx, "87 EMIT 111 EMIT 119 EMIT 33 EMIT", FORTH_SUCCESS, {}, "Wow!");

    forth_destroyContext(ctx);
}

TEST_CASE("emit_question", "[emit_question]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EMIT?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("EMPTY_BUFFERS", "[EMPTY_BUFFERS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EMPTY-BUFFERS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("END_STRUCTURE", "[END_STRUCTURE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "END-STRUCTURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("end_case", "[end_case]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ENDCASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("end_of", "[end_of]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ENDOF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("environment_query", "[environment_query]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ENVIRONMENT?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ERASE", "[ERASE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ERASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("EVALUATE", "[EVALUATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EVALUATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("EXECUTE", "[EXECUTE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EXECUTE", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "-5 EXECUTE", FORTH_FAILURE, {}, "Invalid memory address\n");
    evalTestSection(ctx, "10000 EXECUTE", FORTH_FAILURE, {}, "Invalid memory address\n");
    evalTestSection(ctx, "0 EXECUTE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("EXIT", "[EXIT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "EXIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_store", "[f_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_star", "[f_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_star_star", "[f_star_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F**", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_plus", "[f_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_minus", "[f_minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_dot", "[f_dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_slash", "[f_slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_zero_less_than", "[f_zero_less_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F0<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_zero_equals", "[f_zero_equals]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F0=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_to_d", "[f_to_d]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F>D", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("F_to_S", "[F_to_S]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F>S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_fetch", "[f_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_abs", "[f_abs]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FABS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_cos", "[f_a_cos]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FACOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_cosh", "[f_a_cosh]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FACOSH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_align", "[f_align]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_aligned", "[f_aligned]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_log", "[f_a_log]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FALOG", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FALSE", "[FALSE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FALSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_sine", "[f_a_sine]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FASIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_cinch", "[f_a_cinch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FASINH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_tan", "[f_a_tan]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FATAN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_tan_two", "[f_a_tan_two]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FATAN2", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_a_tan_h", "[f_a_tan_h]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FATANH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_constant", "[f_constant]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FCONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_cos", "[f_cos]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FCOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_cosh", "[f_cosh]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FCOSH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_depth", "[f_depth]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FDEPTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_drop", "[f_drop]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FDROP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_dupe", "[f_dupe]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FDUP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_e_dot", "[f_e_dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FE.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_e_x_p", "[f_e_x_p]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FEXP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_e_x_p_m_one", "[f_e_x_p_m_one]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FEXPM1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_field_colon", "[f_field_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("field_colon", "[field_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FILE_POSITION", "[FILE_POSITION]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FILE-POSITION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FILE_SIZE", "[FILE_SIZE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FILE-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FILE_STATUS", "[FILE_STATUS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FILE-STATUS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FILL", "[FILL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FILL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FIND", "[FIND]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FIND", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_literal", "[f_literal]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_l_n", "[f_l_n]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_l_n_p_one", "[f_l_n_p_one]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLNP1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("float_plus", "[float_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FLOATS", "[FLOATS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_log", "[f_log]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLOT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FLOOR", "[FLOOR]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLOOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FLUSH", "[FLUSH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLUSH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FLUSH_FILE", "[FLUSH_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FLUSH-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_m_slash_mod", "[f_m_slash_mod]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FM/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_max", "[f_max]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FMAX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_min", "[f_min]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FMIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_negate", "[f_negate]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FNEGATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FORGET", "[FORGET]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FORGET", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FORTH", "[FORTH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FORTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FORTH_WORDLIST", "[FORTH_WORDLIST]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FORTH-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_over", "[f_over]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FOVER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("FREE", "[FREE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FREE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_rote", "[f_rote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FROT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_round", "[f_round]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FROUND", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_s_dot", "[f_s_dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FS.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_sine", "[f_sine]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FSIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_sine_cos", "[f_sine_cos]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FSINCOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_cinch", "[f_cinch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FSINH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_square_root", "[f_square_root]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FSQRT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_swap", "[f_swap]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FSWAP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_tan", "[f_tan]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FTAN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_tan_h", "[f_tan_h]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FTANH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_trunc", "[f_trunc]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FTRUNC", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_value", "[f_value]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FVALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_variable", "[f_variable]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "FVARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("f_proximate", "[f_proximate]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "F~", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("GET_CURRENT", "[GET_CURRENT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "GET-CURRENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("GET_ORDER", "[GET_ORDER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "GET-ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("HERE", "[HERE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "HERE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("HEX", "[HEX]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "HEX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("HOLD", "[HOLD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "HOLD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("HOLDS", "[HOLDS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "HOLDS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("I", "[I]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "I", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("IF", "[IF]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "IF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("IMMEDIATE", "[IMMEDIATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "IMMEDIATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("INCLUDE", "[INCLUDE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "INCLUDE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("INCLUDE_FILE", "[INCLUDE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "INCLUDE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("INCLUDED", "[INCLUDED]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "INCLUDED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("INVERT", "[INVERT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "INVERT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("IS", "[IS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "IS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("J", "[J]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "J", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_ALT_MASK", "[K_ALT_MASK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-ALT-MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_CTRL_MASK", "[K_CTRL_MASK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-CTRL-MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_DELETE", "[K_DELETE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-DELETE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_DOWN", "[K_DOWN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-DOWN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_END", "[K_END]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-END", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_1", "[k_f_1]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_10", "[k_f_10]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F10", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_11", "[k_f_11]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F11", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_12", "[k_f_12]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F12", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_2", "[k_f_2]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F2", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_3", "[k_f_3]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F3", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_4", "[k_f_4]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F4", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_5", "[k_f_5]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F5", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_6", "[k_f_6]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F6", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_7", "[k_f_7]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F7", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_8", "[k_f_8]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F8", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("k_f_9", "[k_f_9]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K-F9", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_HOME", "[K_HOME]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_HOME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_INSERT", "[K_INSERT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_INSERT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_LEFT", "[K_LEFT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_LEFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_NEXT", "[K_NEXT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_NEXT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_PRIOR", "[K_PRIOR]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_PRIOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_RIGHT", "[K_RIGHT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_RIGHT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_SHIFT_MASK", "[K_SHIFT_MASK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_SHIFT_MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("K_UP", "[K_UP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "K_UP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("KEY", "[KEY]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "KEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("key_question", "[key_question]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "KEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("LEAVE", "[LEAVE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LEAVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("LIST", "[LIST]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("LITERAL", "[LITERAL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("LOAD", "[LOAD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LOAD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("locals_bar", "[locals_bar]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LOCALS|", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("LOOP", "[LOOP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LOOP", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");

    forth_destroyContext(ctx);
}

TEST_CASE("l_shift", "[l_shift]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "LSHIFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("m_star", "[m_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "M*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("m_star_slash", "[m_star_slash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "M*/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("m_plus", "[m_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "M+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MARKER", "[MARKER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MARKER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MAX", "[MAX]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MAX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MIN", "[MIN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MOD", "[MOD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MOVE", "[MOVE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MOVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("MS", "[MS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "MS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("n_to_r", "[n_to_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "N>R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("name_to_compile", "[name_to_compile]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NAME>COMPILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("name_to_interpret", "[name_to_interpret]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NAME>INTERPRET", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("name_to_string", "[name_to_string]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NAME>STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("NEGATE", "[NEGATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NEGATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("NIP", "[NIP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NIP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("n_r_from", "[n_r_from]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "NR>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("OF", "[OF]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "OF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ONLY", "[ONLY]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ONLY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("OPEN_FILE", "[OPEN_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "OPEN-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("OR", "[OR]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "OR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ORDER", "[ORDER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("OVER", "[OVER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "OVER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PAD", "[PAD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PAD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PAGE", "[PAGE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PAGE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PARSE", "[PARSE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PARSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PARSE_NAME", "[PARSE_NAME]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PARSE-NAME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PICK", "[PICK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PICK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("POSTPONE", "[POSTPONE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "POSTPONE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PRECISION", "[PRECISION]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PRECISION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("PREVIOUS", "[PREVIOUS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "PREVIOUS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("QUIT", "[QUIT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "QUIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("r_o", "[r_o]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "R/O", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("r_w", "[r_w]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "R/W", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("r_from", "[r_from]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "R>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("r_fetch", "[r_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "R@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("READ_FILE", "[READ_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "READ-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("READ_LINE", "[READ_LINE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "READ-LINE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("RECURSE", "[RECURSE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RECURSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REFILL", "[REFILL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REFILL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("RENAME_FILE", "[RENAME_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RENAME_FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REPEAT", "[REPEAT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REPEAT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REPLACES", "[REPLACES]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REPLACES", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REPOSITION_FILE", "[REPOSITION_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REPOSITION-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REPRESENT", "[REPRESENT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REPRESENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REQUIRE", "[REQUIRE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REQUIRE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("REQUIRED", "[REQUIRED]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "REQUIRED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("RESIZE", "[RESIZE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RESIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("RESIZE_FILE", "[RESIZE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RESIZE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("RESTORE_INPUT", "[RESTORE_INPUT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RESTORE-INPUT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("ROLL", "[ROLL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ROLL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("rote", "[rote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "ROT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("r_shift", "[r_shift]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "RSHIFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_quote", "[s_quote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "S\"", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_to_d", "[s_to_d]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "S>D", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_to_F", "[s_to_F]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "S>F", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SAVE_BUFFERS", "[SAVE_BUFFERS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SAVE-BUFFERS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SAVE_INPUT", "[SAVE_INPUT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SAVE-INPUT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_c_r", "[s_c_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SCR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SEARCH", "[SEARCH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SEARCH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SEARCH_WORDLIST", "[SEARCH_WORDLIST]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SEARCH-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SEE", "[SEE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SEE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SET_CURRENT", "[SET_CURRENT]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SET-CURRENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SET_ORDER", "[SET_ORDER]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SET-ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SET_PRECISION", "[SET_PRECISION]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SET-PRECISION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_f_store", "[s_f_store]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SF!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_f_fetch", "[s_f_fetch]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SF@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_f_align", "[s_f_align]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SFALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_f_aligned", "[s_f_aligned]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SFALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_f_field_colon", "[s_f_field_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SFFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_float_plus", "[s_float_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SFLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_floats", "[s_floats]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SFLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SIGN", "[SIGN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SLITERAL", "[SLITERAL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SLITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_m_slash_rem", "[s_m_slash_rem]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SM/REM", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SOURCE", "[SOURCE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SOURCE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("source_i_d", "[source_i_d]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SOURCE_ID", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SPACE", "[SPACE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SPACE", FORTH_SUCCESS, {}, " ");

    forth_destroyContext(ctx);
}

TEST_CASE("SPACES", "[SPACES]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "0 SPACES", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "-1 SPACES", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "15 SPACES", FORTH_SUCCESS, {}, "               ");

    forth_destroyContext(ctx);
}

TEST_CASE("STATE", "[STATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "STATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SUBSTITURE", "[SUBSTITURE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SUBSTITURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SWAP", "[SWAP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SWAP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("SYNONYM", "[SYNONYM]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "SYNONYM", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("s_backslash_quote", "[s_backslash_quote]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "S\\", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("THEN", "[THEN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "THEN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("THROW", "[THROW]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "THROW", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("THRU", "[THRU]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "THRU", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("time_and_date", "[time_and_date]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TIME&DATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("TO", "[TO]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("TRAVERSE_WORDLIST", "[TRAVERSE_WORDLIST]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TRAVERSE-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("TRUE", "[TRUE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TRUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("TUCK", "[TUCK]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TUCK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("TYPE", "[TYPE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "TYPE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_dot", "[u_dot]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "U.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_dot_r", "[u_dot_r]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "U.R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_less_than", "[u_less_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "U<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_greater_than", "[u_greater_than]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "U>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_m_star", "[u_m_star]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UM*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("u_m_slash_mod", "[u_m_slash_mod]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UM/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("UNESCAPE", "[UNESCAPE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UNESCAPE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("UNLOOP", "[UNLOOP]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UNLOOP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("UNTIL", "[UNTIL]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UNTIL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("UNUSED", "[UNUSED]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UNUSED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("UPDATE", "[UPDATE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "UPDATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("VALUE", "[VALUE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "VALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("VARIABLE", "[VARIABLE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "VARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("w_o", "[w_o]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "W/O", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WHILE", "[WHILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WHILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WITHIN", "[WITHIN]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WITHIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WORD", "[WORD]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WORD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WORDLIST", "[WORDLIST]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WORDS", "[WORDS]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WORDS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WRITE_FILE", "[WRITE_FILE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WRITE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("WRITE_LINE", "[WRITE_LINE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "WRITE-LINE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("X_SIZE", "[X_SIZE]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "X-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("X_WIDTH", "[X_WIDTH]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "X-WIDTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_store_plus", "[x_c_store_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC!+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_store_plus_query", "[x_c_store_plus_query]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC!+?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_comma", "[x_c_comma]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_size", "[x_c_size]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_width", "[x_c_width]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC-WIDTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_c_fetch_plus", "[x_c_fetch_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XC@+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_char_plus", "[x_char_plus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XCHAR+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_char_minus", "[x_char_minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XCHAR-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_emit", "[x_emit]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XEMIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_hold", "[x_hold]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XHOLD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_key", "[x_key]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_key_query", "[x_key_query]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XKEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_or", "[x_or]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "XOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("x_string_minus", "[x_string_minus]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "X\\STRING-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("left_bracket", "[left_bracket]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_tick", "[bracket_tick]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[']", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_char", "[bracket_char]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[CHAR]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_compile", "[bracket_compile]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[COMPILE]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_defined", "[bracket_defined]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[DEFINED]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_else", "[bracket_else]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[ELSE]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_if", "[bracket_if]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[IF]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_then", "[bracket_then]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[THEN]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("bracket_undefined", "[bracket_undefined]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "[UNDEFINED]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("backslash", "[backslash]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "\\", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("right_bracket", "[right_bracket]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}

TEST_CASE("brace_colon", "[brace_colon]")
{
    ForthContext* ctx = forth_createContext();

    evalTestSection(ctx, "{:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroyContext(ctx);
}
