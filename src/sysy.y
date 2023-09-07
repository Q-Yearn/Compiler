%code requires 
{
  #include <memory>
  #include <string>
  #include <cstring>
  #include "ast.h"
  #include "stable.h"
  #include <vector>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include "ast.h"
#include <vector>
#include "stable.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(unique_ptr<BaseAST> &ast, const char *s);

int Vanum=0;
int iflag=1;
int elseflag=1;
int endnum=1;
int Temvanum=1;
int isblockend=1;
int whilenum=1;
int whileflag=0;
string breakend="";
int iswhile=0;
int isif=0;
string continuentry="";

using namespace std;


%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union 
{
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  BlockAST *ast_val1;
  ConstDeclAST *ast_val2;
  VarDeclAST *ast_val3;
  FuncFparamsAST *ast_val4;
  FuncRparamsAST *ast_val5;
  ConstDefAST *ast_val6;
  ConstInitValAST *ast_val7;
  InitValAST *ast_val8;
  LValAST *ast_val9;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT UnaryOp MulOp  RelOp EqOp LAndOp LOrOp
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef BType Block Stmt ExpOption Exp 
%type <ast_val> PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl VarDecl ConstDef ConstInitVal ConstExp VarDef InitVal BlockItem LVal Matched Unmatched Other
%type <ast_val> CompUnit Comoption Funparam FuncFparams FuncFoption FuncRparams Array
%type <ast_val> ArrayExp
%type <ast_val1> BlockItems
%type <ast_val2> ConstDefs 
%type <ast_val3> VarDefs
%type <ast_val4> Funparams
%type <ast_val5> Exps
%type <ast_val6> ConstExps
%type <ast_val7> ConstInitVals
%type <ast_val8> InitVals
%type <ast_val9> ArrayExps
%type <int_val> Number 

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
Begin
  : CompUnit
  {
    auto begin = make_unique<BeginAST>();
    begin->compunit = unique_ptr<BaseAST>($1);
    ast = move(begin);   
  }


// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担


CompUnit
  : Comoption FuncDef 
  {
    auto ast = new CompUnitAST();
    ast->comoption = unique_ptr<BaseAST>($1);
    ast->func_def = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Comoption Decl
  {
    auto ast = new CompUnitAST();
    ast->comoption = unique_ptr<BaseAST>($1);
    ast->decl = unique_ptr<BaseAST>($2);
    $$ = ast;  
  }
  ;


Comoption
  : CompUnit
  {
    auto ast = new ComoptionAST();
    ast->compunit = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
  {
    auto ast = new ComoptionAST();
    ast->compunit = nullptr;
    $$ = ast;
  }
  ;


Decl
  : ConstDecl
  {
    auto ast = new DeclAST();
    ast->constdecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl
  {
    auto ast = new DeclAST();
    ast->vardecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


ConstDecl
  : CONST BType ConstDefs ';'
  {
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    for (auto& element : $3->constdef) 
    {
      ast->constdef.emplace_back(move(element));  
      element = nullptr; 
     }
    $$ = ast;
  }
  ;


ConstDefs
  : ConstDefs ',' ConstDef
  {
    ($1->constdef).emplace_back(move($3));
    $$ = $1;
  }
  | ConstDef
  {
    ConstDeclAST* condecl=new ConstDeclAST();
    (condecl->constdef).emplace_back(move($1));
    $$ = condecl;
  }
  ;


ConstDef
  : IDENT ConstExps '=' ConstInitVal
  {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constinitval = unique_ptr<BaseAST>($4);
    for (auto& element : $2->constexp) 
    {
      ast->constexp.emplace_back(move(element));  
      element = nullptr; 
     }
    $$ = ast;
  }
  ;


Array
 : '[' ConstExp ']'
 {
    auto ast = new ArrayAST();
    ast->constexp = unique_ptr<BaseAST>($2);
    $$ = ast;   
 }
 ;

ConstExps
  : ConstExps Array
  {
    ($1->constexp).emplace_back(move($2));
    $$ = $1;
  }
  |
  {
    ConstDefAST* condef=new ConstDefAST();
    $$ = condef; 
  }
  ;


ConstInitVal
  : ConstExp
  {
    auto ast = new ConstInitValAST();
    ast->constexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitVals '}'
  {
    auto ast = new ConstInitValAST();
    for (auto& element : $2->constinitval) 
    {
      ast->constinitval.emplace_back(move(element));  
      element = nullptr; 
     }
    $$ = ast; 
  }
  | '{' '}'
  {
    auto ast = new ConstInitValAST();
    $$ = ast;    
  }
  ;


ConstInitVals
 : ConstInitVals ',' ConstInitVal
 {
    ($1->constinitval).emplace_back(move($3));
    $$ = $1;
 }
 | ConstInitVal
 {
    ConstInitValAST* constiv=new ConstInitValAST();
    (constiv->constinitval).emplace_back(move($1));
    $$ = constiv;
 }
 ;


VarDecl
  : BType VarDefs ';' 
  {
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    for (auto& element : $2->vardef) 
    {
      ast->vardef.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  ;


VarDefs
  : VarDefs ',' VarDef
  {
    ($1->vardef).emplace_back(move($3));
    $$ = $1;
  }
  | VarDef
  {
    VarDeclAST* vdecl=new VarDeclAST();
    (vdecl->vardef).emplace_back(move($1));
    $$ = vdecl;
  }
  ;


VarDef
  : IDENT ConstExps
  {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    for (auto& element : $2->constexp) 
    {
      ast->constexp.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  | IDENT ConstExps '=' InitVal
  {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    for (auto& element : $2->constexp) 
    {
      ast->constexp.emplace_back(move(element));  
      element = nullptr; 
    }
    ast->initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;


InitVal
  : Exp
  {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' InitVals '}'
  {
    auto ast = new InitValAST();
    for (auto& element : $2->initval) 
    {
      ast->initval.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  | '{' '}'
  {
    auto ast = new InitValAST();
    $$ = ast;   
  }
  ;


InitVals
 : InitVals ',' InitVal
 {
    ($1->initval).emplace_back(move($3));
    $$ = $1;
 }
 | InitVal
 {
    InitValAST* initv=new InitValAST();
    (initv->initval).emplace_back(move($1));
    $$ = initv;
 }
 ;


FuncDef
  : BType IDENT '(' FuncFoption ')' Block 
  {
    auto ast = new FuncDefAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->funcfoption = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;


BType
  : VOID
  {
    auto ast= new BTypeAST();
    ast->Void = "void";
    $$ = ast;
  }
  | INT
  {
    auto ast= new BTypeAST();
    ast->Int = "int";
    $$ = ast;   
  }
  ;


FuncFoption
  : FuncFparams
  {
    auto ast = new FuncFoptionAST();
    ast->funcfparams = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
  {
    auto ast = new FuncFoptionAST();
    ast->funcfparams = nullptr;
    $$ = ast;
  }
  ;


FuncFparams
  : Funparams
  {
    auto ast = new FuncFparamsAST();
    for (auto& element : $1->funparam) 
    {
      ast->funparam.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  ;


Funparams
  : Funparams ',' Funparam
  {
    ($1->funparam).emplace_back(move($3));
    $$ = $1;
  }
  | Funparam
  {
    FuncFparamsAST* funcfparams=new FuncFparamsAST();
    (funcfparams->funparam).emplace_back(move($1));
    $$ = funcfparams;
  }
  ;


Funparam
  : INT IDENT '[' ']' ConstExps
  {
    auto ast = new FunparamArrayAST();
    ast->ident = *unique_ptr<string>($2);
    for (auto& element : $5->constexp) 
    {
      ast->constexp.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  | INT IDENT
  {
    auto ast = new FunparamIntAST();
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;


Block
  : '{' BlockItems '}' 
  {
    auto ast = new BlockAST();
    for (auto& element : $2->blockitem) 
    {
      ast->blockitem.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  | '{' '}'
  {
    auto ast = new BlockAST();
    $$ = ast;  
  }
  ;


BlockItems
  : BlockItems BlockItem
  {
    ($1->blockitem).emplace_back(move($2));
    $$ = $1;    
  }
  | BlockItem
  {
    BlockAST *blos=new BlockAST();
    (blos->blockitem).emplace_back(move($1));
    $$ = blos;
  }
  ;


BlockItem
  : Decl
  {
    auto ast= new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt
  {
    auto ast= new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


Stmt
  : Matched
  {
    auto ast= new StmtAST();
    ast->matched = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Unmatched
  {
    auto ast= new StmtAST();
    ast->unmatched = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


Matched
  : IF '(' Exp ')' Matched ELSE Matched 
  {
    auto ast= new MatchedAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->matched1 = unique_ptr<BaseAST>($5);
    ast->matched2 = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | Other
  {
    auto ast= new MatchedAST();
    ast->other = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Unmatched
  : IF '(' Exp ')' Stmt
  {
    auto ast= new UnmatchedAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' Matched ELSE Unmatched
  {
    auto ast= new UnmatchedAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->matched = unique_ptr<BaseAST>($5);
    ast->unmatched = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;


Other
  : LVal '=' Exp ';'
  {
    auto ast= new OtherAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ExpOption ';'
  {
    auto ast= new OtherAST();
    ast->expoption = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block
  {
    auto ast= new OtherAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt
  {
    auto ast = new OtherAST();
    ast->While = "while";
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';'
  {
    auto ast = new OtherAST();
    ast->Break = "break";
    $$ = ast;
  }
  | CONTINUE ';'
  {
    auto ast = new OtherAST();
    ast->Continue = "continue";
    $$ = ast;
  }
  | RETURN ExpOption ';' 
  {
    auto ast = new OtherAST();
    ast->Return = "return";
    ast->expoption = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;


ExpOption
  :
  {
    auto ast = new ExpOptionAST();
    ast->exp = nullptr;
    $$ = ast;    
  }
  | Exp 
  {
    auto ast = new ExpOptionAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast; 
  }
  ;


Exp
  : LOrExp 
  {
    auto ast = new ExpAST();
    ast->lorexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


LVal
  : IDENT ArrayExps
  {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    for (auto& element : $2->arrayexp) 
    {
      ast->arrayexp.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
  }
  ;


ArrayExp
 : '[' Exp ']'
 {
    auto ast = new ArrayExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
 }
 ;


ArrayExps
 : ArrayExps ArrayExp
 {
    ($1->arrayexp).emplace_back(move($2));
    $$ = $1;  
 }
 |
 {
   LValAST *arrayexps=new LValAST();
   $$ = arrayexps;
 }
 ;

PrimaryExp
  : '(' Exp ')' 
  {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal
  {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number
   {
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    $$ = ast;
  }
  ;


Number
  : INT_CONST 
  {
    $$ = $1;
  }
  ;


UnaryExp
  : PrimaryExp 
  {
    auto ast = new UnaryExpAST();
    ast->primaryexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp
  {
    auto ast = new UnaryExpAST();
    ast->unaryop = *unique_ptr<string>($1);
    ast->unaryexp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '!' UnaryExp
  {
    auto ast = new UnaryExpAST();
    ast->unaryop = "!";
    ast->unaryexp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRparams ')'
  {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->funcrparams = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT '(' ')'
  {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;


FuncRparams
 : Exps
 {
    auto ast = new FuncRparamsAST();
    for (auto& element : $1->exp) 
    {
      ast->exp.emplace_back(move(element));  
      element = nullptr; 
    }
    $$ = ast;
 }
;


Exps
 : Exps ',' Exp
 {
    ($1->exp).emplace_back(move($3));
    $$ = $1;
 }
 | Exp
 {
    FuncRparamsAST* funcrparams=new FuncRparamsAST();
    (funcrparams->exp).emplace_back(move($1));
    $$ = funcrparams;
 }
 ;


MulExp
  : UnaryExp 
  {
    auto ast = new MulExpAST();
    ast->unaryexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp
  {
    auto ast = new MulExpAST();
    ast->mulexp = unique_ptr<BaseAST>($1);
    ast->mulop = *unique_ptr<string>($2);
    ast->unaryexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


AddExp
  : MulExp 
  {
    auto ast = new AddExpAST();
    ast->mulexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp UnaryOp MulExp
  {
    auto ast = new AddExpAST();
    ast->addexp = unique_ptr<BaseAST>($1);
    ast->addop = *unique_ptr<string>($2);
    ast->mulexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


RelExp
  : AddExp 
  {
    auto ast = new RelExpAST();
    ast->addexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp
  {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->relop = *unique_ptr<string>($2);
    ast->addexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


EqExp
  : RelExp 
  {
    auto ast = new EqExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp
  {
    auto ast = new EqExpAST();
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->eqop = *unique_ptr<string>($2);
    ast->relexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


LAndExp
  : EqExp 
  {
    auto ast = new LAndExpAST();
    ast->eqexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAndOp EqExp
  {
    auto ast = new LAndExpAST();
    ast->landexp = unique_ptr<BaseAST>($1);
    ast->eqexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


LOrExp
  : LAndExp 
  {
    auto ast = new LOrExpAST();
    ast->landexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOrOp LAndExp
  {
    auto ast = new LOrExpAST();
    ast->lorexp = unique_ptr<BaseAST>($1);
    ast->landexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;


ConstExp
  : Exp
  {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) 
{
  cerr << "error: " << s << endl;
}
