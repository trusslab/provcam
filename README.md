# ProvCam: A Camera Module with Self-Contained TCB for Producing Verifiable Videos

:paperclip: [ProvCam Paper](https://doi.org/10.1145/3636534.3649383) 

:computer: [ProvCam Main Repository](https://github.com/trusslab/provcam)
This repo hosts the documentation of ProvCam and other misc content. 

:computer: [ProvCam Hardware Repository](https://github.com/trusslab/provcam_hw)
This repo hosts ProvCam's hardware system design.

:computer: [ProvCam Firmware Repository](https://github.com/trusslab/provcam_ctrl)
This repo hosts firmware running the microcontroller of ProvCam trusted camera module.

:computer: [ProvCam OS Repository](https://github.com/trusslab/provcam_linux)
This repo hosts OS(a custom version of Petalinux) running on ProvCam's system. 
Notice that the OS represents the main camera OS, which is untrusted in ProvCam. 

:computer: [ProvCam Software Repository](https://github.com/trusslab/provcam_libs/tree/main)
This repo hosts some software and libraries running in the OS.

Authors: Yuxin (Myles) Liu, Zhihao Yao, Mingyi Chen, Ardalan Amiri Sani, Sharad Agarwal, Gene Tsudik

The work of UCI authors was supported in part by the NSF Awards #1763172, #1953932, #1956393, and #2247880 as well as NSA Awards #H98230-20-1-0345 and #H98230-22-1-0308.

We provide a step-by-step guide to recreate ProvCam's hardware and software prototype mentioned in our paper. 

---

## Table of Contents

- [ProvCam](#provcam-a-camera-module-with-self-contained-tcb-for-producing-verifiable-videos)
    - [Table of Contents](#table-of-contents)
    - [System Requirements](#system-requirements)
    - [Hardware Design](#hadware-design)
    - 

## System Requirements

# Hardware
We use [Xilinx Zynq UltraScale+ MPSoC ZCU106 Evaluation Kit](https://www.xilinx.com/products/boards-and-kits/zcu106.html) and [LI-IMX274MIPI-FMC Camera](https://leopardimaging.com/product/platform-partners/amd/li-imx274mipi-fmc/) in our ProvCam's prototype.
You also need a compatiable SD card (We use a 32 GB SD card), and a USB drive (optional) to copy the captured video without turning the board off. 

# Xilinx Vivado and Vitis
We use Xilinx Vivado 2023.2.1 and Xilinx Vitis 2023.2.1 (https://www.xilinx.com/support/download.html) for designing both hardware and firmware.
Please follow this [official guide](https://www.xilinx.com/support/download.html) provided by Xilinx to install both Vivado and Vitis. 
(You may skip (part or all) Xilix Design Suite installation if you only want to try ProvCam's hardware prototype, as we provide both pre-compiled hardware and firmware)

We use an Intel Xeon E5-2697 CPU with 72 threads with 192 GB memory running Ubuntu 22.04 on a SATA SSD to prepare the hardware and firmware. 
The total machine time is about 3 hours, and it might take around 5-8 hours of manual work time depending on your familiarity with the tools. 

We will call this MACHINE_0 from now on, which runs both Vivado and Vitis.

# Xilinx Petalinux
We use a [custom version of Xilinx Petalinux 2020.1](https://github.com/trusslab/provcam_linux) to reprsents the OS. 
To compile it, the Xilinx's official Petalinux 2020.1 toolset (https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools/archive.html) has to be first installed. 
Please follow this [official guide](https://docs.xilinx.com/v/u/2020.1-English/ug1144-petalinux-tools-reference-guide) provided by Xilinx to install the Petalinux toolset. 
(Again, you may skip the Petalinux toolset installtion if you only want to try ProvCam's hardware prototype, as we provide pre-compiled images)

We use an Intel Core I7-5775C CPU with 8 threads with 16 GB memory running Ubuntu 18.04 on a SATA SSD to prepare the OS.
The total machine time is about 3 hours, and it mgiht take around 3-6 hours of manual work time depending on your familiarity with the tools.

We will call this MACHINE_1 from now on, which is for compiling Petalinux. 
Note that this MACHINE_1 may potentially be the same physical machine as of MACHINE_0, but we recommend the use of two different physical machines for preventing any library conflictions.

# Misc.
You might need a Windows machine to adjust some board's settings (as it appears that the corresponding Xilinx tool is only available in Windows).

## Hadware Design

# Preparing the hardware
1. Connect both JTAG and UART of the board to MACHINE_0.
2. Set the board's SW6 switches to the same configuration as shown in the figure below, which is for telling the board to boot from SD card.
![ZCU106 SW6 Switches](docs/img/sw6_switches.png)
3. Connect the LI-IMX274MIPI-FMC image sensor to the FMC0 connector and set the FMC0's VADJ to 1.2V (please follow [the official guide](https://support.xilinx.com/s/article/67308?language=en_US) to adjust the voltage).
4. 

## Firmware

## OS

## Debug & Run

## References

https://docs.xilinx.com/v/u/en-US/dh0013-vivado-installation-and-licensing-hub
https://docs.xilinx.com/r/en-US/ug1144-petalinux-tools-reference-guide/Introduction
https://www.xilinx.com/products/boards-and-kits/zcu106.html
https://leopardimaging.com/product/platform-partners/amd/li-imx274mipi-fmc/
https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/460948332/Zynq+UltraScale+MPSoC+VCU+TRD+2020.1
