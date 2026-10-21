# Table of Chisel designs; edit by hand.
# <design>_EMIT  : main class of the generator (sbt runMain)
# <design>_FLAGS : arguments passed to the generator
# <design>_VOPTS : extra Verilator options for simulation builds
# <design>_OUTS  : extra files the generator writes next to <design>.sv

# hdl/chisel/src/bus
DESIGNS += Axi2TLUL
Axi2TLUL_EMIT  := bus.EmitAxi2TLUL
Axi2TLUL_SV    := Axi2TLUL.sv
Axi2TLUL_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += AxiRead2TLUL
AxiRead2TLUL_EMIT  := bus.EmitAxiRead2TLUL
AxiRead2TLUL_SV    := AxiRead2TLUL.sv
AxiRead2TLUL_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += AxiWrite2TLUL
AxiWrite2TLUL_EMIT  := bus.EmitAxiWrite2TLUL
AxiWrite2TLUL_SV    := AxiWrite2TLUL.sv
AxiWrite2TLUL_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += AxiAddressGenerator
AxiAddressGenerator_EMIT  := bus.EmitAxiAddressGenerator
AxiAddressGenerator_SV    := AxiAddressGenerator.sv
AxiAddressGenerator_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TLUL2Axi
TLUL2Axi_EMIT  := bus.EmitTLUL2Axi
TLUL2Axi_SV    := TLUL2Axi.sv
TLUL2Axi_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulFifoAsync128
TlulFifoAsync128_EMIT  := bus.TlulFifoAsync128Emitter
TlulFifoAsync128_SV    := TlulFifoAsync128.sv
TlulFifoAsync128_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulSocket1N_128
TlulSocket1N_128_EMIT  := bus.TlulSocket1N_128Emitter
TlulSocket1N_128_SV    := TlulSocket1N_128.sv
TlulSocket1N_128_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulFifoSync
TlulFifoSync_EMIT  := bus.TlulFifoSyncEmitter
TlulFifoSync_SV    := TlulFifoSync.sv
TlulFifoSync_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulSocketM1_2_128
TlulSocketM1_2_128_EMIT  := bus.TlulSocketM1_2_128Emitter
TlulSocketM1_2_128_SV    := TlulSocketM1_2_128.sv
TlulSocketM1_2_128_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulSocketM1_3_128
TlulSocketM1_3_128_EMIT  := bus.TlulSocketM1_3_128Emitter
TlulSocketM1_3_128_SV    := TlulSocketM1_3_128.sv
TlulSocketM1_3_128_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulToSram
TlulToSram_EMIT  := bus.TlulToSramEmitter
TlulToSram_SV    := TlulToSram.sv
TlulToSram_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulLocalXbar
TlulLocalXbar_EMIT  := bus.TlulLocalXbarEmitter
TlulLocalXbar_SV    := TlulLocalXbar.sv
TlulLocalXbar_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += TlulIntegrityTestbench
TlulIntegrityTestbench_EMIT  := bus.EmitTlulIntegrityTestbench
TlulIntegrityTestbench_SV    := TlulIntegrityTestbench.sv
TlulIntegrityTestbench_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += SecdedEncoderTestbench128
SecdedEncoderTestbench128_EMIT  := bus.EmitSecdedEncoderTestbench
SecdedEncoderTestbench128_SV    := SecdedEncoderTestbench128.sv
SecdedEncoderTestbench128_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += SecdedEncoderTestbench32
SecdedEncoderTestbench32_EMIT  := bus.EmitSecdedEncoderTestbench32
SecdedEncoderTestbench32_SV    := SecdedEncoderTestbench32.sv
SecdedEncoderTestbench32_FLAGS := 

# hdl/chisel/src/bus
DESIGNS += SecdedEncoderTestbench57
SecdedEncoderTestbench57_EMIT  := bus.EmitSecdedEncoderTestbench57
SecdedEncoderTestbench57_SV    := SecdedEncoderTestbench57.sv
SecdedEncoderTestbench57_FLAGS := 

# hdl/chisel/src/coralnpu
DESIGNS += Alu
Alu_EMIT  := coralnpu.EmitAlu
Alu_SV    := Alu.sv
Alu_FLAGS := 

