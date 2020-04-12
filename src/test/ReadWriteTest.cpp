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
#include <AS_02_IAB.h>
#include <Metadata.h>
#include <assert.h>
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

 /* authoring identification info written to file headers */

class IABWriterInfo : public ASDCP::WriterInfo {
public:
  IABWriterInfo() {
    static byte_t default_ProductUUID_Data[ASDCP::UUIDlen] =
    { 0x92, 0x7f, 0xc4, 0xd1, 0x89, 0xa3, 0x4f, 0x88, 0x88, 0xbb, 0xd3, 0x63, 0xed, 0x33, 0x08, 0x4a };

    memcpy(this->ProductUUID, default_ProductUUID_Data, ASDCP::UUIDlen);
    this->CompanyName = "Sandflow Consulting LLC";
    this->ProductName = "iab-in-imf";
    this->ProductVersion = "1.0-beta1";
    this->LabelSetType = ASDCP::LS_MXF_SMPTE;

    Kumu::GenRandomUUID(this->AssetUUID);
  }
};


int main(int argc, const char* argv[]) {

  Kumu::Result_t result = ASDCP::RESULT_OK;


  const ASDCP::Dictionary* g_dict = &ASDCP::DefaultSMPTEDict();

  if (!g_dict) {
    return 3;
  }

  /* MXF writer */

  AS_02::IAB::MXFWriter writer;

  /* information about this software that will be written in the header metadata*/

  IABWriterInfo writer_info;

  /* IAB subdescriptor */

  ASDCP::MXF::IABSoundfieldLabelSubDescriptor sub(g_dict);

  sub.RFC5646SpokenLanguage = "en";

  /* open the file for writing */

  std::vector<ASDCP::UL> conformsToSpec = { g_dict->ul(MDD_IMF_IABTrackFileLevel0) };

  result = writer.OpenWrite(
    "iab.mxf",
    writer_info,
    sub,
    conformsToSpec,
    ASDCP::Rational(24, 1)
  );

  if (result.Failure()) {
    return result;
  }

  /* write frames */

  const int frame_sz = 1024;

  std::vector<ui8_t> frame_header = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x40, 0x00 };

  /* frame 1 */

  std::vector<ui8_t> frame_1(frame_header);

  std::fill_n(std::back_inserter(frame_1), 1024, 0x11);

  result = writer.WriteFrame(frame_1.data(), frame_1.size());

  if (result.Failure()) {
    return result;
  }


  /* frame 2 */

  std::vector<ui8_t> frame_2(frame_header);

  std::fill_n(std::back_inserter(frame_2), 1024, 0x22);

  result = writer.WriteFrame(frame_2.data(), frame_2.size());

  if (ASDCP_FAILURE(result)) {
    throw std::runtime_error(result.Message());
  }

  /* write the file */

  result = writer.Finalize();

  if (result.Failure()) {
    return result;
  }

  /* MXF reader */

  AS_02::IAB::MXFReader reader;

  /* open the file for reading */

  result = reader.OpenRead("iab.mxf");

  if (result.Failure()) {
    return result;
  }

  /* get file info */

  ASDCP::WriterInfo info;

  result = reader.FillWriterInfo(info);

  if (result.Failure()) {
    return result;
  }

  /* read frame 1 */

  AS_02::IAB::MXFReader::Frame frame;

  result = reader.ReadFrame(0, frame);

  if (result.Failure()) {
    return result;
  }

  bool equal = std::equal(std::begin(frame_1), std::end(frame_1), frame.second);

  if (!equal) {
    return result;
  }

  /* read frame 2 */

  result = reader.ReadFrame(1, frame);

  if (result.Failure()) {
    return result;
  }

  equal = std::equal(std::begin(frame_2), std::end(frame_2), frame.second);

  if (!equal) {
    return 2;
  }

  /* close reader */

  result = reader.Close();

  if (result.Failure()) {
    return result;
  }

  return 0;
}
