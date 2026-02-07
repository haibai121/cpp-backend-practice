#include <iostream>
#include "BookManage.h"

using namespace std;
int main(){
    BookManage bookmanage;
    while(1){
        cout << "图书管理系统\n";
        cout << "1.添加图书\n";
        cout << "2.借书\n";
        cout << "3.还书\n";
        cout << "4.按ISBN查询图书信息\n";
        cout << "5.打印所有图书\n";
        cout << "6.退出程序\n";
        int chioce = 0;
        cin >> chioce;
        switch(chioce){
            case 1: {
                cout << "请输入书名，书编号，作者\n";
                string book_name, ISBN, author;
                cin >> book_name;
                cin >> ISBN;
                cin >> author;
                bool res = bookmanage.add_book(book_name, ISBN, author);
                if(res){
                    cout <<"\n";
                    break;
                }else {
                    cout << "出现错误：";
                    break;
                }
            }
            case 2:{
                string ISBN;
                cout << "请输入书籍编号：";
                cin >> ISBN;
                bool res = bookmanage.borrow_book(ISBN);
                if(res){
                    cout << "\n";
                    break;
                }else{
                    break;
                }
            }
            case 3:{
                string ISBN;
                cout << "请输入书籍编号：";
                cin >> ISBN;
                bool res = bookmanage.return_book(ISBN);
                if(res){
                    cout <<"\n";
                    break;
                }else{
                    break;
                }
            }
            case 4:{
                string ISBN;
                cout << "请输入书籍编号：";
                cin >> ISBN;
                bookmanage.search_book(ISBN);
                break;
            }
            case 5:{
                bookmanage.prift_all_book();
                break;
            }
            case 6:{
                cout << "正在退出";
                return 0;
            }
            default:cout << "请正确输入序号！！";break;
        }
    }
}