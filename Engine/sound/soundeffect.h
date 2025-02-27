#pragma once

#include <Tempest/Sound>
#include <cstdint>

namespace Tempest {

class SoundDevice;

class SoundEffect final {
  public:
    SoundEffect()=default;
    SoundEffect(SoundEffect&& s);
    ~SoundEffect();

    SoundEffect& operator=(SoundEffect&& s);

    void play();
    void pause();

    bool     isEmpty()     const;
    bool     isFinished()  const;
    uint64_t timeLength()  const;
    uint64_t currentTime() const;

    void  setPosition(float x,float y,float z);
    void  setMaxDistance(float dist);
    void  setRefDistance(float dist);
    void  setVolume(float val);
    float volume() const;

    std::array<float,3> position() const;
    float x() const;
    float y() const;
    float z() const;

  private:
    SoundEffect(SoundDevice& dev,const Sound &src);

    uint32_t                     source = 0;
    void*                        ctx    = nullptr;
    std::shared_ptr<Sound::Data> data;

  friend class SoundDevice;
  };

}
