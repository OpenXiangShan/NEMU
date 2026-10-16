#include "vfexp2.h"

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

typedef __uint128_t uint128_t;

typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t c;
} vfexp2_coeff_t;

typedef struct {
  int exp_width;
  int frac_width;
  int bias;
  int t_frac_bits;
  int coeff_frac_bits;
  int segment_bits;
  int coeff_count;
  const vfexp2_coeff_t *coeffs;
} vfexp2_format_t;

typedef struct {
  uint64_t bits;
  uint32_t fflags;
} vfexp2_packed_t;

static const vfexp2_coeff_t vfexp2_coeffs_16[] = {
  {16088, 45417, 65536}, {16800, 47428, 68438}, {17544, 49528, 71468}, {18321, 51721, 74632},
  {19132, 54011, 77936}, {19979, 56402, 81386}, {20864, 58899, 84990}, {21787, 61507, 88753},
  {22752, 64230, 92682}, {23759, 67074, 96785}, {24811, 70043, 101070}, {25910, 73144, 105545},
  {27057, 76383, 110218}, {28255, 79765, 115098}, {29506, 83296, 120194}, {30812, 86984, 125515},
};

static const vfexp2_coeff_t vfexp2_coeffs_128[] = {
  {252950, 726809, 1048576},
  {253926, 730761, 1054270},
  {254980, 734730, 1059994},
  {256412, 738718, 1065750},
  {257726, 742731, 1071537},
  {259293, 746764, 1077355},
  {260584, 750818, 1083205},
  {262669, 754890, 1089087},
  {263402, 758994, 1095000},
  {265682, 763108, 1100946},
  {266896, 767255, 1106924},
  {268249, 771422, 1112935},
  {269856, 775609, 1118978},
  {270942, 779826, 1125054},
  {272731, 784057, 1131163},
  {274108, 788315, 1137305},
  {275254, 792597, 1143480},
  {276908, 796903, 1149689},
  {278100, 801228, 1155932},
  {279529, 805580, 1162209},
  {281304, 809954, 1168519},
  {283099, 814350, 1174864},
  {284983, 818766, 1181244},
  {285690, 823219, 1187658},
  {288096, 827682, 1194106},
  {289646, 832176, 1200590},
  {290339, 836702, 1207109},
  {291996, 841244, 1213664},
  {293601, 845812, 1220254},
  {296120, 850396, 1226880},
  {296819, 855026, 1233542},
  {298510, 859667, 1240240},
  {300888, 864326, 1246974},
  {302509, 869019, 1253745},
  {303538, 873746, 1260553},
  {304834, 878491, 1267397},
  {307404, 883253, 1274279},
  {308248, 888055, 1281198},
  {310203, 892879, 1288155},
  {311596, 897726, 1295150},
  {313215, 902601, 1302182},
  {314924, 907502, 1309253},
  {316670, 912429, 1316362},
  {318433, 917383, 1323510},
  {320550, 922365, 1330696},
  {321870, 927373, 1337922},
  {323636, 932409, 1345187},
  {325744, 937471, 1352491},
  {327100, 942563, 1359835},
  {328840, 947681, 1367219},
  {330701, 952826, 1374642},
  {332423, 958001, 1382107},
  {334962, 963200, 1389611},
  {336053, 968433, 1397157},
  {338326, 973692, 1404743},
  {340673, 978970, 1412371},
  {341642, 984293, 1420040},
  {344340, 989631, 1427751},
  {345351, 995011, 1435503},
  {347160, 1000415, 1443298},
  {349023, 1005847, 1451135},
  {350939, 1011309, 1459014},
  {353892, 1016791, 1466937},
  {355748, 1022313, 1474902},
  {356749, 1027871, 1482910},
  {359515, 1033447, 1490962},
  {360727, 1039068, 1499058},
  {362578, 1044706, 1507198},
  {364836, 1050381, 1515382},
  {367230, 1056081, 1523610},
  {368444, 1061818, 1531883},
  {370456, 1067583, 1540201},
  {372502, 1073380, 1548564},
  {375841, 1079196, 1556973},
  {376701, 1085066, 1565427},
  {379767, 1090950, 1573927},
  {380714, 1096883, 1582474},
  {382795, 1102839, 1591066},
  {384788, 1108828, 1599706},
  {387315, 1114849, 1608392},
  {389808, 1120899, 1617125},
  {392156, 1126981, 1625906},
  {393258, 1133108, 1634735},
  {395339, 1139261, 1643611},
  {397555, 1145446, 1652536},
  {399642, 1151667, 1661509},
  {401887, 1157919, 1670531},
  {404018, 1164208, 1679601},
  {406254, 1170528, 1688722},
  {409472, 1176877, 1697891},
  {410593, 1183276, 1707110},
  {412826, 1189701, 1716380},
  {416312, 1196150, 1725700},
  {418211, 1202651, 1735070},
  {420790, 1209176, 1744491},
  {423106, 1215742, 1753964},
  {424270, 1222351, 1763488},
  {427901, 1228977, 1773063},
  {428797, 1235663, 1782691},
  {431131, 1242373, 1792371},
  {433866, 1249120, 1802103},
  {437235, 1255889, 1811888},
  {438159, 1262721, 1821727},
  {440612, 1269577, 1831618},
  {443011, 1276470, 1841564},
  {445900, 1283405, 1851563},
  {449388, 1290356, 1861617},
  {450194, 1297378, 1871726},
  {452657, 1304422, 1881889},
  {455145, 1311505, 1892107},
  {458950, 1318615, 1902381},
  {460053, 1325787, 1912711},
  {462621, 1332984, 1923097},
  {465107, 1340223, 1933539},
  {467296, 1347507, 1944038},
  {470130, 1354817, 1954594},
  {472668, 1362174, 1965207},
  {475262, 1369571, 1975878},
  {479458, 1376992, 1986607},
  {481464, 1384481, 1997394},
  {483083, 1392001, 2008240},
  {485649, 1399560, 2019144},
  {488276, 1407160, 2030108},
  {492271, 1414790, 2041131},
  {493665, 1422482, 2052214},
  {497718, 1430195, 2063358},
  {498990, 1437972, 2074562},
  {503247, 1445767, 2085826}
};

