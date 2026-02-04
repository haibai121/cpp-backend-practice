
#include <iostream>
#include <vector>
#include <random>
#include "Question.h"

using namespace std;

int main(){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> num_dist(1, 100); //数字0~99
    uniform_int_distribution<> op_dist(0, 1); // 0 或者 1

    vector<Question> questions;

    //生成10道题
    for(int i = 0; i < 10; i++){
        int a = num_dist(gen);
        int b = num_dist(gen);
        char op = (op_dist(gen) == 0) ? '+' : '-';
        //确保减法不出现负数（可选优化）
        if(op == '-' && a < b){
            swap(a, b);
        }
        questions.emplace_back(a, b, op);
    }

    //开始答题
    int score = 0;
    for(const auto& q : questions){
        cout << q.toString();
        int ans;
        cin >> ans;
        if(q.check(ans)){
            score++;
        }
    }

    cout << "Your score: " << score << "/10" << endl;
}