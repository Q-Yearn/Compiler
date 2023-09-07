#ifndef STABLE_H
#define STABLE_H

#include <unordered_map>
#include <variant>
#include <string>
#include <stack>
#include <vector>

using namespace std;

#define c_num 1
#define c_array 2
#define c_func_array 3

typedef struct{
  bool flag; //true为变量(包括函数),false为常量
  int value; //常量则为具体的值，变量则为标记
}Info; 


typedef struct{
  int tag; //数组的标记
  vector<int> dim;//数组的维度a
  vector<int> finish; //补全0后的值
  bool is_const;
}ArrayInfo;


typedef struct{
  int tag;
  int len;
  vector<int> dim;
}ParamInfo;//函数参数中的数组


class Area{
 public:
   int tag;
   Area *father=NULL;
   unordered_map<string, Info> table;
   unordered_map<string, ArrayInfo> array;
   unordered_map<string, ParamInfo> func_array; 

   void insert(string def, Info info); //插入一个变量或常量
   void merge(); //将当前函数形参加入符号表
   void merge_array(); //将当前函数数组形参加入符号表
   Info find(string def); //查找符号表是否存在变量或者常量
   ArrayInfo* find_array(string def); //查找符号表是否存在数组
   ParamInfo* find_func_array(string def);
   int find_category(string def);
   bool find_current(string def);
};

class Nest{
 public:
   bool is_num;
   int num;
   vector<Nest> struc;
};//声明时是单个值还是数组

extern Area *top;
extern vector<string> para; //函数形参
extern unordered_map<string, vector<int>> para_array; //函数数组形参
extern unordered_map<string, bool> ret_func;//是否使用函数返回值
extern int tag;
extern bool whether_load;

//计算表达式的值
int evaluateExpression(const string& exp);
//提取表达式
vector<string> extractExpressions(const string& input);

#endif