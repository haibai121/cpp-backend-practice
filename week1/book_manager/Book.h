#include <iostream>

class Book{
    public:
        Book(std::string book_name, std::string ISBN, std::string author)
        : book_name_(book_name), ISBN_(ISBN), author_(author), status_(true){}
        //get
        std::string get_BookName() const{
            return book_name_;
        }
        std::string get_ISBN() const{
            return ISBN_;
        }
        std::string get_Author() const{
            return author_;
        }
        bool get_status() const{
            return status_;
        } 
        //set
        void set_status(bool status){
            status_ = status;
        }
    private:
        std::string book_name_;
        std::string ISBN_;
        std::string author_;
        bool status_;
};