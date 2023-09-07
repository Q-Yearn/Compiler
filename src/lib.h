#ifndef LIB_H
#define LIB_H
#include <string>

using namespace std;

const string lib_decl= "decl @getint(): i32\n"\
                       "decl @getch(): i32\n"\
                       "decl @getarray(*i32): i32\n"\
                       "decl @putint(i32)\n"\
                       "decl @putch(i32)\n"\
                       "decl @putarray(i32, *i32)\n"\
                       "decl @starttime()\n"\
                       "decl @stoptime()\n";

const string lib_func[8] = {"getint", "getch", "getarray", 
                            "putint", "putch", "putarray", 
                            "starttime", "stoptime"};
#endif