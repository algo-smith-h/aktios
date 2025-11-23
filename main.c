#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>  
#include "pci.h"
#include "sys/io.h"

bool is_bridge(unsigned main_address){
    outl(main_address + 0x0C, 0xCF8);
    return (inl(0xCFC) >> 16) != 0;
}

void device_info(unsigned reg_out){
    unsigned short vendor_name = reg_out;
    unsigned short device_name = reg_out >> 16;
    bool vendor_found = false;
    bool device_found = false;

    for(int i = 0; i < PCI_VENTABLE_LEN; i++){
        if((vendor_found = (vendor_name == PciVenTable[i].VendorId))){
            printf("Vendor's 16-bit code: %04X\n", vendor_name);
            printf("Vendor: %s\n", PciVenTable[i].VendorName); 
            break;            
        }
    }

    if(vendor_found){
        for(int i = 0; i < PCI_DEVTABLE_LEN; i++){
            if((device_found = (device_name == PciDevTable[i].DeviceId))){
                printf("Device's 16-bit code: %04X\n", device_name);
                printf("Device: %s\n", PciDevTable[i].DeviceName); 
                break;   
            }
        }
        if(!device_found){
            printf("Device not found.\n");
        }
    }
    else{
        printf("Vendor not found.\n");
    }
}

void decode_bars(unsigned main_address){
    if(is_bridge(main_address)){
        return;
    }
    
    printf("----- Base Address Registers (BARs) Analysis -----\n");
    for(int i = 0; i < 6; i++){
        outl(main_address + 0x10 + i * 4, 0xCF8);
        unsigned bar_value = inl(0xCFC);
        bool is_memory = (bar_value & 1) == 0;
        bool is_prefetchable = (bar_value & 8);
        printf("BAR %d, %s: ", i+1, is_memory ? "Memory" : "I/O");
        switch(bar_value & 6){
            case 0:
                printf("Any 32 bit address space\n");
                break;
            case 2:
                printf("Below 1 MB\n");
                break;
            case 4:
                printf("Any 64 bit address space\n");
                break;
            default:
                printf("Reserved\n");
                break;
        }
        printf("Prefetchable: %s\n", is_prefetchable ? "True" : "False");
        printf("Base address: %08X\n", bar_value >> 4);
    }
}

void decode_IO_base_limits(unsigned main_address){
    if(!is_bridge(main_address)){
        return;
    }

    printf("----- I/O Base and Limits Registers Analysis -----\n");

    outl(main_address + 0x1C, 0xCF8);
    unsigned io_reg_low = inl(0xCFC);

    unsigned io_base_low = io_reg_low & 0xFF;
    unsigned io_limit_low = (io_reg_low >> 8) & 0xFF;

    outl(main_address + 0x30, 0xCF8);
    unsigned io_reg_high = inl(0xCFC);

    unsigned io_base_high = io_reg_high & 0xFFFF;
    unsigned io_limit_high = (io_reg_high >> 16) & 0xFFFF;
    
    unsigned io_base = (io_base_high << 16) | (io_base_low << 8);
    unsigned io_limit = (io_limit_high << 16) | (io_limit_low << 8) | 0xFFF;

    printf("I/O Base-Low: 0x%08X\n", io_base_low);
    printf("I/O Limit-Low: 0x%08X\n", io_limit_low);
    printf("I/O Base: 0x%08X\n", io_base);
    printf("I/O Limit: 0x%08X\n", io_limit);
    printf("Range: 0x%08X - 0x%08X\n", io_base, io_limit); 
}

void decode_interrupt(unsigned main_address){
    if(!is_bridge(main_address)){
        return;
    }

    printf("----- Interrupt -----\n"); 

    outl(main_address + 0x3C, 0xCF8);
    unsigned inter = inl(0xCFC); 
    printf("Interrupt line: %02X\n", (unsigned char)inter);
    printf("Interrupt pin: %02X\n", (inter >> 8));
}

void print_info(int bus, int dev, int func){
    unsigned main_address = (1 << 31) | (bus << 16) | (dev << 11) | (func << 8); 
    outl(main_address, 0xCF8);
    unsigned reg_out = inl(0xCFC);

    if ((reg_out & 0xFFFF) == 0xFFFF){
        return;
    }

    printf("\n==========================================\n");
    printf("PCI Device at Bus: %02X, Device: %02X, Function: %02X\n",
           bus, dev, func);
    printf("Configuration Address: 0x%08X\n", main_address);

    device_info(reg_out);
    printf("Bridge: %s\n", is_bridge(main_address) ? "True" : "False"); 
    decode_bars(main_address);
    decode_IO_base_limits(main_address);
    decode_interrupt(main_address);
}

int scanning_devices(){
    if(iopl(3) == -1){
        printf("Error: Insufficient rights to execute the program.\n");
        return -1;
    }

    printf("Scanning all PCI devices:\n");

    for(int bus_num = 0; bus_num < 256; bus_num++){
        for(int dev_num = 0; dev_num < 32; dev_num++){
            for(int func_num = 0; func_num < 8; func_num++){
                print_info(bus_num, dev_num, func_num);
            }
        }
    }

    printf("PCI scanning complete\n"); 

    return 0;
}

int main(){
    return scanning_devices();
}
