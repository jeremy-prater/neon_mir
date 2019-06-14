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

  template <class T> void add(T &container) {
    // std::cout << "Added Audio" << std::endl;
    std::lock_guard<std::mutex> lock(bufferMutex);
    auto current = container.begin();
    auto end = container.end();
    int count = 0;
    while (current != end) {
      Real value = *current;
      if ((value < -1) || (value > 1)) {
        std::cout << "Audio value out of range! " << value << std::endl;
        assert(0);
      } else {
        // std::cout << "Added Audio Count " << count++ << "value ==> " << value
        //           << std::endl;
      }
      buffer->push_front(value);
      current++;
    }
    // std::cout << "Added Audio count ==> " << count << std::endl;
  }

  AlgorithmStatus process();

  void declareParameters();
  void configure();
  void reset();

  static const char *name;
  static const char *category;
  static const char *description;
};

} // namespace streaming
} // namespace essentia

#endif // ESSENTIA_STREAMING_BOOSTBUFFER_H
