#pragma once

#include <Tempest/SystemApi>
#include <initializer_list>
#include <memory>
#include <atomic>

#include "flags.h"

namespace Tempest {
  class UniformsLayout;
  class Pixmap;

  namespace Decl {
  enum ComponentType:uint8_t {
    float0 = 0, // just trick
    float1 = 1,
    float2 = 2,
    float3 = 3,
    float4 = 4,

    color  = 5,
    short2 = 6,
    short4 = 7,

    half2  = 8,
    half4  = 9,
    count
    };
  }

  class AbstractGraphicsApi {
    protected:
      AbstractGraphicsApi() =default;
      ~AbstractGraphicsApi()=default;

    public:

      struct Shared {
        virtual ~Shared()=default;
        std::atomic_uint_fast32_t counter{1};
        };

      struct Device       {};
      struct Swapchain    {
        virtual ~Swapchain(){}
        virtual uint32_t imageCount() const=0;
        };
      struct Texture:Shared {};
      struct Image          {};
      struct Fbo            {};
      struct Pass           {};
      struct Pipeline       {};
      struct Shader         {};
      struct Uniforms       {};
      struct UniformsLay    {
        virtual ~UniformsLay()=default;
        };
      struct Buffer         {};
      struct Desc           {
        virtual ~Desc()=default;
        virtual void set(size_t id,AbstractGraphicsApi::Texture *tex)=0;
        virtual void set(size_t id,AbstractGraphicsApi::Buffer* buf,size_t offset,size_t size)=0;
        };
      struct CommandBuffer  {
        virtual ~CommandBuffer()=default;
        virtual void begin()=0;
        virtual void end()  =0;
        virtual void beginRenderPass(AbstractGraphicsApi::Fbo* f,
                                     AbstractGraphicsApi::Pass*  p,
                                     uint32_t width,uint32_t height)=0;
        virtual void endRenderPass()=0;

        virtual void clear(Image& img,float r, float g, float b, float a)=0;
        virtual void setPipeline(Pipeline& p)=0;
        virtual void setUniforms(Pipeline& p,Desc& u)=0;
        virtual void exec(const AbstractGraphicsApi::CommandBuffer& buf)=0;
        virtual void setVbo(const Buffer& b)=0;
        virtual void draw(size_t vertexCount)=0;
        };
      struct CmdPool        {};

      struct Fence          {
        virtual ~Fence()=default;
        virtual void wait() =0;
        virtual void reset()=0;
        };
      struct Semaphore      {};

      virtual Device*    createDevice(SystemApi::Window* w)=0;
      virtual void       destroy(Device* d)=0;

      virtual Swapchain* createSwapchain(SystemApi::Window* w,AbstractGraphicsApi::Device *d)=0;
      virtual void       destroy(Swapchain* d)=0;

      virtual Pass*      createPass(Device *d,Swapchain *s)=0;
      virtual void       destroy(Pass* pass)=0;

      virtual Fbo*       createFbo(Device *d,Swapchain *s,Pass* pass,uint32_t imageId)=0;
      virtual void       destroy(Fbo* pass)=0;

      virtual std::shared_ptr<AbstractGraphicsApi::UniformsLay>
                         createUboLayout(Device *d,const UniformsLayout&)=0;

      virtual Pipeline*  createPipeline(Device* d,Pass* pass,
                                        uint32_t width, uint32_t height,
                                        const Tempest::Decl::ComponentType *decl, size_t declSize,
                                        size_t stride,
                                        const UniformsLayout& ulay,
                                        std::shared_ptr<UniformsLay> &ulayImpl,
                                        const std::initializer_list<Shader*>& sh)=0;
      virtual void       destroy(Pipeline* pass)=0;

      virtual Shader*    createShader(Device *d,const char* file)=0;
      virtual void       destroy(Shader* shader)=0;

      virtual Fence*     createFence(Device *d)=0;
      virtual void       destroy(Fence* fence)=0;

      virtual Semaphore* createSemaphore(Device *d)=0;
      virtual void       destroy(Semaphore* semaphore)=0;

      virtual CmdPool*   createCommandPool(Device* d)=0;
      virtual void       destroy(CmdPool* cmd)=0;

      virtual CommandBuffer*
                         createCommandBuffer(Device* d,CmdPool* pool,bool secondary)=0;
      virtual void       destroy(CommandBuffer* cmd)=0;

      virtual Buffer*    createBuffer(Device* d,const void *mem,size_t size,MemUsage usage,BufferFlags flg)=0;
      virtual void       destroy(Buffer* cmd)=0;

      virtual Desc*      createDescriptors(Device* d,UniformsLay* p)=0;
      virtual void       destroy(Desc* cmd)=0;

      virtual Texture*   createTexture(Device* d,const Pixmap& p,bool mips)=0;
      //virtual void       destroy(Texture* t)=0;

      virtual uint32_t   nextImage(Device *d,Swapchain* sw,Semaphore* onReady)=0;
      virtual Image*     getImage(Device *d,Swapchain* sw,uint32_t id)=0;
      virtual void       present  (Device *d,Swapchain* sw,uint32_t imageId,const Semaphore *wait)=0;
      virtual void       draw     (Device *d,Swapchain *sw,CommandBuffer* cmd,Semaphore* wait,Semaphore* onReady,Fence* onReadyCpu)=0;

    };
}
