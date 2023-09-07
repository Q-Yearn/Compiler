#ifndef VIR_H
#define VIR_H

#include <iostream>
#include <memory>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "stable.h"
#include "koopa.h"


using namespace std;

extern unordered_map<string, bool> ret_func;
extern string Riscv;
extern string Koopa_IR;
class ris_ret {
  public:
    int val;
    bool reg;
    bool elemptr;
    string var;
  ris_ret(int v = 0, bool r = false, string str = "", bool addr = false){
      val = v; reg = r; elemptr = addr; var = str;
    }
};

// 函数声明
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
ris_ret Visit(const koopa_raw_value_t &value);

void Visit(const koopa_raw_global_alloc_t &gt, string name, koopa_raw_type_t ty);
void Visit(const koopa_raw_aggregate_t &at);
ris_ret Visit(const koopa_raw_binary_t &bi);
void Visit(const koopa_raw_jump_t &jt);
void Visit(const koopa_raw_branch_t &bt);
ris_ret Visit(const koopa_raw_get_elem_ptr_t &elem);
ris_ret Visit(const koopa_raw_get_ptr_t &ptr);
void Visit(const koopa_raw_store_t &st);
ris_ret Visit(const koopa_raw_load_t &lt);
void Visit(const koopa_raw_return_t &ret);
ris_ret Visit(const koopa_raw_call_t &c);
ris_ret Visit(const koopa_raw_func_arg_ref_t &ft);
int Visit(const koopa_raw_integer_t &val);

#endif