#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "socket/client.hpp"

/**
 * @brief basic starting point
 *
 *      Author: gash
 */
int main(int argc, char **argv) {
  basic::BasicClient clt;
  clt.connect();

  std::stringstream msg;
  msg << "hello. My name is inigo montoya." << std::ends;
  clt.sendMessage(msg.str());

  std::cout << "sleeping a bit before exiting..." << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
