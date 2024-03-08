#include <checkpoint/checkpoint.pb.h>
#include <checkpoint/fill_protobuf.h>
#include <pb_encode.h>
#include <stdio.h>

int cpt_header_encode(void *gcpt_mmio, checkpoint_header *cpt_header, single_core_rvgc_rvv_rvh_memlayout *cpt_memlayout) {
  int status;

  if (cpt_header == NULL) {
    cpt_header = &default_cpt_header;
  }
  if (cpt_memlayout == NULL) {
    cpt_memlayout = &default_cpt_percpu_layout;
  }

  pb_ostream_t stream =
    pb_ostream_from_buffer((void *)gcpt_mmio, sizeof(checkpoint_header) + sizeof(single_core_rvgc_rvv_rvh_memlayout));
  status = pb_encode_ex(&stream, checkpoint_header_fields, cpt_header, PB_ENCODE_NULLTERMINATED);
  if (!status) {
    printf("LOG: header encode error %s\n", stream.errmsg);
    return 0;
  }

  status = pb_encode_ex(&stream, single_core_rvgc_rvv_rvh_memlayout_fields, cpt_memlayout, PB_ENCODE_NULLTERMINATED);
  if (!status) {
    printf("LOG: body encode error %s\n", stream.errmsg);
    return 0;
  }

  return cpt_header->cpt_offset;
}
