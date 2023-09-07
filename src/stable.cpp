#include <iostream>
#include <stack>
#include <string>
#include "stable.h"
#include <vector>
#include <regex>
#include <unordered_map>

using namespace std;

Area *top;
vector<string> para; //函数形参
unordered_map<string, vector<int>> para_array; //函数数组形参
unordered_map<string, bool> ret_func;//是否使用函数返回值
int tag=1;
bool whether_load=true;

void Area::insert(string def, Info info) 
{
   table.insert(make_pair(def,info));
}

void Area::merge()
{
   for(auto it=para.begin();it!=para.end();it++)
   {
      table.insert(make_pair(*it,Info{true,top->tag}));
   }
}

void Area::merge_array()
{
   for(auto it=para_array.begin();it!=para_array.end();it++)
   {
      string ident=it->first;
      ParamInfo p;
      p.dim.assign((it->second).begin(),(it->second).end());
      p.tag=top->tag;
      p.len=p.dim.size();
      func_array.insert(make_pair(ident,p));
   }
}

Info Area::find(string def)
{
   Area* tmp=top;
   while(tmp)
   {
      auto found=tmp->table.find(def);
      if(found!=tmp->table.end()) return found->second;
      tmp=tmp->father;
   }
   return Info{false, 0};
}

ArrayInfo* Area::find_array(string def)
{
   Area *tmp=top;
   while(tmp)
   {
      auto found=tmp->array.find(def);
      if(found!=tmp->array.end()) return &(found->second);
      tmp=tmp->father;
   }
   return NULL;
}

ParamInfo* Area::find_func_array(string def)
{
   Area *tmp=top;
   while(tmp)
   {
      auto found=tmp->func_array.find(def);
      if(found!=tmp->func_array.end()) return &(found->second);
      tmp=tmp->father;
   }
   return NULL;
}

int Area::find_category(string def)
{
   Area* tmp = top;
   while(tmp)
   {
      if(tmp->table.find(def) != tmp->table.end()) return c_num;
      if(tmp->array.find(def) != tmp->array.end()) return c_array;
      if(tmp->func_array.find(def) != tmp->func_array.end()) return c_func_array;
      tmp = tmp->father;
   }
   return 0;
}

bool Area::find_current(string def)
{
    if(top->table.find(def)!=top->table.end()) return true;
    if(top->array.find(def)!=top->array.end()) return true;
    if(top->func_array.find(def)!=top->func_array.end()) return true;
    return false;
}


/*******************************************/
//以下函数实现带变量的字符串表达式的求值

// 函数：判断运算符的优先级
// o 为逻辑或运算
// a 为逻辑与运算
// e 为逻辑等于运算 n为逻辑不等于运算
// g 为大于等于运算 l为小于等于运算
int getPriority(char op) {
    switch (op) {
        case 'o':
            return 1;
        case 'a':
            return 2;
        case 'e':
        case 'n':
            return 3;
        case 'g':
        case 'l':
        case '>':
        case '<':
            return 4;
        case '+':
        case '-':
            return 5;
        case '*':
        case '/':
            return 6;
        case '!':
            return 7;
        default:
            return 0;
    }
}

// 函数：执行二元操作
int executeBinaryOperation(int operand1, char op, int operand2) {
    switch (op) {
        case 'o':
            return operand1 || operand2;
        case 'a':
            return operand1 && operand2;
        case 'e':
            return operand1 == operand2;
        case 'n':
            return operand1 != operand2;
        case 'g':
            return operand1 >= operand2;
        case 'l':
            return operand1 <= operand2;
        case '>':
            return operand1 > operand2;
        case '<':
            return operand1 < operand2;
        case '+':
            return operand1 + operand2;
        case '-':
            return operand1 - operand2;
        case '*':
            return operand1 * operand2;
        case '/':
            return operand1 / operand2;
        default:
            return 0;
    }
}

