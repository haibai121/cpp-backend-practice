#include <iostream>

class Student{
    public:
        Student(std::string StuName, int StuID, double StuScoure)
            : name_(StuName), id_(StuID), scoure_(StuScoure){}
        int getID() const{
            return id_;
        }
        std::string getName() const{
            return name_;
        }
        double getScoure() const{
            return scoure_;
        }
    private:
        std::string name_;
        int id_;
        double scoure_;
};