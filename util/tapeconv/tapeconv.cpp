
// tapeconv.cpp
// Copyright (C) 2010 Istvan Varga <istvanv@users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "ep128emu.hpp"
#include "tape.hpp"

#include <cmath>
#include <typeinfo>

using Ep128Emu::Exception;

static void printUsage()
{
  std::printf("Usage:\n");
  std::printf("    tapeconv [OPTIONS...] <INFILE> <OUTFILE>\n");
  std::printf("Options:\n");
  std::printf("    --\n");
  std::printf("        interpret all remaining arguments as file names\n");
  std::printf("    -h | -help | --help\n");
  std::printf("        print this message\n");
  std::printf("    -n | --new\n");
  std::printf("        force creating new ep128emu format output file\n");
  std::printf("    -a | --append\n");
  std::printf("        append to output file if it already exists\n");
  std::printf("    -ic | --input-channel <N>\n");
  std::printf("        channel number to be used in audio input file\n");
  std::printf("    -oc | --output-channel <N>\n");
  std::printf("        channel number to be used in audio output file\n");
  std::printf("    -fmin | --min-freq <N>\n");
  std::printf("    -fmax | --max-freq <N>\n");
  std::printf("        audio input file filter parameters (disabled if "
              "fmin >= fmax)\n");
  std::printf("    -sr | --sample-rate <N>\n");
  std::printf("        sample rate for new output file\n");
  std::printf("    -r | --resample <N>\n");
  std::printf("        force resampling for a ratio of out_sr/in_sr = N\n");
  std::printf("    -k | --skip <N>\n");
  std::printf("        skip N seconds of input\n");
  std::printf("    -l | --length <N>\n");
  std::printf("        limit output length to N seconds\n");
}

