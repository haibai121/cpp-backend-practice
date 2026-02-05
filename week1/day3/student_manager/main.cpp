#include "StudentManager.h"
#include <iostream>

using namespace std;

int main(){
    StudentManager manager;
    int choice;
    while(1){
        cout << "请选择你要的功能：（数字）\n" 
        << "1.添加学生\n"
        << "2.按学号查学生\n"
        << "3.计算所有学生的平均分\n"
        << "4.打印全部学生信息\n"
        << "5.退出程序\n";
        cin >> choice;
        switch(choice){
            case 1: manager.add_student();break;
            case 2: {
                int id;
                cout << "输入ID";
                cin >> id;
                manager.search_by_ID(id);
                break;
            }
            case 3: manager.average();break;
            case 4: manager.printStudent();break;
            case 5: return 0;
            default:cout << "输入错误，程序退出...";return 0;
        }
    }
    return 0;
}
