#include <isa.h>
#ifndef __ICS_EXPORT
#include <memory/vaddr.h>
#include <stdlib.h>
#endif

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */

#ifndef __ICS_EXPORT
  TK_NEQ, TK_OR, TK_AND, TK_NUM, TK_REG, TK_REF, TK_NEG
#endif
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
#ifndef __ICS_EXPORT
  {"0x[0-9a-fA-F]{1,16}", TK_NUM},  // hex
  {"[0-9]{1,10}", TK_NUM},         // dec
  {"\\$[a-z0-9]{1,31}", TK_REG},   // register names
  {"\\+", '+'},
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"%", '%'},
  {"==", TK_EQ},
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"!", '!'},
  {"\\(", '('},
  {"\\)", ')'}
#endif
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

#ifdef __ICS_EXPORT
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
#else
        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //    i, rules[i].regex, position, substr_len, substr_len, substr_start);
#endif

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
#ifdef __ICS_EXPORT
          default: TODO();
#else
          case TK_NOTYPE: break;
          case TK_NUM:
          case TK_REG: sprintf(tokens[nr_token].str, "%.*s", substr_len, substr_start);
          default: tokens[nr_token].type = rules[i].token_type;
                   nr_token ++;
#endif
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

#ifndef __ICS_EXPORT
static int op_prec(int t) {
  switch (t) {
    case '!': case TK_NEG: case TK_REF: return 0;
    case '*': case '/': case '%': return 1;
    case '+': case '-': return 2;
    case TK_EQ: case TK_NEQ: return 4;
    case TK_AND: return 8;
    case TK_OR: return 9;
    default: assert(0);
  }
}

static inline int op_prec_cmp(int t1, int t2) {
  return op_prec(t1) - op_prec(t2);
}

static int find_dominated_op(int s, int e, bool *success) {
  int i;
  int bracket_level = 0;
  int dominated_op = -1;
  for (i = s; i <= e; i ++) {
    switch (tokens[i].type) {
      case TK_REG: case TK_NUM: break;

      case '(':
        bracket_level ++;
        break;

      case ')':
        bracket_level --;
        if (bracket_level < 0) {
          *success = false;
          return 0;
        }
        break;

      default:
        if (bracket_level == 0) {
          if (dominated_op == -1 ||
              op_prec_cmp(tokens[dominated_op].type, tokens[i].type) < 0 ||
              (op_prec_cmp(tokens[dominated_op].type, tokens[i].type) == 0 &&
               tokens[i].type != '!' && tokens[i].type != '~' &&
               tokens[i].type != TK_NEG && tokens[i].type != TK_REF) ) {
            dominated_op = i;
          }
        }
        break;
    }
  }

  *success = (dominated_op != -1);
  return dominated_op;
}

static rtlreg_t eval(int s, int e, bool *success) {
  if (s > e) {
    // bad expression
    *success = false;
    return 0;
  }
  else if (s == e) {
    // single token
    rtlreg_t val;
    switch (tokens[s].type) {
      case TK_REG: val = isa_reg_str2val(tokens[s].str + 1, success); // +1 to skip '$'
                if (!*success) { return 0; }
                break;

      case TK_NUM: val = strtoul(tokens[s].str, NULL, 0); break;
      default: assert(0);
    }

    *success = true;
    return val;
  }
  else if (tokens[s].type == '(' && tokens[e].type == ')') {
    return eval(s + 1, e - 1, success);
  }
  else {
    int dominated_op = find_dominated_op(s, e, success);
    if (!*success) { return 0; }

    int op_type = tokens[dominated_op].type;
    if (op_type == '!' || op_type == TK_NEG || op_type == TK_REF) {
      rtlreg_t val = eval(dominated_op + 1, e, success);
      if (!*success) { return 0; }

      switch (op_type) {
        case '!': return !val;
        case TK_NEG: return -val;
        case TK_REF: return vaddr_read_safe(val, 4);
        default: assert(0);
      }
    }

    rtlreg_t val1 = eval(s, dominated_op - 1, success);
    if (!*success) { return 0; }
    rtlreg_t val2 = eval(dominated_op + 1, e, success);
    if (!*success) { return 0; }

    switch (op_type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case '%': return val1 % val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;
      default: assert(0);
    }
  }
}
#endif

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
#ifdef __ICS_EXPORT
  TODO();

  return 0;
#else

  /* Detect TK_REF and TK_NEG tokens */
  int i;
  int prev_type;
  for (i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '-') {
      if (i == 0) {
        tokens[i].type = TK_NEG;
        continue;
      }

      prev_type = tokens[i - 1].type;
      if ( !(prev_type == ')' || prev_type == TK_NUM ||
            prev_type == TK_REG) ) {
        tokens[i].type = TK_NEG;
      }
    }

    else if (tokens[i].type == '*') {
      if (i == 0) {
        tokens[i].type = TK_REF;
        continue;
      }

      prev_type = tokens[i - 1].type;
      if ( !(prev_type == ')' || prev_type == TK_NUM ||
            prev_type == TK_REG) ) {
        tokens[i].type = TK_REF;
      }
    }
  }

  return eval(0, nr_token - 1, success);
#endif
}
