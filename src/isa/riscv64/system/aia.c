#include "isa.h"
#include "cpu/cpu.h"
#include "../local-include/aia.h"

#ifdef CONFIG_RV_IMSIC

int interrupt_default_prio[IPRIO_ENABLE_NUM] = {
  // custom highest group
  63, 31, 62,
  61, 30, 60,
  // local high group
  47, 23, 46,
  45, 22, 44,
  43, 21, 42,
  41, 20, 40,
  // custom middle high group
  59, 29, 58,
  57, 28, 56,
  // priv arch group
  IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
  IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
  IRQ_SGEI,
  IRQ_VSEIP, IRQ_VSSIP, IRQ_VSTIP,
  IRQ_LCOFI, 14, 15,
  // custom middle low group
  55, 27, 54,
  53, 26, 52,
  // local low group
  39, 19, 38,
  37, 18, 36,
  35, 17, 34,
  33, 16, 32,
  // custom lowest group
  51, 25, 50,
  49, 24, 48
};

uint8_t get_prio_idx_in_group(uint8_t irq) {
  uint8_t idx = 0;
  for (int i = 0; i < IPRIO_ENABLE_NUM; i++) {
    if (irq == interrupt_default_prio[i]) {
      idx = i;
    }
  }
  return idx;
}

bool intr_enable(uint64_t topi_gather, uint64_t idx) {
  return (topi_gather >> interrupt_default_prio[idx]) & 0x1;
}

void set_iprios_sort(uint64_t topi_gather, IpriosSort* iprios_sort, IpriosModule* iprios, uint8_t xei, mtopei_t* xtopei) {
  for (int i = 0; i < IPRIO_ENABLE_NUM; i++) {
    if (intr_enable(topi_gather, i)) {
      iprios_sort->ipriosEnable[i].enable = true;
      if (interrupt_default_prio[i] == xei) {
        iprios_sort->ipriosEnable[i].isZero = !cpu.external_interrupt_select;
        iprios_sort->ipriosEnable[i].greaterThan255 = xtopei->iprio > 255;
        iprios_sort->ipriosEnable[i].priority = xtopei->iprio & 0xff;
      } else {
        iprios_sort->ipriosEnable[i].isZero = ((iprios->iprios[interrupt_default_prio[i]/8].val >> (8 * (interrupt_default_prio[i]%8))) & 0xff) == 0;
        iprios_sort->ipriosEnable[i].greaterThan255 = false;
        iprios_sort->ipriosEnable[i].priority = (iprios->iprios[interrupt_default_prio[i]/8].val >> (8 * (interrupt_default_prio[i]%8))) & 0xff;
      }
    } else {
      iprios_sort->ipriosEnable[i].enable = false;
      iprios_sort->ipriosEnable[i].isZero = false;
      iprios_sort->ipriosEnable[i].greaterThan255 = false;
      iprios_sort->ipriosEnable[i].priority = 0;
    }
  }
}

void handle_irq_hviprio(uint8_t irq, uint64_t topi, uint8_t priority) {
  word_t idx = get_prio_idx_in_group(irq);
  if (intr_enable(topi, idx)) {
    cpu.VSIpriosSort->ipriosEnable[idx].isZero = priority == 0;
    cpu.VSIpriosSort->ipriosEnable[idx].priority = priority;
  }
}

word_t get_hviprio2(uint8_t idx) {
  return (hviprio2->val >> 8 * idx) & 0xf;
}

void set_hviprios(Hviprios hprios) {
  hprios.hviprios[1] = hviprio1->ssi;
  hprios.hviprios[5] = hviprio1->sti;
  hprios.hviprios[13] = hviprio1->coi;
  hprios.hviprios[14] = hviprio1->intr14;
  hprios.hviprios[15] = hviprio1->intr15;
  for (int i = 0; i < 8; i++) {
    hprios.hviprios[i+16] = get_hviprio2(i);
  }
}

void set_viprios_sort(uint64_t topi_gather) {
  for (int i = 0; i < IPRIO_ENABLE_NUM; i++) {
    if (intr_enable(topi_gather, i)) {
      cpu.VSIpriosSort->ipriosEnable[i].enable = true;
      cpu.VSIpriosSort->ipriosEnable[i].isZero = true;
    } else {
      cpu.VSIpriosSort->ipriosEnable[i].enable = false;
    }
  }

  Hviprios hviprios;
  for (int i = 0; i < 24; i++) {
    hviprios.hviprios[i] = 0;
  }
  
  set_hviprios(hviprios);
  for (int i = 0; i < 24; i++) {
    handle_irq_hviprio(i, topi_gather, hviprios.hviprios[i]);
  }
}

uint8_t high_iprio(IpriosSort* ipriosSort, uint8_t xei) {
  uint8_t high_prio_idx = 0;

  for (int i = 1; i < IPRIO_ENABLE_NUM; i ++) {
    // enable
    bool left_enable  = ipriosSort->ipriosEnable[high_prio_idx].enable;
    bool right_enable = ipriosSort->ipriosEnable[i].enable;

    bool left_disenable = !left_enable;

    // zero
    bool left_prio_is_zero = ipriosSort->ipriosEnable[high_prio_idx].isZero;
    bool right_prio_is_zero = ipriosSort->ipriosEnable[i].isZero;

    bool left_prio_is_not_zero = !left_prio_is_zero;
    bool right_prio_is_not_zero = !right_prio_is_zero;

    // idx
    bool left_idx_leq_xei = high_prio_idx <= get_prio_idx_in_group(xei);
    bool right_idx_leq_xei = i <= get_prio_idx_in_group(xei);

    bool left_idx_greater_xei = !left_idx_leq_xei;

    // greater than 255
    bool left_prio_greater_255 = ipriosSort->ipriosEnable[high_prio_idx].greaterThan255;
    bool right_prio_greater_255 = ipriosSort->ipriosEnable[i].greaterThan255;

    bool left_prio_leq_255 = !left_prio_greater_255;
    bool right_prio_leq_255 = !right_prio_greater_255;

    // priority
    uint8_t left_prio = ipriosSort->ipriosEnable[high_prio_idx].priority;
    uint8_t right_prio = ipriosSort->ipriosEnable[i].priority;

    bool left_prio_leq_right_prio = left_prio <= right_prio;
    bool left_prio_greater_right_prio = !left_prio_leq_right_prio;

    if ((left_disenable && right_enable) ||
      (left_enable && right_enable &&
        (
          (left_prio_is_zero && right_prio_is_not_zero && left_idx_greater_xei) ||
          (left_prio_is_not_zero && right_prio_is_zero && right_idx_leq_xei) ||
          (left_prio_is_not_zero && right_prio_is_not_zero &&
            (
              (left_prio_greater_255 && right_prio_leq_255) ||
              (left_prio_leq_255 && right_prio_leq_255 && left_prio_greater_right_prio)
            )
          )
        )
      )
    ) {
      high_prio_idx = i;
    }
  }

  return high_prio_idx;
}

bool no_mtopi() {
  return mtopi->iid == 0;
}
bool no_stopi() {
  return stopi->iid == 0;
}
bool no_vstopi() {
  return vstopi->iid == 0;
}

#endif
