#pragma once
#include <vector>
#include <iostream>
#include <limits>      // for numeric_limits
#include <ios>         // for std::streamsize
#include "Student.h"

class StudentManager {
public:
    // 添加学生
    void add_student() {
        std::string name;
        int id;
        double score;

        std::cout << "姓名: ";
        std::cin >> name;

        std::cout << "学号: ";
        std::cin >> id;

        std::cout << "成绩: ";
        std::cin >> score;

        // 清除输入缓冲区（防止后续 cin 失败）
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // ✅ 只 push 一次！不要重复添加
        students.push_back(Student(name, id, score));
        std::cout << "学生添加成功！\n";
    }

    // 按学号查询
    int search_by_ID(int sid) const {  // const 放后面
        for (const auto& stu : students) {
            if (stu.getID() == sid) {
                std::cout << "姓名: " << stu.getName()
                          << ", 学号: " << stu.getID()
                          << ", 成绩: " << stu.getScoure() << "\n";
                return sid;
            }
        }
        std::cout << "没找到\n";
        return -1;
    }

    // 计算所有学生的平均分
    void average() const {  // const 放后面
        if (students.empty()) {
            std::cout << "没有学生，无法计算平均分。\n";
            return;
        }

        double total = 0.0;
        for (const auto& stu : students) {
            total += stu.getScoure();
        }
        double avg = total / students.size();
        std::cout << "平均分: " << avg << "\n";
    }

    // 打印学生信息
    void printStudent() const {  // const 放后面
        if (students.empty()) {
            std::cout << "没找到学生\n";
            return;
        }
        for (const auto& stu : students) {
            std::cout << "姓名: " << stu.getName()
                      << ", 学号: " << stu.getID()
                      << ", 成绩: " << stu.getScoure() << "\n";
        }
    }

private:
    std::vector<Student> students;
};