static const vfexp2_coeff_t vfexp2_coeffs_128_fp32[] = {
  {4047204, 11628951, 16777216},
  {4062809, 11692177, 16868315},
  {4079680, 11755674, 16959908},
  {4102585, 11819495, 17051999},
  {4123608, 11883694, 17144589},
  {4148689, 11948223, 17237683},
  {4169347, 12013085, 17331282},
  {4202709, 12078237, 17425389},
  {4214439, 12143904, 17520007},
  {4250909, 12209723, 17615139},
  {4270344, 12276077, 17710787},
  {4291982, 12342758, 17806955},
  {4317697, 12409741, 17903645},
  {4335079, 12477208, 18000860},
  {4363696, 12544918, 18098603},
  {4385726, 12613047, 18196877},
  {4404063, 12681558, 18295684},
  {4430535, 12750450, 18395027},
  {4449597, 12819643, 18494911},
  {4472467, 12889272, 18595336},
  {4500867, 12959263, 18696307},
  {4529585, 13029593, 18797826},
  {4559735, 13100256, 18899897},
  {4571043, 13171500, 19002521},
  {4609537, 13242907, 19105703},
  {4634340, 13314820, 19209445},
  {4645418, 13387234, 19313750},
  {4671944, 13459907, 19418622},
  {4697618, 13532989, 19524063},
  {4737925, 13606342, 19630078},
  {4749109, 13680421, 19736666},
  {4776157, 13754666, 19843835},
  {4814208, 13829212, 19951585},
  {4840142, 13904306, 20059920},
  {4856607, 13979941, 20168843},
  {4877342, 14055851, 20278358},
  {4918459, 14132049, 20388468},
  {4931967, 14208885, 20499175},
  {4963254, 14286058, 20610483},
  {4985539, 14363612, 20722396},
  {5011442, 14441621, 20834917},
  {5038784, 14520036, 20948048},
  {5066727, 14598870, 21061794},
  {5094927, 14678131, 21176158},
  {5128801, 14757841, 21291142},
  {5149921, 14837972, 21406751},
  {5178170, 14918537, 21522988},
  {5211897, 14999535, 21639855},
  {5233595, 15081003, 21757357},
  {5261447, 15162901, 21875498},
  {5291211, 15245215, 21994280},
  {5318771, 15328012, 22113706},
  {5359392, 15411207, 22233781},
  {5376855, 15494921, 22354509},
  {5413219, 15579065, 22475891},
  {5450772, 15663527, 22597934},
  {5466271, 15748682, 22720638},
  {5509437, 15834103, 22844009},
  {5525617, 15920177, 22968050},
  {5554562, 16006638, 23092764},
  {5584373, 16093558, 23218155},
  {5615020, 16180938, 23344227},
  {5662269, 16268660, 23470985},
  {5691969, 16357015, 23598430},
  {5707980, 16445940, 23726567},
  {5752240, 16535156, 23855400},
  {5771625, 16625093, 23984932},
  {5801245, 16715303, 24115168},
  {5837368, 16806089, 24246111},
  {5875684, 16897300, 24377765},
  {5895111, 16989086, 24510133},
  {5927302, 17081332, 24643221},
  {5960024, 17174074, 24777031},
  {6013453, 17267129, 24911569},
  {6027222, 17361058, 25046836},
  {6076274, 17455197, 25182838},
  {6091424, 17550124, 25319578},
  {6124719, 17645416, 25457061},
  {6156616, 17741249, 25595290},
  {6197043, 17837586, 25734270},
  {6236930, 17934381, 25874004},
  {6274498, 18031690, 26014498},
  {6292133, 18129722, 26155754},
  {6325425, 18228176, 26297777},
  {6360881, 18327138, 26440571},
  {6394267, 18426669, 26584141},
  {6430198, 18526707, 26728490},
  {6464293, 18627332, 26873623},
  {6500056, 18728451, 27019544},
  {6551551, 18830034, 27166258},
  {6569486, 18932411, 27313768},
  {6605223, 19035211, 27462079},
  {6660998, 19138405, 27611196},
  {6691377, 19242412, 27761121},
  {6732640, 19346820, 27911862},
  {6769693, 19451865, 28063421},
  {6788319, 19557624, 28215802},
  {6846411, 19663635, 28369011},
  {6860750, 19770614, 28523052},
  {6898096, 19877965, 28677929},
  {6941859, 19985922, 28833647},
  {6995754, 20094222, 28990212},
  {7010538, 20203541, 29147625},
  {7049791, 20313227, 29305894},
  {7088172, 20423525, 29465022},
  {7134395, 20534488, 29625014},
  {7190208, 20645690, 29785876},
  {7203108, 20758050, 29947609},
  {7242516, 20870753, 30110222},
  {7282316, 20984073, 30273718},
  {7343200, 21097839, 30438101},
  {7360855, 21212585, 30603377},
  {7401943, 21327751, 30769550},
  {7441713, 21443565, 30936626},
  {7476731, 21560108, 31104608},
  {7522077, 21677080, 31273503},
  {7562687, 21794788, 31443315},
  {7604195, 21913135, 31614049},
  {7671323, 22031874, 31785711},
  {7703424, 22151701, 31958304},
  {7729330, 22272016, 32131834},
  {7770385, 22392964, 32306307},
  {7812421, 22514559, 32481727},
  {7876339, 22636645, 32658100},
  {7898645, 22759709, 32835430},
  {7963482, 22883120, 33013723},
  {7983832, 23007558, 33192984},
  {8051957, 23132274, 33373219}
};

