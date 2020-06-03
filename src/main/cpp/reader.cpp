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
#include <sstream>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif

int main(int argc, const char* argv[]) {

  ASDCP::Result_t result = ASDCP::RESULT_OK;

  /* initialize command line options */

  boost::program_options::options_description cli_opts{ "Wraps a directory of IA Frames into an IMF IAB Track File" };

  cli_opts.add_options()
    ("help", "Prints usage")
    ("out", boost::program_options::value<std::string>()->required(), "Output directory path")
    ("in", boost::program_options::value<std::string>()->required(), "Input file")
    ("single", boost::program_options::value<std::string>(), "If present, IA frames are output in a single file with the specified name");

  boost::program_options::variables_map cli_args;

  try {

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cli_opts), cli_args);

    boost::program_options::notify(cli_args);

    /* display help options */

    if (cli_args.count("help")) {
      std::cout << cli_opts << "\n";
      return 1;
    }

    /* open input file */
     
    AS_02::IAB::MXFReader reader;

    result = reader.OpenRead(cli_args["in"].as<std::string>());

    if (result.Failure()) {
        throw std::runtime_error("Cannot open input file");
    }


    /* get file info */

    ASDCP::WriterInfo info;

    result = reader.FillWriterInfo(info);

    if (result.Failure()) {
        throw std::runtime_error("Cannot retrieve file info");
    }

    /* read and output frames */
    
    AS_02::IAB::MXFReader::Frame frame;

    uint32_t count;
    
    result = reader.GetFrameCount(count);

    if (result.Failure()) {
        throw std::runtime_error("Cannot read frame count");
    }

    bool single_frame = ! cli_args["single"].empty();

    std::ofstream f;

    if (single_frame) {
        
        f.open(cli_args["out"].as<std::string>() + "/" + cli_args["single"].as<std::string>());

    }
    
    for(uint32_t i = 0; i < count; i++) {

        result = reader.ReadFrame(i, frame);

        if (result.Failure()) {
            throw std::runtime_error("Cannot read frame");
        }

        if (! single_frame) {

            std::stringstream ss;

            ss << cli_args["out"].as<std::string>() << "/" << i << ".iab";

            f.open(ss.str());
        }

        if (! f.good()) {
            throw std::runtime_error("Cannot open output file");
        }

        f.write((const char*) frame.second, frame.first);

        if (!f.good()) {
            throw std::runtime_error("Cannot write file");
        }

        if (!single_frame) {

            f.close();

        }
    }

    /* close reader */

    result = reader.Close();

    if (ASDCP_FAILURE(result)) {
      throw std::runtime_error("Cannot close file");
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
