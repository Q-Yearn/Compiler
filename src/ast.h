#ifndef AST_H
#define AST_H

#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include "stable.h"
#include "lib.h"

using namespace std;

//库函数
extern const string lib_decl;
extern const string lib_func[8] ;

//产生的字符串形式的Koopa IR
extern string Koopa_IR;
//Koopa_IR中的"寄存器"个数
extern int koopa_reg;
//疑惑
class koo_ret 
{
  public:
    int val;
    bool reg;
    koo_ret(int v=0, bool r=false)
    {
      val=v; reg=r;
    }
};
koo_ret koo_parse(string str);
string koo_binary(koo_ret l, koo_ret r, string op);
//疑惑:产生Koopa_IR的一些函数
void Alloc(string dst);
string Load(string ident);
void Store(string dst,string src);
void Global(string ident,int value);
// 所有 AST 的基类
class BaseAST 
{
 public:
  virtual ~BaseAST()=default;
  virtual string Dump() const=0;
  virtual string GenerateIR(BaseAST *next=NULL) const=0;
  //迭代获得数组初始化{}中的值
  virtual Nest ComputeArray() {return Nest{};}
};

//block相关定义
extern unordered_map<BaseAST*, int> block_begin;
extern unordered_map<int, int> block_end;
extern int block_num;
extern int block_now;
extern stack<int> while_entry;
extern stack<int> while_end;
//疑惑:函数定义
//看是否是最后一个块
bool block_begin_exit(BaseAST* ptr);
//疑惑
void block_begin_insert(BaseAST* ptr, int num);
//打印块结尾(如果要跳转)
void Jump(int block);
void Bind(int begin, int end);
void Inherit(int predecessor, int successor);
void Branch(string cond, int then_block, int else_block, int end);
void Branch_(string cond, int then_block, int end);
//打印块开头
void print_block_begin(int num);
//完成数组初始化(适时补0，辅助函数)
void CompleteInit(Nest init, vector<int> boundary, ArrayInfo *tmp);
string Getelemptr_only(string ident, vector<string> offset);
//完成全局数组声明初始化，Koopa_IR生成
void GlobalArray(string ident, vector<int> init, vector<int> boundary);
//完成局部数组声明初始化，Koopa_IR生成
void AllocArray(string ident, vector<int> init, vector<int> boundary);

//开始符号
class BeginAST : public BaseAST 
{
 public:
  // 用智能指针管理对象
  unique_ptr<BaseAST> compunit;

  // 转化为AST
  string Dump() const override 
  {
    return compunit->Dump();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    //全局作用域
    top=new Area();
    top->father=NULL;
    top->tag=0;

    //添加库函数
    Koopa_IR+=lib_decl;
    Koopa_IR+="\n";
    for(int i=0;i<8;i++)
    {
      top->insert(lib_func[i],Info{true,top->tag});
      //只有@getint()、@getch()、@getarray()会被使用返回值
      if(i<3)
        ret_func.insert(make_pair(lib_func[i],true));
      else
        ret_func.insert(make_pair(lib_func[i],false));
    }

    compunit->GenerateIR();

    delete top;
    return "";
  }

};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST 
{
 public:
  // 用智能指针管理对象
  unique_ptr<BaseAST> func_def;
  unique_ptr<BaseAST> decl;
  unique_ptr<BaseAST> comoption;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << comoption->Dump();
    if(func_def) ss << func_def->Dump() ; 
    else 
    {
      ss << "Global" << decl->Dump(); 
    }
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    comoption->GenerateIR();
    if(func_def) func_def->GenerateIR();
    else decl->GenerateIR();
    return "";
  }

};

//带有option的豆豉重复0或1次
class ComoptionAST : public BaseAST 
{
 public:
  // 用智能指针管理对象
  unique_ptr<BaseAST> compunit;

  // 转化为AST
  string Dump() const override 
  {
    if(compunit)  return compunit->Dump();
    return "";
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    if(compunit)  compunit->GenerateIR();
    return "";
  }

};


class DeclAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> constdecl;
  unique_ptr<BaseAST> vardecl;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    if(constdecl)  
    ss << "ConDefAST { " << constdecl->Dump() << " }" << endl;
    else 
    ss << "VarDefAST { " << vardecl->Dump() << " }" << endl;
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    if(constdecl) constdecl->GenerateIR();
    else vardecl->GenerateIR(next);
    return "";
  } 

};


class ConstDeclAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> btype;
  vector <unique_ptr<BaseAST>> constdef;
  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << btype->Dump() << " ";
    for(int i=0;i<constdef.size();i++)
    {
      if(i==0) ss << constdef[i]->Dump();
      else
      {
        ss << "," << constdef[i]->Dump();
      }
    }
    ss << ";";
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    for(int i=0;i<constdef.size();i++)
    {
      constdef[i]->GenerateIR();
    }
    return "";
  }

};


class ConstDefAST : public BaseAST 
{
 public:
  string ident;
  vector <unique_ptr<BaseAST>> constexp;
  unique_ptr<BaseAST> constinitval;

