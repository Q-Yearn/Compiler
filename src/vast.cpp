#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <stack>
#include "ast.h"
#include "stable.h"

using namespace std;

int koopa_reg=0;
unordered_map<BaseAST*, int> block_begin;
unordered_map<int, int> block_end; //负的说明不用jump
int block_num = 1; //基本块的序号，防止块名字重复
int block_now = 0; //当前所在基本块
stack<int> while_entry; //当前基本块的while入口
stack<int> while_end; //当前基本块的while出口


bool block_begin_exit(BaseAST* ptr)
{
   if(ptr==NULL) return false;
   return block_begin.find(ptr)!=block_begin.end();
}

void block_begin_insert(BaseAST* ptr,int num)
{
  if(ptr==NULL)
    return;
  block_begin.insert(make_pair(ptr,num));
}

void Bind(int begin,int end)
{
  block_end.insert(make_pair(begin,end));
}

void Inherit(int predecessor, int successor)
{
  auto peace = block_end.find(predecessor);
  int asset = peace->second;
  block_end[predecessor] = -1;

  // 不是新开辟的next就不用继承 新开辟的就得继承
  if(successor == asset)
    return;
  block_end.insert(make_pair(successor, asset));
}

void Branch(string cond, int then_block, int else_block, int end)
{
  stringstream ss;
  ss << "  br " << cond << ", %_" << then_block << ", %_"  << else_block << endl;
  Koopa_IR+=ss.str();
  print_block_begin(then_block);
}

void Branch_(string cond, int then_block, int end)
{
  stringstream ss;
  ss << "  br " << cond << ", %_" << then_block << ", %_" << end << endl;
  Koopa_IR+=ss.str();
  print_block_begin(then_block);
}

void Jump(int block)
{
  auto it=block_end.find(block);
  if(it->second<0) return;
  stringstream ss;
  ss << "  jump %_" << it->second << endl;
  Koopa_IR+=ss.str();
  it->second=-1;
}


void print_block_begin(int num)
{
  stringstream ss;
  ss << endl << "%_" << num << ":" << endl;
  Koopa_IR+=ss.str();
}


/******************************************************/
//以下三个函数按规定完成数组初始化(适时补0)

//计算数组的大小
int Compute_len(vector<int> boundary)
{
   int len=1;
   for(int i=0;i<boundary.size();i++) len*=boundary[i];
   return len;
}

//规范输入单值个数一定和某个维度对齐，判断是哪个维度
int Align(vector<int> boundary, int num_sum, int end)
{
  int len=boundary.size();
  int i=1,j=len-1;
  vector<int> tmp;
  tmp.push_back(end);
  //如果数组长度为5*4*3 那么tmp的值分别为3 3*4 3*4*5
  for(;i<len;i++)
    tmp.push_back(tmp[i-1]*boundary[len-i-1]);
  for(;j>=0;j--)
  {
    if(num_sum%tmp[j]==0)
    {
      if(j==len-1) return 1;
      break;
    }
  }
  return len-1-j;
}

void CompleteInit(Nest init, vector<int> boundary, ArrayInfo *tmp)
{
  int num_sum=0,should_sum=Compute_len(boundary);
  int end=boundary[boundary.size()-1]; //最后一维
  auto list=init.struc;
  for(int i=0;i<list.size();i++)
  {
    if(list[i].is_num)
    {
      num_sum++;
      tmp->finish.push_back(list[i].num);
    }
    else
    {
      vector<int> new_boundary;
      new_boundary.assign(boundary.begin()+Align(boundary,num_sum,end),boundary.end());
      num_sum+=Compute_len(new_boundary);
      CompleteInit(list[i],new_boundary,tmp);
    }
  }

  // 最后对齐我负责的boundary
  for(; num_sum < should_sum; num_sum++)
    tmp->finish.push_back(0);

}
/******************************************************/ 


/*****************************************************/
//以下两个函数完成全局数组的声明定义
vector<int> init_tmp_global;
int init_where_global=0;

//迭代输出大括号的内容
void global_assist(vector<int> boundary)
{
  stringstream ss;
  if(boundary.size()==1)
  {
    Koopa_IR+=to_string(init_tmp_global[init_where_global++]);
    for(int i=1;i<boundary[0];i++)
    {
      ss.str("");
      ss << ", " << init_tmp_global[init_where_global++];
      Koopa_IR+=ss.str();
    }
    return;
  }

  vector<int> tmp;
  tmp.assign(boundary.begin() + 1, boundary.end());
  Koopa_IR += "{";
  global_assist(tmp);
  Koopa_IR += "}";
  for(int i = 1; i < boundary[0]; i++)
  {
    Koopa_IR += ", {";
    global_assist(tmp);
    Koopa_IR += "}";
  }
}