# hdl/chisel/src/coralnpu
DESIGNS += CoreMini
CoreMini_EMIT  := coralnpu.EmitCore
CoreMini_SV    := CoreMini.sv
CoreMini_FLAGS := --fetchDataBits=128 --lsuDataBits=128 --moduleName=CoreMini
CoreMini_OUTS  := VCoreMini_parameters.h

# hdl/chisel/src/coralnpu
DESIGNS += CoreMiniAxi
CoreMiniAxi_EMIT  := coralnpu.EmitCore
CoreMiniAxi_SV    := CoreMiniAxi.sv
CoreMiniAxi_FLAGS := --fetchDataBits=128 --lsuDataBits=128 --enableFloat=True --enableZfbfmin=True --moduleName=CoreMini --useAxi --exposeDebugPorts=True
CoreMiniAxi_VOPTS := -DUSE_GENERIC -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-BLKANDNBLK -Wno-BLKSEQ
CoreMiniAxi_OUTS  := VCoreMiniAxi_parameters.h CoreMiniAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += CoreMiniVerificationAxi
CoreMiniVerificationAxi_EMIT  := coralnpu.EmitCore
CoreMiniVerificationAxi_SV    := CoreMiniVerificationAxi.sv
CoreMiniVerificationAxi_FLAGS := --fetchDataBits=128 --lsuDataBits=128 --enableFloat=True --enableZfbfmin=True --moduleName=CoreMiniVerification --useAxi --enableVerification=True
CoreMiniVerificationAxi_VOPTS := -DUSE_GENERIC -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-BLKANDNBLK -Wno-BLKSEQ
CoreMiniVerificationAxi_OUTS  := VCoreMiniVerificationAxi_parameters.h CoreMiniVerificationAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += CoreMiniHighmemAxi
CoreMiniHighmemAxi_EMIT  := coralnpu.EmitCore
CoreMiniHighmemAxi_SV    := CoreMiniHighmemAxi.sv
CoreMiniHighmemAxi_FLAGS := --fetchDataBits=128 --lsuDataBits=128 --enableFloat=True --enableZfbfmin=True --moduleName=CoreMini --useAxi --itcmSizeKBytes=1024 --dtcmSizeKBytes=1024
CoreMiniHighmemAxi_VOPTS := -DUSE_GENERIC -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-BLKANDNBLK -Wno-BLKSEQ
CoreMiniHighmemAxi_OUTS  := VCoreMiniHighmemAxi_parameters.h CoreMiniHighmemAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += CoreMini_ITCM512KB_DTCM512KBAxi
CoreMini_ITCM512KB_DTCM512KBAxi_EMIT  := coralnpu.EmitCore
CoreMini_ITCM512KB_DTCM512KBAxi_SV    := CoreMini_ITCM512KB_DTCM512KBAxi.sv
CoreMini_ITCM512KB_DTCM512KBAxi_FLAGS := --fetchDataBits=128 --lsuDataBits=128 --enableFloat=True --enableZfbfmin=True --moduleName=CoreMini --useAxi --itcmSizeKBytes=512 --dtcmSizeKBytes=512
CoreMini_ITCM512KB_DTCM512KBAxi_VOPTS := -DUSE_GENERIC -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-BLKANDNBLK -Wno-BLKSEQ
CoreMini_ITCM512KB_DTCM512KBAxi_OUTS  := VCoreMini_ITCM512KB_DTCM512KBAxi_parameters.h CoreMini_ITCM512KB_DTCM512KBAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += RvvCoreMiniTlul
RvvCoreMiniTlul_EMIT  := coralnpu.EmitCore
RvvCoreMiniTlul_SV    := RvvCoreMiniTlul.sv
RvvCoreMiniTlul_FLAGS := --moduleName=RvvCoreMini --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --useTlul=True
RvvCoreMiniTlul_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc
RvvCoreMiniTlul_OUTS  := VRvvCoreMiniTlul_parameters.h RvvCoreMiniTlul.zip

