#include "insn_macros.h"

require_privilege(PRV_M);
for (int i = 0; i < 32; i++) {
  fprintf(stderr, "GPR[%d] = 0x%08" PRIx64 "\n", i, STATE.XPR[i]);
}
exit(0);
