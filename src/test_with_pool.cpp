#include <iostream>
#include <thread>

#include "CommonConnectionPool.h"
#include "Connection.h"
#include <cstdlib>
using namespace std;

int main() {
  // 提前获取连接池实例，确保 ini 配置被加载，并建立初始长连接
  ConnectionPool* conn = ConnectionPool::get_instance();

  auto start = chrono::steady_clock::now();

  auto task = [conn]() {
    for (int i = 0; i < 2500; ++i) {
      char sql[1024] = {0};
      sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')", "zhang san", 20, "male");
      // 直接从连接池获取空闲的已有连接
      shared_ptr<Connection> sp = conn->get_connection();
      if (sp != nullptr) {
        sp->update(sql);
      } else {
        cout << "database connect failed." << endl;
        exit(EXIT_FAILURE);
      }
    }
  };

  thread t1(task);
  thread t2(task);
  thread t3(task);
  thread t4(task);

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  auto end = chrono::steady_clock::now();
  auto cost = chrono::duration_cast<chrono::milliseconds>(end - start).count();
  cout << "with-pool to insert 10k sql with 10 threads cost: " << cost << "ms" << endl;

  return 0;
}