void GlobalArray(string ident, vector<int> init, vector<int> boundary)
{
  int dim_num = boundary.size();
  stringstream ss;
  ss << "global @" << ident << "_0 = alloc ";
  for(int i = 0; i < dim_num; i++)
    ss << "[";
  ss << "i32";
  for(int i = dim_num-1; i >= 0; i--)
    ss <<  ", " << boundary[i] << "]";
  ss << ", ";
  Koopa_IR+=ss.str();

  int tmp = init.size();
  if(tmp == 0)
    Koopa_IR += "zeroinit\n";
  else{
    // 清零
    init_tmp_global.clear();
    init_tmp_global.assign(init.begin(), init.end());
    init_where_global = 0;

    Koopa_IR += "{";
    global_assist(boundary);
    Koopa_IR += "}\n";
  }
}
/*****************************************************/


/*****************************************************/
//以下三个函数完成局部数组的声明定义
vector<int> init_tmp;
int init_where = 0;
bool is_zero = false;

//迭代将每个元素存入数组中
void Getelemptr(string ident, vector<int> boundary)
{
  stringstream ss;
  if(boundary.size() == 1)
  {
    for(int i = 0; i < boundary[0]; i++){
      ss.str("");
      ss <<  "  %" << koopa_reg++ << " = getelemptr " << ident << ", " << i << endl;
      Koopa_IR+=ss.str();
      if(is_zero)
      {
        ss.str("");
        ss << "  store 0, %" << koopa_reg-1 << endl;
        Koopa_IR+=ss.str();
      }
      else
      {
        ss.str("");
        ss << "  store " << init_tmp[init_where++] << ", %" << koopa_reg-1 << endl;
        Koopa_IR+=ss.str();
      }
    }
    return;
  }

  vector<int> tmp;
  tmp.assign(boundary.begin() + 1, boundary.end());
  for(int i = 0; i < boundary[0]; i++)
  {
    int tmp_reg = koopa_reg;
    ss.str("");
    ss << "  %" << koopa_reg++ << " = getelemptr " << ident + ", " << i << endl;
    Koopa_IR+=ss.str();
    Getelemptr("%" + to_string(tmp_reg), tmp);
  }
} 

string Getelemptr_only(string ident, vector<string> offset)
{
  stringstream ss;
  if(offset.size() != 0)
  {
    ss << "  %" << koopa_reg++ << " = getelemptr " << ident << ", " << offset[0] << endl;
    Koopa_IR+=ss.str();
    for(int i = 1; i < offset.size(); i++)
    {
      ss.str("");
      ss << "  %" << koopa_reg << " = getelemptr %" << koopa_reg-1 << ", " << offset[i] << endl;
      Koopa_IR+=ss.str();
      koopa_reg++;
    }
    return "%" + to_string(koopa_reg-1);
  }
  return ident;
}

void AllocArray(string ident, vector<int> init, vector<int> boundary)
{
  int dim_num = boundary.size();
  stringstream ss;
  ss << "  @" << ident << " = alloc ";
  for(int i = 0; i < dim_num; i++)
    ss << "[";
  ss << "i32";
  for(int i = dim_num-1; i >= 0; i--)
    ss << ", " << boundary[i] << "]";
  ss << endl;
  Koopa_IR+=ss.str();

  int tmp = init.size();
  if(tmp == 0)
  {
    // 清零
    is_zero = true;
  }
  else
  {
    // 清零
    is_zero = false;
    init_tmp.clear();
    init_tmp.assign(init.begin(), init.end());
    init_where = 0;
  }
  Getelemptr("@" + ident, boundary);
}
/*****************************************************/


void Alloc(string dst)
{
  stringstream ss;
  ss << "  @" << dst << " = alloc i32" << endl;
  Koopa_IR+=ss.str();
}

string Load(string ident)
{
  stringstream ss;
  ss << "  %" << koopa_reg++ << " = load @" << ident << endl;
  Koopa_IR+=ss.str();
  return "%" + to_string(koopa_reg-1);
}

void Store(string dst,string src)
{
  stringstream ss;
  ss << "  store " << src << ", @" << dst << endl;
  Koopa_IR+=ss.str();
}

void Global(string ident,int value)
{
  stringstream ss;
  ss << "global @" << ident << "_0 = alloc i32, ";
  Koopa_IR+=ss.str();
  if(value==0)
    Koopa_IR+="zeroinit\n";
  else
  {
    Koopa_IR+=to_string(value);
    Koopa_IR+="\n";
  }

}

//疑惑
map<bool, string> P = {{true, " %"}, {false, " "}};
map<string, string> K = {{"!", "eq"}, {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"},
{"|", "or"}, {"&", "and"}, {"!=", "ne"}, {"==", "eq"}, 
{">", "gt"}, {"<", "lt"}, {">=", "ge"}, {"<=", "le"}};

koo_ret koo_parse(string str)
{
  koo_ret ret;
  if(str[0] == '%')
  {
    ret.reg = true;
    str.erase(str.begin());
  }
  else
    ret.reg = false;
  ret.val = atoi(str.c_str());
  return ret;
}

string koo_binary(koo_ret l, koo_ret r, string op)
{
    stringstream ss;
    ss << "  %" << koopa_reg++ << " = " << K[op] 
    << P[l.reg] << l.val << "," << P[r.reg] << r.val << endl;
    Koopa_IR+=ss.str();
    return "%" + to_string(koopa_reg-1);
}