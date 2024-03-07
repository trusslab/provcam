VITIS_INSTALLATION="/tools/Xilinx/Vitis/2022.2"
PROJECT_DIR="../build_petalinux/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/xilinx-vcu-zcu106-v2020.1-final"
XSA_DIR="../build_petalinux/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/xsa_folder"
XSA_NAME=${1:-"bd_wrapper"}

echo "Merging elf into bitstream"
${VITIS_INSTALLATION}/bin/updatemem -bit $XSA_DIR/$XSA_NAME.bit \
        -meminfo $XSA_DIR/$XSA_NAME.mmi \
        -data $PROJECT_DIR/tcm_controller.elf \
        -proc bd_i/microblaze_0 \
        -out $PROJECT_DIR/images/linux/system.bit -force