// 函数：执行一元操作
int executeUnaryOperation(char op, int operand) {
    switch (op) {
        case '!':
            return !operand;
        default:
            return 0;
    }
}

vector<string> extractIdentifiers(const string& expression) 
{
    vector<string> identifiers;

    // 正则表达式模式，匹配标识符（由字母或下划线开头，后跟字母、数字或下划线）
    regex identifierPattern(R"(([a-zA-Z_][a-zA-Z0-9_]*|\[\d+\])+)");

    // 使用迭代器遍历匹配结果
    sregex_iterator iterator(expression.begin(), expression.end(), identifierPattern);
    sregex_iterator endIterator;

    // 提取匹配到的标识符
    while (iterator != endIterator) 
    {
        smatch match = *iterator;
        string identifier = match.str();
        identifiers.push_back(identifier);
        ++iterator;
    }

    return identifiers;
}

vector<int> extractNumbers(const string& str) 
{
    vector<int> numbers;
    regex pattern(R"(\[(\d+)\])"); //正则表达式模式，匹配形如 [数字] 的字符串
    smatch matches;

    string::const_iterator searchStart = str.begin();
    while (regex_search(searchStart, str.end(), matches, pattern)) 
    {
        //将匹配到的数字转换为整数并存入数组
        numbers.push_back(stoi(matches[1]));
        searchStart = matches.suffix().first;
    }

    return numbers;
}

// 函数：计算表达式的值
int evaluateExpressionWithoutIdent(const string& exp) {
    stack<int> operandStack;
    stack<char> operatorStack;
    string number;

    bool expectUnary = true; // 标记是否期望一元操作符
    char ch;
    for (int i=0;i<exp.length();i++) {
        ch=exp[i];
        if (isspace(ch)) {
            continue; // 忽略空格
        } else if (isdigit(ch)) {
            number+=ch;
            while(exp[i+1]>='0'&&exp[i+1]<='9')
            {
                number+=exp[i+1];
                i++;
            }
            int operand = stoi(number);
            operandStack.push(operand);
            number.clear();
            expectUnary = false;
        } else if (ch == '-') {
            if (expectUnary) {
                operandStack.push(0);
                operatorStack.push(ch);
            } else {
                while (!operatorStack.empty() && getPriority(operatorStack.top()) >= getPriority(ch)) {
                    char op = operatorStack.top();
                    operatorStack.pop();

                    if (op == '!') {
                        int operand = operandStack.top();
                        operandStack.pop();
                        int result = executeUnaryOperation(op, operand);
                        operandStack.push(result);
                    } else {
                        int operand2 = operandStack.top();
                        operandStack.pop();
                        int operand1 = operandStack.top();
                        operandStack.pop();
                        int result = executeBinaryOperation(operand1, op, operand2);
                        operandStack.push(result);
                    }
                }

                operatorStack.push(ch);
                expectUnary = true;
            }
        } else if (ch == '(') {
            operatorStack.push(ch);
            expectUnary = true;
        } else if (ch == ')') {
            while (!operatorStack.empty() && operatorStack.top() != '(') {
                char op = operatorStack.top();
                operatorStack.pop();

                if (op == '!') {
                    int operand = operandStack.top();
                    operandStack.pop();
                    int result = executeUnaryOperation(op, operand);
                    operandStack.push(result);
                } else {
                    int operand2 = operandStack.top();
                    operandStack.pop();
                    int operand1 = operandStack.top();
                    operandStack.pop();
                    int result = executeBinaryOperation(operand1, op, operand2);
                    operandStack.push(result);
                }
            }

            if (!operatorStack.empty()) {
                operatorStack.pop(); // 弹出左括号
            }
            expectUnary = false;
        } else {
            while(exp[i+1]==' ') i++;
            if(ch=='|'&&exp[i+1]=='|') 
            {
              ch='o';
              i++;
            }
            if(ch=='&'&&exp[i+1]=='&') 
            {
              ch='a';
              i++;
            }
            if(ch=='='&&exp[i+1]=='=') 
            {
              ch='e';
              i++;
            }
            if(ch=='!'&&exp[i+1]=='=') 
            {
              ch='n';
              i++;
            }
            if(ch=='>'&&exp[i+1]=='=') 
            {
              ch='g';
              i++;
            }
            if(ch=='<'&&exp[i+1]=='=') 
            {
              ch='l';
              i++;
            }
            while (!operatorStack.empty() && getPriority(operatorStack.top()) >= getPriority(ch)) {
                char op = operatorStack.top();
                operatorStack.pop();

                if (op == '!') {
                    int operand = operandStack.top();
                    operandStack.pop();
                    int result = executeUnaryOperation(op, operand);
                    operandStack.push(result);
                } else {
                    int operand2 = operandStack.top();
                    operandStack.pop();
                    int operand1 = operandStack.top();
                    operandStack.pop();
                    int result = executeBinaryOperation(operand1, op, operand2);
                    operandStack.push(result);
                }
            }

            operatorStack.push(ch);
            expectUnary = true;
        }
    }

    while (!operatorStack.empty()) {
        char op = operatorStack.top();
        operatorStack.pop();

        if (op == '!') {
            int operand = operandStack.top();
            operandStack.pop();
            int result = executeUnaryOperation(op, operand);
            operandStack.push(result);
        } else {
            int operand2 = operandStack.top();
            operandStack.pop();
            int operand1 = operandStack.top();
            operandStack.pop();
            int result = executeBinaryOperation(operand1, op, operand2);
            operandStack.push(result);
        }
    }

    return operandStack.top();
}

