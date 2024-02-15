#include <cstdlib> // for std::atoi
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <num_messages>\n";
    return 1;
  }

  unsigned int numMessages = std::atoi(argv[1]);
  if (numMessages <= 0) {
    std::cerr << "Number of messages must be a positive integer.\n";
    return 1;
  }

  basic::BasicClient clt;
  clt.connect();

  for (int i = 0; i < numMessages; ++i) {
    std::stringstream msg;
    msg << "hello. My name is inigo montoya." << std::to_string(i) << std::ends;
    clt.sendMessage(msg.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(7));
  }

  std::cout << "sleeping a bit before exiting..." << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
