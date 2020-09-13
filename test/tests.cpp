/*
MIT License

Copyright (c) 2020 David St-Louis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

#define FORTH_IMPLEMENT
#include <forth/forth.h>

#include "eval_check.h"

TEST_CASE("forth_context", "[forth_context]")
{
    SECTION("Not enough memory for standard WORDs")
    {
        forth_context* ctx = forth_create_context(100);
        REQUIRE_FALSE(ctx);
    }

    SECTION("Dictionnary too small for standard WORDs")
    {
        forth_context* ctx = forth_create_context(-1, -1, -1, 100);
        REQUIRE_FALSE(ctx);
    }

    SECTION("Stack overflow")
    {
        forth_context* ctx = forth_create_context(-1, 1);

        evalTestSection(ctx, "1", FORTH_SUCCESS, {1});
        evalTestSection(ctx, "1 2", FORTH_FAILURE, {}, "Stack overflow\n");

        forth_destroy_context(ctx);
    }
}

// Testing examples and exercices from the book titled "Starting FORTH"
TEST_CASE("Starting FORTH", "[StartingForth]")
{
    forth_context* ctx = forth_create_context();

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
    
    // Chapter 2
    evalTest(ctx, "17 5 + .", FORTH_SUCCESS, {}, "22 ");
    evalTest(ctx, "7 8 * .", FORTH_SUCCESS, {}, "56 ");
    evalTest(ctx, "21 4 / .", FORTH_SUCCESS, {}, "5 ");
    evalTest(ctx, "17 12 * 4 + .", FORTH_SUCCESS, {}, "208 ");
    evalTest(ctx, "3 9 + 4 6 + * .", FORTH_SUCCESS, {}, "120 ");
    evalTest(ctx, ": YARDS>IN   36 * ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": FT>IN   12 * ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "10 YARDS>IN .", FORTH_SUCCESS, {}, "360 ");
    evalTest(ctx, "2 FT>IN .", FORTH_SUCCESS, {}, "24 ");
    evalTest(ctx, ": YARDS   36 * ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": FEET   12 * ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": INCHES ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "10 YARDS 2 FEET + 9 INCHES + .", FORTH_SUCCESS, {}, "393 ");
    evalTest(ctx, ": YARD   YARDS ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": FOOT   FEET ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": INCH   INCHES ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "1 YARD  2 FEET +  1 INCH + .", FORTH_SUCCESS, {}, "61 ");
    evalTest(ctx, "2 YARDS  1 FOOT + .", FORTH_SUCCESS, {}, "84 ");
    evalTest(ctx, "17 20 + 132 + 3 + 9 + .", FORTH_SUCCESS, {}, "181 ");
    evalTest(ctx, "17 20 132 3 9 + + + + .", FORTH_SUCCESS, {}, "181 ");
    evalTest(ctx, ": 5#SUM   + + + + ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "17 20 132 3 9 5#SUM .", FORTH_SUCCESS, {}, "181 ");
    evalTest(ctx, ": FLIGHT-DISTANCE   + * ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "5 600 25 FLIGHT-DISTANCE .", FORTH_SUCCESS, {}, "3125 ");
    evalTest(ctx, "22 4 /MOD . . ", FORTH_SUCCESS, {}, "5 2 ");
    evalTest(ctx, ": QUARTERS   4 /MOD . .\" ONES AND \" . .\" QUARTERS \" ;", FORTH_SUCCESS, {});
    evalTest(ctx, "22 QUARTERS", FORTH_SUCCESS, {}, "5 ONES AND 2 QUARTERS ");
    evalTest(ctx, "22 4 MOD . ", FORTH_SUCCESS, {}, "2 ");
    evalTest(ctx, "1 2 . . ", FORTH_SUCCESS, {}, "2 1 ");
    evalTest(ctx, "1 2 SWAP . . ", FORTH_SUCCESS, {}, "1 2 ");
    evalTest(ctx, "2 10 4 - SWAP / .", FORTH_SUCCESS, {}, "3 ");
    //evalTest(ctx, ": MY.S CR 'S SO @ 2- DO I @ . -2 +LOOP ;", FORTH_SUCCESS, {});
    // ( 1.)
    evalTest(ctx, ": flip-3-items ( n1 n2 n3 -- n3 n2 n1) SWAP ROT ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 flip-3-items", FORTH_SUCCESS, {3, 2, 1});
    evalTest(ctx, ". . .", FORTH_SUCCESS, {}, "1 2 3 ");
    // ( 2.)
    evalTest(ctx, ": my-over ( n1 n2 -- n1 n2 n1) SWAP DUP ROT SWAP ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 my-over", FORTH_SUCCESS, {1, 2, 1});
    evalTest(ctx, ". . .", FORTH_SUCCESS, {}, "1 2 1 ");
    // ( 3.)
    evalTest(ctx, ": <ROT ( n1 n2 n3 -- n3 n1 n2) ROT ROT ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 <ROT", FORTH_SUCCESS, {3, 1, 2});
    evalTest(ctx, ". . .", FORTH_SUCCESS, {}, "2 1 3 ");
    // ( 4.)
    evalTest(ctx, ": 2.4 ( n -- result) DUP 1 + SWAP / ;", FORTH_SUCCESS, {});
    evalTest(ctx, "3 2.4 .", FORTH_SUCCESS, {}, "1 ");
    // ( 5.)
    evalTest(ctx, ": 2.5 ( x -- result) DUP 7 * 5 + * ;", FORTH_SUCCESS, {});
    evalTest(ctx, "3 2.5 .", FORTH_SUCCESS, {}, "78 ");
    // ( 6.)
    evalTest(ctx, ": 2.6 ( a b -- result) OVER 9 * SWAP - * ;", FORTH_SUCCESS, {});
    evalTest(ctx, "2 3 2.6 .", FORTH_SUCCESS, {}, "30 ");
    // ( F2.)
    evalTest(ctx, ": 4reverse ( n1 n2 n3 n4 -- n4 n3 n2 n1) SWAP 2SWAP SWAP ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 4 4reverse", FORTH_SUCCESS, {4, 3, 2, 1}, "");
    evalTest(ctx, "2DROP 2DROP", FORTH_SUCCESS, {}, "");
    // ( F3.)
    evalTest(ctx, ": 3DUP ( n1 n2 n3 -- n1 n2 n3 n1 n2 n3) DUP 2OVER ROT ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 3DUP", FORTH_SUCCESS, {1, 2, 3, 1, 2, 3}, "");
    evalTest(ctx, "2DROP 2DROP 2DROP", FORTH_SUCCESS, {}, "");
    // ( F4.)
    evalTest(ctx, ": 2.F4 ( c a b -- result) OVER + * + ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 2.F4 .", FORTH_SUCCESS, {}, "11 ");
    // ( F5.)
    evalTest(ctx, ": 2.F5 ( a b -- result) 2DUP - ROT ROT + / ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 2.F5 .", FORTH_SUCCESS, {}, "-1 ");
    // ( F6.)
    evalTest(ctx, ": CONVICTED-OF 0 ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": ARSON 10 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": HOMICIDE 20 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": BOOKMAKING 2 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": TAX-EVASION 5 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": WILL-SERVE . .\" years\" ;", FORTH_SUCCESS, {});
    evalTest(ctx, "CONVICTED-OF ARSON HOMICIDE TAX-EVASION", FORTH_SUCCESS, {35}, "");
    evalTest(ctx, "WILL-SERVE", FORTH_SUCCESS, {}, "35 years");
    // ( F7.)
    evalTest(ctx, ": EGG.CARTONS 12 /MOD . .\" cartons and \" . .\" left over\";", FORTH_SUCCESS, {});
    evalTest(ctx, "53 EGG.CARTONS", FORTH_SUCCESS, {}, "4 cartons and 5 left over");

    // Chapter 3
    // ... Editor stuff, not relevant

    // Chapter 4
    evalTest(ctx, ": ?FULL 12 = IF .\" It's full \" THEN ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "11 ?FULL", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "12 ?FULL", FORTH_SUCCESS, {}, "It's full ");
    evalTest(ctx, ": ?TOO-HOT 220 > IF .\" DANGER -- Reduce heat \" THEN  ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "290 ?TOO-HOT", FORTH_SUCCESS, {}, "DANGER -- Reduce heat ");
    evalTest(ctx, "130 ?TOO-HOT", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": EGGSIZE DUP 18 < IF .\" REJECT \"       ELSE"
                  "          DUP 21 < IF .\" SMALL \"        ELSE"
                  "          DUP 24 < IF .\" MEDIUM \"       ELSE"
                  "          DUP 27 < IF .\" LARGE \"        ELSE"
                  "          DUP 30 < IF .\" EXTRA LARGE \"  ELSE"
                  "                      .\" ERROR \"  "
                  "              THEN THEN THEN THEN THEN  DROP ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "16 EGGSIZE", FORTH_SUCCESS, {}, "REJECT ");
    evalTest(ctx, "20 EGGSIZE", FORTH_SUCCESS, {}, "SMALL ");
    evalTest(ctx, "22 EGGSIZE", FORTH_SUCCESS, {}, "MEDIUM ");
    evalTest(ctx, "25 EGGSIZE", FORTH_SUCCESS, {}, "LARGE ");
    evalTest(ctx, "28 EGGSIZE", FORTH_SUCCESS, {}, "EXTRA LARGE ");
    evalTest(ctx, "31 EGGSIZE", FORTH_SUCCESS, {}, "ERROR ");
    evalTest(ctx, "0 NOT .", FORTH_SUCCESS, {}, "-1 ");
    evalTest(ctx, "-1 NOT .", FORTH_SUCCESS, {}, "0 ");
    evalTest(ctx, ": VEGETABLE DUP 0< SWAP 10 MOD 0= + IF .\" ARTICHOKE \" THEN ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": ?DAY DUP 1 < SWAP 31 > + IF .\" NO WAY \" ELSE .\" THANK YOU \" THEN ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "-2 ?DAY", FORTH_SUCCESS, {}, "NO WAY ");
    evalTest(ctx, "0 ?DAY", FORTH_SUCCESS, {}, "NO WAY ");
    evalTest(ctx, "1 ?DAY", FORTH_SUCCESS, {}, "THANK YOU ");
    evalTest(ctx, "31 ?DAY", FORTH_SUCCESS, {}, "THANK YOU ");
    evalTest(ctx, "32 ?DAY", FORTH_SUCCESS, {}, "NO WAY ");
    evalTest(ctx, "39 ?DAY", FORTH_SUCCESS, {}, "NO WAY ");
    evalTest(ctx, "1 -1 + .", FORTH_SUCCESS, {}, "0 ");
    evalTest(ctx, ": BOXTEST ( length width height -- ) 6 > ROT 22 > ROT 19 > AND AND IF .\" BIG ENOUGH \" THEN ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "23 20 7 BOXTEST", FORTH_SUCCESS, {}, "BIG ENOUGH ");
    evalTest(ctx, ": /CHECK DUP 0= ABORT\" ZERO DENOMINATOR \" / ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "8 0 /CHECK", FORTH_FAILURE, {}, "ZERO DENOMINATOR ");
    evalTest(ctx, ": ENVELOPE /CHECK .\" THE ANSWER IS \" . ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "8 4 ENVELOPE", FORTH_SUCCESS, {}, "THE ANSWER IS 2 ");
    evalTest(ctx, "8 0 ENVELOPE", FORTH_FAILURE, {}, "ZERO DENOMINATOR ");

    // Chapter 5
    evalTest(ctx, ": DIFFERENCE - ABS ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "52 37 DIFFERENCE .", FORTH_SUCCESS, {}, "15 ");
    evalTest(ctx, "37 52 DIFFERENCE .", FORTH_SUCCESS, {}, "15 ");
    evalTest(ctx, ": COMMISSION 10 / 50 MIN ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "600 COMMISSION .", FORTH_SUCCESS, {}, "50 ");
    evalTest(ctx, "450 COMMISSION .", FORTH_SUCCESS, {}, "45 ");
    evalTest(ctx, "50 COMMISSION .", FORTH_SUCCESS, {}, "5 ");

    forth_destroy_context(ctx);
}

TEST_CASE("store", "[store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("number_sign", "[number_sign]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "#", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("number_sign_greater", "[number_sign_greater]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "#>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("number_sign_s", "[number_sign_s]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "#S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("tick", "[tick]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "'", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("paren", "[paren]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "(", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( )", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment )", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment)", FORTH_SUCCESS, {});
    evalTestSection(ctx, "( Comment) 4 5 +", FORTH_SUCCESS, {9});
    evalTestSection(ctx, "2 3 + ( Comment) 4 5 +", FORTH_SUCCESS, {5, 9});
    evalTestSection(ctx, ": foo ( n --  ) . ; 5 foo", FORTH_SUCCESS, {}, "5 ");

    forth_destroy_context(ctx);
}

TEST_CASE("paren_local_paren", "[paren_local_paren]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "(LOCAL)", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("star", "[star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "*", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 *", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 *", FORTH_SUCCESS, {1, 6});
    evalTestSection(ctx, "0 0 *", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0 *", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 *", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "2 1 *", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "3 3 *", FORTH_SUCCESS, {9});
    evalTestSection(ctx, "-3 3 *", FORTH_SUCCESS, {-9});
    evalTestSection(ctx, "3 -3 *", FORTH_SUCCESS, {-9});
    evalTestSection(ctx, "-3 -3 *", FORTH_SUCCESS, {9});

    forth_destroy_context(ctx);
}

TEST_CASE("star_slash", "[star_slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "*/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("star_slash_mod", "[star_slash_mod]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "*/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("plus", "[plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "+", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 +", FORTH_FAILURE, {}, "Stack underflow\n");
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

    forth_destroy_context(ctx);
}

