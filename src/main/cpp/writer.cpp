/*
 * Copyright (c), Pierre-Anthony Lemieux (pal@palemieux.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <KM_fileio.h>
#include <KM_prng.h>
#include <AS_02.h>
#include <AS_02_IAB.h>
#include <Metadata.h>
#include <assert.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif


namespace ASDCP {

  /* overloads needed for boost::program_options */

  std::istream& operator>>(std::istream& is, ASDCP::Rational& r) {

    std::string s;

    is >> s;

    std::vector<std::string> params;

    boost::split(params, s, boost::is_any_of("/"));

    if (params.size() == 0 || params.size() > 2) {
      throw std::runtime_error("Cannot read rational");
    }

    r = ASDCP::Rational(
      std::stoi(params[0]),
      params.size() == 1 ? 1 : std::stoi(params[1])
    );

    return is;
  }

  /* overloads needed for boost::program_options */

  std::ostream& operator<<(std::ostream& os, const ASDCP::Rational& r) {

    os << r.Numerator << "/" << r.Denominator;

    return os;
  }

}

namespace Kumu {
  /* overloads needed for boost::program_options */

  std::istream& operator>>(std::istream& is, Kumu::UUID& uuid) {

    std::string s;

    is >> s;

    if (!uuid.DecodeHex(s.c_str())) {
      throw std::invalid_argument("Bad UUID");
    }

    return is;
  }


  /* overloads needed for boost::program_options */

  std::ostream& operator<<(std::ostream& os, const Kumu::UUID& uuid) {

    char buf[128];

    if (uuid.EncodeString(buf, sizeof buf)) {

      os << buf;

    } else {

      throw std::invalid_argument("Bad UUID");

    }

    return os;
  }

}


int main(int argc, const char* argv[]) {

  ASDCP::Result_t result = ASDCP::RESULT_OK;

  /* initialize command line options */

  boost::program_options::options_description cli_opts{ "Wraps a directory of IA Frames into an IMF IAB Track File" };

  cli_opts.add_options()
    ("help", "Prints usage")
    ("fps", boost::program_options::value<ASDCP::Rational>()->default_value(ASDCP::EditRate_24), "Edit rate in the form of <numerator> '/' <denominator>, e.g. 24/1")
    ("assetid", boost::program_options::value<Kumu::UUID>(), "Asset UUID in hex notation, e.g. 8538b543169743dd9a08c6d8b4b1b7df")
    ("lang", boost::program_options::value<std::string>()->default_value("en"), "RFC 5646 Language Tag")
    ("out", boost::program_options::value<std::string>()->required(), "Output file path")
    ("in", boost::program_options::value<std::string>()->required(), "Input directory path");

  boost::program_options::variables_map cli_args;

  try {

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cli_opts), cli_args);

    boost::program_options::notify(cli_args);

    /* display help options */

    if (cli_args.count("help")) {
      std::cout << cli_opts << "\n";
      return 1;
    }

    const ASDCP::Dictionary* g_dict = &ASDCP::DefaultSMPTEDict();

    if (!g_dict) {
      throw std::runtime_error("Cannot open SMPTE dictionary");
    }

    /* build the list of IA Frames */

    std::vector<std::string> file_list;
    char next_file[Kumu::MaxFilePath];
    Kumu::DirScanner scanner;

    std::string dir_name = cli_args["in"].as<std::string>();

    result = scanner.Open(dir_name);

    if (result.Failure()) {
      throw std::runtime_error("Cannot open IA Frame directory");
    }

    while (scanner.GetNext(next_file).Success()) {

      /* skip hidden and special files */

      if (next_file[0] == '.') continue;

      std::string path = dir_name + "/" + next_file;

      /* skip directories*/

      if (Kumu::PathIsDirectory(path)) continue;

      file_list.push_back(path);
    }

    /* MXF writer */

    AS_02::IAB::MXFWriter writer;

    /* information about this software that will be written in the header metadata*/

    ASDCP::WriterInfo writer_info;

    const byte_t product_uuid[ASDCP::UUIDlen] = { 0x92, 0x7f, 0xc4, 0xd1, 0x89, 0xa3, 0x4f, 0x88, 0x88, 0xbb, 0xd3, 0x63, 0xed, 0x33, 0x08, 0x4a };

    std::copy(product_uuid, product_uuid + ASDCP::UUIDlen, writer_info.ProductUUID);
    writer_info.CompanyName = "Sandflow Consulting LLC";
    writer_info.ProductName = "iab-in-imf";
    writer_info.ProductVersion = "1.0-beta1";
    writer_info.LabelSetType = ASDCP::LS_MXF_SMPTE;

    if (cli_args.count("assetid")) {

      const Kumu::UUID& uuid = cli_args["assetid"].as<ASDCP::UUID>();

      std::copy(uuid.Value(), uuid.Value() + Kumu::UUID_Length, writer_info.AssetUUID);

    } else {

      Kumu::GenRandomUUID(writer_info.AssetUUID);

    }

    /* IAB subdescriptor */

    ASDCP::MXF::IABSoundfieldLabelSubDescriptor sub(g_dict);

    sub.RFC5646SpokenLanguage = "en";

    /* open the file for writing */

    std::vector<ASDCP::UL> conformsToSpec = { g_dict->ul(MDD_IMF_IABTrackFileLevel0) };

    result = writer.OpenWrite(
      cli_args["out"].as<std::string>(),
      writer_info,
      sub,
      conformsToSpec,
      cli_args["fps"].as<ASDCP::Rational>()
    );

    if (result.Failure()) {
      throw std::runtime_error("Cannot open output file");
    }

    /* write frames to file */

    std::vector<uint8_t> buffer;

    for (std::vector<std::string>::const_iterator i = file_list.begin(); i < file_list.end(); i++) {

      Kumu::FileReader m_File;

      result = m_File.OpenRead(*i);

      if (result.Failure()) {
        throw std::runtime_error("Cannot open frame: " + *i);
      }

      buffer.resize(m_File.Size());

      ui32_t read_count;

      result = m_File.Read(&buffer[0], buffer.size(), &read_count);

      if (result.Failure()) {
        throw std::runtime_error("Cannor read frame: " + *i);
      }

      result = writer.WriteFrame(&buffer[0], read_count);

      if (result.Failure()) {
        throw std::runtime_error("Write error frame: " + *i);
      }

      result = m_File.Close();

      if (result.Failure()) {
        throw std::runtime_error("Cannot close frame: " + *i);
      }

    }

    result = writer.Finalize();

    if (ASDCP_FAILURE(result)) {
      throw std::runtime_error(result.Message());
    }

  } catch (boost::program_options::error& e) {

    std::cout << e.what() << std::endl;
    std::cout << cli_opts << std::endl;
    return 1;

  } catch (std::runtime_error& e) {

    std::cout << e.what() << std::endl;
    return 1;
  }

  return 0;
}