static const vfexp2_format_t vfexp2_fp16 = {
  .exp_width = 5,
  .frac_width = 10,
  .bias = 15,
  .t_frac_bits = 17,
  .coeff_frac_bits = 20,
  .segment_bits = 7,
  .coeff_count = 128,
  .coeffs = vfexp2_coeffs_128,
};

static const vfexp2_format_t vfexp2_bf16 = {
  .exp_width = 8,
  .frac_width = 7,
  .bias = 127,
  .t_frac_bits = 12,
  .coeff_frac_bits = 16,
  .segment_bits = 4,
  .coeff_count = 16,
  .coeffs = vfexp2_coeffs_16,
};

static const vfexp2_format_t vfexp2_fp32 = {
  .exp_width = 8,
  .frac_width = 23,
  .bias = 127,
  .t_frac_bits = 24,
  .coeff_frac_bits = 24,
  .segment_bits = 7,
  .coeff_count = 128,
  .coeffs = vfexp2_coeffs_128_fp32,
};

static inline uint64_t mask64(int width) {
  return width >= 64 ? ~0ULL : ((1ULL << width) - 1ULL);
}

static inline uint128_t mask128(int width) {
  return width >= 128 ? (uint128_t)(-1) : (((uint128_t)1 << width) - 1);
}

