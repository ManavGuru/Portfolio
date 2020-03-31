/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2018 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

// correct way to include ndn-cxx headers
// #include <ndn-cxx/face.hpp>
// #include <ndn-cxx/security/key-chain.hpp>
#include "face.hpp"
#include "security/key-chain.hpp"
#include "fifo-cache.hpp"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <queue>

#define ERROR_INVALID_ARGUMENT 1

namespace po = boost::program_options;

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces can be used to prevent/limit name conflicts
namespace examples {

class Producer : noncopyable
{
public:
  Producer(int verbose, int size, std::string prefix, int cache)
    : m_verbose(verbose)
    , m_size(size)
    , m_prefix(prefix)
    , m_wire_size_printed(false)
    , m_cache(cache)
  {
  }

  void
  run()
  {
    m_face.setInterestFilter(m_prefix,
                             bind(&Producer::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&Producer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  void
  onInterest(const InterestFilter& filter, const Interest& interest)
  {
    switch (m_verbose) {
      case 2:
        std::cout << "<< I: " << interest << std::endl;
        break;
      case 1:
        std::cout << "." << std::flush;
        break;
      case 0:
      default:
        break;
    }
    Name iName = interest.getName();
    Name::const_reverse_iterator it = iName.rbegin();
    while (!it->isNumber() && it != iName.rend()) {
      it++;
    }
    uint64_t id = 0;
    if (it != iName.rend()) {
      id = it->toNumber();
    }

    // Create new name, based on Interest's name
    Name dataName(iName);
    dataName
      .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

    std::string content;
    if (m_size == -1) {
      content = "HELLO KITTY " + std::to_string(id);
    }
    else {
      content = std::string(m_size, 'a');
    }

    if (Name("/vikingvillage").isPrefixOf(iName)) {
      /*if (m_cache > 0) {
        fifo_cache c1(m_cache);
        std::cout << "Cache created of size: " << c1.get_size() << std::endl;
      }*/

      content = "SJ";  // magic marker
      std::size_t HEADER_LEN = 40;
      std::string payload = "";
      std::string path_prefix = "/mnt/c/videos/complete_videos/";
      ndn::name::Component last = iName.at(-1);
      std::string req_str = "";
      if (last.isVersion()) {
        req_str = iName.at(-2).toUri();
      }
      else {
        req_str = iName.at(-1).toUri();
      }
      if (m_verbose > 2) {
        std::cout << "req_str=" << req_str << std::endl;
      }
      std::size_t i;
      do {
        i = req_str.find("%2C");
        std::string filename_str;
        if (i != std::string::npos) {
          filename_str = req_str.substr(0, i);
          req_str = req_str.substr(i + std::strlen("%2C"));
        }
        else {
          filename_str = req_str;
        }
        std::string fid = filename_str.substr(0, filename_str.find("_"));
        std::ifstream filename(path_prefix + filename_str);
        std::string file_content;
        if (!filename.is_open()) {
          std::cerr << "Error opening file `" << filename_str << "'" << std::endl;
        }
        else if (!filename.good()) {
          std::cerr << "Error: state of stream for file `" << filename_str << "' not good" << std::endl;
        }
        else {
          if (m_cache > 0) {
            fifo_cache c1(m_cache);
            file_content = c1.get_frames(filename_str);
            std::cout << "getting data from cache of size: " << c1.get_size() << std::endl;
          }
          else
          file_content = std::move(std::string((std::istreambuf_iterator<char>(filename)), (std::istreambuf_iterator<char>())));
        }
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(6) << fid;
        content += ss.str();
        ss.str("");
        ss << std::setfill('0') << std::setw(6) << file_content.size();
        content += ss.str();
        payload += file_content;
      } while (i != std::string::npos);
      if (content.length() < HEADER_LEN) {
        int append = HEADER_LEN - content.length();
        content += std::string(append, '0');
      }
      assert(content.length() == HEADER_LEN);
      content += payload;
    }

    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());

    // Sign Data packet with default identity
    m_keyChain.sign(*data);
    // m_keyChain.sign(data, <identityName>);
    // m_keyChain.sign(data, <certificate>);
    // Return Data packet to the requester
    if (!m_wire_size_printed) {
      std::cout << "\nData pkt wire size=" << data->wireEncode().size() << std::endl;
      m_wire_size_printed = true;
    }
    switch (m_verbose) {
      case 2:
        std::cout << ">> D: " << *data << std::endl;
        break;
      case 1:
        std::cout << "o" << std::flush;
        break;
      case 0:
      default:
        break;
    }
    m_face.put(*data);
  }


  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  int m_verbose;
  int m_size;
  std::string m_prefix;
  bool m_wire_size_printed;
  int m_cache;
};

} // namespace examples
} // namespace ndn

static void
printUsage(std::ostream& os, const char* programName, const po::options_description& opts)
{
  os << "Usage: " << programName << " [options]\n"
     << "\n"
     << "Produce data packets for NDN consumers\n"
     << "\n"
     << opts;
}

int
main(int argc, char** argv)
{
  int verbose;
  int size;
  std::string prefix;
  int cache;
  po::options_description description("Options");
  description.add_options()
    ("help,h",    "print this message and exit")
    ("version,V", "show version information and exit")
    ("size,s",    po::value<int>(&size)->default_value(-1),
                  "custom data payload size in bytes (default: -1 for HELLO KITTY)")
    ("verbose,v", po::value<int>(&verbose)->default_value(2),
                  "verbosity level, 0 to 2 (default: 2)")
    ("prefix,e", po::value<std::string>(&prefix)->default_value("/vikingvillage"),
                  "produce data packet for this name prefix (default: /vikingvillage)")
    ("cache,c", po::value<int>(&cache)->default_value(0),
                   "enabling caching of data (default: 0 for no caching)")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    printUsage(std::cerr, argv[0], description);
    return 2;
  }

  if (vm.count("help") > 0) {
    printUsage(std::cout, argv[0], description);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "0.5.0" << std::endl;
    return 0;
  }

  if (size < -1) {
    std::cerr << "Error: invalid data payload size!" << std::endl;
    return ERROR_INVALID_ARGUMENT;
  }

  if (verbose < 0 || verbose > 2) {
    std::cerr << "Error: verbosity level out of range!" << std::endl;
    return ERROR_INVALID_ARGUMENT;
  }

  if (cache < 0) {
    std::cerr << "Error: invalid cache info" << std::endl;
    return ERROR_INVALID_ARGUMENT;
  }

  ndn::examples::Producer producer(verbose, size, prefix, cache);
  try {
    producer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
