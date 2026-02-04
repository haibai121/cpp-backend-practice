#include "Question.h"
#include <string>

// 构造函数：设置操作数、运算符并计算正确答案
Question::Question(int a, int b, char op) : a_(a), b_(b), op_(op) {
    // 修改：修正加法与减法的正确答案计算（原代码在构造函数内嵌入了方法定义且对减法处理错误）
    if (op_ == '+') {
        correct_answer_ = a_ + b_;
    } else if (op_ == '-') {
        correct_answer_ = a_ - b_;
    } else {
        // 若遇到未知运算符，默认答案为0
        correct_answer_ = 0;
    }
}

// 修改：将方法从构造函数外部分离（原代码将方法定义放在构造函数内部，导致语法错误）
bool Question::check(int user_answer) const {
    return user_answer == correct_answer_;
}

// 修改：修正 toString 中的拼接，使用 std::to_string 而非不存在的 std::toString
std::string Question::toString() const {
    return std::to_string(a_) + " " + op_ + " " + std::to_string(b_) + " = ";
}