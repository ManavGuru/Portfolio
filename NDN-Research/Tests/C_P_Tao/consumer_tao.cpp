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
#include "face.hpp"
#include "util/logger.hpp"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <string>

#define ERROR_INVALID_ARGUMENT 1
#define BUFLEN 1024
#define MAX_PAYLOAD_OUTPUT_SIZE 20

NDN_LOG_INIT(consumer);

namespace po = boost::program_options;

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces can be used to prevent/limit name conflicts
namespace examples {

namespace ip = boost::asio::ip;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

class Consumer : noncopyable
{
public:
  Consumer(std::string prefix, int verbose, boost::asio::io_service& io_service, int port, bool udp, std::string chunk)
  : m_prefix(prefix)
  , m_verbose(verbose)
  , m_chunk(chunk)
  {
    m_listen = false;
    if (chunk == "-1") {
      m_listen = true;
      if (udp) {
        std::cout << "Listening on UDP port " << port << std::endl;
        m_udp_socket_ptr = boost::make_shared<udp::socket>(boost::ref(io_service));
        m_udp_socket_ptr->open(udp::v4());
        m_udp_socket_ptr->set_option(ip::udp::socket::reuse_address(true));
        m_udp_socket_ptr->bind(udp::endpoint(ip::address_v4::any(), port));
      }
      else {
        std::cout << "Listening on TCP port " << port << std::endl;
        m_socket_ptr   = boost::make_shared<tcp::socket>(boost::ref(io_service));
        m_acceptor_ptr = boost::make_shared<tcp::acceptor>(boost::ref(io_service), tcp::endpoint(tcp::v4(), port));
        m_acceptor_ptr->accept(*m_socket_ptr);
      }
    }
  }

  ~Consumer()
  {
    if (m_udp_socket_ptr != nullptr) {
      m_udp_socket_ptr->close();
    }
    if (m_socket_ptr != nullptr) {
      m_socket_ptr->close();
    }
    if (m_acceptor_ptr != nullptr) {
      m_acceptor_ptr->close();
    }
  }

  void
  run()
  {
    if (m_listen) {
      // cf. https://www.boost.org/doc/libs/1_58_0/doc/html/boost_asio/tutorial/tutdaytime5/src.html
      for (;;) {
        boost::array<char, BUFLEN> recv_buf; // TODO: magic number
        boost::system::error_code error;
        size_t len = 0;
        if (m_udp_socket_ptr != nullptr) {
          len = m_udp_socket_ptr->receive_from(boost::asio::buffer(recv_buf, BUFLEN),
              m_remote_endpoint, 0, error);
        }
        else {
          assert(m_socket_ptr != nullptr);
          len = m_socket_ptr->receive(boost::asio::buffer(recv_buf, BUFLEN), 0);
        }
        if (m_verbose > 2) {
          std::cout << "B: ";
          std::cout.write(recv_buf.data(), len);
          std::cout << std::endl;
        }

        if (error && error != boost::asio::error::message_size)
          throw boost::system::system_error(error);

        Name n(recv_buf.data());
        sendInterest(n);
      }
    }
    else {
      Name n(m_prefix);
      if (m_prefix == "/example") {
        n = "/example/testApp/data";
      }
      if (!boost::starts_with(m_prefix, "/vikingvillage")) {
        // Assume it is a chunk ID.
        int chunk_id;
        try {
          chunk_id = std::stoi(m_chunk);
        }
        catch (const std::exception& e) {
          std::cerr << "Error: invalid chunk ID. Nonnegative integer required." << std::endl;
          exit(1);
        }
        n.appendNumber(chunk_id);
      }
      else {
        n.append(Name(m_chunk));
      }
      sendInterest(n);
    }
  }

private:
  void
  sendInterest(const Name& name)
  {
    Interest interest;
    interest.setCanBePrefix(true);
    interest.setName(name);
    interest.setInterestLifetime(time::milliseconds(1000));
    interest.setMustBeFresh(true);

    m_tx_time = time::steady_clock::now();
    if (m_verbose > 2) {
      NDN_LOG_WARN("sendInterest"); // log tx timestamp
    }
    m_face.expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2),
                           bind(&Consumer::onNack, this, _1, _2),
                           bind(&Consumer::onTimeout, this, _1));