static inline uint64_t low64(uint64_t value, int width) {
  return value & mask64(width);
}

static inline uint128_t low128(uint128_t value, int width) {
  return value & mask128(width);
}

static inline int fmt_width(const vfexp2_format_t *fmt) {
  return 1 + fmt->exp_width + fmt->frac_width;
}

static inline uint64_t fmt_inf(const vfexp2_format_t *fmt) {
  return ((1ULL << fmt->exp_width) - 1ULL) << fmt->frac_width;
}

static inline uint64_t fmt_max_exp_field(const vfexp2_format_t *fmt) {
  return (1ULL << fmt->exp_width) - 2ULL;
}

static inline uint64_t fmt_max_finite(const vfexp2_format_t *fmt) {
  return (fmt_max_exp_field(fmt) << fmt->frac_width) | ((1ULL << fmt->frac_width) - 1ULL);
}

static inline uint64_t fmt_canonical_nan(const vfexp2_format_t *fmt) {
  return fmt_inf(fmt) | (1ULL << (fmt->frac_width - 1));
}

static inline int fmt_coeff_width(const vfexp2_format_t *fmt) {
  return fmt->coeff_frac_bits + 2;
}

static inline int fmt_local_t_keep(const vfexp2_format_t *fmt) {
  return fmt->t_frac_bits - fmt->segment_bits;
}

static inline int fmt_sig_frac_bits(const vfexp2_format_t *fmt) {
  return fmt->coeff_frac_bits + 2 * fmt->t_frac_bits;
}

static inline int fmt_horner1_width(const vfexp2_format_t *fmt) {
  return fmt_coeff_width(fmt) + fmt->t_frac_bits + 1;
}

static inline int fmt_horner1_keep(const vfexp2_format_t *fmt) {
  return fmt_coeff_width(fmt);
}

static inline int fmt_horner1_shift(const vfexp2_format_t *fmt) {
  return fmt_horner1_width(fmt) - fmt_horner1_keep(fmt);
}

static inline int fmt_product2_trunc_width(const vfexp2_format_t *fmt) {
  return fmt_horner1_keep(fmt) + fmt_local_t_keep(fmt);
}

static inline int fmt_product2_width(const vfexp2_format_t *fmt) {
  return fmt_product2_trunc_width(fmt) + fmt_horner1_shift(fmt);
}

static inline int fmt_c_aligned_width(const vfexp2_format_t *fmt) {
  return fmt_coeff_width(fmt) + 2 * fmt->t_frac_bits;
}

static inline int fmt_sig_scaled_width(const vfexp2_format_t *fmt) {
  int a = fmt_product2_width(fmt);
  int b = fmt_c_aligned_width(fmt);
  return (a > b ? a : b) + 1;
}

bool vfexp2_rm_valid(uint32_t rm) {
  return rm <= FPCALL_RM_RMM;
}

static uint128_t shift_left_trunc_u128(uint128_t x, uint32_t shamt, int out_width) {
  if (out_width <= 0 || shamt >= 128 || (int)shamt >= out_width) {
    return 0;
  }
  return low128(x << shamt, out_width);
}

static void shift_right_jam_u128(uint128_t x, uint32_t shamt, uint128_t *out, bool *sticky) {
  if (shamt == 0) {
    *out = x;
    *sticky = false;
    return;
  }
  if (shamt >= 128) {
    *out = 0;
    *sticky = x != 0;
    return;
  }
  *out = x >> shamt;
  *sticky = (x & (((uint128_t)1 << shamt) - 1)) != 0;
}

