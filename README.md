Exercise_1_KundrandaPonnappa\Debug\Exercise0.out
Could Not Find C:\Users\odkha\mct\workspace\Exercise_1_KundrandaPonnappa\Debug\main.obj
Finished clean
 

**** Build Finished ****

**** Build of configuration Debug for project Exercise_1_KundrandaPonnappa ****

"C:\\ti\\ccs1281\\ccs\\utils\\bin\\gmake" -k -j 28 all -O 
 
Building file: "../main.c"
Invoking: MSP430 Compiler
"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmsp --use_hw_mpy=none --include_path="C:/ti/ccs1281/ccs/ccs_base/msp430/include" --include_path="C:/Users/odkha/mct/workspace/Exercise_1_KundrandaPonnappa" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430G2553__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="main.d_raw"  "../main.c"
"..\templateEMP.h", line 123: remark #1528-D: (ULP 3.1) Detected flag polling using IFG2. Recommend using an interrupt combined with enter LPMx and ISR
"..\templateEMP.h", line 127: remark #1528-D: (ULP 3.1) Detected flag polling using IFG2. Recommend using an interrupt combined with enter LPMx and ISR
"..\templateEMP.h", line 141: remark #1530-D: (ULP 5.1) Detected divide operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 144: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 147: remark #1530-D: (ULP 5.1) Detected divide operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 150: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 153: remark #1530-D: (ULP 5.1) Detected divide operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 156: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 159: remark #1530-D: (ULP 5.1) Detected divide operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 162: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 165: remark #1530-D: (ULP 5.1) Detected divide operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 178: remark #2553-D: (ULP 14.1) Array index (involving "i") of type "int". Recommend using "unsigned int"
"..\templateEMP.h", line 182: remark #1544-D: (ULP 13.1) Detected loop counting up. Recommend loops count down as detecting zeros is easier
"..\templateEMP.h", line 186: remark #2553-D: (ULP 14.1) Array index (involving "b") of type "int". Recommend using "unsigned int"
"..\templateEMP.h", line 260: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 281: remark #1527-D: (ULP 2.1) Detected SW delay loop using empty loop. Recommend using a timer module instead
"..\templateEMP.h", line 298: remark #1533-D: (ULP 6.1) Detected use of multiplication on a device that has no hardware multiplier
"..\templateEMP.h", line 281: remark #1527-D: (ULP 2.1) Detected SW delay loop using empty loop. Recommend using a timer module instead
"..\templateEMP.h", line 317: remark #1528-D: (ULP 3.1) Detected flag polling using IFG2. Recommend using an interrupt combined with enter LPMx and ISR
"..\templateEMP.h", line 314: remark #1530-D: (ULP 5.1) Detected modulo operation(s). Recommend moving them to RAM during run time or not using as these are processing/power intensive
"..\templateEMP.h", line 338: remark #1527-D: (ULP 2.1) Detected SW delay loop using empty loop. Recommend using a timer module instead
"../main.c", line 104: remark #1527-D: (ULP 2.1) Detected SW delay loop using __delay_cycles. Recommend using a timer module instead
"../main.c", line 125: remark #1527-D: (ULP 2.1) Detected SW delay loop using __delay_cycles. Recommend using a timer module instead
Finished building: "../main.c"
 
Building target: "Exercise_1_KundrandaPonnappa.out"
Invoking: MSP430 Linker
"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmsp --use_hw_mpy=none --advice:power=all --define=__MSP430G2553__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number -z -m"Exercise_1_KundrandaPonnappa.map" --heap_size=80 --stack_size=80 -i"C:/ti/ccs1281/ccs/ccs_base/msp430/include" -i"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/lib" -i"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="Exercise_1_KundrandaPonnappa_linkInfo.xml" --use_hw_mpy=none --rom_model -o "Exercise_1_KundrandaPonnappa.out" "./main.obj" "../lnk_msp430g2553.cmd"  -llibc.a 
<Linking>
remark #10371-D: (ULP 1.1) Detected no uses of low power mode state changing instructions
remark #10372-D: (ULP 4.1) Detected uninitialized Port 2 in this project. Recommend initializing all unused ports to eliminate wasted current consumption on unused pins.
remark #10372-D: (ULP 4.1) Detected uninitialized Port 3 in this project. Recommend initializing all unused ports to eliminate wasted current consumption on unused pins.
Finished building target: "Exercise_1_KundrandaPonnappa.out"
 

**** Build Finished ****
