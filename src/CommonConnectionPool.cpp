#include "CommonConnectionPool.h"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>

#include "Connection.h"
#include <fstream>


ConnectionPool::ConnectionPool() {
  if (load_config_file() == false) {
    throw runtime_error("load config file failed.");
  }
  // 预先创建连接放入池子
  for (int i = 0; i < m_initial_size; ++i) {
    Connection* conn = new Connection();
    if (conn->connect(m_host, m_port, m_username, m_password, m_dbname)) {
      conn->refresh_alaive_time();
      m_connection_queue.push(conn);
      m_connection_count++;
    } else {
      delete conn;
      conn = nullptr;
      throw runtime_error("load config file failed.");
    }
  }
  m_producer_thread = thread(&ConnectionPool::produce_connection_task, this);
  m_scanner_thread = thread(&ConnectionPool::scanner_connection_task, this);
}


ConnectionPool::~ConnectionPool() {
  m_is_running = false;
  m_cv_consumer.notify_all();
  m_cv_producer.notify_all();
  m_cv_scanner.notify_all();

  if (m_producer_thread.joinable()) {
    m_producer_thread.join();
  }
  if (m_scanner_thread.joinable()) {
    m_scanner_thread.join();
  }

  while (!m_connection_queue.empty()) {
    Connection* conn = m_connection_queue.front();
    m_connection_queue.pop();
    delete conn;
    conn = nullptr;
  }
}


bool ConnectionPool::load_config_file() {
  ifstream ifs("mysql.ini");
  if (ifs.is_open() == false) {
    cout << "load config file failed!" << endl;
    return false;
  }
  string buf_line;
  while (getline(ifs, buf_line)) {
    if (buf_line.empty() || buf_line[0] == '#') {
      continue;
    }

    size_t pos = buf_line.find('=');
    if (pos == string::npos) {
      continue;
    }
    string key = buf_line.substr(0, pos);
    string value = buf_line.substr(pos + 1);

    if (key == "ip") {
      m_host = value;
    } else if (key == "port") {
      m_port = atoi(value.c_str());
    } else if (key == "username") {
      m_username = value;
    } else if (key == "password") {
      m_password = value;
    } else if (key == "dbname") {
      m_dbname = value;
    } else if (key == "initSize") {
      m_initial_size = atoi(value.c_str());
    } else if (key == "maxSize") {
      m_max_size = atoi(value.c_str());
    } else if (key == "maxIdleTime") {
      m_max_idle_time = atoi(value.c_str());
    } else if (key == "connectionTimeOut") {
      m_connection_timeout = atoi(value.c_str());
    }
  }
  return true;
}

// 懒汉单例
ConnectionPool* ConnectionPool::get_instance() {
  static ConnectionPool pool;
  return &pool;
}


// 生产者：创建新连接
// 队列空&未达到最大连接数时，创建新连接
// 新连接要重置存活时间再放入队列
void ConnectionPool::produce_connection_task() {
  while (true) {
    {
      unique_lock<mutex> lock(m_queue_mutex);
      m_cv_producer.wait(lock, [this]() {
        return (m_connection_queue.empty() && (m_connection_count < m_max_size)) || !m_is_running;
      });
      if (!m_is_running) {
        break;
      }
    }

    auto conn = make_unique<Connection>();
    bool isConnected = conn->connect(m_host, m_port, m_username, m_password, m_dbname);

    {
      unique_lock<mutex> lock(m_queue_mutex);
      if (isConnected) {
        conn->refresh_alaive_time();
        m_connection_queue.push(conn.release());
        m_connection_count++;
      }
    }
    m_cv_consumer.notify_one();

    // 延时延时
    if (!isConnected && m_is_running) {
      this_thread::sleep_for(chrono::milliseconds(1000));
    }
  }
}


// 消费者：获取一个连接
// 在等待时间内，是否有可用连接，没有return nullptr
// 有，将连接包装成自定义删除器的shared_ptr返回给他
// 删除器的逻辑：连接用完后，重置存活时间再放入队列中
shared_ptr<Connection> ConnectionPool::get_connection() {
  unique_lock<mutex> lock(m_queue_mutex);
  bool haveConn =
    m_cv_consumer.wait_for(lock, chrono::milliseconds(m_connection_timeout), [this]() {
      return !m_connection_queue.empty() || !m_is_running;
    });

  if (!haveConn || m_connection_queue.empty()) {
    return nullptr;
  }

  shared_ptr<Connection> sp_conn(m_connection_queue.front(), [this](Connection* conn) {
    unique_lock<mutex> lock(m_queue_mutex);
    if (m_is_running) {
      conn->refresh_alaive_time();
      m_connection_queue.push(conn);
      m_cv_consumer.notify_one();
    } else {
      delete conn;
      conn = nullptr;
    }
  });

  m_connection_queue.pop();

  if (m_connection_queue.empty()) {
    m_cv_producer.notify_one();
  }

  return sp_conn;
}


// 扫描线程：回收超过maxIdleTime的空闲连接
void ConnectionPool::scanner_connection_task() {
  while (m_is_running) {
    {
      unique_lock<mutex> lock(m_queue_mutex);
      m_cv_scanner.wait_for(lock, chrono::milliseconds(m_max_idle_time), [this]() {
        return !m_is_running;
      });
    }

    if (!m_is_running) {
      break;
    }

    bool is_connRemoved = false;
    while (true) {
      Connection* conn = nullptr;

      {
        unique_lock<mutex> lock(m_queue_mutex);
        if ((m_connection_count > m_initial_size) && !m_connection_queue.empty()) {
          conn = m_connection_queue.front();
          if (conn->get_alive_time() >= m_max_idle_time) {
            m_connection_queue.pop();
            m_connection_count--;
            is_connRemoved = true;
          } else {
            conn = nullptr;
            break; // 队头未超时，后面的更加不会超时
          }
        } else {
          break;
        }
      }

      if (conn != nullptr) {
        delete conn;
        conn = nullptr;
      }
    } // while
    if (is_connRemoved) {
      m_cv_producer.notify_one();
    }
  } // while
}
