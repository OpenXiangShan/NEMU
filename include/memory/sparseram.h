#ifndef __SPARSE_RAM__

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <common.h>
#include <debug.h>

#ifdef __cplusplus
#include <string>
#include <map>
#include <functional>
#include <tuple>
#endif

// comm functions
#define OUTPUT(o, fmt, ...)             \
    {                                   \
        fprintf(o, fmt, ##__VA_ARGS__); \
    }

#define DEBUG(fmt, ...)                         \
    {                                           \
        if (0)                           \
        {                                       \
            OUTPUT(stderr, "%s", "debug> ");    \
            OUTPUT(stderr, fmt, ##__VA_ARGS__); \
            OUTPUT(stderr, "%s\n", "")          \
        }                                       \
    }

#define FATAL(fmt, ...)                     \
    {                                       \
        OUTPUT(stderr, fmt, ##__VA_ARGS__); \
        exit(-1);                           \
    }

#ifndef MUXDEF
typedef uint64_t paddr_t;
#endif

#ifndef MUXDEF
#define CHOOSE2nd(a, b, ...) b
#define MUX_WITH_COMMA(contain_comma, a, b) CHOOSE2nd(contain_comma a, b)
#define MUX_MACRO_PROPERTY(p, macro, a, b) MUX_WITH_COMMA(concat(p, macro), a, b)
#define MUXDEF(macro, X, Y)  MUX_MACRO_PROPERTY(__P_DEF_, macro, X, Y)
#endif
// typedef MUXDEF(CONFIG_ISA64, uint64_t, uint32_t) word_t;

#ifdef __cplusplus

// sparse ram define
typedef uint64_t reg_t;
typedef reg_t addr_t;

typedef enum {
  endianness_little,
  endianness_big
} endianness_t;


inline void vassert(bool c, std::string msg)
{
    if (!c)
    {
        FATAL("Assert error: %s\n", msg.c_str());
    }
}

typedef std::function<void (paddr_t addr, size_t len, void *bytes)> copy_mem_func;

class SparseRam
{
    typedef struct 
    {
        paddr_t start;
        paddr_t end;
        u_int8_t * blk;
    } sp_mm_blk;
    
public:
    unsigned block_size;
    std::map<paddr_t, u_int8_t *> mem;
    std::map<std::string, sp_mm_blk *> big_block;

    ~SparseRam();
    SparseRam(u_int block_count = 4, u_int chunk_size=1024){
        this->block_size = block_count * chunk_size;
        DEBUG("init SparseRam with block_size= %.2f kB (chunk_size=%d)", float(this->block_size)/1024.0, chunk_size);
    }
 
    bool load_bin(const char *file, paddr_t addr);
    bool load_elf(const char *file);

    void read(paddr_t addr, size_t len, void* bytes);
    void write(paddr_t addr, size_t len, const void* bytes);
    bool add_blk(char *name, paddr_t start, paddr_t end);
    void *blk_host_addr(char *name);

    word_t read(paddr_t addr, int len);
    void write(paddr_t addr, int len, word_t data);
    void copy(SparseRam *dst);

    endianness_t get_target_endianness();

    void copy_nzero_bytes(copy_mem_func copy_handler);
    void copy_bytes(copy_mem_func copy_handler);

    void print_info();

private:
    u_int max_cache_size = 5;
    std::map<paddr_t, std::tuple<u_int8_t*, u_long>> cache;
    bool _fast_count(paddr_t index);
    u_int8_t *_fast_mem(paddr_t index);
    void __update_cache(paddr_t index, u_int8_t*m);
    sp_mm_blk *_blk_find(paddr_t addr);
    bool _blk_read(paddr_t addr, size_t len, void* bytes);
    bool _blk_write(paddr_t addr, size_t len, const void* bytes);
};
#endif


// c APIs
#ifdef __cplusplus
extern "C"
{
#endif

    void*  sparse_mem_new(u_int block_count, u_int chunk_size);
    void   sparse_mem_del(void* mem);
    int    sparse_mem_elf(void* self, const char * fname);
    int    sparse_mem_bin(void* self, const char * fname, paddr_t addr);
    void   sparse_mem_read(void* self, paddr_t addr, size_t len, void* bytes);
    void   sparse_mem_write(void* self, paddr_t addr, size_t len, const void* bytes);
    void   sparse_mem_wwrite(void* self, paddr_t addr, int len, word_t data);
    word_t sparse_mem_wread(void *self, paddr_t addr, int len);
    void   sparse_mem_info(void* self);
    void   sparse_mem_copy(void *dst, void *src);
    void*  sparse_mem_blk_get(void *self, char *name);
    int    sparse_mem_blk_add(void *self, char *name, paddr_t start, paddr_t end);
    int    file_is_elf(const char *fn);

#ifdef __cplusplus
}
#endif

#endif
