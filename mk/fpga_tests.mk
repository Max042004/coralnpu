# FPGA SoC simulation tests (see mk/fpga.mk).

FPGA_SIM_TESTS += fpga_trivial_pass_sim_test
fpga_trivial_pass_sim_test_ELF := fpga/trivial_pass_sim_test_binary
fpga_trivial_pass_sim_test_TIMEOUT := 30
fpga_trivial_pass_sim_test_HIGHMEM := 0
fpga_trivial_pass_sim_test_LOADER := spi
fpga_trivial_pass_sim_test_ITCM := 8
fpga_trivial_pass_sim_test_DTCM := 32

FPGA_SIM_TESTS += fpga_clk_sim_test
fpga_clk_sim_test_ELF := fpga/clk_sim_test_binary
fpga_clk_sim_test_TIMEOUT := 30
fpga_clk_sim_test_HIGHMEM := 0
fpga_clk_sim_test_LOADER := backdoor
fpga_clk_sim_test_ITCM := 8
fpga_clk_sim_test_DTCM := 32

FPGA_SIM_TESTS += fpga_trivial_pass_highmem_sim_test
fpga_trivial_pass_highmem_sim_test_ELF := fpga/trivial_pass_highmem_sim_test_binary
fpga_trivial_pass_highmem_sim_test_TIMEOUT := 30
fpga_trivial_pass_highmem_sim_test_HIGHMEM := 1
fpga_trivial_pass_highmem_sim_test_LOADER := backdoor
fpga_trivial_pass_highmem_sim_test_ITCM := 1024
fpga_trivial_pass_highmem_sim_test_DTCM := 1024

FPGA_SIM_TESTS += fpga_dma_sim_test
fpga_dma_sim_test_ELF := fpga/dma_sim_test_binary
fpga_dma_sim_test_TIMEOUT := 120
fpga_dma_sim_test_HIGHMEM := 0
fpga_dma_sim_test_LOADER := backdoor
fpga_dma_sim_test_ITCM := 8
fpga_dma_sim_test_DTCM := 32

FPGA_SIM_TESTS += fpga_spi_flash_sim_test
fpga_spi_flash_sim_test_ELF := fpga/spi_flash_sim_test_binary
fpga_spi_flash_sim_test_TIMEOUT := 600
fpga_spi_flash_sim_test_HIGHMEM := 0
fpga_spi_flash_sim_test_LOADER := backdoor
fpga_spi_flash_sim_test_ITCM := 8
fpga_spi_flash_sim_test_DTCM := 32