static void round_shift_right_positive_u128(uint128_t x, uint32_t shamt, uint32_t rm, uint128_t *rounded, bool *inexact) {
  uint128_t shifted = 0;
  bool ignored = false;
  shift_right_jam_u128(x, shamt, &shifted, &ignored);

  uint128_t shifted_m1 = 0;
  bool sticky_low = false;
  uint32_t shamt_m1 = shamt == 0 ? 0 : (shamt - 1);
  shift_right_jam_u128(x, shamt_m1, &shifted_m1, &sticky_low);

  bool guard = shamt == 0 ? false : ((shifted_m1 & 1) != 0);
  bool sticky = shamt <= 1 ? false : sticky_low;
  *inexact = guard || sticky;

  bool round_up = false;
  switch (rm) {
    case FPCALL_RM_RNE: round_up = guard && (sticky || ((shifted & 1) != 0)); break;
    case FPCALL_RM_RTZ: round_up = false; break;
    case FPCALL_RM_RDN: round_up = false; break;
    case FPCALL_RM_RUP: round_up = *inexact; break;
    case FPCALL_RM_RMM: round_up = guard; break;
    default: panic("Unsupported vfexp2 rm = %u", rm);
  }

  *rounded = shifted + (round_up ? 1 : 0);
}

static uint64_t overflow_result(const vfexp2_format_t *fmt, uint32_t rm) {
  return (rm == FPCALL_RM_RTZ || rm == FPCALL_RM_RDN) ? fmt_max_finite(fmt) : fmt_inf(fmt);
}

static vfexp2_packed_t round_and_pack_positive(uint128_t sig_scaled, const vfexp2_format_t *fmt, int32_t k, uint32_t rm, bool has_frac_input) {
  const int sig_frac_bits = fmt_sig_frac_bits(fmt);
  const int normal_shift = sig_frac_bits - fmt->frac_width;
  const uint128_t overflow_sig = (uint128_t)1 << (fmt->frac_width + 1);
  const uint128_t normal_threshold = (uint128_t)1 << fmt->frac_width;
  const int sig_scaled_width = fmt_sig_scaled_width(fmt);

  uint128_t normalized_sig = low128(sig_scaled, sig_scaled_width);
  int32_t normalized_k = k;
  if (sig_scaled >= ((uint128_t)2 << sig_frac_bits)) {
    normalized_sig = (sig_scaled + 1) >> 1;
    normalized_k = k + 1;
  }

  const int32_t normal_exp = normalized_k + fmt->bias;
  uint128_t normal_rounded_raw = 0;
  bool normal_round_inexact = false;
  round_shift_right_positive_u128(normalized_sig, normal_shift, rm, &normal_rounded_raw, &normal_round_inexact);
  const bool normal_carry = normal_rounded_raw >= overflow_sig;
  const uint128_t normal_rounded = normal_carry ? (normal_rounded_raw >> 1) : normal_rounded_raw;
  const int32_t normal_exp_adj = normal_exp + (normal_carry ? 1 : 0);
  const bool normal_overflow = normal_exp_adj > (int32_t)fmt_max_exp_field(fmt);
  const uint64_t normal_result =
    ((uint64_t)low64((uint64_t)normal_exp_adj, fmt->exp_width) << fmt->frac_width) |
    low64((uint64_t)normal_rounded, fmt->frac_width);

  const int32_t sub_shift = sig_frac_bits - (normalized_k + (fmt->bias - 1 + fmt->frac_width));
  const int sub_width = sig_scaled_width + fmt->frac_width + 4;
  const uint128_t sub_sig_wide = low128(normalized_sig, sub_width);
  uint128_t sub_rounded_raw = 0;
  bool sub_round_inexact = false;
  if (sub_shift <= 0) {
    sub_rounded_raw = shift_left_trunc_u128(sub_sig_wide, (uint32_t)(-sub_shift), sub_width + 1);
  } else {
    round_shift_right_positive_u128(sub_sig_wide, (uint32_t)sub_shift, rm, &sub_rounded_raw, &sub_round_inexact);
  }

  const bool sub_carry = sub_rounded_raw >= normal_threshold;
  const bool sub_zero = sub_rounded_raw == 0;
  const uint64_t sub_result = sub_carry
    ? ((uint64_t)1 << fmt->frac_width)
    : low64((uint64_t)sub_rounded_raw, fmt->frac_width);
  const bool sub_underflow = !sub_carry && !sub_zero && sub_round_inexact;
  const bool zero_underflow = sub_zero && sub_round_inexact;

  uint64_t result = 0;
  bool of = false;
  bool uf = false;
  bool nx_round = false;
  if (normal_exp > 0) {
    result = normal_overflow ? overflow_result(fmt, rm) : normal_result;
    of = normal_overflow;
    uf = false;
    nx_round = normal_round_inexact || normal_overflow;
  } else {
    result = sub_result;
    of = false;
    uf = sub_underflow || zero_underflow;
    nx_round = sub_round_inexact;
  }

  const bool nx = has_frac_input || nx_round;
  vfexp2_packed_t out = {
    .bits = low64(result, fmt_width(fmt)),
    .fflags = (of ? FPCALL_EX_OF : 0) | (uf ? FPCALL_EX_UF : 0) | (nx ? FPCALL_EX_NX : 0),
  };
  return out;
}

