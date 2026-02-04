#pragma once
#include <string>

class Question {
    public:
        //构造函数：生成一道题（a op b）
        Question(int a, int b, char op);

        //判断用户答案是否正确
        bool check(int user_answer) const;

        //返回题目字符串，如“23 + 45 = ”
        std::string toString() const;
    private:
        int a_, b_, correct_answer_;
        char op_;
};