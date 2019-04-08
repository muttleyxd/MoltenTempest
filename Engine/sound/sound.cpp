#include "sound.h"

#include <Tempest/IDevice>
#include <Tempest/MemReader>
#include <vector>
#include <cstring>

#include <AL/al.h>
#include <AL/alc.h>

using namespace Tempest;

struct Sound::Header final {
  char     id[4];
  uint32_t size;
  bool     is(const char* n) const { return std::memcmp(id,n,4)==0; }
  };

struct Sound::WAVEHeader final {
  char     riff[4];  //'RIFF'
  uint32_t riffSize;
  char     wave[4];  //'WAVE'
  };

struct Sound::FmtChunk final {
  uint16_t format;
  uint16_t channels;
  uint32_t samplesPerSec;
  uint32_t bytesPerSec;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  };

const uint16_t Sound::stepTable[89] = {
  7, 8, 9, 10, 11, 12, 13, 14,
  16, 17, 19, 21, 23, 25, 28, 31,
  34, 37, 41, 45, 50, 55, 60, 66,
  73, 80, 88, 97, 107, 118, 130, 143,
  157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 876, 963, 1060, 1166, 1282, 1411,
  1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
  3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
  7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
  32767
  };

const int32_t Sound::indexTable[] = {
  /* adpcm data size is 4 */
  -1, -1, -1, -1, 2, 4, 6, 8
  };