  // 转化为AST
  string Dump() const override 
  {
    string s=constinitval->Dump();
    stringstream ss;
    ss << ident;
    for(int i=0;i<constexp.size();i++)
    {
      ss << constexp[i]->Dump();
    }
    ss << "=" << s;
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    int size=constexp.size();
    if(size!=0)
    {
      int value;
      vector<int> boundary;
      for(int i=0;i<size;i++)
      {
        value=evaluateExpression(constexp[i]->Dump());
        if(value<0)
        {
        cout << "\033[31m" << "An error has occurred: size of array '" << ident << "' is negative !" << "\033[0m" << endl;
        exit(EXIT_FAILURE);

        }
        boundary.push_back(value);
      }
        

      //存取数组相关信息
      ArrayInfo tmp;
      tmp.tag=top->tag;
      tmp.dim=boundary;
      tmp.is_const=true;

      //获取常量数组的值
      /****************************/
      string s=constinitval->Dump();

      //数组使用单值赋值或者未被初始化会报错
      if(s.find("{")==string::npos)
      {
        cout << "\033[31m" << "An error has occurred: Arrays need to be initialized using {} !" << "\033[0m" << endl;
        exit(EXIT_FAILURE);
      }

      Nest init=constinitval->ComputeArray();

      //如果{}没有任何值，则使用zereinit初始化，跳过这步
      if(init.struc.size()!=0)
        CompleteInit(init,boundary,&tmp);

      top->array.insert(make_pair(ident,tmp));

      //全局常数组
      if(top->tag==0)
        GlobalArray(ident,tmp.finish,boundary);
      else
        AllocArray(ident+"_"+to_string(top->tag),tmp.finish,boundary);
       
    }
    else
    {
      if(!top->find_current(ident))
      {
        int value=evaluateExpression(constinitval->Dump());
        top->insert(ident,Info{false,value});
      }
      else
      {
        cout << "\033[31m" << "An error has occurred: '" << ident << "' has already been defined!" << "\033[0m" << endl;
        exit(EXIT_FAILURE);        
      }
    }
    return "";
  }
};


class ArrayAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> constexp;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << constexp->Dump();
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    //疑惑
    constexp->GenerateIR();
    return "";
  }

};


class ConstInitValAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> constexp;
  vector <unique_ptr<BaseAST>> constinitval;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    if(constexp) ss << constexp->Dump();
    else
    {
      ss << "{"; 
      for(int i=0;i<constinitval.size();i++)
      {
        if(i==0) ss << constinitval[i]->Dump(); 
        else ss << "," << constinitval[i]->Dump();
      }
      ss << "}";
    }
    return ss.str();
  }
  
  //迭代计算数组初始化的值{}
  Nest ComputeArray() override
  {
    if(constexp)
    {
      Nest tmp;
      tmp.is_num=true;
      tmp.num=evaluateExpression(constexp->Dump());
      return tmp;
    }
    else
    {
      Nest n;
      n.is_num=false;
      for(int i=0;i<constinitval.size();i++)
      {
        Nest tmp=constinitval[i]->ComputeArray();
        n.struc.push_back(tmp);
      }
      return n;
    }
    return Nest{};
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    return "";
  }

};


class VarDeclAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> btype;
  vector <unique_ptr<BaseAST>> vardef;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << btype->Dump() << " ";
    for(int i=0;i<vardef.size();i++)
    {
      if(i==0) ss << vardef[i]->Dump();
      else
      {
        ss << "," << vardef[i]->Dump();
      }
    }
    ss << ";";
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    for(int i=0;i<vardef.size();i++)
    {
      vardef[i]->GenerateIR(next);
    }
    return "";
  }

};


class VarDefAST : public BaseAST 
{
 public:
  string ident;
  vector <unique_ptr<BaseAST>> constexp;
  unique_ptr<BaseAST> initval;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    if(initval)
    {
      ss << ident;
      for(int i=0;i<constexp.size();i++)
      {
        ss << constexp[i]->Dump();
      }
      ss << "=" << initval->Dump();
    }
    else
    {
      ss << ident;
      for(int i=0;i<constexp.size();i++)
      {
        ss << constexp[i]->Dump();
      }
    }
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    int size=constexp.size();
    if(initval)
    {
      if(size!=0)
      {
        vector<int> boundary;
        for(int i=0;i<size;i++)
          boundary.push_back(evaluateExpression(constexp[i]->Dump()));
        ArrayInfo tmp;
        tmp.tag=top->tag;
        tmp.dim=boundary;
        tmp.is_const=false;

        string s=initval->Dump();

        //数组使用单值赋值会报错
        if(s.find("{")==string::npos)
        {
          cout << "\033[31m" << "An error has occurred: Arrays need to be initialized using {} !" << "\033[0m" << endl;
          exit(EXIT_FAILURE);
        }

        Nest init=initval->ComputeArray();
        if(init.struc.size()!=0)
          CompleteInit(init,boundary,&tmp);
        top->array.insert(make_pair(ident,tmp));

      //全局数组
      if(top->tag == 0)
        GlobalArray(ident, tmp.finish, boundary);
      else
        AllocArray(ident+"_"+to_string(top->tag), tmp.finish, boundary);        
      }

      else
      {
        if(!top->find_current(ident))
        {
          top->insert(ident, Info{true, top->tag});
          //全局变量
          if(top->tag == 0)
          {
            int value=evaluateExpression(initval->Dump());
            Global(ident, value);
          }
          else
          {
            Alloc(ident+"_"+to_string(top->tag));
            string from=initval->GenerateIR(next);
            Store(ident+"_"+to_string(top->tag), from);
          }
        }
        else
        {
          cout << "\033[31m" << "An error has occurred: '" << ident << "' has already been defined!" << "\033[0m" << endl;
          exit(EXIT_FAILURE);             
        }

      }
    }

