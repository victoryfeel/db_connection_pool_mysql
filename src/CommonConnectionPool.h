#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "Connection.h"


using namespace std;
class ConnectionPool {
public:
  static ConnectionPool* get_instance();
  shared_ptr<Connection> get_connection();

  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;
  ConnectionPool(ConnectionPool&& other) = delete;
  ConnectionPool& operator=(ConnectionPool&& other) = delete;

private:
  ConnectionPool();
  ~ConnectionPool();

  bool load_config_file();
  void produce_connection_task();
  void scanner_connection_task();

  string m_host;
  uint16_t m_port;
  string m_username;
  string m_password;
  string m_dbname;

  int m_initial_size;
  int m_max_size;
  int m_max_idle_time;
  int m_connection_timeout;

  queue<Connection*> m_connection_queue;
  mutex m_queue_mutex;
  atomic<int> m_connection_count{0};
  atomic<bool> m_is_running{true};

  condition_variable m_cv_consumer;
  condition_variable m_cv_producer;
  condition_variable m_cv_scanner;

  thread m_producer_thread;
  thread m_scanner_thread;
};