vector<string> extractExpressions(const string& input) 
{
    vector<string> expressions;
    string expression;

    for (char c: input) {
        if (c=='{'||c==' ') {
            continue;
        } else if (c == '}') {
            break;
        } else if ( c == ',') {
            expressions.push_back(expression);
            expression.clear();
            continue;
        }

        expression += c;
    }

    if (!expression.empty()) {
        expressions.push_back(expression);
    }

    return expressions;
}


int evaluateExpression(const string& exp) {
    string replacexp=exp;
    vector<string> ident=extractIdentifiers(exp);
    for(int i=0;i<ident.size();i++)
    {
        int pos = 0;
        auto tmp1=top->table.find(ident[i]);
        if(tmp1!=top->table.end()) 
        {
            if(!(tmp1->second.flag))
            {
              int value = tmp1->second.value;
              pos = replacexp.find(ident[i], pos);
              replacexp.replace(pos, ident[i].length(), to_string(value));
              continue;
            }
            else
            {
              cout << "\033[31m" << "An error has occurred: The constant expression cannot contain variables '"<< ident[i] << "'!\033[0m" << endl; 
              exit(EXIT_FAILURE);               
            }
        }
        int index=ident[i].find("[");
        string array;
        array.assign(ident[i],0,index);
        auto tmp2=top->array.find(array);
        if(tmp2!=top->array.end()) 
        {
          vector<int> dim=extractNumbers(ident[i]);
          int size=0;
          for(int i=0;i<dim.size()-1;i++) 
          {
            int a=1;
            for(int j=i+1;j<dim.size();j++) 
              a*=tmp2->second.dim[j];
            size+=(dim[i]*a);
          }
          size+=dim[dim.size()-1];
          int value = tmp2->second.finish[size];
          pos = replacexp.find(ident[i], pos);
          replacexp.replace(pos, ident[i].length(), to_string(value));
          return evaluateExpressionWithoutIdent(replacexp);
        }
        else
        {
          cout << "\033[31m" << "An error has occurred: '" << ident[i] << "' has not been declared yet!" << "\033[0m" << endl;
          exit(EXIT_FAILURE);
        }    
    }
    return evaluateExpressionWithoutIdent(replacexp);
}
/*******************************************/