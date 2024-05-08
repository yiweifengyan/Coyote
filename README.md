<picture>
  <source media="(prefers-color-scheme: dark)" srcset="img/cyt_logo_dark.png" width = 220>
  <source media="(prefers-color-scheme: light)" srcset="img/cyt_logo_light.png" width = 220>
  <img src="img/cyt_logo_light.png" width = 220>
</picture>

[![Build benchmarks](https://github.com/fpgasystems/Coyote/actions/workflows/build_base.yml/badge.svg?branch=master)](https://github.com/fpgasystems/Coyote/actions/workflows/build_base.yml)
[![Build benchmarks](https://github.com/fpgasystems/Coyote/actions/workflows/build_net.yml/badge.svg?branch=master)](https://github.com/fpgasystems/Coyote/actions/workflows/build_net.yml)
[![Build benchmarks](https://github.com/fpgasystems/Coyote/actions/workflows/build_mem.yml/badge.svg?branch=master)](https://github.com/fpgasystems/Coyote/actions/workflows/build_mem.yml)
[![Build benchmarks](https://github.com/fpgasystems/Coyote/actions/workflows/build_pr.yml/badge.svg?branch=master)](https://github.com/fpgasystems/Coyote/actions/workflows/build_pr.yml)

## _OS for FPGAs_

Framework providing operating system abstractions and a range of shared networking (*RDMA*, *TCP/IP*) and memory services to common modern heterogeneous platforms.

Some of Coyote's features:
 * Multiple isolated virtualized vFPGA regions
 * Dynamic reconfiguration 
 * RTL and HLS user logic coding support
 * Unified host and FPGA memory with striping across virtualized DRAM channels
 * TCP/IP service
 * RDMA service
 * HBM support
 * Runtime scheduler for different host user processes

## Prerequisites

Full `Vivado/Vitis` suite is needed to build the hardware side of things. Hardware server will be enough for deployment only scenarios. Coyote runs with `Vivado 2022.1`. Previous versions can be used at one's own peril.  

Following AMD platforms are supported: `vcu118`, `Alveo u50`, `Alveo u55c`, `Alveo u200`, `Alveo u250` and `Alveo u280`. Coyote is currently being developed on the HACC cluster at ETH Zurich. For more information and possible external access check out the following link: https://systems.ethz.ch/research/data-processing-on-modern-hardware/hacc.html


`CMake` is used for project creation. Additionally `Jinja2` template engine for Python is used for some of the code generation. The API is writen in `C++`, 17 should suffice (for now).

## System `HW`

The following picture shows the high level overview of Coyote's hardware architecture.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="img/cyt_hw_dark.png" width = 500>
  <source media="(prefers-color-scheme: light)" srcset="img/cyt_hw_light.png" width = 500>
  <img src="img/cyt_hw_light.png" width = 500>
</picture>

## System `SW`

Coyote contains the following software layers, each adding higher level of abstraction and parallelisation potential:

1. **cService** - Coyote daemon, targets a single *vFPGA*. Library of loadable functions and scheduler for submitted user tasks.
1. **cProc** - Coyote process, targets a single *vFPGA*. Multiple *cProc* objects can run within a single *vFPGA*.
2. **cThread** - Coyote thread, running on top of *cProc*. Allows the exploration of task level parallelisation.
3. **cTask** - Coyote task, arbitrary user variadic function with arbitrary parameters executed by *cThreads*.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="img/cyt_sw_dark.png" width = 600>
  <source media="(prefers-color-scheme: light)" srcset="img/cyt_sw_light.png" width = 600>
  <img src="img/cyt_sw_light.png" width = 600>
</picture>

# How to run it
Added/Modified by Shien 2024-02-20

## Init
 - Book a server alveo-u55c-03/04/05/… running Vivado 2022.1  
   Booking address https://hacc-booking.inf.ethz.ch/index.php 
 - Login to build servers first. They are faster than those with FPGAs
 - Add bashrc to source **Vivado 2022.1** upon login
 - Clone the mem_bypass branch
~~~~
$ ssh -x shizhu@alveo-u55c-03.inf.ethz.ch

# .bashrc
# Xilinx Tools (XRT, Vivado, Vitis, Vitis_HLS) need to be enabled for proper operation:
source /opt/sgrt/cli/enable/xrt
source /opt/sgrt/cli/enable/vivado
source /opt/sgrt/cli/enable/vitis

$ git clone -b mem_bypass https://github.com/yiweifengyan/Coyote.git
~~~~

## Build `HW`
#### Create a build directory :
~~~~
$ cd hw && mkdir build && cd build
~~~~
#### Enter a valid system configuration :
~~~~
$ cmake .. -DFDEV_NAME=u55c -DEXAMPLE=perf_dlm
zhu@hacc-build-01:~/Coyote/hw/build$ cmake .. -DFDEV_NAME=u55c -DEXAMPLE=perf_dlm

# This specifies these augments and adds the Coyote/hw/hdl/operators/examples/dlm 
$ cmake .. -DFDEV_NAME=u55c -DEN_HLS=0 -DEN_BPSS=1 -DEN_STRM=1 -DEN_MEM_BPSS=1 -DN_CARD_AXI=4 -DHBM_BPSS=1 -DACLK_F=250 -DEN_RDMA_0=1 -DEN_RPC=1

# I have added these augments to Coyote/hw/examples.cmake
elseif(EXAMPLE STREQUAL "perf_dlm")
    message("** dlm: Distributed Lock Management with mem_bypass. Force config.")
    set(EN_HLS 0)
    set(EN_BPSS 1)
    set(EN_STRM 1)
    set(EN_MEM_BPSS 1)
    set(N_CARD_AXI 4)
    set(HBM_BPSS 1)
    # set(EN_UCLK 1)
    # set(UCLK_F 250)
    set(ACLK_F 250)
    # Flags related to RDMA
    set(EN_RDMA_0 1)
    set(EN_RPC 1)

# I have added an entry to the Coyote/hw/scripts/example.tcl.in
"perf_dlm" {
        puts "Add the dlm: Distributed Lock Management"
        add_files "$hw_dir/hdl/operators/examples/dlm/"
        update_compile_order -fileset sources_1
    }
~~~~

Following configuration options are provided:

| Name       | Values                   | Desription                                    |
| ---------- | ------------------------ | --------------------------------------------- |
| FDEV_NAME  | <**u250**, u280, u200, u50, u55c, vcu118> | Supported devices                  |
| EN_HLS     | <**0**,1>                | HLS (*High Level Synthesis*) wrappers         |
| N_REGIONS  | <**1**:16>               | Number of independent regions (vFPGAs)        |
| EN_STRM    | <0, **1**>               | Enable direct host-fpga streaming channels    |
| EN_DDR     | <**0**, 1>               | Enable local FPGA (DRAM) memory stack         |
| EN_HBM     | <**0**, 1>               | Enable local FPGA (HBM) memory stack          |
| EN_PR      | <**0**, 1>               | Enable partial reconfiguration of the regions |
| N_CONFIG   | <**1**:>                 | Number of different configurations for each PR region (can be expanded at any point) |
| N_OUTSTANDING | <**8**:>              | Supported number of outstanding rd/wr request packets |
| N_DDR_CHAN | <0:4>                    | Number of memory channels in striping mode    |
| EN_BPSS    | <0,**1**>                | Bypass descriptors in user logic (transfer init without CPU involvement) |
| EN_AVX     | <0,**1**>                | AVX support                                   |
| EN_TLBF    | <0,**1**>                | Fast TLB mapping via dedicated DMA channels   |
| EN_WB      | <0,**1**>                | Status writeback (polling on host memory)     |
| EN_RDMA_0  | <**0**,1>                | RDMA network stack on *QSFP-0* port           |
| EN_RDMA_1  | <**0**,1>                | RDMA network stack on *QSFP-1* port           |
| EN_TCP_0   | <**0**,1>                | TCP/IP network stack on *QSFP-0* port         |
| EN_TCP_1   | <**0**,1>                | TCP/IP network stack on *QSFP-1* port         |
| EN_RPC     | <**0**,1>                | Enables receive queues for RPC invocations over the network stack |
| EXAMPLE    | <**0**:>                 | Build one of the existing example designs     |
| PMTU_BYTES | <:**4096**:>             | System wide packet size (bytes)               |
| COMP_CORES | <:**4**:>                | Number of compilation cores                   |
| PROBE_ID   | <:**1**:>                | Deployment ID                                 |
| EN_ACLK    | <0,**1**:>               | Separate shell clock (def: 250 MHz)           |
| EN_NCLK    | <0,**1**:>               | Separate network clock (def: 250 MHz)         |
| EN_UCLK    | <0,**1**:>               | Separate user logic clock (def: 300 MHz)      |
| ACLK_F     | <**250**:>               | Shell clock frequency                         |
| NCLK_F     | <**250**:>               | Network clock frequency                       |
| UCLK_F     | <**300**:>               | User logic clock frequency                    |
| TLBS_S     | <**10**:>                | TLB (small) size (2 to the power of)          | 
| TLBS_A     | <**4**:>                 | TLB (small) associativity                     | 
| TLBL_S     | <**9**:>                 | TLB (huge) (2 to the power of)                |
| TLBL_A     | <**2**:>                 | TLB (huge) associativity                      |
| TLBL_BITS  | <**21**:>                | TLB (huge) page order (2 MB def.)             |
| EN_NRU     | <**0**:1>                | NRU policy                                    |

#### Create the shell and the project (10-30 minues) :
~~~~
$ make shell
zhu@hacc-build-01:~/Coyote/hw/build$ make shell 
~~~~

The project is created once the above command completes. Arbitrary user logic can then be inserted. If any of the existing examples are chosen, nothing needs to be done at this step.

User logic wrappers can be found under build project directory in the **hdl/config_X** where **X** represents the chosen PR configuration. Both HLS and HDL wrappers are placed in the same directories.

copy the user defined logic (No need if use the automated user_logic insertion)  
to /home/shizhu/Coyote/hw/build/lynx/hdl/  
-- config_0: Should be the user logic code  
-- wrappers  
|--common  
|--config_0: Should be the user logic wrapper  

~~If multiple PR configurations are present it is advisable to put the most complex configuration in the initial one (**config_0**). Additional configurations can always be created with `make dynamic`. Explicit floorplanning should be done manually after synthesis (providing default floorplanning generally makes little sense).~~

Project can always be managed from Vivado GUI, for those more experienced with FPGA design flows.


#### When the user design is ready, compilation can be started with the following command (1-3 hours):
~~~~
$ make compile
zhu@hacc-build-01:~/Coyote/hw/build$ make compile 
~~~~
Once the compilation finishes the initial bitstream with the static region can be loaded to the FPGA via JTAG. All compiled bitstreams, including partial ones, can be found in the build directory under **bitstreams**.

#### ~~User logic can be simulated by creating the testbench project :~~
~~~~
$ make sim
~~~~
The logic integration, stimulus generation and scoreboard checking should be adapted for each individual DUT.

## Build Driver

#### After the bitstream has been loaded, the driver can be compiled on the target host machine :
Note: Switch to FPGA server to make
~~~~
$ cd driver && make
zhu@alveo-u55c-01:~/Coyote/driver$ make
~~~~

#### ~~Insert the driver into the kernel (don't forget privileges) :~~
~~~~
$ insmod fpga_drv.ko
~~~~
~~Restart of the machine might be necessary after this step if the `util/hot_reset.sh` script is not working (as is usually the case).~~  
I don't and shouldn't restart the server. If there is some errors, call Dario or Max to hot-reset the target FPGA board only.

#### Generate ssh key (one server only once)
```
zhu@hacc-build-01:~$ ssh-keygen
zhu@hacc-build-01:~$ ssh-copy-id zhu@alveo-u55c-03
```

#### Insert the Driver
Note: Switch to build server to insert
```
zhu@hacc-build-01:~$ cd workspace/Coyote/
zhu@hacc-build-01:~/workspace/Coyote$  ./flow_alveo.sh hw/build/bitstreams/cyt_top driver 0
*** Enter server IDs:
3
*** Activating server ...
 **
[1] 17:06:48 [SUCCESS] alveo-u55c-03
*** Enabling Vivado hw_server ...
```

## Build `SW`
Note: Switch to FPGA server to make

What Runbin changed to do dlm test:
+ coyote/sw/include/
  - ibvQpConn.hpp
  - ibvQpMap.hpp
  - sLock.hpp
+ coyote/sw/src/
  - ibvQpConn.cpp
  - ibvQpMap.cpp

Note: /usr/bin/cmake ../ -DTARGET_DIR=examples/perf_dlm  
Available `sw` projects (as well as any other) can be built with the following commands :
~~~~
$ cd sw && mkdir build && cd build
$ cmake ../ -DTARGET_DIR=<example_path>
zhu@alveo-u55c-03:~/workspace/Coyote/sw/build$ /usr/bin/cmake ../ -DTARGET_DIR=examples/perf_rdma
-- Configuring done
-- Generating done
-- Build files have been written to: /home/shizhu/workspace/Coyote/sw/build

$ make
zhu@alveo-u55c-03:~/workspace/Coyote/sw/build$ make
Scanning dependencies of target main
[ 10%] Building CXX object CMakeFiles/main.dir/examples/perf_rdma/main.cpp.o
[ 20%] Building CXX object CMakeFiles/main.dir/src/cArbiter.cpp.o
[ 30%] Building CXX object CMakeFiles/main.dir/src/cProcess.cpp.o
[ 40%] Building CXX object CMakeFiles/main.dir/src/cSched.cpp.o
[ 50%] Building CXX object CMakeFiles/main.dir/src/cService.cpp.o
[ 60%] Building CXX object CMakeFiles/main.dir/src/cThread.cpp.o
[ 70%] Building CXX object CMakeFiles/main.dir/src/ibvQpConn.cpp.o
[ 80%] Building CXX object CMakeFiles/main.dir/src/ibvQpMap.cpp.o
[ 90%] Building CXX object CMakeFiles/main.dir/src/ibvStructs.cpp.o
[100%] Linking CXX executable main
[100%] Built target main

# run main
zhu@alveo-u55c-03:~/workspace/Coyote/sw/build$ ls
CMakeCache.txt  CMakeFiles  Makefile  cmake_install.cmake  main
zhu@alveo-u55c-03:~/workspace/Coyote/sw/build$ ./main
-- PARAMS
-----------------------------------------------
IBV IP address: 10.253.74.76
Number of allocated pages: 1
Read operation
Min size: 128
Max size: 32768
Number of throughput reps: 1000
Number of latency reps: 100
~~~~

# Other things

## Publication

#### If you use Coyote, cite us :

```bibtex
@inproceedings{coyote,
    author = {Dario Korolija and Timothy Roscoe and Gustavo Alonso},
    title = {Do {OS} abstractions make sense on FPGAs?},
    booktitle = {14th {USENIX} Symposium on Operating Systems Design and Implementation ({OSDI} 20)},
    year = {2020},
    pages = {991--1010},
    url = {https://www.usenix.org/conference/osdi20/presentation/roscoe},
    publisher = {{USENIX} Association}
}
```

## Repository structure

~~~
├── driver
│   └── eci
│   └── pci
├── hw
│   └── ext/network
│   └── ext/eci
│   └── hdl
│   └── ip
│   └── scripts
│   └── sim
├── sw
│   └── examples
│   └── include
│   └── src
├── util
├── img
~~~

## Additional requirements

If networking services are used, to generate the design you will need a valid [UltraScale+ Integrated 100G Ethernet Subsystem](https://www.xilinx.com/products/intellectual-property/cmac_usplus.html) license set up in `Vivado`/`Vitis`.
