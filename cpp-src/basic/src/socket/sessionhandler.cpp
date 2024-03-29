
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "payload/basicbuilder.hpp"
#include "socket/sessionhandler.hpp"

void basic::Session::incr(unsigned int by) {
  if (by > 0) {
    this->count += by;
  }
}

basic::Session::Session(const basic::Session &s)
    : fd(s.fd), count(s.count),
      start(std::chrono::high_resolution_clock::now()) {
  overflow_buffer.reserve(MESSAGE_SIZE);
}

basic::Session &basic::Session::operator=(const Session &from) {
  this->fd = from.fd;
  this->count = from.count;
  return *this;
}

/*
 * Pushes the buffer from pos to len to the overflow buffer
 */
void basic::Session::pushToOverflow(const char *buf, int len) {
  overflow_buffer.insert(overflow_buffer.end(), &buf[0], &buf[len]);
}

basic::SessionHandler::SessionHandler() {
  this->good = true;
  this->refreshRate = 0;
}

void basic::SessionHandler::stop() {
  this->good = false;
  for (auto &sitr : this->sessions) {
    Session &session = sitr;
    try {
      if (session.fd >= 0) {
        ::close(session.fd);
      }
    } catch (...) {
    }
    session.fd = -1;
  }
  this->sessions.clear();

  // wait for the thread to complete before ending
  try {
    this->sessionThread->join();
  } catch (...) {
  }
}

void basic::SessionHandler::start() {
  // C++17 lambda dependent
  this->sessionThread = std::make_shared<std::thread>([this] { run(); });
}

void basic::SessionHandler::addSession(int sessionSock) {
  Session s(sessionSock, 0);
  s.start = std::chrono::high_resolution_clock::now();
  this->sessions.emplace_back(std::move(s));
}

/**
 *
 */
void basic::SessionHandler::run() {
  while (this->good) {
    auto idle = true;
    try {
      idle = cycle();
    } catch (const std::exception &e) {
      // for purposes of the lab only. Normally, we would
      // try to manage (contain) the error and continue running.
      std::cerr << "---> aborting handler failure: " << e.what() << std::endl;
      if (sDebug > 0) {
        std::abort();
      }
    }

    // This is a hook for adaptive polling strategies. You can
    // experiment with priorization and fairness algorithms.
    optimizeAndWait(idle);
  }
}

/**
 *
 */
bool basic::SessionHandler::cycle() {
  bool idle = true;
  std::array<char, BUFFER_SIZE> raw = {0};
  for (auto &session : this->sessions) {
    if (session.fd == -1) {
      continue;
    }

    /*
       important errno values:
       35 (EWOULDBLOCK)  - nonblocking mode, no data is available (not an
       error) 38 (ENOTSOCK)     - socket descriptor (fd) is not valid 42
       (EPROTOTYPE)   - socket option not supported 54 (ECONNRESET)   -
       connection not available (bad) 60 (ETIMEDOUT)    - connection (read)
       timed out (maybe bad)

       Ref:
       https://www.ibm.com/docs/en/zos/2.2.0?topic=errnos-sockets-return-code
    */

    // reusable buffer to minimize memory fragmentation
    std::memset(raw.data(), 0, raw.size());

    // auto n = ::recv(session.fd,raw,Session::sOFSize-1,0);
    auto n = ::read(session.fd, raw.data(), raw.size() - 1);

    if (sDebug > 1 && n > 0) {
      std::cerr << "---> session " << session.fd << " got n = " << n
                << ", errno = " << errno << std::endl;
    }

    if (n > 0) {
      idle = false;
      auto results = splitter(session, raw.data(), n);
      session.incr(results.size());
      process(results);
      results.clear();
    } else if (n == -1) {
      if (errno == EWOULDBLOCK) {
      } /*read timeout - okay*/
      else if (errno == ECONNRESET) {
      }
    } else {

      if (!session.done) {
        session.done = true;
        // No Data, Client Closed Connection
        session.end = std::chrono::high_resolution_clock::now();

        // print start and end time
        std::chrono::duration<double> elapsed_seconds =
            session.end - session.start;
        std::cerr << "Session " << session.fd << " lasted "
                  << elapsed_seconds.count() << "s\n"
                  << std::endl;
      }
      std::cerr << "--> a session was closed, [id: " << session.fd
                << ", cnt: " << session.count << "]" << std::endl;

      // ref: https://en.wikipedia.org/wiki/Erase–remove_idiom
      auto xfd = session.fd;
      this->sessions.erase(
          std::remove_if(this->sessions.begin(), this->sessions.end(),
                         [&xfd](const Session &s) { return s.fd == xfd; }),
          this->sessions.end());
      idle = false;
      break;
    }
  }

  return idle;
}