# hdl/chisel/src/coralnpu
DESIGNS += RvvCoreMiniAxi
RvvCoreMiniAxi_EMIT  := coralnpu.EmitCore
RvvCoreMiniAxi_SV    := RvvCoreMiniAxi.sv
RvvCoreMiniAxi_FLAGS := --moduleName=RvvCoreMini --itcmSizeKBytes=8 --dtcmSizeKBytes=32 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi
RvvCoreMiniAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc
RvvCoreMiniAxi_OUTS  := VRvvCoreMiniAxi_parameters.h RvvCoreMiniAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += RvvCoreMiniVerificationAxi
RvvCoreMiniVerificationAxi_EMIT  := coralnpu.EmitCore
RvvCoreMiniVerificationAxi_SV    := RvvCoreMiniVerificationAxi.sv
RvvCoreMiniVerificationAxi_FLAGS := --moduleName=RvvCoreMiniVerification --enableVerification=True --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi
RvvCoreMiniVerificationAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc
RvvCoreMiniVerificationAxi_OUTS  := VRvvCoreMiniVerificationAxi_parameters.h RvvCoreMiniVerificationAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += RvvCoreMiniHighmemAxi
RvvCoreMiniHighmemAxi_EMIT  := coralnpu.EmitCore
RvvCoreMiniHighmemAxi_SV    := RvvCoreMiniHighmemAxi.sv
RvvCoreMiniHighmemAxi_FLAGS := --moduleName=RvvCoreMini --itcmSizeKBytes=1024 --dtcmSizeKBytes=1024 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi
RvvCoreMiniHighmemAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc
RvvCoreMiniHighmemAxi_OUTS  := VRvvCoreMiniHighmemAxi_parameters.h RvvCoreMiniHighmemAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += RvvCoreMini_ITCM512KB_DTCM512KBAxi
RvvCoreMini_ITCM512KB_DTCM512KBAxi_EMIT  := coralnpu.EmitCore
RvvCoreMini_ITCM512KB_DTCM512KBAxi_SV    := RvvCoreMini_ITCM512KB_DTCM512KBAxi.sv
RvvCoreMini_ITCM512KB_DTCM512KBAxi_FLAGS := --moduleName=RvvCoreMini --itcmSizeKBytes=512 --dtcmSizeKBytes=512 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi
RvvCoreMini_ITCM512KB_DTCM512KBAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc
RvvCoreMini_ITCM512KB_DTCM512KBAxi_OUTS  := VRvvCoreMini_ITCM512KB_DTCM512KBAxi_parameters.h RvvCoreMini_ITCM512KB_DTCM512KBAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += VmeCoreMiniAxi
VmeCoreMiniAxi_EMIT  := coralnpu.EmitCore
VmeCoreMiniAxi_SV    := VmeCoreMiniAxi.sv
VmeCoreMiniAxi_FLAGS := --moduleName=VmeCoreMini --itcmSizeKBytes=8 --dtcmSizeKBytes=32 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi --enableVme=True
VmeCoreMiniAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON
VmeCoreMiniAxi_OUTS  := VVmeCoreMiniAxi_parameters.h VmeCoreMiniAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += VmeCoreMiniVerificationAxi
VmeCoreMiniVerificationAxi_EMIT  := coralnpu.EmitCore
VmeCoreMiniVerificationAxi_SV    := VmeCoreMiniVerificationAxi.sv
VmeCoreMiniVerificationAxi_FLAGS := --moduleName=VmeCoreMiniVerification --enableVerification=True --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi --enableVme=True
VmeCoreMiniVerificationAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON
VmeCoreMiniVerificationAxi_OUTS  := VVmeCoreMiniVerificationAxi_parameters.h VmeCoreMiniVerificationAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += VmeCoreMiniHighmemAxi
VmeCoreMiniHighmemAxi_EMIT  := coralnpu.EmitCore
VmeCoreMiniHighmemAxi_SV    := VmeCoreMiniHighmemAxi.sv
VmeCoreMiniHighmemAxi_FLAGS := --moduleName=VmeCoreMini --itcmSizeKBytes=1024 --dtcmSizeKBytes=1024 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi --enableVme=True
VmeCoreMiniHighmemAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON
VmeCoreMiniHighmemAxi_OUTS  := VVmeCoreMiniHighmemAxi_parameters.h VmeCoreMiniHighmemAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += VmeCoreMiniVerificationHighmemAxi
VmeCoreMiniVerificationHighmemAxi_EMIT  := coralnpu.EmitCore
VmeCoreMiniVerificationHighmemAxi_SV    := VmeCoreMiniVerificationHighmemAxi.sv
VmeCoreMiniVerificationHighmemAxi_FLAGS := --moduleName=VmeCoreMiniVerification --enableVerification=True --itcmSizeKBytes=1024 --dtcmSizeKBytes=1024 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi --enableVme=True
VmeCoreMiniVerificationHighmemAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON
VmeCoreMiniVerificationHighmemAxi_OUTS  := VVmeCoreMiniVerificationHighmemAxi_parameters.h VmeCoreMiniVerificationHighmemAxi.zip