    else
    {
      if(size!=0)
      {
        vector<int> boundary;
        for(int i=0;i<size;i++)
          boundary.push_back(evaluateExpression(constexp[i]->Dump()));
        ArrayInfo tmp;
        tmp.tag=top->tag;
        tmp.dim=boundary;
        tmp.is_const=false;

        Nest init;
        init.is_num=false;
        top->array.insert(make_pair(ident,tmp));

        //全局数组
        if(top->tag==0)
          GlobalArray(ident,tmp.finish,boundary);
        else
          AllocArray(ident+"_"+to_string(top->tag),tmp.finish,boundary);
      }
      else
      {
        top->insert(ident,Info{true,top->tag});
        if(top->tag==0)
          Global(ident,0);
        else
          Alloc(ident+"_"+to_string(top->tag));
      }
    }

    return "";
  }

};

class InitValAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> exp;
  vector <unique_ptr<BaseAST>> initval;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    if(exp) ss << exp->Dump();
    else
    {
      ss << "{";
      for(int i=0;i<initval.size();i++)
      {
        if(i==0) ss << initval[i]->Dump();
        else ss << "," << initval[i]->Dump();
      }
      ss << "}";
    }
    return ss.str();
  }

  //迭代计算数组初始化的值{}
  Nest ComputeArray() override
  {
    if(exp)
    {
      Nest tmp;
      tmp.is_num=true;
      tmp.num=evaluateExpression(exp->Dump());
      return tmp;
    }
    else
    {
      Nest n;
      n.is_num=false;
      for(int i=0;i<initval.size();i++)
      {
        Nest tmp=initval[i]->ComputeArray();
        n.struc.push_back(tmp);
      }
      return n;
    }
    return Nest{};
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    if(exp) return exp->GenerateIR(next);
    return "";
  }

};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> btype;
  string ident;
  unique_ptr<BaseAST> funcfoption;
  unique_ptr<BaseAST> block;

  // 转化为AST
  string Dump() const override 
  {
     stringstream ss;
     ss << "FuncDefAST" << endl
     << "{" << endl
     << btype->Dump() << " " << ident << " " << funcfoption->Dump() << endl
     << block->Dump() << endl
     << "}"  << endl;
     return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
     //新的函数清空块的信息
     block_begin.clear();
     block_end.clear();
     block_num=1;
     block_now=0;
     tag=1;

     //把0基本块加到block_end中
     block_end.insert(make_pair(block_now,0));

     //把函数加到全局作用域
     top->insert(ident,Info{true,top->tag});

     //产生函数的koopa ir
     stringstream ss;
     ss << endl
     << "fun @" << ident << "(";
     Koopa_IR+=ss.str();
     ss.str("");

     funcfoption->GenerateIR();

     Koopa_IR+=")";

     btype->GenerateIR();
     //看情况修改
     string type=btype->Dump();
     if(type.find("int")!=string::npos)
       ret_func.insert(make_pair(ident,true));
     else 
       ret_func.insert(make_pair(ident,false));
 
     ss << "{" << endl << "%entry:" << endl;
     Koopa_IR+=ss.str();

     //Koopa_IR规定要给函数参数重新分配空间
     for(auto it=para.begin();it!=para.end();it++)
     {
       string name=*it+"_1";
       Alloc(name);
       Store(name,"%"+name);
     }

     //数组参数
     for(auto it=para_array.begin(); it != para_array.end(); it++)
     {
       string name=it->first+"_1";
       stringstream ss;
       ss << "  @" << name << "=alloc *";
       Koopa_IR+=ss.str();
       for(int i=0;i<(it->second).size();i++)
         Koopa_IR+="[";
       Koopa_IR+="i32";
       for(int i=(it->second).size()-1;i>=0;i--)
       {
         ss.str("");
         ss << ", " << it->second[i] << "]";
         Koopa_IR+=ss.str();
       }
       Koopa_IR+="\n";

       Store(name,"%"+name);
     }

     block->GenerateIR();

     //疑惑:判断当前块时跳转还是返回
     if(block_end[block_now]>0)
       Jump(block_now);
     else if(block_end[block_now]==0)
     {
       Koopa_IR+="  ret\n";
       block_end[block_now]=-1;
     }

     for(auto it=block_end.begin(); it != block_end.end(); it++)
     {
       if(it->second!=-1)
       {
         print_block_begin(it->first);
         Koopa_IR+="  ret\n";
       }
     }
     Koopa_IR+="}\n";
     para.clear();
     para_array.clear();
     return "";
  }

};


