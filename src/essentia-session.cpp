AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

Algorithm* audio = factory.create("MonoLoader",
                                  "filename", audioFilename,
                                  "sampleRate", sampleRate);

Algorithm* fc    = factory.create("FrameCutter",
                                  "frameSize", frameSize,
                                  "hopSize", hopSize);

Algorithm* w     = factory.create("Windowing",
                                  "type", "blackmanharris62");

Algorithm* spec  = factory.create("Spectrum");
Algorithm* mfcc  = factory.create("MFCC");