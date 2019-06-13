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

#ifndef ESSENTIA_STREAMING_BOOSTBUFFER_H
#define ESSENTIA_STREAMING_BOOSTBUFFER_H

#include "../streamingalgorithm.h"
#include <boost/circular_buffer.hpp>
#include <mutex>

namespace essentia {
namespace streaming {

class BoostRingBufferInput : public Algorithm {
private:
  std::mutex bufferMutex;
  boost::circular_buffer<essentia::Real> *buffer;
  uint32_t frameSize;

protected:
  Source<Real> _outputAudio;

public:
  BoostRingBufferInput();
  ~BoostRingBufferInput();

  void add(Real *inputData, int size);

  AlgorithmStatus process();

  void declareParameters() {
    declareParameter("bufferSize", "the size of the ringbuffer", "", 8192);
  }

  void configure();
  void reset();

  static const char *name;
  static const char *category;
  static const char *description;
};

} // namespace streaming
} // namespace essentia

#endif // ESSENTIA_STREAMING_BOOSTBUFFER_H
