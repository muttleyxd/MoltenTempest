#pragma once

#include <cstdint>

namespace Tempest {

enum class MemUsage : uint8_t {
  TransferSrc =1<<0,
  TransferDst =1<<1,
  UniformBit  =1<<2,
  VertexBuffer=1<<3,
  IndexBuffer =1<<4,
  };

inline MemUsage operator | (MemUsage a,const MemUsage& b) {
  return MemUsage(uint8_t(a)|uint8_t(b));
  }

inline MemUsage operator & (MemUsage a,const MemUsage& b) {
  return MemUsage(uint8_t(a)&uint8_t(b));
  }


enum class BufferFlags : uint8_t {
  Staging = 1<<0,
  Static  = 1<<1,
  Dynamic = 1<<2
  };

inline BufferFlags operator | (BufferFlags a,const BufferFlags& b) {
  return BufferFlags(uint8_t(a)|uint8_t(b));
  }

inline BufferFlags operator & (BufferFlags a,const BufferFlags& b) {
  return BufferFlags(uint8_t(a)&uint8_t(b));
  }


enum class FboMode : uint8_t {
  Discard    =0,
  PreserveIn =1<<0,
  PreserveOut=1<<1,
  PresentOut =1<<2,
  Clear      =1<<3,
  Preserve   =(PreserveOut|PreserveIn),
  };

inline FboMode operator | (FboMode a,const FboMode& b) {
  return FboMode(uint8_t(a)|uint8_t(b));
  }

inline FboMode operator & (FboMode a,const FboMode& b) {
  return FboMode(uint8_t(a)&uint8_t(b));
  }


enum class Stage : uint8_t {
  Vertex   = 1,
  Fragment = 1<<1
  };

inline Stage operator | (Stage a,const Stage& b) {
  return Stage(uint8_t(a)|uint8_t(b));
  }

inline Stage operator & (Stage a,const Stage& b) {
  return Stage(uint8_t(a)&uint8_t(b));
  }


enum class TextureLayout : uint8_t {
  Undefined,
  Sampler,
  ColorAttach,
  };
}
