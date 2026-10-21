# Table of Verilator models; edit by hand.
# Verilated cocotb models: see mk/model.mk.

# hdl/chisel/src/bus
MODELS += tlul_socket_m1_2_128_model
tlul_socket_m1_2_128_model_TOP    := TlulSocketM1_2_128
tlul_socket_m1_2_128_model_DESIGN := TlulSocketM1_2_128
tlul_socket_m1_2_128_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_socket_m1_2_128_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_fifo_async_128_model
tlul_fifo_async_128_model_TOP    := TlulFifoAsync128
tlul_fifo_async_128_model_DESIGN := TlulFifoAsync128
tlul_fifo_async_128_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_fifo_async_128_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_fifo_sync_model
tlul_fifo_sync_model_TOP    := TlulFifoSync
tlul_fifo_sync_model_DESIGN := TlulFifoSync
tlul_fifo_sync_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_fifo_sync_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_local_xbar_model
tlul_local_xbar_model_TOP    := TlulLocalXbar
tlul_local_xbar_model_DESIGN := TlulLocalXbar
tlul_local_xbar_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_local_xbar_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += axi2tlul_model
axi2tlul_model_TOP    := Axi2TLUL
axi2tlul_model_DESIGN := Axi2TLUL
axi2tlul_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
axi2tlul_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += axi_addr_gen_model
axi_addr_gen_model_TOP    := AxiAddressGenerator
axi_addr_gen_model_DESIGN := AxiAddressGenerator
axi_addr_gen_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
axi_addr_gen_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul2axi_model
tlul2axi_model_TOP    := TLUL2Axi
tlul2axi_model_DESIGN := TLUL2Axi
tlul2axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul2axi_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_to_sram_model
tlul_to_sram_model_TOP    := TlulToSram
tlul_to_sram_model_DESIGN := TlulToSram
tlul_to_sram_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_to_sram_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_integrity_testbench_model
tlul_integrity_testbench_model_TOP    := TlulIntegrityTestbench
tlul_integrity_testbench_model_DESIGN := TlulIntegrityTestbench
tlul_integrity_testbench_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_integrity_testbench_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += tlul_socket_1n_128_model
tlul_socket_1n_128_model_TOP    := TlulSocket1N_128
tlul_socket_1n_128_model_DESIGN := TlulSocket1N_128
tlul_socket_1n_128_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
tlul_socket_1n_128_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += secded_encoder_testbench_model
secded_encoder_testbench_model_TOP    := SecdedEncoderTestbench128
secded_encoder_testbench_model_DESIGN := SecdedEncoderTestbench128
secded_encoder_testbench_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
secded_encoder_testbench_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += secded_encoder_testbench_32_model
secded_encoder_testbench_32_model_TOP    := SecdedEncoderTestbench32
secded_encoder_testbench_32_model_DESIGN := SecdedEncoderTestbench32
secded_encoder_testbench_32_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
secded_encoder_testbench_32_model_TRACE  := 1

# hdl/chisel/src/bus
MODELS += secded_encoder_testbench_57_model
secded_encoder_testbench_57_model_TOP    := SecdedEncoderTestbench57
secded_encoder_testbench_57_model_DESIGN := SecdedEncoderTestbench57
secded_encoder_testbench_57_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
secded_encoder_testbench_57_model_TRACE  := 1

# hdl/chisel/src/dma
MODELS += dma_core_model
dma_core_model_TOP    := DmaCore
dma_core_model_DESIGN := DmaCore
dma_core_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
dma_core_model_TRACE  := 1

