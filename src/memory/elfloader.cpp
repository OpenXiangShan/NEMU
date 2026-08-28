/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <memory/elfloader.h>

#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

namespace {

struct LoadSegment {
  uint64_t address;
  uint64_t file_offset;
  uint64_t file_size;
  uint64_t memory_size;
};

struct MappedFile {
  const uint8_t *data = nullptr;
  size_t size = 0;
  int fd = -1;

  ~MappedFile() {
    if (data != nullptr) {
      munmap(const_cast<uint8_t *>(data), size);
    }
    if (fd >= 0) {
      close(fd);
    }
  }
};

static bool range_in_file(uint64_t offset, uint64_t length, size_t file_size) {
  return offset <= file_size && length <= file_size - offset;
}

static bool range_in_memory(uint64_t address, uint64_t length,
                            uint64_t memory_base, size_t memory_size,
                            size_t *offset) {
  if (address < memory_base) {
    return false;
  }
  uint64_t relative = address - memory_base;
  if (relative > memory_size || length > memory_size - relative) {
    return false;
  }
  *offset = static_cast<size_t>(relative);
  return true;
}

static bool map_file(const char *filename, MappedFile *file) {
  file->fd = open(filename, O_RDONLY);
  if (file->fd < 0) {
    return false;
  }

  struct stat statbuf;
  if (fstat(file->fd, &statbuf) < 0 || statbuf.st_size <= 0) {
    return false;
  }
  if (static_cast<uint64_t>(statbuf.st_size) > SIZE_MAX) {
    return false;
  }

  file->size = static_cast<size_t>(statbuf.st_size);
  void *mapped = mmap(nullptr, file->size, PROT_READ, MAP_PRIVATE, file->fd, 0);
  if (mapped == MAP_FAILED) {
    file->data = nullptr;
    return false;
  }
  file->data = static_cast<const uint8_t *>(mapped);
  return true;
}

static bool parse_load_segments(const MappedFile &file, std::vector<LoadSegment> *segments,
                                uint64_t *entry) {
  if (file.size < EI_NIDENT || memcmp(file.data, ELFMAG, SELFMAG) != 0) {
    return false;
  }

  const unsigned char elf_class = file.data[EI_CLASS];
  if (file.data[EI_DATA] != ELFDATA2LSB) {
    return false;
  }

  uint64_t program_offset;
  uint16_t program_count;
  uint16_t program_entry_size;

  if (elf_class == ELFCLASS32) {
    if (file.size < sizeof(Elf32_Ehdr)) {
      return false;
    }
    const Elf32_Ehdr *header = reinterpret_cast<const Elf32_Ehdr *>(file.data);
    if (header->e_phentsize < sizeof(Elf32_Phdr)) {
      return false;
    }
    program_offset = header->e_phoff;
    program_count = header->e_phnum;
    program_entry_size = header->e_phentsize;
    *entry = header->e_entry;
  } else if (elf_class == ELFCLASS64) {
    if (file.size < sizeof(Elf64_Ehdr)) {
      return false;
    }
    const Elf64_Ehdr *header = reinterpret_cast<const Elf64_Ehdr *>(file.data);
    if (header->e_phentsize < sizeof(Elf64_Phdr)) {
      return false;
    }
    program_offset = header->e_phoff;
    program_count = header->e_phnum;
    program_entry_size = header->e_phentsize;
    *entry = header->e_entry;
  } else {
    return false;
  }

  uint64_t program_table_size = static_cast<uint64_t>(program_count) * program_entry_size;
  if (!range_in_file(program_offset, program_table_size, file.size)) {
    return false;
  }

  for (uint16_t i = 0; i < program_count; i++) {
    const uint8_t *program_header = file.data + program_offset +
                                    static_cast<uint64_t>(i) * program_entry_size;
    uint32_t type;
    LoadSegment segment{};

    if (elf_class == ELFCLASS32) {
      Elf32_Phdr header;
      memcpy(&header, program_header, sizeof(header));
      type = header.p_type;
      segment.address = header.p_paddr;
      segment.file_offset = header.p_offset;
      segment.file_size = header.p_filesz;
      segment.memory_size = header.p_memsz;
    } else {
      Elf64_Phdr header;
      memcpy(&header, program_header, sizeof(header));
      type = header.p_type;
      segment.address = header.p_paddr;
      segment.file_offset = header.p_offset;
      segment.file_size = header.p_filesz;
      segment.memory_size = header.p_memsz;
    }

    if (type != PT_LOAD || segment.memory_size == 0) {
      continue;
    }
    if (segment.file_size > segment.memory_size ||
        !range_in_file(segment.file_offset, segment.file_size, file.size)) {
      return false;
    }
    segments->push_back(segment);
  }

  std::sort(segments->begin(), segments->end(),
            [](const LoadSegment &a, const LoadSegment &b) {
              return a.address < b.address;
            });
  return !segments->empty();
}

}  // namespace

extern "C" bool is_elf_file(const char *filename) {
  if (filename == nullptr) {
    return false;
  }

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return false;
  }

  unsigned char ident[EI_CLASS + 1];
  ssize_t bytes_read = read(fd, ident, sizeof(ident));
  close(fd);
  return bytes_read == sizeof(ident) && memcmp(ident, ELFMAG, SELFMAG) == 0 &&
         (ident[EI_CLASS] == ELFCLASS32 || ident[EI_CLASS] == ELFCLASS64);
}

extern "C" long load_elf_image(const char *filename, uint8_t *memory,
                                size_t memory_size, uint64_t memory_base) {
  if (filename == nullptr || memory == nullptr) {
    return -1;
  }

  MappedFile file;
  if (!map_file(filename, &file)) {
    fprintf(stderr, "cannot map ELF image '%s'\n", filename);
    return -1;
  }

  std::vector<LoadSegment> segments;
  uint64_t entry = 0;
  if (!parse_load_segments(file, &segments, &entry)) {
    fprintf(stderr, "invalid ELF image '%s'\n", filename);
    return -1;
  }

  uint64_t loaded_end = 0;
  for (const LoadSegment &segment : segments) {
    size_t destination_offset = 0;
    if (!range_in_memory(segment.address, segment.memory_size, memory_base,
                         memory_size, &destination_offset)) {
      fprintf(stderr, "ELF segment at 0x%lx is outside physical memory\n",
              static_cast<unsigned long>(segment.address));
      return -1;
    }

    memset(memory + destination_offset, 0, static_cast<size_t>(segment.memory_size));
    memcpy(memory + destination_offset, file.data + segment.file_offset,
           static_cast<size_t>(segment.file_size));
    uint64_t segment_end = segment.address + segment.memory_size - memory_base;
    loaded_end = std::max(loaded_end, segment_end);
    printf("Loading %lu bytes at address 0x%lx at offset 0x%lx\n",
           static_cast<unsigned long>(segment.memory_size),
           static_cast<unsigned long>(segment.address),
           static_cast<unsigned long>(segment.address - memory_base));
  }

  if (loaded_end > LONG_MAX) {
    return -1;
  }
  printf("ELF entry point: 0x%lx\n", static_cast<unsigned long>(entry));
  return static_cast<long>(loaded_end);
}