int main(int argc, char **argv)
{
  bool    forceNewFile = false;
  bool    appendMode = false;
  int     inputChannel = 0;
  int     outputChannel = 0;
  float   filterMinFreq = 0.0f;
  float   filterMaxFreq = -1.0f;
  float   resampleRatio = -1.0f;
  float   skipSeconds = -1.0f;
  float   lengthLimit = -1.0f;
  long    sampleRate = -1L;
  Ep128Emu::Tape  *inputFile = (Ep128Emu::Tape *) 0;
  Ep128Emu::Tape  *outputFile = (Ep128Emu::Tape *) 0;

  try {
    std::vector< std::string >  fileNames;
    bool    endOfOptions = false;
    for (int i = 1; i < argc; i++) {
      if (argv[i] == (char *) 0 || argv[i][0] == '\0')
        continue;
      if (endOfOptions || argv[i][0] != '-') {
        fileNames.push_back(std::string(argv[i]));
        continue;
      }
      std::string s;
      if (argv[i][1] == '-') {
        if (argv[i][2] == '\0') {
          endOfOptions = true;
          continue;
        }
        s = &(argv[i][1]);
      }
      else {
        s = argv[i];
      }
      if (s == "-h" || s == "-help" || s == "--help") {
        printUsage();
        return 0;
      }
      else if (s == "-n" || s == "-new") {
        forceNewFile = true;
      }
      else if (s == "-a" || s == "-append") {
        appendMode = true;
      }
      else if (s == "-ic" || s == "-input-channel") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --input-channel");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --input-channel: "
                          "must be an integer");
        }
        if (n < 0L || n > 15L) {
          printUsage();
          throw Exception("--input-channel parameter is out of range");
        }
        inputChannel = int(n);
      }
      else if (s == "-oc" || s == "-output-channel") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --output-channel");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --output-channel: "
                          "must be an integer");
        }
        if (n < 0L || n > 15L) {
          printUsage();
          throw Exception("--output-channel parameter is out of range");
        }
        outputChannel = int(n);
      }
      else if (s == "-fmin" || s == "-min-freq") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --min-freq");
        }
        char    *endPtr = (char *) 0;
        double  n = std::strtod(argv[i], &endPtr);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --min-freq: "
                          "must be a floating point number");
        }
        if (n < 0.0 || n > 60000.0) {
          printUsage();
          throw Exception("--min-freq parameter is out of range");
        }
        filterMinFreq = float(n);
      }
      else if (s == "-fmax" || s == "-max-freq") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --max-freq");
        }
        char    *endPtr = (char *) 0;
        double  n = std::strtod(argv[i], &endPtr);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --max-freq: "
                          "must be a floating point number");
        }
        if (n < 0.0 || n > 60000.0) {
          printUsage();
          throw Exception("--max-freq parameter is out of range");
        }
        filterMaxFreq = float(n);
      }
      else if (s == "-sr" || s == "-sample-rate") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --sample-rate");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --sample-rate: "
                          "must be an integer");
        }
        if (n < 10000L || n > 120000L) {
          printUsage();
          throw Exception("--sample-rate parameter is out of range");
        }
        sampleRate = n;
      }
      else if (s == "-r" || s == "-resample") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --resample");
        }
        char    *endPtr = (char *) 0;
        double  n = std::strtod(argv[i], &endPtr);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --resample: "
                          "must be a floating point number");
        }
        if (n < 0.0833 || n > 12.0) {
          printUsage();
          throw Exception("--resample parameter is out of range");
        }
        resampleRatio = float(n);
      }
      else if (s == "-k" || s == "-skip") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --skip");
        }
        char    *endPtr = (char *) 0;
        double  n = std::strtod(argv[i], &endPtr);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --skip: "
                          "must be a floating point number");
        }
        if (n < 0.0 || n > 7200.0) {
          printUsage();
          throw Exception("--skip parameter is out of range");
        }
        skipSeconds = float(n);
      }
      else if (s == "-l" || s == "-length") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --length");
        }
        char    *endPtr = (char *) 0;
        double  n = std::strtod(argv[i], &endPtr);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --length: "
                          "must be a floating point number");
        }
        if (n < 0.0 || n > 7200.0) {
          printUsage();
          throw Exception("--length parameter is out of range");
        }
        lengthLimit = float(n);
      }
      else {
        printUsage();
        throw Exception("invalid command line option");
      }
    }
    if (fileNames.size() < 2) {
      printUsage();
      throw Exception("missing file name");
    }
    if (fileNames.size() > 2) {
      printUsage();
      throw Exception("too many file names");
    }

    inputFile = Ep128Emu::openTapeFile(fileNames[0].c_str(), 2, 24000L, 8);
    if (sampleRate <= 0L)
      sampleRate = inputFile->getSampleRate();
    outputFile = Ep128Emu::openTapeFile(fileNames[1].c_str(),
                                        (forceNewFile ? 3 : 0), sampleRate, 1);
    sampleRate = outputFile->getSampleRate();
    if (outputFile->getIsReadOnly())
      throw Exception("invalid output file");
    if (typeid(*inputFile) == typeid(Ep128Emu::Tape_SoundFile)) {
      reinterpret_cast<Ep128Emu::Tape_SoundFile *>(inputFile)->setParameters(
          inputChannel, (filterMaxFreq > filterMinFreq),
          filterMinFreq, filterMaxFreq);
    }
    if (typeid(*outputFile) == typeid(Ep128Emu::Tape_SoundFile)) {
      reinterpret_cast<Ep128Emu::Tape_SoundFile *>(outputFile)->setParameters(
          outputChannel, false, 0.0f, 0.0f);
    }
    double  inputStep = double(inputFile->getSampleRate()) / double(sampleRate);
    if (resampleRatio > 0.0f)
      inputStep = 1.0 / double(resampleRatio);
    inputFile->play();
    inputFile->setIsMotorOn(true);
    outputFile->record();
    outputFile->setIsMotorOn(true);
    if (skipSeconds > 0.0f) {
      inputFile->seek(skipSeconds);
      if (inputFile->getPosition() <= 0.0) {
        long    n =
            long(double(skipSeconds) * double(inputFile->getSampleRate())
                 + 0.5);
        while (n-- > 0L && !inputFile->getIsEndOfTape()) {
          inputFile->runOneSample();
          if ((n & 0x7FFFL) == 0L) {
            std::fprintf(stderr, "\rSkipping %7.1f seconds  ",
                         inputFile->getPosition());
          }
        }
      }
    }
    if (appendMode)
      outputFile->seek(outputFile->getLength());
    double  minSignal = 2.0;
    double  maxSignal = 4.0;
    double  avgSignal = 3.0;
    double  inputPos = 0.0;
    double  prvInputSignal = 0.0;
    double  inputSignal = 0.0;
    double  c1 = 1.0 - std::pow(0.5, 44100.0 / double(sampleRate));
    double  c2 = 1.0 - std::pow(0.9999, 44100.0 / double(sampleRate));
    long    outputSamples = 0x7FFFFFFFL;
    if (lengthLimit >= 0.0f)
      outputSamples = long(double(lengthLimit) * double(sampleRate) + 0.5);
    int     inputState = 0;
    while (outputSamples-- > 0L && !inputFile->getIsEndOfTape()) {
      inputPos = inputPos + inputStep;
      while (inputPos >= 0.0) {
        inputPos = inputPos - 1.0;
        inputFile->runOneSample();
        prvInputSignal = inputSignal;
        inputSignal = double(inputFile->getOutputSignal());
      }
      double  f = std::cos(inputPos * 3.1415926535898) * 0.5 + 0.5;
      double  x = prvInputSignal + ((inputSignal - prvInputSignal) * f) + 1.0;
      if (x < minSignal) {
        minSignal = minSignal + ((x - minSignal) * c1);
        avgSignal = (minSignal + maxSignal) * 0.5;
        inputState = 0;
      }
      else if (x > maxSignal) {
        maxSignal = maxSignal + ((x - maxSignal) * c1);
        avgSignal = (minSignal + maxSignal) * 0.5;
        inputState = 1;
      }
      else {
        minSignal = minSignal + ((avgSignal - minSignal) * c2);
        maxSignal = maxSignal + ((avgSignal - maxSignal) * c2);
        if (x < ((minSignal * 0.625) + (maxSignal * 0.375)))
          inputState = 0;
        else if (x > ((maxSignal * 0.625) + (minSignal * 0.375)))
          inputState = 1;
      }
      outputFile->setInputSignal(inputState);
      outputFile->runOneSample();
      if ((outputSamples & 0x7FFFL) == 0L) {
        std::fprintf(stderr, "\rConverting %7.1f seconds  ",
                     outputFile->getPosition());
      }
    }
    std::fprintf(stderr, "\n");
    delete inputFile;
    delete outputFile;
  }
  catch (std::exception& e) {
    if (inputFile)
      delete inputFile;
    if (outputFile)
      delete outputFile;
    std::fprintf(stderr, "\n *** tapeconv: %s\n", e.what());
    return -1;
  }
  return 0;
}