vfexp2_result_t vfexp2_compute(uint64_t src_bits, int sew, bool is_bf16, uint32_t rm) {
  const vfexp2_format_t *fmt = NULL;
  if (is_bf16) {
    fmt = &vfexp2_bf16;
  } else if (sew == 1) {
    fmt = &vfexp2_fp16;
  } else if (sew == 2) {
    fmt = &vfexp2_fp32;
  } else {
    panic("Unsupported vfexp2 format: sew=%d is_bf16=%d", sew, is_bf16);
  }

  Assert(vfexp2_rm_valid(rm), "Unsupported vfexp2 rm = %u", rm);

  const int width = fmt_width(fmt);
  const int frac_width = fmt->frac_width;
  const uint64_t src = low64(src_bits, width);
  const uint64_t exp = (src >> frac_width) & ((1ULL << fmt->exp_width) - 1ULL);
  const uint64_t frac = src & ((1ULL << frac_width) - 1ULL);
  const bool sign = ((src >> (width - 1)) & 1ULL) != 0;

  const bool exp_is_zero = exp == 0;
  const bool exp_is_ones = exp == ((1ULL << fmt->exp_width) - 1ULL);
  const bool frac_not_zero = frac != 0;
  const bool is_subnormal = exp_is_zero && frac_not_zero;
  const bool is_inf = exp_is_ones && !frac_not_zero;
  const bool is_nan = exp_is_ones && frac_not_zero;
  const bool is_snan = is_nan && (((frac >> (frac_width - 1)) & 1ULL) == 0);

  if (is_nan) {
    return (vfexp2_result_t){.result = fmt_canonical_nan(fmt), .fflags = is_snan ? FPCALL_EX_NV : 0};
  }
  if (is_inf) {
    return (vfexp2_result_t){.result = sign ? 0ULL : fmt_inf(fmt), .fflags = 0};
  }
  if (is_subnormal) {
    return (vfexp2_result_t){.result = ((uint64_t)fmt->bias) << frac_width, .fflags = FPCALL_EX_NX};
  }

  const int sig_width = frac_width + 1;
  const uint64_t sig = exp_is_zero ? frac : ((1ULL << frac_width) | frac);
  const int32_t p = exp_is_zero ? (1 - fmt->bias - frac_width) : ((int32_t)exp - (fmt->bias + frac_width));
  const uint64_t int_mag_max = (1ULL << 15) - 1ULL;

  uint64_t int_mag = 0;
  uint64_t rem = 0;
  uint64_t sig_shifted_right = 0;
  if (p >= 15) {
    int_mag = int_mag_max;
    rem = 0;
  } else if (p >= 0) {
    const uint64_t shifted = low64((uint64_t)shift_left_trunc_u128(sig, (uint32_t)p, 16), 16);
    const bool overflow = ((uint64_t)sig >> (15 - p)) != 0;
    int_mag = (overflow || shifted > int_mag_max) ? int_mag_max : shifted;
    rem = 0;
  } else {
    const uint32_t rshift = (uint32_t)(-p);
    sig_shifted_right = rshift >= 64 ? 0 : (sig >> rshift);
    const uint64_t sig_reconstructed = low64((uint64_t)shift_left_trunc_u128(sig_shifted_right, rshift, sig_width), sig_width);
    int_mag = sig_shifted_right > int_mag_max ? int_mag_max : sig_shifted_right;
    rem = sig - low64(sig_reconstructed, sig_width);
  }

  const bool has_frac_exact = p < 0 && rem != 0;
  const int local_t_keep = fmt_local_t_keep(fmt);
  const uint64_t q_zero = 0;
  const uint64_t q_max = (1ULL << fmt->t_frac_bits) - 1ULL;
  uint64_t frac_q_pos = q_zero;
  uint64_t frac_q_neg = q_zero;
  if (has_frac_exact) {
    const uint32_t rshift = (uint32_t)(-p);
    if (rshift <= (uint32_t)fmt->t_frac_bits) {
      const uint32_t left_shift = fmt->t_frac_bits - rshift;
      frac_q_pos = low64((uint64_t)shift_left_trunc_u128(rem, left_shift, fmt->t_frac_bits), fmt->t_frac_bits);
      const uint64_t complement_base = low64((uint64_t)shift_left_trunc_u128(1, rshift, fmt->t_frac_bits + 1), fmt->t_frac_bits + 1);
      const uint64_t complement = low64(complement_base - rem, fmt->t_frac_bits);
      frac_q_neg = low64((uint64_t)shift_left_trunc_u128(complement, left_shift, fmt->t_frac_bits), fmt->t_frac_bits);
    } else {
      uint128_t pos_rounded = 0;
      bool inexact_unused = false;
      round_shift_right_positive_u128(rem, rshift - fmt->t_frac_bits, FPCALL_RM_RMM, &pos_rounded, &inexact_unused);
      const uint64_t pos_quantized = low64((uint64_t)pos_rounded, fmt->t_frac_bits);
      frac_q_pos = pos_quantized;
      frac_q_neg = pos_quantized == 0 ? q_max : low64((1ULL << fmt->t_frac_bits) - pos_quantized, fmt->t_frac_bits);
    }
  }

  int32_t k = 0;
  uint64_t frac_q = 0;
  if (!sign) {
    k = (int32_t)int_mag;
    frac_q = frac_q_pos;
  } else {
    k = has_frac_exact ? -(int32_t)(int_mag + 1ULL) : -(int32_t)int_mag;
    frac_q = frac_q_neg;
    if (has_frac_exact && sig_shifted_right == 0) {
      k = -1;
    }
  }

  const uint32_t seg_idx = (uint32_t)(frac_q >> local_t_keep);
  const uint64_t local_t = low64(frac_q, local_t_keep);
  Assert(seg_idx < (uint32_t)fmt->coeff_count, "vfexp2 seg_idx overflow");

  const vfexp2_coeff_t *coeff = &fmt->coeffs[seg_idx];
  const uint128_t product1 = (uint128_t)coeff->a * local_t;
  const uint128_t horner1 = product1 + ((uint128_t)coeff->b << fmt->t_frac_bits);
  const uint64_t horner1_t = low64((uint64_t)(horner1 >> fmt_horner1_shift(fmt)), fmt_horner1_keep(fmt));
  const uint128_t product2_trunc = (uint128_t)horner1_t * local_t;
  const uint128_t sig_scaled =
    (product2_trunc << fmt_horner1_shift(fmt)) + ((uint128_t)coeff->c << (2 * fmt->t_frac_bits));

  const vfexp2_packed_t packed = round_and_pack_positive(sig_scaled, fmt, k, rm, has_frac_exact);
  return (vfexp2_result_t){.result = packed.bits, .fflags = packed.fflags};
}
