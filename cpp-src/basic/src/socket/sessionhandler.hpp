#ifndef SESSIONHANDLER_HPP
#define SESSIONHANDLER_HPP

#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define BUFFER_SIZE 65536
#define MESSAGE_SIZE 2048
#define HEADER_SIZE 5
#define MAX_REFRESH_RATE 3000
#define REFRESH_RATE_INCREMENT 250

namespace basic {

/**
 * @brief container for managing session metadata
 */
class Session {
public:
  int fd;
  unsigned long count;
  uint64_t lastTime;
  std::vector<char> overflow_buffer;

  // time tracking for session duration
  bool done = false;
  std::chrono::_V2::system_clock::time_point start;
  std::chrono::_V2::system_clock::time_point end;

public:
  Session() {
    fd = -1;
    count = 0;
    start = std::chrono::high_resolution_clock::now();
    overflow_buffer.reserve(MESSAGE_SIZE);
  }
  Session(int sock, unsigned long c) {
    fd = sock;
    count = c;
    start = std::chrono::high_resolution_clock::now();
    overflow_buffer.reserve(MESSAGE_SIZE);
  }

  Session(const Session &s);

  void pushToOverflow(const char *buf, int len);
  void incr(unsigned int by = 1);
  Session &operator=(const Session &from);
};

/**
 * @brief manages all connections (sessions) to the server
 */
class SessionHandler {
private:
  // compiler optimizer (-On) removes/leaves debug code
  static const int sDebug = 1; // 0,1,2

  bool good;
  unsigned int refreshRate;
  std::shared_ptr<std::thread> sessionThread;
  std::vector<Session> sessions;
  // char buf[2048];

public:
  SessionHandler();
  virtual ~SessionHandler() { stop(); }

  void start();
  void stop();
  void addSession(int sessionSock);
  virtual void process(const std::vector<std::string> &results);

  // if we use a handler per connection like the other lab examples in
  // python and java, we will create a problem with the resizing. Reason:
  // The container at some point will resize/order contents, this will fail
  // as move semantics do not work on the thread proc, cannot be copied.
  // Consequently, neither can the class.

  SessionHandler(const SessionHandler &sh) = delete;
  SessionHandler &operator=(const SessionHandler &from) = delete;

private:
  void run();
  bool cycle();
  std::vector<std::string> splitter(Session &s, const char *raw, int len);
  void optimizeAndWait(bool idle);
};

} // namespace basic

#endif
