# 线程数据竞争实验

这个文件目录下有三个相同结构的版本实例：
- 没有使用任何操作的数据争抢
- 使用std::mutex进行修复的
- 使用std::atomic进行修复的

## 编译
g++ -std=c++17 -pthread -O0 data_race_raw.cpp -o data_race_raw

g++ -std=c++17 -pthread -O0 data_race_mutex.cpp -o data_race_mutex

g++ -std=c++17 -pthread -O0 data_race_atomic.cpp -o data_race_atomic