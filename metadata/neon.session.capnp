@0xce3cc6ea0246fc83; # unique file ID, generated by `capnp id'

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("neon::session");

struct SessionConfig {
    uuid        @0 :Text;
    sampleRate  @1 :UInt32;
    channels    @2 :UInt8;
    width       @3 :UInt8;
    duration    @4 :Float64;
}

struct SessionAudioPacket {
    uuid @0 :Text;
    segment @1 :List(Float32);
}

struct BPMData {
    bpm        @0  :Float32;
    confidence @1  :Float32;
}

struct SpectrumData {
    max       @0  :List(Float32);
    mean      @1  :List(Float32);
    median    @2  :List(Float32);
    min       @3  :List(Float32);
}


interface Controller {
    createSession        @0 (name :Text) -> (uuid :Text);
    releaseSession       @1 (uuid :Text);
    shutdown             @2 ();
    updateSessionConfig  @3 (config :SessionConfig);
    pushAudioData        @4 (data :SessionAudioPacket);

    releasePipeline      @5 (uuid: Text);

    createBPMPipeLine    @6 (uuid :Text) -> (uuid: Text);
    getBPMPipeLineData   @7 (uuid :Text) -> (data :BPMData);

    createSpectrumPipe   @8 (uuid :Text) -> (uuid: Text);
    getSpectrumData      @9 (uuid :Text) -> (data: SpectrumData);
}