# hdl/chisel/src/coralnpu
DESIGNS += DBus2AxiV2
DBus2AxiV2_EMIT  := coralnpu.EmitDBus2Axi
DBus2AxiV2_SV    := DBus2AxiV2.sv
DBus2AxiV2_FLAGS := 

# hdl/chisel/src/coralnpu
DESIGNS += BankedDtcm
BankedDtcm_EMIT  := coralnpu.BankedDtcmEmitter
BankedDtcm_SV    := BankedDtcm.sv
BankedDtcm_FLAGS := --lsuDataBits=128

# hdl/chisel/src/coralnpu
DESIGNS += BankedItcm
BankedItcm_EMIT  := coralnpu.BankedItcmEmitter
BankedItcm_SV    := BankedItcm.sv
BankedItcm_FLAGS := --lsuDataBits=128

# hdl/chisel/src/coralnpu
DESIGNS += Dvu
Dvu_EMIT  := coralnpu.EmitDvu
Dvu_SV    := Dvu.sv
Dvu_FLAGS := 

# hdl/chisel/src/coralnpu
DESIGNS += Mlu
Mlu_EMIT  := coralnpu.EmitMlu
Mlu_SV    := Mlu.sv
Mlu_FLAGS := 

# hdl/chisel/src/coralnpu/prod
DESIGNS += VmeCoreMini_ITCM8KB_DTCM128KBAxi
VmeCoreMini_ITCM8KB_DTCM128KBAxi_EMIT  := coralnpu.EmitCore
VmeCoreMini_ITCM8KB_DTCM128KBAxi_SV    := VmeCoreMini_ITCM8KB_DTCM128KBAxi.sv
VmeCoreMini_ITCM8KB_DTCM128KBAxi_FLAGS := --moduleName=VmeCoreMini --itcmSizeKBytes=8 --dtcmSizeKBytes=128 --fetchDataBits=128 --lsuDataBits=128 --enableRvv=True --enableFloat=True --enableZfbfmin=True --enableVectorBf16=True --useAxi --enableVme=True
VmeCoreMini_ITCM8KB_DTCM128KBAxi_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON
VmeCoreMini_ITCM8KB_DTCM128KBAxi_OUTS  := VVmeCoreMini_ITCM8KB_DTCM128KBAxi_parameters.h VmeCoreMini_ITCM8KB_DTCM128KBAxi.zip

# hdl/chisel/src/dma
DESIGNS += DmaCore
DmaCore_EMIT  := dma.DmaCoreEmitter
DmaCore_SV    := DmaCore.sv
DmaCore_FLAGS := 

# hdl/chisel/src/dma
DESIGNS += DmaCsr
DmaCsr_EMIT  := dma.DmaCsrEmitter
DmaCsr_SV    := DmaCsr.sv
DmaCsr_FLAGS := 

# hdl/chisel/src/dma
DESIGNS += TransposeBuffer
TransposeBuffer_EMIT  := dma.TransposeBufferEmitter
TransposeBuffer_SV    := TransposeBuffer.sv
TransposeBuffer_FLAGS := 

# hdl/chisel/src/soc
DESIGNS += CoralNPUXbar
CoralNPUXbar_EMIT  := coralnpu.soc.CoralNPUXbarEmitter
CoralNPUXbar_SV    := CoralNPUXbar.sv
CoralNPUXbar_FLAGS := 