class BTypeAST : public BaseAST
{
 public:
   string Int;
   string Void;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     if(!Int.empty())  ss << "BTypeAST { " << Int << " }";
     else ss << "BTypeAST { " << Void << " }";
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(!Int.empty()) Koopa_IR+=": i32";
     return "";
   }

};


class FuncFoptionAST : public BaseAST
{
 public:
  unique_ptr<BaseAST> funcfparams;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     if(funcfparams) 
     {
       ss << "ParamListAST ( " << funcfparams->Dump() << " )";
     }
     else
     {
       ss << "ParamListAST ()";
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(funcfparams) funcfparams->GenerateIR();
     return "";
   }

};


class FuncFparamsAST : public BaseAST
{
 public:
  vector <unique_ptr<BaseAST>> funparam;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     for(int i=0;i<funparam.size();i++) 
     {
       if(i==0) ss << funparam[i]->Dump();
       else ss << ", " << funparam[i]->Dump();
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     for(int i=0;i<funparam.size();i++) 
     {
       funparam[i]->GenerateIR();
       if(i!=funparam.size()-1) Koopa_IR+=", ";
     }
     return "";
   }

};


class FunparamIntAST : public BaseAST
{
 public:
  string ident;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     ss << "int " << ident;
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     //加入函数形参符号表
     para.push_back(ident);
     stringstream ss;
     ss << "%" << ident << "_1: i32";
     Koopa_IR+=ss.str();
     return "";
   }

};


class FunparamArrayAST : public BaseAST
{
 public:
  string ident;
  vector <unique_ptr<BaseAST>> constexp;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     ss << "int " << ident << "[]";
     for(int i=0;i<constexp.size();i++)
     {
       ss << "[" << constexp[i]->Dump() << "]";
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     int size=constexp.size();
     vector<int> tmp;
     stringstream ss; 

     ss << "%" << ident << "_1: *";
     Koopa_IR+=ss.str();

     //计算输出参数每个维度的大小(不包含0维)
     for(int i=0;i<size;i++)
       tmp.push_back(evaluateExpression(constexp[i]->Dump())); 
     for(int i=0;i<size;i++)
       Koopa_IR+="[";
     Koopa_IR+="i32";
     for(int i=size-1;i>=0;i--)
     {
       ss.str("");
       ss << "," << tmp[i] << "]";
       Koopa_IR+=ss.str();
     }

     //加入函数数组参数符号表
     para_array.insert(make_pair(ident, tmp));
     return "";
   }

};


class BlockAST : public BaseAST 
{
 public:
   vector <unique_ptr<BaseAST>> blockitem;

   // 转化为AST
   string Dump() const override 
   {
     if(blockitem.size())
     {
       stringstream ss;
       ss << "BolckAST"<< endl
       << "{" << endl;
       for(int i=0;i<blockitem.size();i++)
       ss << blockitem[i]->Dump();
       ss << "}";
       return ss.str();
     }
     else
     {
       stringstream ss;
       ss << "BolckAST" << " {} ";
       return ss.str();
     }

   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     Area* tmp=top;
     top=new Area();
     top->father=tmp;
     top->tag=tag++;
     
     //如果是一个函数新开始的作用域，就把函数参数和数组参数加进去
     if(top->tag==1)
     {
       top->merge();
       top->merge_array();
     }
     for(int i=0;i<blockitem.size();i++)
     {
       if(block_begin_exit(blockitem[i].get()))
       {
         Jump(block_now);
         block_now=block_begin.find(blockitem[i].get())->second;
         print_block_begin(block_now);
       }
       else
       {
         if(block_end[block_now]<0) continue;
       }
       if(i<blockitem.size()-1) blockitem[i]->GenerateIR(blockitem[i+1].get());
       else blockitem[i]->GenerateIR(next);
     }

     //恢复当前符号表
     tmp=top;
     top=top->father;
     delete tmp;
     return "";
   }

};


class BlockItemAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> decl;
  unique_ptr<BaseAST> stmt;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    if(decl)  ss << decl->Dump();
    else ss << stmt->Dump();
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    if(decl) decl->GenerateIR(next);
    else stmt->GenerateIR(next);
    return "";
  }

};


class StmtAST : public BaseAST 
{
 public:
   unique_ptr<BaseAST> matched;
   unique_ptr<BaseAST> unmatched;

   // 转化为AST
   string Dump() const override 
   {
    stringstream ss;
     if(matched)
     {
       ss << matched->Dump();
     }
     else
     {
       ss << unmatched->Dump();
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(matched) matched->GenerateIR(next);
     else unmatched->GenerateIR(next);
     return "";
   }
  
};

class MatchedAST : public BaseAST 
{
 public:
   unique_ptr<BaseAST> exp;
   unique_ptr<BaseAST> matched1;
   unique_ptr<BaseAST> matched2;
   unique_ptr<BaseAST> other;

