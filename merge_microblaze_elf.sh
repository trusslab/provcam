VITIS_INSTALLATION="/tools/Xilinx/Vitis/2022.2"
HOME_DIR="/home/myles"
PROJECT_DIR="$HOME_DIR/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/vronicam_plinux"
XSA_DIR="$HOME_DIR/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/xsa_with_direct_csi_uram"
XSA_NAME=${1:-"zcu106_trd_wrapper_with_custom_xbar_4_enc_v16"}

echo "Merging elf into bitstream"
${VITIS_INSTALLATION}/bin/updatemem -bit $XSA_DIR/$XSA_NAME.bit \
        -meminfo $XSA_DIR/$XSA_NAME.mmi \
        -data $PROJECT_DIR/tcm_controller.elf \
        -proc bd_i/microblaze_0 \
        -out $PROJECT_DIR/images/linux/system.bit -force
