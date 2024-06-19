# Microblaze_Data_Transfer_Ethernet

**Aim**: To transfer data from Counter_Stream-IP to DDR using AXI DMA, and then from DDR to Laptop(server) over Ethernet using Microblaze Processor.

**Theory**: In this project, we are transferring counter data (32-bit), from Counter_Stream-IP by creating a “sample_generator.v – Custom AXI Stream IP” which generates the counter data, which is then transmitted to the DDR, by using AXI DMA in the write mode and then finally writing the code in Vitis to program the DMA, Interrupt handler, sample_generator  from the Microblaze Processor to see the counter data on the serial terminal, at the destination DDR’s base address and finally transferring this data from DDR to Laptop(server) over Ethernet using UDP protocol and the resulting received packets can be seen using Wireshark.