# hdl/chisel/src/soc
DESIGNS += CoralNPUXbarTestHarness
CoralNPUXbarTestHarness_EMIT  := coralnpu.soc.CoralNPUXbarEmitter
CoralNPUXbarTestHarness_SV    := CoralNPUXbarTestHarness.sv
CoralNPUXbarTestHarness_FLAGS := --enableTestHarness

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystem
CoralNPUChiselSubsystem_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystem_SV    := CoralNPUChiselSubsystem.sv
CoralNPUChiselSubsystem_FLAGS := 
CoralNPUChiselSubsystem_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystem64
CoralNPUChiselSubsystem64_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystem64_SV    := CoralNPUChiselSubsystem64.sv
CoralNPUChiselSubsystem64_FLAGS := --xlen 64 --moduleName CoralNPUChiselSubsystem64
CoralNPUChiselSubsystem64_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemTestHarness
CoralNPUChiselSubsystemTestHarness_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystemTestHarness_SV    := CoralNPUChiselSubsystemTestHarness.sv
CoralNPUChiselSubsystemTestHarness_FLAGS := --enableTestHarness
CoralNPUChiselSubsystemTestHarness_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystem64TestHarness
CoralNPUChiselSubsystem64TestHarness_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystem64TestHarness_SV    := CoralNPUChiselSubsystem64TestHarness.sv
CoralNPUChiselSubsystem64TestHarness_FLAGS := --enableTestHarness --xlen 64 --moduleName CoralNPUChiselSubsystem64
CoralNPUChiselSubsystem64TestHarness_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemHighmem
CoralNPUChiselSubsystemHighmem_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystemHighmem_SV    := CoralNPUChiselSubsystemHighmem.sv
CoralNPUChiselSubsystemHighmem_FLAGS := --itcmSizeKBytes 1024 --dtcmSizeKBytes 1024
CoralNPUChiselSubsystemHighmem_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemMatrix
CoralNPUChiselSubsystemMatrix_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemMatrixEmitter
CoralNPUChiselSubsystemMatrix_SV    := CoralNPUChiselSubsystemMatrix.sv
CoralNPUChiselSubsystemMatrix_FLAGS := 
CoralNPUChiselSubsystemMatrix_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemMatrixHighmem
CoralNPUChiselSubsystemMatrixHighmem_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemMatrixEmitter
CoralNPUChiselSubsystemMatrixHighmem_SV    := CoralNPUChiselSubsystemMatrixHighmem.sv
CoralNPUChiselSubsystemMatrixHighmem_FLAGS := --itcmSizeKBytes 1024 --dtcmSizeKBytes 1024
CoralNPUChiselSubsystemMatrixHighmem_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc -DZVT_ON -DZVTBF16FMM_ON

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB
CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB_SV    := CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB.sv
CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB_FLAGS := --itcmSizeKBytes 512 --dtcmSizeKBytes 512
CoralNPUChiselSubsystem_ITCM512KB_DTCM512KB_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemHighmemTestHarness
CoralNPUChiselSubsystemHighmemTestHarness_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystemHighmemTestHarness_SV    := CoralNPUChiselSubsystemHighmemTestHarness.sv
CoralNPUChiselSubsystemHighmemTestHarness_FLAGS := --enableTestHarness --itcmSizeKBytes 1024 --dtcmSizeKBytes 1024
CoralNPUChiselSubsystemHighmemTestHarness_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemHighmem64
CoralNPUChiselSubsystemHighmem64_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystemHighmem64_SV    := CoralNPUChiselSubsystemHighmem64.sv
CoralNPUChiselSubsystemHighmem64_FLAGS := --itcmSizeKBytes 1024 --dtcmSizeKBytes 1024 --xlen 64 --moduleName CoralNPUChiselSubsystemHighmem64
CoralNPUChiselSubsystemHighmem64_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

# hdl/chisel/src/soc
DESIGNS += CoralNPUChiselSubsystemHighmem64TestHarness
CoralNPUChiselSubsystemHighmem64TestHarness_EMIT  := coralnpu.soc.CoralNPUChiselSubsystemEmitter
CoralNPUChiselSubsystemHighmem64TestHarness_SV    := CoralNPUChiselSubsystemHighmem64TestHarness.sv
CoralNPUChiselSubsystemHighmem64TestHarness_FLAGS := --enableTestHarness --itcmSizeKBytes 1024 --dtcmSizeKBytes 1024 --xlen 64 --moduleName CoralNPUChiselSubsystemHighmem64
CoralNPUChiselSubsystemHighmem64TestHarness_VOPTS := -DUSE_GENERIC -DTB_SUPPORT -DVLEN_128 -DZVE32F_ON -Wno-WIDTH -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-SIDEEFFECT -Wno-MULTIDRIVEN -Wno-BLKANDNBLK -Wno-CASEX -Wno-ASCRANGE -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-UNSIGNED -Wno-WIDTHCONCAT -Ihdl/verilog/rvv/design/FPnew/common_cells/inc

