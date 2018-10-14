#pragma once

#include <cstdint>
#include <forward_list>

namespace Tempest {
namespace Detail {

//TODO: aligmnet
template<class MemoryProvider>
class DeviceAllocator {
  struct Page;
  struct Block;
  public:
    enum {
      DEFAULT_PAGE_SIZE=128*1024
      };
    using Memory=typename MemoryProvider::DeviceMemory;
    static const constexpr Memory null=Memory{};

    explicit DeviceAllocator(MemoryProvider& device):device(device){}

    DeviceAllocator(const DeviceAllocator&)=delete;

    ~DeviceAllocator(){
      for(auto& i:pages)
        device.free(i.memory);
      }

    struct Allocation {
      Page*  page  =nullptr;
      size_t offset=0,size=0;
      };

    Allocation alloc(size_t size, uint32_t typeBits){
      for(auto& i:pages){
        if(i.type==typeBits && i.allocated+size<=i.allSize){
          auto ret=i.alloc(size,device);
          if(ret.page!=nullptr)
            return ret;
          }
        }
      return rawAlloc(size,typeBits);
      }

    void free(const Allocation& a){
      a.page->free(a);
      if(a.page->allocated==0){
        device.free(a.page->memory);
        pages.remove(*a.page);
        }
      }

  private:
    Allocation rawAlloc(size_t size, uint32_t typeBits){
      Page pg(std::max<uint32_t>(DEFAULT_PAGE_SIZE,size));
      pg.memory = device.alloc(pg.allSize,typeBits);
      pg.type   = typeBits;
      if(pg.memory==null)
        throw std::bad_alloc();
      pages.push_front(std::move(pg));
      return pages.front().alloc(size,device);
      }

    void       freeDevMemory(Memory mem){
      return device.free(mem);
      }

    MemoryProvider&         device;
    std::forward_list<Page> pages;
  };

template<class MemoryProvider>
struct DeviceAllocator<MemoryProvider>::Block {
  Block*   next  =nullptr;
  uint32_t size  =0;
  uint32_t offset=0;
  };

template<class MemoryProvider>
struct DeviceAllocator<MemoryProvider>::Page : Block {
  using    Block::next;
  using    Block::size;
  using    Block::offset;

  Memory   memory =null;
  uint32_t type   =0;
  uint32_t allSize=0;
  uint32_t allocated=0;

  Page(uint32_t sz){
    size   =sz;
    allSize=sz;
    }

  Page(Page&& p){
    std::swap(next,p.next);
    size  =p.size;
    offset=p.offset;
    std::swap(memory,p.memory);
    type=p.type;
    allSize=p.allSize;
    }

  ~Page(){
    Block* b=next;
    while(b!=nullptr){
      Block* p=b->next;
      delete b;
      b=p;
      }
    }

  bool operator==(const Page& other) const {
    return memory==other.memory;
    }

  Allocation alloc(size_t size,MemoryProvider& /*prov*/){
    Block*   b=this;
    while(b!=nullptr){
      if(size<=b->size) {
        return alloc(*b,size);
        }
      b=b->next;
      }
    return Allocation{};
    }

  Allocation alloc(Block& b,size_t size) {
    Allocation a;
    a.offset=b.offset;
    a.page  =this;
    a.size  =size;

    b.offset +=size;
    allocated+=size;
    return a;
    }

  void free(const Allocation& a) {
    allocated -= a.size;

    Block* b=this;
    while(b!=nullptr && (b->offset+b->size)<a.offset)
      b=b->next;

    if(b->offset+b->size==a.offset){
      b->size+=a.size;
      mergeWithNext(b);
      return;
      }
    if(b->offset==a.offset+a.size){
      b->offset=a.offset;
      b->size +=a.size;
      return;
      }

    Block* r=new(std::nothrow) Block();
    if(r==nullptr)
      return; // no error, but sort of leak in Page
    *r=*b;
    b->next  =r;
    b->size  =a.size;
    b->offset=a.offset;
    }

  void mergeWithNext(Block* b){
    while(b->next!=nullptr){
      if(b->offset+b->size==b->next->offset){
        b->size+=b->next->size;
        b->next=b->next->next;
        delete b->next;
        } else {
        return;
        }
      }
    }
  };
}}

