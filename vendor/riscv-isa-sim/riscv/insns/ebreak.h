if (!STATE.debug_mode && (
        (!STATE.v && STATE.prv == PRV_M && STATE.dcsr->ebreakm) ||
        (!STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreaks) ||
        (!STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreaku) ||
        (STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreakvs) ||
        (STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreakvu))) {
	throw trap_debug_mode();
} else {
    // CoralNPU Deviation: EBREAK should just halt the simulation cleanly.
    // Dump GPRs for ISG verification before exiting.
    for (int i = 0; i < 32; i++) {
        fprintf(stderr, "GPR[%d] = 0x%08lx\n", i, (unsigned long)STATE.XPR[i]);
    }
	exit(0);
}