   // 转化为AST
   string Dump() const override 
   {
    stringstream ss;
     if(exp)
     {
       ss << "if(" << exp->Dump() << ")"  << endl
       << matched1->Dump()
       << "else " << matched2->Dump();
     }
     else
     {
       ss << other->Dump() << endl;
     }
     return ss.str();
   
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(exp)
     {
       string cond=exp->GenerateIR();
       int then_block=block_num,else_block=block_num+1,next_block;
       block_num+=2;
       if(!block_begin_exit(next))
       {
         next_block=block_num++;
         block_begin_insert(next,next_block);
       }
       else
       {
         next_block=block_begin.find(next)->second;
       }
       Bind(then_block,next_block);
       Bind(else_block,next_block);
       //更改当前基本块的结构
       Inherit(block_now,next_block);

       Branch(cond,then_block,else_block,next_block);
       //进入另一个基本块
       block_now=then_block;
       matched1->GenerateIR(next);

       Jump(block_now);
       print_block_begin(else_block);
       //进入另一个基本块
       block_now=else_block;
       matched2->GenerateIR(next);
     }
     else
     {
       other->GenerateIR(next);
     }
     return "";
   }

};


class UnmatchedAST : public BaseAST 
{
 public:
   unique_ptr<BaseAST> exp;
   unique_ptr<BaseAST> matched;
   unique_ptr<BaseAST> unmatched;
   unique_ptr<BaseAST> stmt;

   // 转化为AST
   string Dump() const override 
   {
    stringstream ss;
     if(stmt)
     {
       ss << "if(" << exp->Dump() << ")" << endl
       << stmt->Dump();
     }
     else
     {
       ss << "if(" << exp->Dump() << ")" << endl
       << matched->Dump()
       << "else " << unmatched->Dump();
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(stmt)
     {
       string cond=exp->GenerateIR();
       int then_block=block_num++,next_block;
       if(!block_begin_exit(next))
       {
         next_block=block_num++;
         block_begin_insert(next,next_block);
       }
       else
       {
         next_block=block_begin.find(next)->second;
       }
       Bind(then_block, next_block);
       Inherit(block_now, next_block);

       Branch_(cond, then_block, next_block);
       block_now=then_block; // 进入另一个基本块
       stmt->GenerateIR(next);
     }
     else
     {
       string cond=exp->GenerateIR();
    
       int then_block=block_num, else_block=block_num + 1, next_block;
       block_num += 2;
       if(!block_begin_exit(next))
       {
         next_block=block_num++;
         block_begin_insert(next, next_block);
       }
       else
       {
         next_block=block_begin.find(next)->second;
       }
       Bind(then_block, next_block);
       Bind(else_block, next_block);
       Inherit(block_now, next_block);

       Branch(cond, then_block, else_block, next_block);
       block_now=then_block; // 进入另一个基本块
       matched->GenerateIR(next);
    
       Jump(block_now);
       print_block_begin(else_block);
       block_now=else_block; // 进入另一个基本块
       unmatched->GenerateIR(next);
     }
     return "";
   }
   
};


class OtherAST : public BaseAST 
{
 public:
   unique_ptr<BaseAST> lval;
   unique_ptr<BaseAST> exp;
   unique_ptr<BaseAST> stmt;
   unique_ptr<BaseAST> expoption;
   unique_ptr<BaseAST> block;
   string While;
   string Break;
   string Continue;
   string Return;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;