Sound::Sound(IDevice& f) {
  std::vector<char> buf;

  {
    char b[2048];
    size_t sz = f.read(b,sizeof(b));
    while( sz>0 ){
      buf.insert( buf.end(), b, b+sz );
      sz = f.read(b,sizeof(b));
      }
  }

  Tempest::MemReader mem(buf.data(), buf.size());

  WAVEHeader      header={};
  FmtChunk        fmt={};
  uint32_t        dataSize=0;
  std::unique_ptr<char> data = readWAVFull(mem,header,fmt,dataSize);

  if(data) {
    switch(fmt.bitsPerSample) {
      case 4:
        decodeAdPcm(fmt,reinterpret_cast<uint8_t*>(data.get()),dataSize,uint32_t(-1));
        return;
      case 8:
        format = (fmt.channels==1) ? AL_FORMAT_MONO8  : AL_FORMAT_STEREO8;
        break;
      case 16:
        format = (fmt.channels==1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        break;
      default:
        return;
      }

    upload(data.get(),dataSize,fmt.samplesPerSec);
    } else {
    /*
    OggVorbis_File vf;

    vorbis_info* vi = 0;

    Buf input;
    int64_t uiPCMSamples = 0;
    data = convertOggToPCM( vf, &input, buf.size(), &buf[0], vi, uiPCMSamples );

    if( vi->channels == 1 )
      format = AL_FORMAT_MONO16; else
      format = AL_FORMAT_STEREO16;

    upload( data, uiPCMSamples * vi->channels * sizeof(short), vi->rate );
    ov_clear(&vf);
    */
    }
  }

Sound::Sound(Sound &&s)
  :source(s.source),buffer(s.buffer),format(s.format){
  s.source = 0;
  s.buffer = 0;
  s.format = 0;
  }

Sound::~Sound() {
  if(source)
    alDeleteSources(1, &source);
  if(buffer)
    alDeleteBuffers(1, &buffer);
  }

Sound &Sound::operator =(Sound &&s) {
  std::swap(source, s.source);
  std::swap(buffer, s.buffer);
  std::swap(format, s.format);
  return *this;
  }

void Sound::play() {
  if(source==0)
    return;
  alSourcePlay(source);
  }

void Sound::pause() {
  if(source==0)
    return;
  alSourcePause(source);
  }

std::unique_ptr<char> Sound::readWAVFull(IDevice &f, WAVEHeader& header, FmtChunk& fmt, size_t& dataSize) {
  std::unique_ptr<char> buffer;

  size_t res = f.read(&header,sizeof(WAVEHeader));
  if(res!=sizeof(WAVEHeader))
    return nullptr;

  if(std::memcmp("RIFF",header.riff,4)!=0 ||
     std::memcmp("WAVE",header.wave,4)!=0)
    return nullptr;

  while(true){
    Header head={};
    if(f.read(&head,sizeof(head))!=sizeof(head))
      break;

    if(head.is("data")){
      buffer.reset(new char[head.size]);
      if(f.read(buffer.get(),head.size)!=head.size){
        buffer.reset();
        return nullptr;
        }
      dataSize = head.size;
      }
    else if(head.is("fmt ")){
      size_t sz=std::min(head.size,sizeof(fmt));
      if(f.read(&fmt,sz)!=sz)
        return nullptr;
      f.seek(head.size-sz);
      }
    else if(f.seek(head.size)!=head.size)
      return nullptr;
    }
  return buffer;
  }

void Sound::upload(char* data, size_t size, size_t rate) {
  alGenBuffers(1, &buffer);
  alBufferData(buffer, format, data, int(size), int(rate));

  alGenSources(1, &source);
  alSourcei(source, AL_BUFFER, int(buffer));
  }

void Sound::decodeAdPcm(const FmtChunk& fmt,const uint8_t* src,uint32_t dataSize,uint32_t maxSamples) {
  if(fmt.blockAlign==0)
    return;

  uint32_t samples_per_block = (fmt.blockAlign-fmt.channels*4)*(fmt.channels^3)+1;
  uint32_t sample_count      = (dataSize/fmt.blockAlign)*samples_per_block;

  if(sample_count>maxSamples)
    sample_count = maxSamples;

  std::vector<uint8_t> dest;
  unsigned             sample = 0;

  while(sample<sample_count) {
    uint32_t block_adpcm_samples = samples_per_block;
    uint32_t block_pcm_samples   = samples_per_block;

    if(block_adpcm_samples>sample_count) {
      block_adpcm_samples = ((sample_count + 6u) & ~7u)+1u;
      block_pcm_samples   = sample_count;
      }

    size_t ofidx = dest.size();
    dest.resize(dest.size()+block_pcm_samples*fmt.channels*2u);

    decodeAdPcmBlock(reinterpret_cast<int16_t*>(&dest[ofidx]), src, fmt.blockAlign, fmt.channels);
    src    += fmt.blockAlign;
    sample += block_pcm_samples;
    }

  format = AL_FORMAT_MONO16;
  upload(reinterpret_cast<char*>(dest.data()),dest.size(),fmt.samplesPerSec);
  }

int Sound::decodeAdPcmBlock(int16_t *outbuf, const uint8_t *inbuf, size_t inbufsize, uint16_t channels) {
  int32_t samples = 1;
  int32_t pcmdata[2]={};
  int8_t  index[2]={};

  if(inbufsize<channels * 4 || channels>2)
    return 0;

  for(int ch=0; ch<channels; ch++) {
    *outbuf++ = pcmdata[ch] = int16_t(inbuf [0] | (inbuf [1] << 8));
    index[ch] = inbuf[2];

    if(index[ch]<0 || index[ch]>88 || inbuf[3])     // sanitize the input a little...
      return 0;

    inbufsize -= 4;
    inbuf     += 4;
    }

  size_t chunks = inbufsize/(channels*4);
  samples += chunks*8;

  while(chunks--){
    for(int ch=0; ch<channels; ++ch) {
      for(int i=0; i<4; ++i) {
        int step = stepTable[index [ch]], delta = step >> 3;

        if (*inbuf & 1) delta += (step >> 2);
        if (*inbuf & 2) delta += (step >> 1);
        if (*inbuf & 4) delta += step;
        if (*inbuf & 8) delta = -delta;

        pcmdata[ch] += delta;
        index  [ch] += indexTable[*inbuf & 0x7];
        index  [ch] = std::min<int8_t>(std::max<int8_t>(index[ch],0),88);
        pcmdata[ch] = std::min(std::max(pcmdata[ch],-32768),32767);
        outbuf[i*2*channels] = int16_t(pcmdata[ch]);

        step  = stepTable[index[ch]];
        delta = step >> 3;

        if (*inbuf & 0x10) delta += (step >> 2);
        if (*inbuf & 0x20) delta += (step >> 1);
        if (*inbuf & 0x40) delta += step;
        if (*inbuf & 0x80) delta = -delta;

        pcmdata[ch] += delta;
        index  [ch] += indexTable[(*inbuf >> 4) & 0x7];
        index  [ch] = std::min<int8_t>(std::max<int8_t>(index[ch],0),88);
        pcmdata[ch] = std::min(std::max(pcmdata[ch],-32768),32767);
        outbuf [(i*2+1)*channels] = int16_t(pcmdata[ch]);;

        inbuf++;
        }
      outbuf++;
      }
    outbuf += channels*7;
    }
  return samples;
  }