    switch (m_verbose) {
      case 3:
      case 2:
        std::cout << ">> I: " << interest << std::endl;
        break;
      case 1:
        std::cout << "." << std::flush;
        break;
      case 0:
      default:
        break;
    }
    // processEvents will block until requested data received or timeout occurs
    m_face.processEvents();
  }

  void
  onData(const Interest& interest, const Data& data)
  {
    time::steady_clock::TimePoint rx_time = time::steady_clock::now();
    Block content = data.getContent();
    std::string payload = std::string(content.value_begin(), content.value_end());
    if (m_verbose > 2) {
      std::cout << "payload size="<< payload.size() << std::endl;
      if (Name("/vikingvillage").isPrefixOf(interest.getName())) {
        std::cout << "header: " << payload.substr(0, 40) << std::endl;
      }
    }
    if (m_listen) { //Need to handle gracefully
      boost::system::error_code error_ignored;
      if (m_udp_socket_ptr != nullptr) {
        m_udp_socket_ptr->send_to(boost::asio::buffer(payload), m_remote_endpoint, 0, error_ignored);
      }
      else {
        boost::asio::write(*m_socket_ptr, boost::asio::buffer(payload), boost::asio::transfer_all(), error_ignored);
      }

      if (error_ignored && error_ignored != boost::asio::error::message_size)
          throw boost::system::system_error(error_ignored);
    }
    switch (m_verbose) {
      case 3:
      case 2:
        if (m_prefix == "/vikingvillage") {
          std::cout << "<< D: " << data.getName() << std::endl;
        }
        else {
          std::string payload_trimmed = payload;
          if (payload.size() > MAX_PAYLOAD_OUTPUT_SIZE) {
            payload_trimmed = payload.substr(0, MAX_PAYLOAD_OUTPUT_SIZE) + "...";
          }
          std::cout << "<< D: " << data << "Payload: " << payload_trimmed << std::endl;
        }
        break;
      case 1:
        std::cout << "o" << std::flush;
        break;
      case 0:
      default:
        break;
    }
    if (m_verbose >= 2) {
      boost::chrono::duration<double, boost::milli> ms = rx_time - m_tx_time;
      std::cout.setf(std::ios::fixed, std::ios::floatfield);
      std::cout.precision(2);
      std::cout << "RTT: " << ms.count() << " ms" << std::endl;
    }
  }

  void
  onNack(const Interest& interest, const lp::Nack& nack)
  {
    switch (m_verbose) {
      case 3:
      case 2:
        std::cout << "received Nack with reason " << nack.getReason()
          << " for interest " << interest << std::endl;
        break;
      case 1:
        std::cout << "x" << std::flush;
        break;
      default:
        break;
    }
  }

  void
  onTimeout(const Interest& interest)
  {
    switch (m_verbose) {
      case 3:
      case 2:
        std::cout << "Timeout " << interest << std::endl;
      default:
        break;
    }
  }

private:
  Face m_face;
  time::steady_clock::TimePoint m_tx_time;
  std::string m_prefix;
  int m_verbose;
  bool m_listen;
  std::string m_chunk;
  boost::shared_ptr<udp::socket>   m_udp_socket_ptr;
  udp::endpoint m_remote_endpoint;  // remote endpoint when listening on UDP
  boost::shared_ptr<tcp::socket>   m_socket_ptr;
  boost::shared_ptr<tcp::acceptor> m_acceptor_ptr;
};

} // namespace examples
} // namespace ndn

static void
printUsage(std::ostream& os, const char* programName, const po::options_description& opts)
{
  os << "Usage: " << programName << " [options]\n"
     << "\n"
     << "Send interest packets for named data\n"
     << "\n"
     << opts;
}

int
main(int argc, char** argv)
{
  int port;
  int verbose;
  std::string chunk;
  bool udp;
  std::string prefix;
  po::positional_options_description pd;
  pd.add("chunk", 1);
  po::options_description description("Options");
  description.add_options()
    ("help,h",    "print this message and exit")
    ("version,V", "show version information and exit")
    ("verbose,v", po::value<int>(&verbose)->default_value(2),
                  "verbosity level, 0 to 2 (default: 2)")
    ("port,p",    po::value<int>(&port)->default_value(60000),
                  "port number for listening (default: 60000)")
    ("udp,u",     po::value<bool>(&udp)->default_value(false)->implicit_value(true),
                  "listen on UDP socket instead of TCP socket (default: false)")
    ("chunk,c",   po::value<std::string>(&chunk)->default_value("-1"),
                  "data chunk for single request when not listening (default: -1 for listening)")
    ("prefix,e",  po::value<std::string>(&prefix)->default_value("/vikingvillage"),
                  "produce data packet for this name prefix (default: /vikingvillage)")
    ;

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(description).positional(pd).run(), vm);
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

  boost::asio::io_service io_service;
  ndn::examples::Consumer consumer(prefix, verbose, io_service, port, udp, chunk);
  try {
    consumer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
