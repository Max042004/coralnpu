// vcompress vd, vs2, vs1
require(P.any_vector_extensions());
require(P.VU.vstart->read() == 0);
require_align(insn.rd(), P.VU.vflmul);
require_align(insn.rs2(), P.VU.vflmul);
require(insn.rd() != insn.rs2());
require_noover(insn.rd(), P.VU.vflmul, insn.rs1(), 1);

// Force a log entry for the destination register group 
// to ensure writeback visibility in logs even with an empty mask.
if (unlikely(P.get_log_commits_enabled())) {
  for (reg_t i = 0; i < P.VU.vflmul; ++i) {
    P.get_state()->log_reg_write[((insn.rd() + i) << 4) | 2] = {0, 0};
  }
}

reg_t pos = 0;

VI_GENERAL_LOOP_BASE
  if (P.VU.mask_elt(rs1_num, i)) {
    switch (sew) {
    case e8:
      P.VU.elt<uint8_t>(rd_num, pos, true) = P.VU.elt<uint8_t>(rs2_num, i);
      break;
    case e16:
      P.VU.elt<uint16_t>(rd_num, pos, true) = P.VU.elt<uint16_t>(rs2_num, i);
      break;
    case e32:
      P.VU.elt<uint32_t>(rd_num, pos, true) = P.VU.elt<uint32_t>(rs2_num, i);
      break;
    default:
      P.VU.elt<uint64_t>(rd_num, pos, true) = P.VU.elt<uint64_t>(rs2_num, i);
      break;
    }

    ++pos;
  }

VI_LOOP_END;
