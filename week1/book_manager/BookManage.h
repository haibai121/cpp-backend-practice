#include <iostream>
#include <vector>
#include "Book.h"

class BookManage{
    public:
        //便利容器找书，有该书返回false，没该书返回true
        bool search_ISBN(std::string ISBN) const{
            for(const auto& book : books){
                if(ISBN == book.get_ISBN()){
                    return false;
                }
            }
            return true;
        }
        //便利容器查书是否可借，若可借则返回对应下标
        int search_status(std::string ISBN) const{
            int i = 1;
            for(const auto& book : books){
                if(ISBN == book.get_ISBN() && book.get_status()){
                    return i;
                }else if(ISBN == book.get_ISBN() && book.get_status() == false){
                    return -i;
                }
                i++;
            }
            return 0;
        }
        //添加图书功能
        bool add_book(std::string book_name, std::string ISBN, std::string author){
            if(search_ISBN(ISBN)){
                books.emplace_back(book_name, ISBN, author);
                std::cout << "添加成功\n";
                return true;
            }else{
                std::cout << "已有该书籍\n";
                return false;
            }
        }
        //借书功能
        bool borrow_book(std::string ISBN){
            int res = search_status(ISBN);
            if(res == 0){
                std::cout << "没有该书籍\n";
                return false;
            }else if(res < 0){
                std::cout << "该书籍已被借出\n";
                return false;
            }else {
                std::cout << "序号为" << ISBN << "可以借出\n";
                std::cout << "借阅选1，不借阅并退出选2\n";
                int chioce = 0;
                std::cin >> chioce;
                while(1){
                    switch(chioce){
                        case 1:{
                            books[res-1].set_status(false);
                            std::cout << "借阅成功\n";
                            return true;
                            break;
                        }
                        case 2:{
                            std::cout << "正在退出...\n";
                            return false;
                            break;
                        }
                        default:std::cout <<"重新选择：\n";continue;break;
                    }
                }
            }
        }
        //还书功能
        bool return_book(std::string ISBN){
            int res = search_status(ISBN);
            if(res == 0){
                std::cout << "没有该书籍\n";
                return false;
            }else if(res < 0){
                std::cout << "该书籍已借出\n";
                std::cout << "还书选1，不还书且退出选2\n";
                int chioce = 0;
                std::cin >> chioce;
                while(1){
                    switch(chioce){
                        case 1:{
                            books[-res-1].set_status(true);
                            std::cout << "还书成功\n";
                            return true;
                            break;
                        }
                        case 2:{
                            std::cout << "正在退出...\n";
                            return false;
                            break;
                        }
                        default:std::cout <<"重新选择：\n";continue;break;
                    }
                }
            }else {
                std::cout << "该书籍未借出\n" << res;
                return false;
                
            }
        }
        //按ISBN查询图书信息
        void search_book(std::string ISBN){
            int res = search_status(ISBN);
            std::cout << "作者：" << books[res-1].get_Author() << "\n";
            std::cout << "书名：" << books[res-1].get_BookName() << "\n";
            if(books[res-1].get_status()){
                std::cout << "状态：可借\n";
            }else{
                std::cout << "状态不可借\n";
            }
        }
        //打印所有图书
        void prift_all_book(){
            for(const auto& book : books){
                std::cout << "作者：" << book.get_Author() << "\n";
                std::cout << "书名：" << book.get_BookName() << "\n";
                std::cout << "书编号：" << book.get_ISBN() << "\n";
                if(book.get_status()){
                    std::cout << "状态: 可借\n";  
                }else{
                    std::cout << "状态：不可借\n";
                }
            }
        }
    private:
        std::vector<Book> books;
};