#include "audio-processor.hpp"
#include "capnp/list.h"
#include "debuglogger.hpp"
#include "pulse_audio_stream.hpp"
#include "test-shader-1.hpp"
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/Vector.h>
#include <Magnum/Shaders/VertexColor.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <deque>
#include <unistd.h>

#define NUM_SLICES 513
#define RING_SIZE 200

using namespace Magnum;

struct TriangleVertex {
  Vector2 position;
  Vector2 textureUV;
};

class NeonSpectrumGFX : public Platform::Application {
public:
  static NeonSpectrumGFX *getInstance();

  explicit NeonSpectrumGFX(const Arguments &arguments);
  virtual ~NeonSpectrumGFX();


  float *spectrumDataMeanGetSlice() const noexcept;
  void spectrumDataMeanPushSlice(float *) noexcept;
  inline bool spectrumDataMeanEmpty() const noexcept;
  void addSlices(std::vector<float> &newData) noexcept;

private:
  const Vector2 start = {-0.9f, -0.75f};
  const Vector2 end = {0.5f, 0.9f};

  static NeonSpectrumGFX *instance;
  TriangleVertex spectrumData[NUM_SLICES + 1];

  // Generic ring buffer function
  inline void spectrumDataCheckSlice(uint8_t &position) const;

  // There could also be max, min, median...
  // WTF even is this? It's all mutable...
  mutable std::mutex audioFrameMutex;
  mutable float spectrumDataMean[RING_SIZE][NUM_SLICES + 1];
  mutable uint8_t spectrumDataMeanHead;
  mutable uint8_t spectrumDataMeanTail;

  // Main loop functions
  void initalizeRenderData();
  void updateRenderData();
  void updateData();
  void drawEvent() override;

  // Audio stuff
  std::thread audioWorker;
  NeonPulseInput paInput;
  AudioProcessor audioProcessor;

  // Main thread IO Event loop
  boost::asio::io_service io_service;

  // This should be handled by a resource class
  // This would make better visualizations possible intead of hard coding
  // visualization objects
  GL::Mesh _mesh;
  NeonTestShader1 _shader;

  // For this application to exit correctly. All threads must cleanly exit when
  // shutdown==true
  bool shutdown;

  // Meh...
  DebugLogger logger;
};
