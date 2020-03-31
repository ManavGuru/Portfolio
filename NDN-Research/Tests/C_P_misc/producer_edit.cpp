/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
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

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

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

#include <iostream>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces should be used to prevent/limit name conflicts
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
  onInterest(const InterestFilter&, filter const Interest& interest)
  {
    std::cout << ">> I: " << interest << std::endl;

    static const std::string content("Hello, world!");
	Name iName = interest.getName();
    Name::const_reverse_iterator it = iName.rbegin();
    while (!it->isNumber() && it != iName.rend()) {
      it++;
    }
	uint64_t id = 0;
    if (it != iName.rend()) {
      id = it->toNumber();
    }
	Name dataName(iName);
	if (Name("/vikingvillage").isPrefixOf(iName)) {
	  std::size_t HEADER_LEN = 40;
      std::string payload = "";
      std::string path_prefix = "/home/ndn1/test_files";
      ndn::name::Component last = iName.at(-1);
      std::string req_str = "";	
	    std::string fid = filename_str.substr(0, filename_str.find("_"));
      std::ifstream filename(path_prefix + filename_str);
      std::string file_content;
      filename_str = req_str;
	  if (!filename.is_open()) {
          std::cerr << "Error opening file `" << filename_str << "'" << std::endl;
        }
        else if (!filename.good()) {
          std::cerr << "Error: state of stream for file `" << filename_str << "' not good" << std::endl;
        }
        else
        {   
		      file_content = std::move(std::string((std::istreambuf_iterator<char>(filename)), (std::istreambuf_iterator<char>())));
		    }
		std::stringstream ss;
        ss << std::setfill('0') << std::setw(6) << fid;
        content += ss.str();
        ss.str("");
        ss << std::setfill('0') << std::setw(6) << file_content.size();
        content += ss.str();
        payload += file_content;
		if (content.length() < HEADER_LEN) {
        int append = HEADER_LEN - content.length();
        content += std::string(append, '0');
      }
      assert(content.length() == HEADER_LEN);
      content += payload;
	
	// Create Data packet
   /* auto data = make_shared<Data>(interest.getName());
    data->setFreshnessPeriod(10_s);
    data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());
*/
	shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());
    // Sign Data packet with default identity
    m_keyChain.sign(*data);
    // m_keyChain.sign(*data, signingByIdentity(<identityName>));
    // m_keyChain.sign(*data, signingByKey(<keyName>));
    // m_keyChain.sign(*data, signingByCertificate(<certName>));
    // m_keyChain.sign(*data, signingWithSha256());

    // Return Data packet to the requester
    std::cout << "<< D: " << *data << std::endl;
    m_face.put(*data);
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix '" << prefix
              << "' with the local forwarder (" << reason << ")" << std::endl;
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
};

} // namespace examples
} // namespace ndn

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
