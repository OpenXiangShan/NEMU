#include <memory/sparseram.h>

#ifndef __cplusplus
#endif

#include <cstring>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <memory>
#include <cerrno>

/*******************************************Comm Functions****************************************************************/

template <typename... Args>
std::string sfmt(const std::string &format, Args... args)
{
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
  if (size_s <= 0)
  {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

/*******************************************ELF Define****************************************************************/
// elf functions
// modified from: https://github.com/riscv-software-src/riscv-isa-sim

static inline uint8_t swap(uint8_t n) { return n; }
static inline uint16_t swap(uint16_t n) { return (n >> 8) | (n << 8); }
static inline uint32_t swap(uint32_t n) { return (swap(uint16_t(n)) << 16) | swap(uint16_t(n >> 16)); }
static inline uint64_t swap(uint64_t n) { return (uint64_t(swap(uint32_t(n))) << 32) | swap(uint32_t(n >> 32)); }
static inline int8_t swap(int8_t n) { return n; }
static inline int16_t swap(int16_t n) { return int16_t(swap(uint16_t(n))); }
static inline int32_t swap(int32_t n) { return int32_t(swap(uint32_t(n))); }
static inline int64_t swap(int64_t n) { return int64_t(swap(uint64_t(n))); }

#ifdef HAVE_INT128
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
static inline uint128_t swap(uint128_t n) { return (uint128_t(swap(uint64_t(n))) << 64) | swap(uint64_t(n >> 64)); }
static inline int128_t swap(int128_t n) { return int128_t(swap(uint128_t(n))); }
#endif

#ifdef WORDS_BIGENDIAN
template <typename T>
static inline T from_be(T n)
{
  return n;
}
template <typename T>
static inline T to_be(T n) { return n; }
template <typename T>
static inline T from_le(T n) { return swap(n); }
template <typename T>
static inline T to_le(T n) { return swap(n); }
#else
template <typename T>
static inline T from_le(T n)
{
  return n;
}
template <typename T>
static inline T to_le(T n) { return n; }
template <typename T>
static inline T from_be(T n) { return swap(n); }
template <typename T>
static inline T to_be(T n) { return swap(n); }
#endif

#define ET_EXEC 2
#define EM_RISCV 243
#define EM_NONE 0
#define EV_CURRENT 1

#define IS_ELF(hdr)                                       \
  ((hdr).e_ident[0] == 0x7f && (hdr).e_ident[1] == 'E' && \
   (hdr).e_ident[2] == 'L' && (hdr).e_ident[3] == 'F')

#define ELF_SWAP(hdr, val) (IS_ELFLE(hdr) ? from_le((val)) : from_be((val)))

#define IS_ELF32(hdr) (IS_ELF(hdr) && (hdr).e_ident[4] == 1)
#define IS_ELF64(hdr) (IS_ELF(hdr) && (hdr).e_ident[4] == 2)
#define IS_ELFLE(hdr) (IS_ELF(hdr) && (hdr).e_ident[5] == 1)
#define IS_ELFBE(hdr) (IS_ELF(hdr) && (hdr).e_ident[5] == 2)
#define IS_ELF_EXEC(hdr) (IS_ELF(hdr) && ELF_SWAP((hdr), (hdr).e_type) == ET_EXEC)
#define IS_ELF_RISCV(hdr) (IS_ELF(hdr) && ELF_SWAP((hdr), (hdr).e_machine) == EM_RISCV)
#define IS_ELF_EM_NONE(hdr) (IS_ELF(hdr) && ELF_SWAP((hdr), (hdr).e_machine) == EM_NONE)
#define IS_ELF_VCURRENT(hdr) (IS_ELF(hdr) && ELF_SWAP((hdr), (hdr).e_version) == EV_CURRENT)

#define PT_LOAD 1

#define SHT_NOBITS 8

typedef struct
{
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct
{
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

typedef struct
{
  uint32_t st_name;
  uint32_t st_value;
  uint32_t st_size;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
} Elf32_Sym;

typedef struct
{
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct
{
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} Elf64_Phdr;

typedef struct
{
  uint32_t st_name;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
  uint64_t st_value;
  uint64_t st_size;
} Elf64_Sym;

std::map<std::string, uint64_t> _load_elf(const char *fn, SparseRam *memif, reg_t *entry, unsigned required_xlen = 0)
{
  int fd = open(fn, O_RDONLY);
  struct stat s;
  if (fd == -1)
    throw std::invalid_argument(std::string("Specified ELF can't be opened: ") + strerror(errno));
  if (fstat(fd, &s) < 0)
    abort();
  size_t size = s.st_size;

  char *buf = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buf == MAP_FAILED)
    throw std::invalid_argument(std::string("Specified ELF can't be mapped: ") + strerror(errno));
  close(fd);

  assert(size >= sizeof(Elf64_Ehdr));
  const Elf64_Ehdr *eh64 = (const Elf64_Ehdr *)buf;
  assert(IS_ELF32(*eh64) || IS_ELF64(*eh64));
  unsigned xlen = IS_ELF32(*eh64) ? 32 : 64;
  if (required_xlen != 0 && required_xlen != xlen)
  {
    throw std::invalid_argument(sfmt(std::string("ELF error required_xlen=%d, but get :%d"), required_xlen, xlen));
  }
  assert(IS_ELFLE(*eh64) || IS_ELFBE(*eh64));
  assert(IS_ELF_EXEC(*eh64));
  assert(IS_ELF_RISCV(*eh64) || IS_ELF_EM_NONE(*eh64));
  assert(IS_ELF_VCURRENT(*eh64));

  std::vector<uint8_t> zeros;
  std::map<std::string, uint64_t> symbols;

#define LOAD_ELF(ehdr_t, phdr_t, shdr_t, sym_t, bswap)                         \
  do                                                                           \
  {                                                                            \
    ehdr_t *eh = (ehdr_t *)buf;                                                \
    phdr_t *ph = (phdr_t *)(buf + bswap(eh->e_phoff));                         \
    *entry = bswap(eh->e_entry);                                               \
    assert(size >= bswap(eh->e_phoff) + bswap(eh->e_phnum) * sizeof(*ph));     \
    for (unsigned i = 0; i < bswap(eh->e_phnum); i++)                          \
    {                                                                          \
      if (bswap(ph[i].p_type) == PT_LOAD && bswap(ph[i].p_memsz))              \
      {                                                                        \
        if (bswap(ph[i].p_filesz))                                             \
        {                                                                      \
          assert(size >= bswap(ph[i].p_offset) + bswap(ph[i].p_filesz));       \
          memif->write(bswap(ph[i].p_paddr), bswap(ph[i].p_filesz),            \
                       (uint8_t *)buf + bswap(ph[i].p_offset));                \
        }                                                                      \
        if (size_t pad = bswap(ph[i].p_memsz) - bswap(ph[i].p_filesz))         \
        {                                                                      \
          zeros.resize(pad);                                                   \
          memif->write(bswap(ph[i].p_paddr) + bswap(ph[i].p_filesz), pad,      \
                       zeros.data());                                          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    shdr_t *sh = (shdr_t *)(buf + bswap(eh->e_shoff));                         \
    assert(size >= bswap(eh->e_shoff) + bswap(eh->e_shnum) * sizeof(*sh));     \
    assert(bswap(eh->e_shstrndx) < bswap(eh->e_shnum));                        \
    assert(size >= bswap(sh[bswap(eh->e_shstrndx)].sh_offset) +                \
                       bswap(sh[bswap(eh->e_shstrndx)].sh_size));              \
    char *shstrtab = buf + bswap(sh[bswap(eh->e_shstrndx)].sh_offset);         \
    unsigned strtabidx = 0, symtabidx = 0;                                     \
    for (unsigned i = 0; i < bswap(eh->e_shnum); i++)                          \
    {                                                                          \
      unsigned max_len =                                                       \
          bswap(sh[bswap(eh->e_shstrndx)].sh_size) - bswap(sh[i].sh_name);     \
      assert(bswap(sh[i].sh_name) < bswap(sh[bswap(eh->e_shstrndx)].sh_size)); \
      assert(strnlen(shstrtab + bswap(sh[i].sh_name), max_len) < max_len);     \
      if (bswap(sh[i].sh_type) & SHT_NOBITS)                                   \
        continue;                                                              \
      assert(size >= bswap(sh[i].sh_offset) + bswap(sh[i].sh_size));           \
      if (strcmp(shstrtab + bswap(sh[i].sh_name), ".strtab") == 0)             \
        strtabidx = i;                                                         \
      if (strcmp(shstrtab + bswap(sh[i].sh_name), ".symtab") == 0)             \
        symtabidx = i;                                                         \
    }                                                                          \
    if (strtabidx && symtabidx)                                                \
    {                                                                          \
      char *strtab = buf + bswap(sh[strtabidx].sh_offset);                     \
      sym_t *sym = (sym_t *)(buf + bswap(sh[symtabidx].sh_offset));            \
      for (unsigned i = 0; i < bswap(sh[symtabidx].sh_size) / sizeof(sym_t);   \
           i++)                                                                \
      {                                                                        \
        unsigned max_len =                                                     \
            bswap(sh[strtabidx].sh_size) - bswap(sym[i].st_name);              \
        assert(bswap(sym[i].st_name) < bswap(sh[strtabidx].sh_size));          \
        assert(strnlen(strtab + bswap(sym[i].st_name), max_len) < max_len);    \
        symbols[strtab + bswap(sym[i].st_name)] = bswap(sym[i].st_value);      \
      }                                                                        \
    }                                                                          \
  } while (0)

  if (IS_ELFLE(*eh64))
  {
    if (memif->get_target_endianness() != endianness_little)
    {
      throw std::invalid_argument("Specified ELF is little endian, but system uses a big-endian memory system. Rerun without --big-endian");
    }
    if (IS_ELF32(*eh64))
      LOAD_ELF(Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym, from_le);
    else
      LOAD_ELF(Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Sym, from_le);
  }
  else
  {
#ifndef RISCV_ENABLE_DUAL_ENDIAN
    throw std::invalid_argument("Specified ELF is big endian.  Configure with --enable-dual-endian to enable support");
#else
    if (memif->get_target_endianness() != endianness_big)
    {
      throw std::invalid_argument("Specified ELF is big endian, but system uses a little-endian memory system. Rerun with --big-endian");
    }
    if (IS_ELF32(*eh64))
      LOAD_ELF(Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym, from_be);
    else
      LOAD_ELF(Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Sym, from_be);
#endif
  }

  munmap(buf, size);

  return symbols;
}

int file_is_elf(const char *fn){
  int fd = open(fn, O_RDONLY);
  struct stat s;
  if (fd == -1)
    throw std::invalid_argument(std::string("Specified ELF can't be opened: ") + strerror(errno));
  if (fstat(fd, &s) < 0)
    abort();
  size_t size = s.st_size;

  char *buf = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buf == MAP_FAILED)
    throw std::invalid_argument(std::string("Specified ELF can't be mapped: ") + strerror(errno));
  close(fd);

  if(size < sizeof(Elf64_Ehdr)){
    munmap(buf, size);
    return 0;
  }
  const Elf64_Ehdr *eh64 = (const Elf64_Ehdr *)buf;
  if(!(IS_ELF32(*eh64) || IS_ELF64(*eh64))){
    munmap(buf, size);
    return 0;
  }
  if (!(IS_ELFLE(*eh64) || IS_ELFBE(*eh64))){
    munmap(buf, size);
    return 0;
  }
  if (!(IS_ELF_EXEC(*eh64))){
    munmap(buf, size);
    return 0;
    };
  if (!(IS_ELF_RISCV(*eh64) || IS_ELF_EM_NONE(*eh64))){
    munmap(buf, size);
    return 0;
    };
  if (!(IS_ELF_VCURRENT(*eh64))){
    munmap(buf, size);
    return 0;
  }
  munmap(buf, size);
  return 1;
}

/*******************************************SparseRam Define****************************************************************/

SparseRam::~SparseRam()
{
  for (auto iter = this->mem.begin(); iter != this->mem.end(); iter++)
  {
    free(iter->second);
  }
  for (auto iter = this->big_block.begin(); iter != this->big_block.end(); iter++){
    free(iter->second->blk);
    free(iter->second);
  }
  this->mem.clear();
  this->cache.clear();
  this->big_block.clear();
}

void SparseRam::__update_cache(paddr_t index, u_int8_t *m)
{

  paddr_t big_index = 0;
  u_long big_time = 0;

  for (auto c = this->cache.begin(); c != this->cache.end(); c++)
  {
    auto tdata = c->second;
    auto c_mem = std::get<0>(tdata);
    auto c_tim = std::get<1>(tdata);
    if (c_tim > big_time)
    {
      big_time = c_tim;
      big_index = c->first;
    }
    std::tuple<u_int8_t *, u_long> ndata = std::make_tuple(c_mem, c_tim + 1);
    c->second = ndata;
  }

  std::tuple<u_int8_t *, u_long> a = std::make_tuple(m, 1);
  this->cache[index] = a;

  if (this->cache.size() < this->max_cache_size)
  {
    return;
  }
  // need replace: remove biggest tim
  vassert(big_index != 0, "find oldest item fail!");
  cache.erase(big_index);
}

bool SparseRam::_fast_count(paddr_t index)
{
  if (this->cache.count(index))
  {
    return true;
  }
  // miss
  auto ret = this->mem.count(index);
  if (ret)
  {
    this->__update_cache(index, this->mem[index]);
  }
  return ret;
}

u_int8_t *SparseRam::_fast_mem(paddr_t index)
{
  if (this->cache.count(index))
  {
    return std::get<0>(this->cache[index]);
  }
  // miss
  auto ret = this->mem[index];
  __update_cache(index, ret);
  return ret;
}

bool SparseRam::load_bin(const char *file, paddr_t addr)
{
  int fd = open(file, O_RDONLY);
  struct stat s;
  if (fd == -1)
    throw std::invalid_argument(std::string("Specified BIN can't be opened: ") + strerror(errno));
  if (fstat(fd, &s) < 0)
    abort();
  size_t size = s.st_size;

  char *buf = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buf == MAP_FAILED)
    throw std::invalid_argument(std::string("Specified BIN can't be mapped: ") + strerror(errno));
  close(fd);

  // read to mem
  this->write(addr, size, buf);
  return true;
}

bool SparseRam::load_elf(const char *file)
{
  reg_t entry;
  _load_elf(file, this, &entry);
  return true;
}

void SparseRam::read(paddr_t addr, size_t len, void *bytes)
{
  if (len <= 0)
  {
    return;
  }
  if(this->_blk_read(addr, len, bytes)){
    return;
  }

  auto index_start = addr / this->block_size;
  auto offst_start = addr % this->block_size;
  auto offst_end = offst_start + len;
  auto index_count = 1;
  auto lenrm = len - (this->block_size - offst_start);

  if (unlikely(lenrm > 0))
  {
    offst_end = lenrm % this->block_size;
    index_count += lenrm / this->block_size;
    if (likely(offst_end != 0))
    {
      index_count += 1;
    }
  }

  auto i = 0;
  auto buff = (u_int8_t *)bytes;
  for (auto idx = 0; idx < index_count; idx++)
  {
    auto base = index_start + idx;
    auto empt = !this->_fast_count(base);
    auto of_start = idx == 0 ? offst_start : 0;
    auto of_end = idx == index_count - 1 ? offst_end : this->block_size;
    for (auto p = of_start; p < of_end; p++)
    {
      buff[i] = empt ? 0 : this->_fast_mem(base)[p];
      i += 1;
    }
  }
}

void SparseRam::write(paddr_t addr, size_t len, const void *bytes)
{
  /*
           start(0)                            end
             |      end-start=block_size        |
   Block 1:  ----------------...-----------------
   Block 2:  ----------------...-----------------
   Block N-1:
   Block N:  ----------------...-----------------
   */
  if (len <= 0)
  {
    return;
  }
  if(this->_blk_write(addr, len, bytes)){
    return;
  }

  auto index_start = addr / this->block_size;
  auto offst_start = addr % this->block_size;
  auto offst_end = offst_start + len;
  auto lenrm = len - (this->block_size - offst_start);
  auto index_count = 1;

  if (unlikely(lenrm > 0))
  {
    offst_end = lenrm % this->block_size;
    index_count += lenrm / this->block_size;
    if (likely(offst_end != 0))
    {
      index_count += 1;
    }
  }

  auto i = 0;
  auto buff = (u_int8_t *)bytes;
  for (auto idx = 0; idx < index_count; idx++)
  {
    auto base = index_start + idx;
    if (unlikely(!this->_fast_count(base)))
    {
      this->mem[base] = (u_int8_t *)calloc(this->block_size, sizeof(u_int8_t));
    }
    auto of_start = idx == 0 ? offst_start : 0;
    auto of_end = idx == index_count - 1 ? offst_end : this->block_size;
    for (auto p = of_start; p < of_end; p++)
    {
      this->_fast_mem(base)[p] = buff[i];
      i += 1;
    }
  }
}

bool SparseRam::add_blk(char *name, paddr_t start, paddr_t end){
  vassert(this->mem.empty(), "should first init big_blocks. not write mem");
  vassert(end > start, "big_block size need > 0");
  auto blk_name = std::string(name);
  vassert(!this->big_block.count(blk_name), "big_block is existed");
  auto blk = new sp_mm_blk{
    .start = start,
    .end = end,
    .blk = (u_int8_t *)calloc(end - start, sizeof(u_int8_t))
  };
  this->big_block[blk_name] = blk;
  return true;
}

void *SparseRam::blk_host_addr(char *name){
  auto block_name = std::string(name);
  if(this->big_block.count(block_name)){
    return this->big_block[block_name]->blk;
  }
  return NULL;
}

SparseRam::sp_mm_blk *SparseRam::_blk_find(paddr_t addr){
  if (this->big_block.empty()){
    return NULL;
  }
  for(auto b = this->big_block.begin(); b != this->big_block.end(); b++){
    auto start = b->second->start;
    auto end = b->second->end;
    if (start < addr && addr < end){
      return b->second;
    }
  }
  return NULL;
}

bool SparseRam::_blk_read(paddr_t addr, size_t len, void* bytes){
  auto blk = this->_blk_find(addr);
  if(blk == NULL){
    return false;
  }
  vassert(blk->end >= addr + len, "read outof blk");
  memcpy(bytes, blk->blk + (addr - blk->start), len);
  return true;
}

bool SparseRam::_blk_write(paddr_t addr, size_t len, const void* bytes){
  auto blk = this->_blk_find(addr);
  if(blk == NULL){
    return false;
  }
  vassert(blk->end >= addr + len, "write outof blk");
  memcpy(blk->blk + (addr - blk->start), bytes, len);
  return true;
}

word_t SparseRam::read(paddr_t addr, int len)
{
  vassert(len <= 8, "len error");
  u_int8_t buff[8];
  this->read(addr, 8, (void *)buff);

  switch (len) {
    case 1: return *(uint8_t  *)buff;
    case 2: return *(uint16_t *)buff;
    case 4: return *(uint32_t *)buff;
    IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)buff);
    default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
  }
  vassert(0, "size error");
  return 0;
}

void SparseRam::write(paddr_t addr, int len, word_t data)
{
  vassert(len <= 8, "len error");
  u_int8_t buff[8] = {0};
  switch (len) {
    case 1: *(uint8_t  *)buff = data; break;
    case 2: *(uint16_t *)buff = data; break;
    case 4: *(uint32_t *)buff = data; break;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)buff = data; break);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
  return this->write(addr, len, (const void *)buff);
}

endianness_t SparseRam::get_target_endianness()
{
  return endianness_little;
}

void SparseRam::copy_nzero_bytes(copy_mem_func copy_handler)
{
  for (auto iter = this->mem.begin(); iter != this->mem.end(); iter++)
  {
    auto addr = iter->first * this->block_size;
    auto buff = iter->second;
    u_int astart = 0;
    for (u_int i = 0; i < this->block_size; i++)
    {
      if (buff[i] == 0)
      {
        auto size = i - astart;
        if (size > 0)
        {
          copy_handler(addr + astart, size, &buff[astart]);
        }
        astart = i + 1;
      }
    }
    if (astart < this->block_size)
    {
      copy_handler(addr + astart, this->block_size - astart, &buff[astart]);
    }
  }
}

void SparseRam::copy(SparseRam *dst) {
  // copy big blocks (only copy the blocks with same cfg)
  for(auto bl=this->big_block.begin(); bl != this->big_block.end(); bl++){
    auto name = bl->first;
    auto sbk = bl->second;
    if(!dst->big_block.count(name)){
      continue;
    }
    auto tbk = dst->big_block[name];
    if (sbk->start != tbk->start || sbk->end != tbk->end){
      continue;
    }
    memcpy(tbk->blk, sbk->blk, tbk->end - tbk->start);
  }
  // copy normal mem blocks
  auto fc = [&](paddr_t addr, size_t len, void* buff){
    dst->write(addr, len, buff);
  };
  this->copy_nzero_bytes(fc);
}

void SparseRam::copy_bytes(copy_mem_func copy_handler)
{
  // copy big blocks (only copy the blocks with same cfg)
  for(auto bl=this->big_block.begin(); bl != this->big_block.end(); bl++){
    auto name = bl->first;
    auto sbk = bl->second;
    copy_handler(sbk->start, sbk->end - sbk->start, sbk->blk);
  }
  // copy norm mem
  for (auto iter = this->mem.begin(); iter != this->mem.end(); iter++)
  {
    auto addr = iter->first * this->block_size;
    auto buff = iter->second;
    copy_handler(addr, this->block_size, buff);
  }
}

void SparseRam::print_info()
{
  OUTPUT(stderr, "SpRam blocks: %ld, size: %.2f MB\n", 
         this->mem.size(), float(this->mem.size() * this->block_size) / (1024.0 * 1024.0));
}

/*******************************************Export CAPIs****************************************************************/

void *sparse_mem_new(u_int block_count, u_int chunk_size)
{
  return new SparseRam(block_count, chunk_size);
}

void sparse_mem_del(void *mem)
{
  delete (SparseRam *)mem;
}

int sparse_mem_elf(void *self, const char *fname)
{
  auto m = (SparseRam *)self;
  return m->load_elf(fname);
}

int sparse_mem_bin(void *self, const char *fname, paddr_t addr)
{
  auto m = (SparseRam *)self;
  return m->load_bin(fname, addr);
}

void sparse_mem_read(void *self, paddr_t addr, size_t len, void *bytes)
{
  auto m = (SparseRam *)self;
  return m->read(addr, len, bytes);
}

void sparse_mem_write(void *self, paddr_t addr, size_t len, const void *bytes)
{
  auto m = (SparseRam *)self;
  return m->write(addr, len, bytes);
}

void sparse_mem_wwrite(void *self, paddr_t addr, int len, word_t data)
{
  auto m = (SparseRam *)self;
  return m->write(addr, len, data);
}

word_t sparse_mem_wread(void *self, paddr_t addr, int len)
{
  auto m = (SparseRam *)self;
  return m->read(addr, len);
}

void sparse_mem_info(void *self)
{
  auto m = (SparseRam *)self;
  m->print_info();
}

void* sparse_mem_blk_get(void *self, char *name){
  auto m = (SparseRam *)self;
  return m->blk_host_addr(name);
}

int sparse_mem_blk_add(void *self, char *name, paddr_t start, paddr_t end){
  auto m = (SparseRam *)self;
  if(m->add_blk(name, start, end)){
    return true;
  }
  return false;
}

void sparse_mem_copy(void *dst, void *src){
  auto d = (SparseRam*)dst;
  auto s = (SparseRam*)src;
  s->copy(d);
}