# hdl/chisel/src/dma
MODELS += transpose_buffer_model
transpose_buffer_model_TOP    := TransposeBuffer
transpose_buffer_model_DESIGN := TransposeBuffer
transpose_buffer_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
transpose_buffer_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_xbar_testharness_model
coralnpu_xbar_testharness_model_TOP    := CoralNPUXbarTestHarness
coralnpu_xbar_testharness_model_DESIGN := CoralNPUXbarTestHarness
coralnpu_xbar_testharness_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_xbar_testharness_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_chisel_subsystem_model
coralnpu_chisel_subsystem_model_TOP    := CoralNPUChiselSubsystem
coralnpu_chisel_subsystem_model_DESIGN := CoralNPUChiselSubsystem
coralnpu_chisel_subsystem_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_chisel_subsystem_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_chisel_subsystem_testharness_model
coralnpu_chisel_subsystem_testharness_model_TOP    := CoralNPUChiselSubsystemTestHarness
coralnpu_chisel_subsystem_testharness_model_DESIGN := CoralNPUChiselSubsystemTestHarness
coralnpu_chisel_subsystem_testharness_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_chisel_subsystem_testharness_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_chisel_subsystem_64_testharness_model
coralnpu_chisel_subsystem_64_testharness_model_TOP    := CoralNPUChiselSubsystem64TestHarness
coralnpu_chisel_subsystem_64_testharness_model_DESIGN := CoralNPUChiselSubsystem64TestHarness
coralnpu_chisel_subsystem_64_testharness_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_chisel_subsystem_64_testharness_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_chisel_subsystem_highmem_testharness_model
coralnpu_chisel_subsystem_highmem_testharness_model_TOP    := CoralNPUChiselSubsystemHighmemTestHarness
coralnpu_chisel_subsystem_highmem_testharness_model_DESIGN := CoralNPUChiselSubsystemHighmemTestHarness
coralnpu_chisel_subsystem_highmem_testharness_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_chisel_subsystem_highmem_testharness_model_TRACE  := 1

# hdl/chisel/src/soc
MODELS += coralnpu_chisel_subsystem_highmem_64_testharness_model
coralnpu_chisel_subsystem_highmem_64_testharness_model_TOP    := CoralNPUChiselSubsystemHighmem64TestHarness
coralnpu_chisel_subsystem_highmem_64_testharness_model_DESIGN := CoralNPUChiselSubsystemHighmem64TestHarness
coralnpu_chisel_subsystem_highmem_64_testharness_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
coralnpu_chisel_subsystem_highmem_64_testharness_model_TRACE  := 1

# tests/cocotb
MODELS += core_mini_axi_model
core_mini_axi_model_TOP    := CoreMiniAxi
core_mini_axi_model_DESIGN := CoreMiniAxi
core_mini_axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
core_mini_axi_model_TRACE  := 0

# tests/cocotb
MODELS += rvv_core_mini_axi_model
rvv_core_mini_axi_model_TOP    := RvvCoreMiniAxi
rvv_core_mini_axi_model_DESIGN := RvvCoreMiniAxi
rvv_core_mini_axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
rvv_core_mini_axi_model_TRACE  := 0
rvv_core_mini_axi_model_VLT    := coralnpu_test_utils/dispatch.vlt.tpl

# tests/cocotb
MODELS += rvv_core_mini_highmem_axi_model
rvv_core_mini_highmem_axi_model_TOP    := RvvCoreMiniHighmemAxi
rvv_core_mini_highmem_axi_model_DESIGN := RvvCoreMiniHighmemAxi
rvv_core_mini_highmem_axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
rvv_core_mini_highmem_axi_model_TRACE  := 0

# tests/cocotb
MODELS += rvv_core_mini_itcm512kb_dtcm512kb_axi_model
rvv_core_mini_itcm512kb_dtcm512kb_axi_model_TOP    := RvvCoreMini_ITCM512KB_DTCM512KBAxi
rvv_core_mini_itcm512kb_dtcm512kb_axi_model_DESIGN := RvvCoreMini_ITCM512KB_DTCM512KBAxi
rvv_core_mini_itcm512kb_dtcm512kb_axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic"
rvv_core_mini_itcm512kb_dtcm512kb_axi_model_TRACE  := 0

# tests/cocotb
MODELS += vme_core_mini_axi_model
vme_core_mini_axi_model_TOP    := VmeCoreMiniAxi
vme_core_mini_axi_model_DESIGN := VmeCoreMiniAxi
vme_core_mini_axi_model_CFLAGS := -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-SPLITVAR -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-PINMISSING -DUSE_GENERIC="" -DTB_SUPPORT -DZVE32F_ON -DVLEN_128 -Ihdl/verilog -LDFLAGS "-rdynamic" -DZVT_ON -DZVTBF16FMM_ON
vme_core_mini_axi_model_TRACE  := 0

# tests/cocotb/tlul
MODELS += coralnpu_xbar_model
coralnpu_xbar_model_TOP    := CoralNPUXbar
coralnpu_xbar_model_DESIGN := CoralNPUXbar
coralnpu_xbar_model_CFLAGS := -Wno-fatal
coralnpu_xbar_model_TRACE  := 1

# tests/cocotb/tlul
MODELS += banked_dtcm_model
banked_dtcm_model_TOP    := BankedDtcm
banked_dtcm_model_DESIGN := BankedDtcm
banked_dtcm_model_CFLAGS := -Wno-fatal
banked_dtcm_model_TRACE  := 1