TEST_CASE("plus_store", "[plus_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "+!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("plus_field", "[plus_field]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "+FIELD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("plus_loop", "[plus_loop]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "+LOOP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("plus_x_string", "[plus_x_string]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "+X/STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("comma", "[comma]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ",", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("minus", "[minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "-", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 -", FORTH_FAILURE, {}, "Stack underflow\n");
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

    forth_destroy_context(ctx);
}

TEST_CASE("dash_trailing", "[dash_trailing]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "-TRAILING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("dash_trailing_garbage", "[dash_trailing_garbage]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "-TRAILING-GARBAGE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("dot", "[dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ".", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 .", FORTH_SUCCESS, {}, "1 ");
    evalTestSection(ctx, "1 2 .", FORTH_SUCCESS, {1}, "2 ");
    evalTestSection(ctx, "1 . 2 . 3 . 4 5 6 . . .", FORTH_SUCCESS, {}, "1 2 3 6 5 4 ");

    forth_destroy_context(ctx);
}

TEST_CASE("dot_quote", "[dot_quote]")
{
    forth_context* ctx = forth_create_context();

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

    forth_destroy_context(ctx);
}

TEST_CASE("dot_paren", "[dot_paren]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ".(", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("dot_r", "[dot_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ".R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("dot_s", "[dot_s]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ".S", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "1 .S", FORTH_SUCCESS, {1}, "1 ");
    evalTestSection(ctx, "1 2 .S", FORTH_SUCCESS, {1, 2}, "1 2 ");
    evalTestSection(ctx, "1 2 3 .S", FORTH_SUCCESS, {1, 2, 3}, "1 2 3 ");
    evalTestSection(ctx, "1 2 3 4 .S", FORTH_SUCCESS, {1, 2, 3, 4}, "1 2 3 4 ");
    evalTestSection(ctx, "1 2 3 4 5 .S", FORTH_SUCCESS, {1, 2, 3, 4, 5}, "1 2 3 4 5 ");

    forth_destroy_context(ctx);
}

TEST_CASE("slash", "[slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "/", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 /", FORTH_FAILURE, {}, "Stack underflow\n");
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

    forth_destroy_context(ctx);
}

TEST_CASE("slash_mod", "[slash_mod]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "/MOD", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 /MOD", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 /MOD", FORTH_SUCCESS, {1, 2, 0});
    evalTestSection(ctx, "0 1 /MOD", FORTH_SUCCESS, {0, 0});
    evalTestSection(ctx, "1 1 /MOD", FORTH_SUCCESS, {0, 1});
    evalTestSection(ctx, "2 1 /MOD", FORTH_SUCCESS, {0, 2});
    evalTestSection(ctx, "-1 1 /MOD", FORTH_SUCCESS, {0, -1});
    evalTestSection(ctx, "-2 1 /MOD", FORTH_SUCCESS, {0, -2});
    evalTestSection(ctx, "0 -1 /MOD", FORTH_SUCCESS, {0, 0});
    evalTestSection(ctx, "1 -1 /MOD", FORTH_SUCCESS, {0, -1});
    evalTestSection(ctx, "2 -1 /MOD", FORTH_SUCCESS, {0, -2});
    evalTestSection(ctx, "-1 -1 /MOD", FORTH_SUCCESS, {0, 1});
    evalTestSection(ctx, "-2 -1 /MOD", FORTH_SUCCESS, {0, 2});
    evalTestSection(ctx, "2 2 /MOD", FORTH_SUCCESS, {0, 1});
    evalTestSection(ctx, "7 3 /MOD", FORTH_SUCCESS, {1, 2});
    evalTestSection(ctx, "7 -3 /MOD", FORTH_SUCCESS, {1, -3});
    evalTestSection(ctx, "-7 3 /MOD", FORTH_SUCCESS, {-1, -3});
    evalTestSection(ctx, "-7 -3 /MOD", FORTH_SUCCESS, {-1, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("slash_string", "[slash_string]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "/STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("zero_less", "[zero_less]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "0<", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0<", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 0<", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 0<", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0<", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 0<", FORTH_SUCCESS, {0});

    forth_destroy_context(ctx);
}

TEST_CASE("zero_not_equals", "[zero_not_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "0<>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0<>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0<>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0<>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0<>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 0<>", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("zero_equals", "[zero_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "0>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("zero_greater", "[zero_greater]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "0>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 0>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 0>", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0>", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("one_plus", "[one_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "1+", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 1+", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "1 2 1+", FORTH_SUCCESS, {1, 3});
    evalTestSection(ctx, "0 1+", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-1 1+", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1+", FORTH_SUCCESS, {2});

    forth_destroy_context(ctx);
}

TEST_CASE("one_minus", "[one_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "1-", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 1-", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 1-", FORTH_SUCCESS, {1, 1});
    evalTestSection(ctx, "0 1-", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 1-", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "1 1-", FORTH_SUCCESS, {0});

    forth_destroy_context(ctx);
}

TEST_CASE("two_plus", "[two_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2+", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2+", FORTH_SUCCESS, {3});
    evalTestSection(ctx, "1 2 2+", FORTH_SUCCESS, {1, 4});
    evalTestSection(ctx, "0 2+", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "-1 2+", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 2+", FORTH_SUCCESS, {3});

    forth_destroy_context(ctx);
}

TEST_CASE("two_minus", "[two_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2-", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2-", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 2-", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 2-", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "-1 2-", FORTH_SUCCESS, {-3});
    evalTestSection(ctx, "1 2-", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("two_store", "[two_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_star", "[two_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2*", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2*", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "1 2 2*", FORTH_SUCCESS, {1, 4});
    evalTestSection(ctx, "0 2*", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 2*", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "1 2*", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "4000 2*", FORTH_SUCCESS, {8000});

    forth_destroy_context(ctx);
}

TEST_CASE("two_slash", "[two_slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2/", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2/", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 2/", FORTH_SUCCESS, {1, 1});
    evalTestSection(ctx, "0 2/", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 2/", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2/", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "4000 2/", FORTH_SUCCESS, {2000});

    forth_destroy_context(ctx);
}

TEST_CASE("two_to_r", "[two_to_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2>R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_fetch", "[two_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_constant", "[two_constant]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2CONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_drop", "[two_drop]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2DROP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2DROP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 2DROP", FORTH_SUCCESS, {});
    evalTestSection(ctx, "1 2 3 2DROP", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 2 3 4 2DROP", FORTH_SUCCESS, {1, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("two_dupe", "[two_dupe]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2DUP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2DUP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 2DUP", FORTH_SUCCESS, {1, 2, 1, 2});
    evalTestSection(ctx, "1 2 3 2DUP", FORTH_SUCCESS, {1, 2, 3, 2, 3});
    evalTestSection(ctx, "1 2 3 4 2DUP", FORTH_SUCCESS, {1, 2, 3, 4, 3, 4});

    forth_destroy_context(ctx);
}

TEST_CASE("two_literal", "[two_literal]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2LITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_over", "[two_over]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 2OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 2OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 4 2OVER", FORTH_SUCCESS, {1, 2, 3, 4, 1, 2});
    evalTestSection(ctx, "1 2 3 4 5 2OVER", FORTH_SUCCESS, {1, 2, 3, 4, 5, 2, 3});
    evalTestSection(ctx, "1 2 3 4 5 6 2OVER", FORTH_SUCCESS, {1, 2, 3, 4, 5, 6, 3, 4});

    forth_destroy_context(ctx);
}

TEST_CASE("two_r_from", "[two_r_from]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2R>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_r_fetch", "[two_r_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2R@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_rote", "[two_rote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2ROT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_swap", "[two_swap]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 2SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 2SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 4 2SWAP", FORTH_SUCCESS, {3, 4, 1, 2});
    evalTestSection(ctx, "1 2 3 4 5 2SWAP", FORTH_SUCCESS, {1, 4, 5, 2, 3});
    evalTestSection(ctx, "1 2 3 4 5 6 2SWAP", FORTH_SUCCESS, {1, 2, 5, 6, 3, 4});

    forth_destroy_context(ctx);
}

TEST_CASE("two_value", "[two_value]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2VALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("two_variable", "[two_variable]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "2VARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("colon", "[colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ": foo 100 + ; 1000 foo", FORTH_SUCCESS, {1100});
    evalTestSection(ctx, ": foo : bar ; ;", FORTH_FAILURE, {}, "Undefined word\n");
    evalTestSection(ctx, "foo foo1 foo foo2", FORTH_FAILURE, {}, "Undefined word\n");
    evalTestSection(ctx, ": GDX 123 ; : GDX GDX 234 ; GDX", FORTH_SUCCESS, {123, 234});

    //    forth::eval(ctx, ": print-stack-top  cr dup .\" The top of the stack is \" . cr .\" which looks like '\" dup emit .\" ' in ascii  \" ;");
    //forth::eval(ctx, "48 print-stack-top");

    //REQUIRE(LogCapturer::log == "\nThe top of the stack is 48 \nwhich looks like '0' in ascii  ");

    forth_destroy_context(ctx);
}

TEST_CASE("colon_no_name", "[colon_no_name]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ":NONAME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("semicolon", "[semicolon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ";", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("semicolon_code", "[semicolon_code]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ";CODE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("less_than", "[less_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "<", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 <", FORTH_FAILURE, {}, "Stack underflow\n");
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

    forth_destroy_context(ctx);
}

TEST_CASE("less_number_sign", "[less_number_sign]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "<#", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("not_equals", "[not_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "<>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 <>", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 <>", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 3 <>", FORTH_SUCCESS, {1, -1});
    evalTestSection(ctx, "0 0 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "1 1 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "-1 -1 <>", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "1 0 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "-1 0 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "0 1 <>", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "0 -1 <>", FORTH_SUCCESS, {-1}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("equals", "[equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "=", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 =", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 =", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 3 =", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "0 0 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "1 1 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "-1 -1 =", FORTH_SUCCESS, {-1}, "");
    evalTestSection(ctx, "1 0 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "-1 0 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "0 1 =", FORTH_SUCCESS, {0}, "");
    evalTestSection(ctx, "0 -1 =", FORTH_SUCCESS, {0}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("greater_than", "[greater_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 >", FORTH_FAILURE, {}, "Stack underflow\n");
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

    forth_destroy_context(ctx);
}

TEST_CASE("to_body", "[to_body]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">BODY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("to_float", "[to_float]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">FLOAT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("to_in", "[to_in]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">IN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("to_number", "[to_number]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">NUMBER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("to_r", "[to_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, ">R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("question", "[question]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("question_do", "[question_do]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "?DO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("question_dupe", "[question_dupe]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "?DUP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "-1 ?DUP", FORTH_SUCCESS, {-1, -1});
    evalTestSection(ctx, "0 ?DUP", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 ?DUP", FORTH_SUCCESS, {1, 1});
    evalTestSection(ctx, "1 2 ?DUP", FORTH_SUCCESS, {1, 2, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("fetch", "[fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ABORT", "[ABORT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ABORT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "0 ABORT", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "1 ABORT", FORTH_FAILURE, {}, "");
    evalTestSection(ctx, "1 0 ABORT", FORTH_SUCCESS, {1}, "");
    evalTestSection(ctx, "1 2 3 ABORT", FORTH_FAILURE, {}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("abort_quote", "[abort_quote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ABORT\"", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, "ABORT\" error message\n\"", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, ": foo ABORT\" error message\n\" ; 1 foo", FORTH_FAILURE, {}, "error message\n");
    evalTestSection(ctx, ": foo ABORT\" error message\n\" ; 0 foo", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, ": foo ABORT\" ; foo", FORTH_SUCCESS, {}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("abs", "[abs]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ABS", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 ABS", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 2 ABS", FORTH_SUCCESS, {1, 2});
    evalTestSection(ctx, "-1 ABS", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-13 ABS", FORTH_SUCCESS, {13});
    evalTestSection(ctx, "15 ABS", FORTH_SUCCESS, {15});

    forth_destroy_context(ctx);
}

TEST_CASE("ACCEPT", "[ACCEPT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ACCEPT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ACTION_OF", "[ACTION_OF]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ACTION-OF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("AGAIN", "[AGAIN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "AGAIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("AHEAD", "[AHEAD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "AHEAD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ALIGN", "[ALIGN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ALIGNED", "[ALIGNED]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ALLOCATE", "[ALLOCATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ALLOCATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ALLOT", "[ALLOT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ALLOT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ALSO", "[ALSO]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ALSO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("AND", "[AND]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "AND", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 AND", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 AND", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 3 AND", FORTH_SUCCESS, {1, 2});
    evalTestSection(ctx, "0 0 AND", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 1 AND", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 0 AND", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 AND", FORTH_SUCCESS, {1});

    forth_destroy_context(ctx);
}

TEST_CASE("ASSEMBLER", "[ASSEMBLER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ASSEMBLER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("at_x_y", "[at_x_y]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "AT-XY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BASE", "[BASE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BEGIN", "[BEGIN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BEGIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BEGIN_STRUCTURE", "[BEGIN_STRUCTURE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BEGIN-STRUCTURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BIN", "[BIN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("b_l", "[b_l]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BLANK", "[BLANK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BLANK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("b_l_k", "[b_l_k]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BLK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BLOCK", "[BLOCK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BLOCK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BUFFER", "[BUFFER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BUFFER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("buffer_colon", "[buffer_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BUFFER:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("BYE", "[BYE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "BYE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_store", "[c_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "C!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_quote", "[c_quote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "C\"", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_comma", "[c_comma]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "C,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_fetch", "[c_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "C@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CASE", "[CASE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CATCH", "[CATCH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CATCH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("cell_plus", "[cell_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CELL+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CELLS", "[CELLS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CELLS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_field_colon", "[c_field_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("char", "[char]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("char_plus", "[char_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CHAR+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("chars", "[chars]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CHARS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CLOSE_FILE", "[CLOSE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CLOSE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_move", "[c_move]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CMOVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_move_up", "[c_move_up]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CMOVE>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CODE", "[CODE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CODE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("COMPARE", "[COMPARE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "COMPARE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("compile_comma", "[compile_comma]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "COMPILE,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CONSTANT", "[CONSTANT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("COUNT", "[COUNT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "COUNT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_r", "[c_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CR", FORTH_SUCCESS, {}, "\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CREATE", "[CREATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CREATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("CREATE_FILE", "[CREATE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CREATE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_s_pick", "[c_s_pick]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CS-PICK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("c_s_roll", "[c_s_roll]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "CS-ROLL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_plus", "[d_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_minus", "[d_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_dot", "[d_dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_dot_r", "[d_dot_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D.R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_zero_less", "[d_zero_less]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D0<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_zero_equals", "[d_zero_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D0=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_two_star", "[d_two_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D2*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_two_slash", "[d_two_slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D2/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_less_than", "[d_less_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_equals", "[d_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_to_f", "[d_to_f]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D>F", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_to_s", "[d_to_s]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "D>S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_abs", "[d_abs]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DABS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DECIMAL", "[DECIMAL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DECIMAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DEFER", "[DEFER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DEFER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("defer_store", "[defer_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DEFER!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("defer_fetch", "[defer_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DEFER@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DEFINITIONS", "[DEFINITIONS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DEFINITIONS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DELETE_FILE", "[DELETE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DELETE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DEPTH", "[DEPTH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DEPTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_f_store", "[d_f_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DF!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_f_fetch", "[d_f_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DF@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_f_align", "[d_f_align]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DFALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_f_aligned", "[d_f_aligned]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DFALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_f_field_colon", "[d_f_field_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DFFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_float_plus", "[d_float_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DFLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_floats", "[d_floats]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DFLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_max", "[d_max]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DMAX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_min", "[d_min]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DMIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("d_negate", "[d_negate]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DNEGATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DO", "[DO]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DO", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP ; 5 STARS", FORTH_SUCCESS, {}, "*****");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP ; 5 STARS .\"  <- should see 5 stars\"", FORTH_SUCCESS, {}, "***** <- should see 5 stars");
    evalTestSection(ctx, ": STARS 0 DO 4 2 DO 42 EMIT LOOP LOOP ; 3 STARS", FORTH_SUCCESS, {}, "******");
    evalTestSection(ctx, ": STARS 0 DO 42 EMIT LOOP .\" Carrots\"; 5 STARS", FORTH_SUCCESS, {}, "*****Carrots");

    forth_destroy_context(ctx);
}

TEST_CASE("does", "[does]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DOES>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DROP", "[DROP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DROP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 DROP", FORTH_SUCCESS, {});
    evalTestSection(ctx, "1 2 DROP", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 2 3 DROP", FORTH_SUCCESS, {1, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("d_u_less", "[d_u_less]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DU<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("DUMP", "[DUMP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DUMP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("dupe", "[dupe]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "DUP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 DUP", FORTH_SUCCESS, {1, 1});
    evalTestSection(ctx, "1 2 DUP", FORTH_SUCCESS, {1, 2, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("EDITOR", "[EDITOR]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EDITOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("e_key", "[e_key]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("e_key_to_char", "[e_key_to_char]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EKEY>CHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("e_key_to_f_key", "[e_key_to_f_key]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EKEY>FKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("e_key_to_x_char", "[e_key_to_x_char]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EKEY>XCHAR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("e_key_question", "[e_key_question]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EKEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ELSE", "[ELSE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ELSE", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, "1 ELSE", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, ": foo IF ELSE THEN ; foo", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, ": foo IF ELSE THEN ; 1 foo", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, ": foo IF .\" true \" ELSE .\" false \" THEN .\" then\" ; 0 foo", FORTH_SUCCESS, {}, "false then");
    evalTestSection(ctx, ": foo IF .\" true \" ELSE .\" false \" THEN .\" then\" ; 1 foo", FORTH_SUCCESS, {}, "true then");
    evalTestSection(ctx, ": foo IF .\" if1 \" ELSE .\" else1 \" IF .\" if2 \" ELSE .\" else2 \" THEN .\" then2 \" THEN .\" then1\" ; 0 0 foo", FORTH_SUCCESS, {}, "else1 else2 then2 then1");
    evalTestSection(ctx, ": foo IF .\" if1 \" ELSE .\" else1 \" IF .\" if2 \" ELSE .\" else2 \" THEN .\" then2 \" THEN .\" then1\" ; 0 1 foo", FORTH_SUCCESS, {0}, "if1 then1");
    evalTestSection(ctx, ": foo IF .\" if1 \" ELSE .\" else1 \" IF .\" if2 \" ELSE .\" else2 \" THEN .\" then2 \" THEN .\" then1\" ; 1 0 foo", FORTH_SUCCESS, {}, "else1 if2 then2 then1");
    evalTestSection(ctx, ": foo IF .\" if1 \" ELSE .\" else1 \" IF .\" if2 \" ELSE .\" else2 \" THEN .\" then2 \" THEN .\" then1\" ; 1 1 foo", FORTH_SUCCESS, {1}, "if1 then1");

    forth_destroy_context(ctx);
}

TEST_CASE("EMIT", "[EMIT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EMIT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "65 EMIT EMIT", FORTH_FAILURE, {}, "AStack underflow\n");
    evalTestSection(ctx, "65 EMIT", FORTH_SUCCESS, {}, "A");
    evalTestSection(ctx, "66 65 EMIT", FORTH_SUCCESS, {66}, "A");
    evalTestSection(ctx, "33 119 111 87 EMIT EMIT EMIT EMIT", FORTH_SUCCESS, {}, "Wow!");
    evalTestSection(ctx, "87 EMIT 111 EMIT 119 EMIT 33 EMIT", FORTH_SUCCESS, {}, "Wow!");

    forth_destroy_context(ctx);
}

TEST_CASE("emit_question", "[emit_question]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EMIT?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("EMPTY", "[EMPTY]")
{
    forth_context* ctx = forth_create_context();

    evalTest(ctx, "EMPTY", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": foo 100 + ;", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "20 foo .", FORTH_SUCCESS, {}, "120 ");
    evalTest(ctx, "EMPTY", FORTH_SUCCESS, {}, "");
    evalTest(ctx, "42 foo .", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "EMIT", FORTH_FAILURE, {}, "Stack underflow\n");

    forth_destroy_context(ctx);
}

TEST_CASE("EMPTY_BUFFERS", "[EMPTY_BUFFERS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EMPTY-BUFFERS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("END_STRUCTURE", "[END_STRUCTURE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "END-STRUCTURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("end_case", "[end_case]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ENDCASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("end_of", "[end_of]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ENDOF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("environment_query", "[environment_query]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ENVIRONMENT?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ERASE", "[ERASE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ERASE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("EVALUATE", "[EVALUATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EVALUATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("EXECUTE", "[EXECUTE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EXECUTE", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "-5 EXECUTE", FORTH_FAILURE, {}, "Invalid memory address\n");
    evalTestSection(ctx, "10000 EXECUTE", FORTH_FAILURE, {}, "Invalid memory address\n");
    evalTestSection(ctx, "0 EXECUTE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("EXIT", "[EXIT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "EXIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_store", "[f_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_star", "[f_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_star_star", "[f_star_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F**", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_plus", "[f_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_minus", "[f_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_dot", "[f_dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_slash", "[f_slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_zero_less_than", "[f_zero_less_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F0<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_zero_equals", "[f_zero_equals]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F0=", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_to_d", "[f_to_d]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F>D", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("F_to_S", "[F_to_S]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F>S", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_fetch", "[f_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_abs", "[f_abs]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FABS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_cos", "[f_a_cos]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FACOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_cosh", "[f_a_cosh]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FACOSH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_align", "[f_align]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_aligned", "[f_aligned]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_log", "[f_a_log]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FALOG", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FALSE", "[FALSE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FALSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_sine", "[f_a_sine]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FASIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_cinch", "[f_a_cinch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FASINH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_tan", "[f_a_tan]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FATAN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_tan_two", "[f_a_tan_two]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FATAN2", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_a_tan_h", "[f_a_tan_h]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FATANH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_constant", "[f_constant]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FCONSTANT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_cos", "[f_cos]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FCOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_cosh", "[f_cosh]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FCOSH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_depth", "[f_depth]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FDEPTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_drop", "[f_drop]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FDROP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_dupe", "[f_dupe]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FDUP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_e_dot", "[f_e_dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FE.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_e_x_p", "[f_e_x_p]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FEXP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_e_x_p_m_one", "[f_e_x_p_m_one]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FEXPM1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_field_colon", "[f_field_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("field_colon", "[field_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FILE_POSITION", "[FILE_POSITION]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FILE-POSITION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FILE_SIZE", "[FILE_SIZE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FILE-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FILE_STATUS", "[FILE_STATUS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FILE-STATUS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FILL", "[FILL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FILL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FIND", "[FIND]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FIND", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_literal", "[f_literal]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_l_n", "[f_l_n]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_l_n_p_one", "[f_l_n_p_one]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLNP1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("float_plus", "[float_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FLOATS", "[FLOATS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_log", "[f_log]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLOT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FLOOR", "[FLOOR]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLOOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FLUSH", "[FLUSH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLUSH", FORTH_FAILURE, {}, "Undefined word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FLUSH_FILE", "[FLUSH_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FLUSH-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_m_slash_mod", "[f_m_slash_mod]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FM/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_max", "[f_max]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FMAX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_min", "[f_min]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FMIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_negate", "[f_negate]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FNEGATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FORGET", "[FORGET]")
{
    forth_context* ctx = forth_create_context();
    
    evalTest(ctx, "FORGET", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "FORGET ", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "FORGET word-that-doesn't-exist", FORTH_FAILURE, {}, "Undefined word\n");

    evalTest(ctx, ": 3DUP ( n1 n2 n3 -- n1 n2 n3 n1 n2 n3) DUP 2OVER ROT ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 3DUP", FORTH_SUCCESS, {1, 2, 3, 1, 2, 3}, "");
    evalTest(ctx, "2DROP 2DROP 2DROP", FORTH_SUCCESS, {}, "");
    evalTest(ctx, ": 2.F4 ( c a b -- result) OVER + * + ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 3 2.F4 .", FORTH_SUCCESS, {}, "11 ");
    evalTest(ctx, ": 2.F5 ( a b -- result) 2DUP - ROT ROT + / ;", FORTH_SUCCESS, {});
    evalTest(ctx, "1 2 2.F5 .", FORTH_SUCCESS, {}, "-1 ");
    evalTest(ctx, ": CONVICTED-OF 0 ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": ARSON 10 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": HOMICIDE 20 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": BOOKMAKING 2 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": TAX-EVASION 5 + ;", FORTH_SUCCESS, {});
    evalTest(ctx, ": WILL-SERVE . .\" years\" ;", FORTH_SUCCESS, {});
    evalTest(ctx, "CONVICTED-OF ARSON HOMICIDE TAX-EVASION", FORTH_SUCCESS, {35}, "");
    evalTest(ctx, "WILL-SERVE", FORTH_SUCCESS, {}, "35 years");

    evalTest(ctx, "FORGET CONVICTED-OF", FORTH_SUCCESS, {}, "");

    evalTest(ctx, "CONVICTED-OF", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "ARSON", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "HOMICIDE", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "BOOKMAKING", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "TAX-EVASION", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "WILL-SERVE", FORTH_FAILURE, {}, "Undefined word\n");
    evalTest(ctx, "1 2 2.F5 .", FORTH_SUCCESS, {}, "-1 ");
    evalTest(ctx, "1 2 3 3DUP", FORTH_SUCCESS, {1, 2, 3, 1, 2, 3}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("FORTH", "[FORTH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FORTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FORTH_WORDLIST", "[FORTH_WORDLIST]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FORTH-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_over", "[f_over]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FOVER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("FREE", "[FREE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FREE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_rote", "[f_rote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FROT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_round", "[f_round]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FROUND", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_s_dot", "[f_s_dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FS.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_sine", "[f_sine]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FSIN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_sine_cos", "[f_sine_cos]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FSINCOS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_cinch", "[f_cinch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FSINH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_square_root", "[f_square_root]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FSQRT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_swap", "[f_swap]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FSWAP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_tan", "[f_tan]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FTAN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_tan_h", "[f_tan_h]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FTANH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_trunc", "[f_trunc]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FTRUNC", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_value", "[f_value]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FVALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_variable", "[f_variable]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "FVARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("f_proximate", "[f_proximate]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "F~", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("GET_CURRENT", "[GET_CURRENT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "GET-CURRENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("GET_ORDER", "[GET_ORDER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "GET-ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("HERE", "[HERE]")
{
    forth_context* ctx = forth_create_context();

    auto mem_pointer = ctx->memory_pointer;
    REQUIRE(forth_eval(ctx, "HERE") == FORTH_SUCCESS);
    REQUIRE(ctx->stack_pointer == 1);
    REQUIRE(ctx->stack[0].int_value == mem_pointer);
    ctx->stack_pointer = 0;

    // Compiling a new WORD should increase the HERE cursor
    auto here_before = ctx->memory_pointer;
    REQUIRE(forth_eval(ctx, ": foo 100 + ;") == FORTH_SUCCESS);
    auto here_after = ctx->memory_pointer;
    REQUIRE(here_before < here_after);

    // Calling this simple function shouldn't touch memory
    REQUIRE(forth_eval(ctx, "10 foo") == FORTH_SUCCESS);
    REQUIRE(forth_eval(ctx, "HERE") == FORTH_SUCCESS);
    REQUIRE(ctx->stack_pointer == 2);
    REQUIRE(ctx->stack[1].int_value == here_after);
    ctx->stack_pointer = 0;

    forth_destroy_context(ctx);
}

TEST_CASE("HEX", "[HEX]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "HEX", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("HOLD", "[HOLD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "HOLD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("HOLDS", "[HOLDS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "HOLDS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("I", "[I]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "I", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("IF", "[IF]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "IF", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, "1 IF", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, ": foo IF THEN ; foo", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, ": foo IF THEN ; 1 foo", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, ": foo IF .\" true \" THEN .\" then\" ; 0 foo", FORTH_SUCCESS, {}, "then");
    evalTestSection(ctx, ": foo IF .\" true \" THEN .\" then\" ; 1 foo", FORTH_SUCCESS, {}, "true then");

    forth_destroy_context(ctx);
}

TEST_CASE("IMMEDIATE", "[IMMEDIATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "IMMEDIATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("INCLUDE", "[INCLUDE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "INCLUDE bad_filename.f", FORTH_FAILURE, {}, "No such file or directory\n");
    evalTestSection(ctx, "INCLUDE INCLUDE.f", FORTH_SUCCESS, {});
    evalTestSection(ctx, "INCLUDE INCLUDE.f foo", FORTH_SUCCESS, {}, "foo\n");
    evalTestSection(ctx, "INCLUDE INCLUDE_INCLUDE.f bar", FORTH_SUCCESS, {}, "foo\n");

    forth_destroy_context(ctx);
}

TEST_CASE("INCLUDE_FILE", "[INCLUDE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "INCLUDE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("INCLUDED", "[INCLUDED]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "INCLUDED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("INVERT", "[INVERT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "INVERT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("IS", "[IS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "IS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("J", "[J]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "J", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_ALT_MASK", "[K_ALT_MASK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-ALT-MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_CTRL_MASK", "[K_CTRL_MASK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-CTRL-MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_DELETE", "[K_DELETE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-DELETE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_DOWN", "[K_DOWN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-DOWN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_END", "[K_END]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-END", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_1", "[k_f_1]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F1", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_10", "[k_f_10]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F10", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_11", "[k_f_11]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F11", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_12", "[k_f_12]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F12", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_2", "[k_f_2]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F2", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_3", "[k_f_3]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F3", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_4", "[k_f_4]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F4", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_5", "[k_f_5]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F5", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_6", "[k_f_6]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F6", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_7", "[k_f_7]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F7", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_8", "[k_f_8]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F8", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("k_f_9", "[k_f_9]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K-F9", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_HOME", "[K_HOME]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_HOME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_INSERT", "[K_INSERT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_INSERT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_LEFT", "[K_LEFT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_LEFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_NEXT", "[K_NEXT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_NEXT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_PRIOR", "[K_PRIOR]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_PRIOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_RIGHT", "[K_RIGHT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_RIGHT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_SHIFT_MASK", "[K_SHIFT_MASK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_SHIFT_MASK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("K_UP", "[K_UP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "K_UP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("KEY", "[KEY]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "KEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("key_question", "[key_question]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "KEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("LEAVE", "[LEAVE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LEAVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("LIST", "[LIST]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LIST", FORTH_FAILURE, {}, "Undefined word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("LITERAL", "[LITERAL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("LOAD", "[LOAD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LOAD", FORTH_FAILURE, {}, "Undefined word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("locals_bar", "[locals_bar]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LOCALS|", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("LOOP", "[LOOP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LOOP", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("l_shift", "[l_shift]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "LSHIFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("m_star", "[m_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "M*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("m_star_slash", "[m_star_slash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "M*/", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("m_plus", "[m_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "M+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("MARKER", "[MARKER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MARKER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("MAX", "[MAX]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MAX", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 MAX", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 MAX", FORTH_SUCCESS, {1, 3});
    evalTestSection(ctx, "0 1 MAX", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 2 MAX", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "-1 0 MAX", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 0 MAX", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 MAX", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 0 MAX", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "2 1 MAX", FORTH_SUCCESS, {2});
    evalTestSection(ctx, "0 -1 MAX", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 -1 MAX", FORTH_SUCCESS, {1});

    forth_destroy_context(ctx);
}

TEST_CASE("MIN", "[MIN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MIN", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 MIN", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 MIN", FORTH_SUCCESS, {1, 2});
    evalTestSection(ctx, "0 1 MIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 MIN", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-1 0 MIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "0 0 MIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 MIN", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 0 MIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "2 1 MIN", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "0 -1 MIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 -1 MIN", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("MOD", "[MOD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MOD", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 MOD", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 MOD", FORTH_SUCCESS, {1, 2});
    evalTestSection(ctx, "0 1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "2 1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-2 1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 -1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 -1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "2 -1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-1 -1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "-2 -1 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "2 2 MOD", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "7 3 MOD", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "7 -3 MOD", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "-7 3 MOD", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-7 -3 MOD", FORTH_SUCCESS, {-1});

    forth_destroy_context(ctx);
}

TEST_CASE("MOVE", "[MOVE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MOVE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("MS", "[MS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "MS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("n_to_r", "[n_to_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "N>R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("name_to_compile", "[name_to_compile]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NAME>COMPILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("name_to_interpret", "[name_to_interpret]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NAME>INTERPRET", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("name_to_string", "[name_to_string]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NAME>STRING", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("NEGATE", "[NEGATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NEGATE", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 NEGATE", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "1 2 NEGATE", FORTH_SUCCESS, {1, -2});
    evalTestSection(ctx, "0 NEGATE", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 NEGATE", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 NEGATE", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "2 NEGATE", FORTH_SUCCESS, {-2});
    evalTestSection(ctx, "-2 NEGATE", FORTH_SUCCESS, {2});

    forth_destroy_context(ctx);
}

TEST_CASE("NIP", "[NIP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NIP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("n_r_from", "[n_r_from]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "NR>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("OF", "[OF]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "OF", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ONLY", "[ONLY]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ONLY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("OPEN_FILE", "[OPEN_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "OPEN-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("OR", "[OR]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "OR", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 OR", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 OR", FORTH_SUCCESS, {3});
    evalTestSection(ctx, "1 2 3 OR", FORTH_SUCCESS, {1, 3});
    evalTestSection(ctx, "0 0 OR", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 1 OR", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 0 OR", FORTH_SUCCESS, {1});
    evalTestSection(ctx, "1 1 OR", FORTH_SUCCESS, {1});


    forth_destroy_context(ctx);
}

TEST_CASE("ORDER", "[ORDER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("OVER", "[OVER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 OVER", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 OVER", FORTH_SUCCESS, {1, 2, 1});
    evalTestSection(ctx, "1 2 3 OVER", FORTH_SUCCESS, {1, 2, 3, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("PAD", "[PAD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PAD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PAGE", "[PAGE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PAGE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PARSE", "[PARSE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PARSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PARSE_NAME", "[PARSE_NAME]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PARSE-NAME", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PICK", "[PICK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PICK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("POSTPONE", "[POSTPONE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "POSTPONE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PRECISION", "[PRECISION]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PRECISION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("PREVIOUS", "[PREVIOUS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "PREVIOUS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("QUIT", "[QUIT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "QUIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("r_o", "[r_o]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "R/O", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("r_w", "[r_w]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "R/W", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("r_from", "[r_from]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "R>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("r_fetch", "[r_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "R@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("READ_FILE", "[READ_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "READ-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("READ_LINE", "[READ_LINE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "READ-LINE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("RECURSE", "[RECURSE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RECURSE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REFILL", "[REFILL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REFILL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("RENAME_FILE", "[RENAME_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RENAME_FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REPEAT", "[REPEAT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REPEAT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REPLACES", "[REPLACES]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REPLACES", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REPOSITION_FILE", "[REPOSITION_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REPOSITION-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REPRESENT", "[REPRESENT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REPRESENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REQUIRE", "[REQUIRE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REQUIRE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("REQUIRED", "[REQUIRED]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "REQUIRED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("RESIZE", "[RESIZE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RESIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("RESIZE_FILE", "[RESIZE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RESIZE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("RESTORE_INPUT", "[RESTORE_INPUT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RESTORE-INPUT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("ROLL", "[ROLL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ROLL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("rote", "[rote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "ROT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 ROT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 ROT", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 ROT", FORTH_SUCCESS, {2, 3, 1});
    evalTestSection(ctx, "1 2 3 4 ROT", FORTH_SUCCESS, {1, 3, 4, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("r_shift", "[r_shift]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "RSHIFT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_quote", "[s_quote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "S\"", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_to_d", "[s_to_d]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "S>D", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_to_F", "[s_to_F]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "S>F", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SAVE_BUFFERS", "[SAVE_BUFFERS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SAVE-BUFFERS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SAVE_INPUT", "[SAVE_INPUT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SAVE-INPUT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_c_r", "[s_c_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SCR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SEARCH", "[SEARCH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SEARCH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SEARCH_WORDLIST", "[SEARCH_WORDLIST]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SEARCH-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SEE", "[SEE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SEE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SET_CURRENT", "[SET_CURRENT]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SET-CURRENT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SET_ORDER", "[SET_ORDER]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SET-ORDER", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SET_PRECISION", "[SET_PRECISION]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SET-PRECISION", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_f_store", "[s_f_store]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SF!", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_f_fetch", "[s_f_fetch]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SF@", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_f_align", "[s_f_align]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SFALIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_f_aligned", "[s_f_aligned]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SFALIGNED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_f_field_colon", "[s_f_field_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SFFIELD:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_float_plus", "[s_float_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SFLOAT+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_floats", "[s_floats]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SFLOATS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SIGN", "[SIGN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SIGN", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SLITERAL", "[SLITERAL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SLITERAL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_m_slash_rem", "[s_m_slash_rem]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SM/REM", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SOURCE", "[SOURCE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SOURCE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("source_i_d", "[source_i_d]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SOURCE_ID", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SPACE", "[SPACE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SPACE", FORTH_SUCCESS, {}, " ");

    forth_destroy_context(ctx);
}

TEST_CASE("SPACES", "[SPACES]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "0 SPACES", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "-1 SPACES", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "15 SPACES", FORTH_SUCCESS, {}, "               ");

    forth_destroy_context(ctx);
}

TEST_CASE("STATE", "[STATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "STATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SUBSTITURE", "[SUBSTITURE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SUBSTITURE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("SWAP", "[SWAP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 SWAP", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 SWAP", FORTH_SUCCESS, {2, 1});
    evalTestSection(ctx, "1 2 3 SWAP", FORTH_SUCCESS, {1, 3, 2});

    forth_destroy_context(ctx);
}

TEST_CASE("SYNONYM", "[SYNONYM]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "SYNONYM", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("s_backslash_quote", "[s_backslash_quote]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "S\\", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("THEN", "[THEN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "THEN", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");
    evalTestSection(ctx, "1 THEN", FORTH_FAILURE, {}, "Interpreting a compile-only word\n");

    forth_destroy_context(ctx);
}

TEST_CASE("THROW", "[THROW]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "THROW", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 THROW", FORTH_FAILURE, {}, "");
    evalTestSection(ctx, "1 2 THROW", FORTH_FAILURE, {}, "");
    evalTestSection(ctx, "-1 THROW", FORTH_FAILURE, {}, "");
    evalTestSection(ctx, "0 THROW", FORTH_SUCCESS, {}, "");
    evalTestSection(ctx, "1 0 THROW", FORTH_SUCCESS, {1}, "");
    evalTestSection(ctx, "1 2 0 THROW", FORTH_SUCCESS, {1, 2}, "");

    forth_destroy_context(ctx);
}

TEST_CASE("THRU", "[THRU]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "THRU", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("time_and_date", "[time_and_date]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TIME&DATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("TO", "[TO]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TO", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("TRAVERSE_WORDLIST", "[TRAVERSE_WORDLIST]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TRAVERSE-WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("TRUE", "[TRUE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TRUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("TUCK", "[TUCK]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TUCK", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("TYPE", "[TYPE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "TYPE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_dot", "[u_dot]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "U.", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_dot_r", "[u_dot_r]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "U.R", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_less_than", "[u_less_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "U<", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_greater_than", "[u_greater_than]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "U>", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_m_star", "[u_m_star]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UM*", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("u_m_slash_mod", "[u_m_slash_mod]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UM/MOD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("UNESCAPE", "[UNESCAPE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UNESCAPE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("UNLOOP", "[UNLOOP]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UNLOOP", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("UNTIL", "[UNTIL]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UNTIL", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("UNUSED", "[UNUSED]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UNUSED", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("UPDATE", "[UPDATE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "UPDATE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("VALUE", "[VALUE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "VALUE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("VARIABLE", "[VARIABLE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "VARIABLE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("w_o", "[w_o]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "W/O", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WHILE", "[WHILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WHILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WITHIN", "[WITHIN]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WITHIN", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 WITHIN", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 WITHIN", FORTH_FAILURE, {}, "Stack underflow\n");
    evalTestSection(ctx, "1 2 3 WITHIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 2 3 4 WITHIN", FORTH_SUCCESS, {1, 0});
    evalTestSection(ctx, "2 1 3 WITHIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "2 3 1 WITHIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "3 1 2 WITHIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "1 1 3 WITHIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "3 1 3 WITHIN", FORTH_SUCCESS, {0});
    evalTestSection(ctx, "0 -3 3 WITHIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-3 -3 3 WITHIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-2 -3 -1 WITHIN", FORTH_SUCCESS, {-1});
    evalTestSection(ctx, "-1 -3 -1 WITHIN", FORTH_SUCCESS, {0});

    forth_destroy_context(ctx);
}

TEST_CASE("WORD", "[WORD]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WORD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WORDLIST", "[WORDLIST]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WORDLIST", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WORDS", "[WORDS]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WORDS", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WRITE_FILE", "[WRITE_FILE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WRITE-FILE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("WRITE_LINE", "[WRITE_LINE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "WRITE-LINE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("X_SIZE", "[X_SIZE]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "X-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("X_WIDTH", "[X_WIDTH]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "X-WIDTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_store_plus", "[x_c_store_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC!+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_store_plus_query", "[x_c_store_plus_query]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC!+?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_comma", "[x_c_comma]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC,", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_size", "[x_c_size]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC-SIZE", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_width", "[x_c_width]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC-WIDTH", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_c_fetch_plus", "[x_c_fetch_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XC@+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_char_plus", "[x_char_plus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XCHAR+", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_char_minus", "[x_char_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XCHAR-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_emit", "[x_emit]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XEMIT", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_hold", "[x_hold]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XHOLD", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_key", "[x_key]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XKEY", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_key_query", "[x_key_query]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XKEY?", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_or", "[x_or]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "XOR", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("x_string_minus", "[x_string_minus]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "X\\STRING-", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("left_bracket", "[left_bracket]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_tick", "[bracket_tick]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[']", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_char", "[bracket_char]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[CHAR]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_compile", "[bracket_compile]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[COMPILE]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_defined", "[bracket_defined]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[DEFINED]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_else", "[bracket_else]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[ELSE]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_if", "[bracket_if]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[IF]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_then", "[bracket_then]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[THEN]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("bracket_undefined", "[bracket_undefined]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "[UNDEFINED]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("backslash", "[backslash]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "\\", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("right_bracket", "[right_bracket]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "]", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}

TEST_CASE("brace_colon", "[brace_colon]")
{
    forth_context* ctx = forth_create_context();

    evalTestSection(ctx, "{:", FORTH_FAILURE, {}, "Unimplemented\n");

    forth_destroy_context(ctx);
}