     if(lval)
     {
       ss << lval->Dump() << "=" << exp->Dump() << ";";
     }
     else if(block)
     {
       ss << block->Dump();
     }
     else if(!While.empty())
     {
       ss << While << "(" << exp->Dump() << ")"
       << stmt->Dump();
     }
     else if(!Break.empty())
     {
       ss << "break;";
     }
     else if(!Continue.empty())
     {
       ss << "continue;";
     }
     else if(Return.empty())
     {
       ss << expoption->Dump() << ";";
     }
     else
     {
       ss << "return " << expoption->Dump() << ";";
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(lval)
     {
       stringstream ss;
       string ident=lval->GenerateIR();
       string from=exp->GenerateIR(next);
       if(ident[0] == '%')
       {
         string s=lval->Dump();
         int index=s.find("[");
         string array;
         array.assign(s,0,index);
         if(top->find_array(array))
         {
           if(top->find_array(array)->is_const)
           {
             cout << "\033[31m" << "An error has occurred: constant array '" << array << "' cannot be modified!" << "\033[0m" << endl;
             exit(EXIT_FAILURE);  
           }
         }
         ss << "  store " << from << ", " << ident << endl;
         Koopa_IR+=ss.str();
       }
       else
       {
         if(top->find(ident).flag)
         {
           Store(ident+"_"+to_string(top->find(ident).value), from);
         }
         else
         {
           cout << "\033[31m" << "An error has occurred: constant '" << ident << "' cannot be modified!" << "\033[0m" << endl;
           exit(EXIT_FAILURE);             
         }
       }
     }

     else if(block)
     {
       block->GenerateIR(next);
     }

     else if(!While.empty())
     {
       int entry_block=block_num,body_block=block_num+1,next_block;
       block_num+=2;
       if(!block_begin_exit(next))
       {
         next_block=block_num++;
         block_begin_insert(next,next_block);
       }
       else
       {
         next_block=block_begin.find(next)->second;
       }
       while_entry.push(entry_block);
       while_end.push(next_block);
       block_begin_insert((BaseAST*)(this), entry_block);
       block_end.insert(make_pair(entry_block, -1));
       Bind(body_block, entry_block);
       Inherit(block_now, next_block);

       stringstream ss;
       ss << "  jump %_" << entry_block << endl;
       Koopa_IR+=ss.str();
    
       block_now=entry_block;
       print_block_begin(entry_block);
       string cond=exp->GenerateIR();
       Branch_(cond, body_block, next_block);

       block_now=body_block; // 进入另一个基本块
       stmt->GenerateIR((BaseAST*)(this));
       while_entry.pop();
       while_end.pop();
     }

     else if(!Break.empty())
     {
       stringstream ss;
       ss << "  jump %_" << while_end.top() << endl;
       Koopa_IR+=ss.str();
       block_end[block_now]=-1;
     }

     else if(!Continue.empty())
     {
       stringstream ss;
       ss << "  jump %_" << while_entry.top() << endl;
       Koopa_IR+=ss.str();
       block_end[block_now]=-1;
     }

     else if(Return.empty())
     {
       expoption->GenerateIR(next);
     }

     else
     {
       stringstream ss;
       if(expoption->Dump().empty())
         Koopa_IR += "  ret\n";
       else         
       {
         string ret_val=expoption->GenerateIR(next);
         ss << "  ret " << ret_val << endl;
         Koopa_IR+=ss.str();
       }

       //当前基本块已经结束了 更改block_end为-1
       block_end[block_now]=-1;
     }

     return "";
   }
  
};


class ExpOptionAST : public BaseAST
{
 public:
   unique_ptr<BaseAST> exp;

   // 转化为AST
   string Dump() const override 
   { 
     if(exp)
     return exp->Dump();
     else
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
  {
     if(exp) return exp->GenerateIR(next);
     return "";
  }
};


class ExpAST : public BaseAST
{
 public:
   unique_ptr<BaseAST> lorexp;

   // 转化为AST
   string Dump() const override 
   {
     return lorexp->Dump() ;
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     return lorexp->GenerateIR(next);
   }
    
};


class LValAST : public BaseAST 
{
 public:
  string ident;
  vector <unique_ptr<BaseAST>> arrayexp;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << ident;
    for(int i=0;i<arrayexp.size();i++)
    {
      ss << arrayexp[i]->Dump();
    }
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    stringstream ss;
    //非数组的返回ident 数组getptr/getelemptr后返回%x
    int categ=top->find_category(ident);
    if(!top->find_category(ident))
    {
      cout << "\033[31m" << "An error has occurred: '" << ident << "' has not been declared yet!" << "\033[0m" << endl;
      exit(EXIT_FAILURE);        
    }
    switch(categ)
    {
      case c_num:
      {
        return ident;
      }


      case c_array:
      {
        string name=ident+"_"+to_string(top->find_array(ident)->tag);
        int len=arrayexp.size();
        vector<string> offset;
        for(int i=0;i<len;i++)
          offset.push_back(arrayexp[i]->GenerateIR());
        string re=Getelemptr_only("@"+name,offset);

        //用来传参
        if(!whether_load)
        {
          int para_len=top->find_array(ident)->dim.size();
          if(para_len==len) // 传参是int 要load出来
            ss << "  %" << koopa_reg++ << " = load " << re << endl;
          else // 传参是地址
            ss << "  %" << koopa_reg++ << " = getelemptr " << re << ", 0" << endl;
          Koopa_IR+=ss.str();
          return "%" + to_string(koopa_reg-1);          
        }
        return re;
      }

      case c_func_array:
      {
        string name = ident+"_"+to_string(top->find_func_array(ident)->tag);
        int len = arrayexp.size();

        vector<string> offset;
        for(int i = 0; i < len; i++)
          offset.push_back(arrayexp[i]->GenerateIR());

        ss << "  %" << koopa_reg << " = load @" << name << endl;
        Koopa_IR+=ss.str();
        string re = "%" + to_string(koopa_reg++);
        if(len != 0)
        {
          ss.str("");
          ss << "  %" << koopa_reg << " = getptr %" << koopa_reg-1 << ", " << offset[0] << endl;
          Koopa_IR+=ss.str();
          name = "%" + to_string(koopa_reg++);
          vector<string> cut_offset;
          cut_offset.assign(offset.begin()+1, offset.end());
          re = Getelemptr_only(name, cut_offset);
        }

        // 用来传参
        if(!whether_load)
        {
          ss.str("");
          int para_len = top->find_func_array(ident)->len;
          //传参是int 要load出来
          if(para_len == len - 1) 
           ss << "  %" << koopa_reg++ << " = load " << re << endl;
          else if(len == 0) // 其实不操作也行
            ss << "  %" << koopa_reg++ << " = getptr " << re << ", 0" << endl;
          else
            ss << "  %" << koopa_reg++ << " = getelemptr " << re << ", 0" << endl;
          Koopa_IR+=ss.str();
          return "%" + to_string(koopa_reg-1);
        }
        return re;
      }

      default:
        break;
    }
    return "";
  }
};


class ArrayExpAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> exp;

