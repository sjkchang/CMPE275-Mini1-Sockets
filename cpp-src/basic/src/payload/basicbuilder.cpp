#include <iomanip>
#include <iostream>
#include <sstream>

#include "payload/basicbuilder.hpp"

std::vector<std::string> basic::BasicBuilder::split(const std::string &s) {
  std::vector<std::string> rtn;

  // header: NNNN, the length of the payload
  auto hdr = s.substr(0, 4);
  auto plen = atoi(hdr.c_str());

  // payload - really what could go wrong?
  auto payload = s.substr(5, plen); // +1 for the comma separating header
  std::istringstream iss(payload);
  std::string ss;

  std::getline(iss, ss, ',');
  rtn.push_back(ss);
  std::getline(iss, ss, ',');
  rtn.push_back(ss);
  std::getline(iss, ss);
  rtn.push_back(ss);

  return rtn;
}

std::string basic::BasicBuilder::encode(const basic::Message &m) {

  // payload = group,name,text
  std::string payload = m.group() + "," + m.name() + "," + m.text();

  // a message = header + payload
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(4) << payload.length() << ","
      << payload; // NO! << std::ends;

  return oss.str();
}

basic::Message basic::BasicBuilder::decode(std::string raw) {
  std::vector<std::string> parts;
  std::istringstream iss(raw);
  std::string part;
  while (std::getline(iss, part, ',')) {
    parts.push_back(part);
  }

  if (parts.size() != 4) {
    throw std::invalid_argument("message format error: " + raw);
  }

  basic::Message m(parts[2], parts[1], parts[3]);
  return m;
}