/**
 *
 */
void basic::SessionHandler::process(const std::vector<std::string> &results) {
  basic::BasicBuilder b;
  for (const auto &s : results) {
    auto m = b.decode(s);

    /*
    // PLACEHOLDER: now do something with the message
    std::cerr << "M: [" << m.group() << "] " << m.name() << " - " << m.text()
              << std::endl;
    std::cerr.flush();
    */
  }
}

/**
 *
 */
void basic::SessionHandler::optimizeAndWait(bool idle) {

  // if (optimize) {
  //    // todo add code to optimize processing
  // }

  if (idle) {
    // gradually slow down polling while no activity
    if (this->refreshRate < MAX_REFRESH_RATE) {
      this->refreshRate += REFRESH_RATE_INCREMENT;
    }

    if (sDebug > 0) {
      std::cerr << this->sessions.size() << " sessions, sleeping "
                << this->refreshRate << " ms..." << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(this->refreshRate));
  } else {
    this->refreshRate = 0;
  }
}

/**
 *
 */
std::vector<std::string>
basic::SessionHandler::splitter(Session &s, const char *raw, int len) {
  std::vector<std::string> results;
  if (raw == nullptr || len <= 0) {
    return results;
  }

  // If the overlow buffer is not empty, then we need to append the new raw
  // data to the overflow buffer and process the combined buffer.
  std::string buffer;
  if (!s.overflow_buffer.empty()) {
    buffer = std::string(s.overflow_buffer.begin(), s.overflow_buffer.end()) +
             std::string(raw, len);
    raw = buffer.c_str();
    len = buffer.size();
    s.overflow_buffer.clear();
  }

  auto pos = 0;
  const auto *ptr = &raw[0];
  while (pos < len) {
    try {

      // Message length header is inclomplete, push to overflow
      if (len - pos < HEADER_SIZE) {
        // Message header is incomplete, push to overflow buffer
        s.pushToOverflow(&ptr[pos], len - pos);
        break;
      }

      // Message length header is complete, parse message
      int mlen = std::stoi(std::string(&ptr[pos], HEADER_SIZE - 1));
      if (mlen > len - (pos + HEADER_SIZE)) {
        // Message is incomplete, push to overflow buffer
        s.pushToOverflow(&ptr[pos], len - pos);
        break;
      }

      // Message is complete, parse message
      auto msg = std::string(&ptr[pos], mlen + HEADER_SIZE);
      pos += HEADER_SIZE + mlen; // NNNN+1 for comma

      // Message is Parsed
      if (pos <= len + 1) {
        if (sDebug > 1) {
          std::cerr << "new msg: " << msg << std::endl;
        }
        results.push_back(msg);

        // Skip nulls if any
        while (ptr[pos] == '\0' && pos < len)
          pos++;
      }
    } catch (const std::exception &e) {
      std::stringstream err;
      err << "error processing raw: " << e.what() << std::endl;
      throw std::runtime_error(err.str());
    }
  }

  if (sDebug > 1) {
    std::cerr << "---> got " << results.size() << " messages" << std::endl;
  }

  return results;
}