  // 转化为AST
  string Dump() const override 
  {
    stringstream ss;
    ss << "[" << exp->Dump() << "]";
    return ss.str();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    return exp->GenerateIR();
  }
};


class PrimaryExpAST : public BaseAST
{
 public:
   unique_ptr<BaseAST> lval;
   unique_ptr<BaseAST> exp;
   int number;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s;
     if(exp)
     {
       s=exp->Dump();
       ss << "(" << s << ")";
       return ss.str() ;
     }
     else if(lval)
     {
       return lval->Dump();
     }
     else
     {
       return to_string(number);
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(exp)
     {
       return exp->GenerateIR(next);
     }
     else if(lval)
     {
       stringstream ss;
       string ident = lval->GenerateIR();
       //数组 因为load只能从@移不能从%移
       if(ident[0] == '%')
       { 
         if(whether_load)
         {
           ss << "  %" << koopa_reg++ << " = load " << ident << endl;
           Koopa_IR+=ss.str();
           return "%" + to_string(koopa_reg-1);
         }
         return ident;
       }
       else
       {
         Info val = top->find(ident);
         if(val.flag)
          return Load(ident+"_"+to_string(val.value));
         else
          return to_string(val.value);
      }
     }
     else
     {
       return to_string(number);
     }
     return "";
   }

};


class UnaryExpAST : public BaseAST
{
 public:
   unique_ptr<BaseAST> primaryexp;
   unique_ptr<BaseAST> unaryexp;
   unique_ptr<BaseAST> funcrparams;
   string unaryop;
   string ident;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s;
     if(unaryexp)
     {
       s=unaryexp->Dump();
       ss << unaryop << s;
       return ss.str();
     }
     else if(!ident.empty())
     {
       if(funcrparams)
       {
         s=funcrparams->Dump();
         ss << ident << "(" << s << ")";
       }
       else
         ss << ident << "(" << ")";
       return ss.str(); 
     }
     else
     {
       s=primaryexp->Dump();
       return s;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(unaryexp)
     {
       if(unaryop[0] == '+')
         return unaryexp->GenerateIR(next);
       koo_ret zero;
       koo_ret una=koo_parse(unaryexp->GenerateIR(next));
       return koo_binary(zero, una, unaryop);
     }
     else if(!ident.empty())
     {
       stringstream ss;
       string para_str="";
       if(funcrparams)
         para_str=funcrparams->GenerateIR();
    
       string cat;
       if(ret_func[ident])
       {
         cat="%" + to_string(koopa_reg++);
         ss << "  " << cat << " = "
         << "call @" << ident << "(" << para_str << ")" << endl;
       }
       else
         ss << "  call @" << ident << "(" << para_str << ")" << endl;
       Koopa_IR+=ss.str();
       return cat;
     }
     else
     {
       return primaryexp->GenerateIR(next);
     }
     return "";
   }
};


class FuncRparamsAST : public BaseAST
{
 public:
   vector <unique_ptr<BaseAST>> exp;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     for(int i=0;i<exp.size();i++)
     {
       if(i==0) ss << exp[i]->Dump();
       else ss << ", " << exp[i]->Dump();
     }
     return ss.str();
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
      int len=exp.size();
      vector<string> j;
      string ret_str;

      whether_load=false;
      for(int i=0;i<len;i++)
        j.push_back(exp[i]->GenerateIR());
    
      whether_load=true;
      for(int i=0;i<len;i++)
      {
        ret_str += j[i];
        if(i!=len-1)
          ret_str+=", ";
      }
    return ret_str;
   }
};


class MulExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> unaryexp;
   unique_ptr<BaseAST> mulexp;
   string mulop;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(mulexp)
     {
       s1=mulexp->Dump();
       s2=unaryexp->Dump();
       ss << s1 << mulop << s2;
       return ss.str();
     }
     else
     {
       s1=unaryexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(mulexp)
     {
       koo_ret mul=koo_parse(mulexp->GenerateIR(next));
       koo_ret unary=koo_parse(unaryexp->GenerateIR(next));
       return koo_binary(mul, unary, mulop);
     }
     else
     {
       return unaryexp->GenerateIR(next);
     }
     return "";
   }
};


class AddExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> mulexp;
   unique_ptr<BaseAST> addexp;
   string addop;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(addexp)
     {
       s1=addexp->Dump();
       s2=mulexp->Dump();
       ss << s1 << addop << s2;
       return ss.str();
     }
     else
     {
       s1=mulexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(addexp)
     {
       koo_ret add=koo_parse(addexp->GenerateIR(next));
       koo_ret mul=koo_parse(mulexp->GenerateIR(next));
       return koo_binary(add, mul, addop);
     }
     else
     {
       return mulexp->GenerateIR(next);
     }
     return "";
   }
};


class RelExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> addexp;
   unique_ptr<BaseAST> relexp;
   string relop;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(relexp)
     {
       s1=relexp->Dump();
       s2=addexp->Dump();
       ss << s1 << relop << s2;
       return ss.str();
     }
     else
     {
       s1=addexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(relexp)
     {
       koo_ret rel=koo_parse(relexp->GenerateIR(next));
       koo_ret add=koo_parse(addexp->GenerateIR(next));
       return koo_binary(rel, add, relop);
     }
     else
     {
       return addexp->GenerateIR(next);
     }
     return "";
   }

};


class EqExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> relexp;
   unique_ptr<BaseAST> eqexp;
   string eqop;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(eqexp)
     {
       s1=eqexp->Dump();
       s2=relexp->Dump();
       ss << s1 << eqop << s2;
       return ss.str();
     }
     else
     {
       s1=relexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(eqexp)
     {
       koo_ret eq=koo_parse(eqexp->GenerateIR(next));
       koo_ret rel=koo_parse(relexp->GenerateIR(next));
       return koo_binary(eq, rel, eqop);
     }
     else
     {
       return relexp->GenerateIR(next);
     }
     return "";
   }

};


class LAndExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> eqexp;
   unique_ptr<BaseAST> landexp;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(landexp)
     {
       s1=landexp->Dump();
       s2=eqexp->Dump();
       ss << s1 << "&&" << s2;
       return ss.str();
     }
     else 
     {
       s1=eqexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
   string GenerateIR(BaseAST* next) const override 
   {
     if(landexp)
     {
       string short_circuit=landexp->GenerateIR();

       string tmp_reg="result_" + to_string(block_now);
       Alloc(tmp_reg);
       Store(tmp_reg, "0");
       int then_block=block_num, else_block=block_num + 1, back_block=block_num + 2;
       block_num += 3;
       Bind(then_block, back_block);
       Bind(else_block, back_block);
       Inherit(block_now, back_block);
    
       // short_circuit非零->then_block: 继续算; short_circuit为零->else_block: jump back_block
       Branch(short_circuit, then_block, else_block, back_block);
       block_now=then_block;
       koo_ret zero, eq=koo_parse(eqexp->GenerateIR());
       Store(tmp_reg, koo_binary(zero, eq, "!="));

       Jump(block_now);
       block_now=else_block;
       print_block_begin(else_block);
    
       Jump(block_now);
       block_now=back_block;
       print_block_begin(back_block);
       return Load(tmp_reg);
     }

     else
     {
       return eqexp->GenerateIR(next);
     }
     return "";
   }
  
};


class LOrExpAST : public BaseAST{
 public:
   unique_ptr<BaseAST> landexp;
   unique_ptr<BaseAST> lorexp;

   // 转化为AST
   string Dump() const override 
   {
     stringstream ss;
     string s1,s2;
     if(lorexp)
     {
       s1=lorexp->Dump();
       s2=landexp->Dump();
       ss << s1 << "||" << s2;
       return ss.str();
     }
     else
     {
       s1=landexp->Dump();
       return s1;
     }
     return "";
   }

   // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    if(lorexp)
    {
      string short_circuit=lorexp->GenerateIR();

      string tmp_reg="result_" + to_string(block_now);
      Alloc(tmp_reg);
      Store(tmp_reg, "1");
      int then_block=block_num, else_block=block_num + 1, back_block=block_num + 2;
      block_num += 3;
      Bind(then_block, back_block);
      Bind(else_block, back_block);
      Inherit(block_now, back_block);

      // short_circuit非零->then_block: jump back_block; short_circuit为零->else_block: 继续算
      Branch(short_circuit, then_block, else_block, back_block);
      block_now=then_block;
    
      Jump(block_now);
      block_now=else_block;
      print_block_begin(else_block);
      koo_ret zero, lAnd=koo_parse(landexp->GenerateIR());
      Store(tmp_reg, koo_binary(zero, lAnd, "!="));

      Jump(block_now);
      block_now=back_block;
      print_block_begin(back_block);
      return Load(tmp_reg);
    }
    else
    {
      return landexp->GenerateIR(next);
    }
    return "";
  }

};

class ConstExpAST : public BaseAST 
{
 public:
  unique_ptr<BaseAST> exp;

  // 转化为AST
  string Dump() const override 
  {
    return exp->Dump();
  }

  // 转化为Koopa IR
  string GenerateIR(BaseAST* next) const override 
  {
    return "";
  }

};

#endif