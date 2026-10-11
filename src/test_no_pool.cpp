#include <chrono>
#include <iostream>
#include <thread>

#include "Connection.h"

int main() {
  auto start = chrono::steady_clock::now();

  auto task = []() {
    for (int i = 0; i < 2500; ++i) {
      Connection conn;
      char sql[1024] = {0};
      sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')", "zhang san", 20, "male");
      if (conn.connect("192.168.145.128", 3306, "root", "225166", "conn-pool")) {
        conn.update(sql);

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

  cout << "non-pool to insert 10k sql with 10 threads cost: " << cost << "ms" << endl;

  return 0;
}
