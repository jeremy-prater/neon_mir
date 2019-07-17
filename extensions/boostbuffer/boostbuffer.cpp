/*
 * Copyright (C) 2006-2016  Music Technology Group - Universitat Pompeu Fabra
 *
 * This file is part of Essentia
 *
 * Essentia is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation (FSF), either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the Affero GNU General Public License
 * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
 */

#include "boostbuffer.h"
#include "atomic.h"
#include "sourcebase.h"
#include <boost/foreach.hpp>

using namespace std;

namespace essentia {
namespace streaming {

const char *BoostRingBufferInput::name = "BoostRingBufferInput";
const char *BoostRingBufferInput::category = "Input/Output";
const char *BoostRingBufferInput::description = DOC(
    "This algorithm gets data from a boost circular buffer of type Real that "
    "is fed into the essentia streaming mode.");

BoostRingBufferInput::BoostRingBufferInput()
    : buffer(nullptr), frameSize(44100) {
  declareOutput(_outputAudio, frameSize, "signal",
                "data from the boost circular buffer");

  _outputAudio.setBufferType(BufferUsage::forAudioStream);

  buffer = new boost::circular_buffer<essentia::Real>(frameSize);
  output("signal").setAcquireSize(frameSize);
  output("signal").setReleaseSize(frameSize);
}

BoostRingBufferInput::~BoostRingBufferInput() {
  if (buffer)
    delete buffer;
}

void BoostRingBufferInput::declareParameters() {
  declareParameter("bufferSize", "the size of the ringbuffer", "", 44100);
}

void BoostRingBufferInput::configure() {
  if (buffer)
    delete buffer;

  frameSize = parameter("bufferSize").toInt();

  // std::cout << "configure() : frameSize : " << frameSize << std::endl;

  buffer = new boost::circular_buffer<essentia::Real>(frameSize);

  output("signal").setAcquireSize(frameSize);
  output("signal").setReleaseSize(frameSize);
}

AlgorithmStatus BoostRingBufferInput::process() {

  /// std::cout << "process()" << std::endl;
  AlgorithmStatus status = acquireData();

  /// std::cout << "status : " << status << std::endl;

  int frameOutSize = _outputAudio.acquireSize();

  /// std::cout << "data acquired : frameOutSize : " << frameOutSize
  /// << " ==> frameSize : " << frameSize << std::endl;

  if (status != OK) {
    return status;
  }

  vector<AudioSample> &output = _outputAudio.tokens();

  Real *dataStart = output.data();

  {
    /// std::cout << "lock(bufferMutex)" << std::endl;

    std::lock_guard<std::mutex> lock(bufferMutex);
    // std::pair<essentia::Real *, std::size_t>
    // boost::circular_buffer<essentia::Real>::array_one()
    auto array_one = buffer->array_one();
    auto array_two = buffer->array_two();

    if (array_one.second + array_two.second != frameOutSize) {
      /// std::cout << "PASS" << std::endl;
      _shouldStop = true;
      return PASS;
    }

    /// std::cout << "memcpy : a0 : " << array_one.second
    /// << " : a1 : " << array_two.second << std::endl;

    memcpy(dataStart, array_one.first, array_one.second);
    memcpy(dataStart + array_one.second, array_two.first, array_two.second);

    // buffer->clear();
  }

  /// std::cout << "releasing" << std::endl;
  releaseData();
  /// std::cout << "released" << std::endl;

  _shouldStop = true;

  return OK;
} // namespace streaming

void BoostRingBufferInput::reset() {
  Algorithm::reset();
  if (buffer)
    buffer->clear();
}

} // namespace streaming
} // namespace essentia
