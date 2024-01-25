This code base implements a user space linux application driver that enables the read and write of video frames to the FPGA fabric of a Xilinx 
Zynq FPGA SoC.  The device tree must be updated to map the vdma driver to the correct UIO address of the FPGA VDMA core.  Instructions for updating
the device tree can be found in the Update Device Tree module on the Mathworks File Exchange.
