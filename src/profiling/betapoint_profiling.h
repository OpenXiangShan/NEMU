#include <common.h>
#include <debug.h>
#include <lz4.h>
#include <zstd.h>
#include <boost/dynamic_bitset.hpp>
#include "boost/utility/binary.hpp"

namespace BetaPointNS {

class CompressProfiler {
  protected:
    static const unsigned bufBytes = 64;
    static const unsigned contiguousTokens = 1024 * 1024;
    static const unsigned contiguousBytes = bufBytes*contiguousTokens;

    char inputBuf[contiguousBytes];
    char *inputEnd = &inputBuf[contiguousBytes + 1];
    char outputBuf[contiguousBytes];
    char *outPtr;

    LZ4_stream_t lz4StreamBody{ {0} };
    LZ4_stream_t *lz4Stream {&lz4StreamBody};

    ZSTD_CCtx* const cctx;

    CompressProfiler();

    unsigned compressBlock(size_t input_size, char *in_ptr);

    virtual unsigned tokenBytes() = 0;
};

class ControlProfiler: public CompressProfiler {
    struct ControlInfo {
        vaddr_t pc;
        vaddr_t target;
        uint64_t hist;
        bool taken;
    };
    ControlInfo *info;

    unsigned _tokenBytes = sizeof(ControlInfo);
    unsigned tokenBytes() override {
        return _tokenBytes;
    }

    const unsigned brHistLen{64};
    boost::dynamic_bitset<> brHist;
  public:
    ControlProfiler();

    void controlProfile(vaddr_t pc, vaddr_t target, bool taken);

};

class MemProfiler: public CompressProfiler {
    struct MemInfo{
        vaddr_t vaddr;
        paddr_t paddr;
    };
};

}