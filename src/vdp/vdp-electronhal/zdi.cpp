/*
 * Title:           Electron - HAL
 *                  a playful alternative to Quark
 *                  quarks and electrons combined are matter.
 * Author:          Mario Smit (S0urceror)
*/

// ZDI library

#include "hal.h"
#include "zdi.h"
#include "OneWire_direct_gpio.h"

byte debug_flags = 0x00;
bool zdi_mode_flag = false;
uint8_t upper_address = 0x00;
char szLine[80];
uint8_t charcnt = 0;
bool ihexmode = false;
uint8_t tdi_bitmask = PIN_TO_BITMASK(ZDI_TDI);
uint32_t* tdi_baseReg = PIN_TO_BASEREG(ZDI_TDI);
uint8_t tck_bitmask = PIN_TO_BITMASK(ZDI_TCK);
uint32_t* tck_baseReg = PIN_TO_BASEREG(ZDI_TCK);

extern HardwareSerial host_serial;
extern void do_keys_ps2 ();
extern void do_serial_ez80 ();

// low-level bit stream
///////////////////////
void zdi_start ()
{
    IO_REG_TYPE tdi_mask IO_REG_MASK_ATTR = tdi_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tdi_reg IO_REG_BASE_ATTR = tdi_baseReg;
    IO_REG_TYPE tck_mask IO_REG_MASK_ATTR = tck_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tck_reg IO_REG_BASE_ATTR = tck_baseReg;

    // TDI - OUTPUT - HIGH
    // TCK - OUTPUT - HIGH
    DIRECT_WRITE_HIGH (tdi_reg,tdi_mask);
    DIRECT_MODE_OUTPUT (tdi_reg,tdi_mask);
    DIRECT_WRITE_HIGH (tck_reg,tck_mask);
    // TDI - OUTPUT - LOW
    DIRECT_WRITE_LOW (tdi_reg,tdi_mask);

    // TDI: x^^^\\*____
    // TCK: xx^^^^^^^^^
    
}
void zdi_write_bit (bool bit)
{
    // TDI: xxxxxBBBBBB
    // TCK: ^\\___*//^^
    IO_REG_TYPE tdi_mask IO_REG_MASK_ATTR = tdi_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tdi_reg IO_REG_BASE_ATTR = tdi_baseReg;
    IO_REG_TYPE tck_mask IO_REG_MASK_ATTR = tck_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tck_reg IO_REG_BASE_ATTR = tck_baseReg;

    // clock low
    // TCK - OUTPUT - LOW
    DIRECT_WRITE_LOW (tck_reg,tck_mask);
    // set bit
    // TDI - OUTPUT - BIT
    if (bit)
        DIRECT_WRITE_HIGH (tdi_reg,tdi_mask);
    else
        DIRECT_WRITE_LOW (tdi_reg,tdi_mask);
    DIRECT_MODE_OUTPUT (tdi_reg,tdi_mask);
    // clock up - triggers EZ80 to read bit
    // TCK - OUTPUT - HIGH
    DIRECT_WRITE_HIGH (tck_reg,tck_mask);
}
bool zdi_read_bit ()
{
    // TDI: xxxxBBBBBBB
    // TCK: ^\\*___//^^
    IO_REG_TYPE tdi_mask IO_REG_MASK_ATTR = tdi_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tdi_reg IO_REG_BASE_ATTR = tdi_baseReg;
    IO_REG_TYPE tck_mask IO_REG_MASK_ATTR = tck_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tck_reg IO_REG_BASE_ATTR = tck_baseReg;

    bool bit;
    // clock low, triggers EZ80 to ready bit
    // TCK - OUTPUT - LOW
    DIRECT_WRITE_LOW (tck_reg,tck_mask);
    // read the bit
    // TDI - INPUT - BIT
    DIRECT_MODE_INPUT (tdi_reg,tdi_mask);
    bit = DIRECT_READ (tdi_reg,tdi_mask);
    // clock high
    // TCK - OUTPUT - HIGH
    DIRECT_WRITE_HIGH (tck_reg,tck_mask);
    return bit;
}
void zdi_register (byte regnr,bool read)
{
    // write 7-bits, msb to lsb order
    for (int i=6;i>=0;i--)
    {
        zdi_write_bit (regnr & (0b1<<i));
    }
    // write read/write bit
    zdi_write_bit (read);
}
void zdi_separator (bool done_or_continue)
{
    zdi_write_bit (done_or_continue);
}
void zdi_write (byte value)
{
    // write 8-bits, msb to lsb order
    for (int i=7;i>=0;i--)
    {
        zdi_write_bit (value & (0b1<<i));
    }
}
byte zdi_read ()
{
    byte value = 0;
    // read 8-bits, msb to lsb order
    for (int i=7;i>=0;i--)
    {
        value = value << 1;
        value = value | zdi_read_bit ();
    }
    return value;
}

// medium-level register read and writes
///////////////////////

byte zdi_read_register (byte regnr)
{
    byte value;
    // TCK: xx\\*__//^^
    // TDI: xxxxxBBBBBB

    // TCK: xx^^^^^^^^ xxx___*//^^ ^^^___*//^^ ^^^___*//^^ ^^^___*//^^ ^^\\*__//^^
    // TDI: x^^^\\*___ xxxxxBBBBBB xxxxxBBBBBB xxxxxBBBBBB xxxxxBBBBBB xxxxxBBBBBB
    noInterrupts();
    zdi_start ();
    zdi_register (regnr,ZDI_READ);
    zdi_separator (1);
    value  = zdi_read ();
    zdi_separator (0);
    delayMicroseconds (3);
    interrupts();
    return value;
}
void zdi_write_register (byte regnr, byte value)
{
    noInterrupts();
    zdi_start ();
    zdi_register (regnr,ZDI_WRITE);
    zdi_separator (1);
    zdi_write(value);
    zdi_separator (1);
    delayMicroseconds (3);
    interrupts();
}

void zdi_read_registers (byte startregnr, byte count, byte* values)
{
    byte* ptr = values;
    noInterrupts();
    zdi_start ();
    zdi_register (startregnr,ZDI_READ);
    while (count-- > 0)
    {
        zdi_separator (ZDI_CMD_CONTINUE);
        delayMicroseconds (3);
        *(ptr++)  = zdi_read ();
    }
    zdi_separator (ZDI_CMD_DONE);
    delayMicroseconds (3);
    interrupts();
}

void zdi_write_registers (byte startregnr, byte count, byte* values)
{
    byte* ptr = values;
    noInterrupts();
    zdi_start ();
    zdi_register (startregnr,ZDI_WRITE);
    while (count-- > 0)
    {
        zdi_separator (ZDI_CMD_CONTINUE);
        delayMicroseconds (3);
        zdi_write (*(ptr++));
    }
    zdi_separator (ZDI_CMD_DONE);
    delayMicroseconds (3);
    interrupts();
}

// high-level debugging, register and memory read functions
///////////////////////
uint16_t zdi_get_productid ()
{
    byte pids[2];
    zdi_read_registers (ZDI_ID_L,2,pids);
    return (pids[1]<<8)+pids[0];
}
uint8_t zdi_get_revision ()
{
    return zdi_read_register (ZDI_ID_REV);
}
uint8_t zdi_get_cpu_status ()
{
    return zdi_read_register (ZDI_STAT);
}
uint8_t zdi_get_bus_status ()
{
    return zdi_read_register (ZDI_BUS_STAT);
}
uint32_t zdi_read_cpu (rw_control_t rw)
{
    zdi_write_register (ZDI_RW_CTL,rw);
    if (rw==SET_ADL || rw==RESET_ADL)
        return 0;
    byte values[3];
    zdi_read_registers (ZDI_RD_L,3,values);
    return (values[2]<<16)+(values[1]<<8)+values[0]; // U+H+L
}
void zdi_write_cpu (rw_control_t rw,uint32_t value)
{
    byte values[3];
    values[0] = value & 0xff;       // L
    values[1] = (value>>8) & 0xff;  // H
    values[2] = (value>>16) & 0xff; // U
    zdi_write_registers (ZDI_WR_DATA_L,3,values);
    zdi_write_register (ZDI_RW_CTL,rw | 0b10000000); // set high bit to indicate write
}
void zdi_read_memory (uint32_t address,uint16_t count, byte* memory)
{
    byte* ptr = memory;
    // set start address
    zdi_write_cpu (REG_PC,address);
    // commence read from auto-increment read memory byte register
    zdi_start ();
    zdi_register (ZDI_RD_MEM,ZDI_READ);
    // one by one
    for (int i=0;i<count;i++)
    {
        zdi_separator (ZDI_CMD_CONTINUE);
        delayMicroseconds (3);
        *(ptr++) = zdi_read ();
    }
    // done
    zdi_separator (ZDI_CMD_DONE);
    delayMicroseconds (3);
}
void zdi_write_memory (uint32_t address,uint32_t count, byte* memory)
{
    byte* ptr = memory;

    zdi_write_cpu (REG_PC,address);
    // commence write from auto-increment read memory byte register
    zdi_start ();
    zdi_register (ZDI_WR_MEM,ZDI_WRITE);
    for (uint32_t i=0;i<count;i++)
    {
        zdi_separator (ZDI_CMD_CONTINUE);
        delayMicroseconds (3);
        zdi_write (*(ptr++));
    }
    // done
    zdi_separator (ZDI_CMD_DONE);
    delayMicroseconds (3);
}
void zdi_print_debug_status () 
{
    hal_hostpc_printf ("\r\n(break control: %c%c%c%c%c%c)",debug_flags&0b10000000?'N':'.',
                                                    debug_flags&0b00001000?'1':'.',
                                                    debug_flags&0b00010000?'2':'.',
                                                    debug_flags&0b00100000?'3':'.',
                                                    debug_flags&0b01000000?'4':'.',
                                                    debug_flags&0b00000001?'S':'.');
}

void zdi_debug_break ()
{
    debug_flags |= 0b10000000;
    zdi_write_register (ZDI_BRK_CTL, debug_flags);
    zdi_print_debug_status ();
}
void zdi_debug_continue ()
{
    debug_flags &= 0b01111111;
    zdi_write_register (ZDI_BRK_CTL, debug_flags);
    zdi_print_debug_status ();
}
void zdi_debug_step ()
{
    // set single-step bit, keep status other bp's + break next
    debug_flags |= 0b10000001;
    zdi_write_register (ZDI_BRK_CTL, debug_flags);
    zdi_print_debug_status ();

    // reset single-step bit, keep status other bp's + break next
    debug_flags &= 0b11111110;
    zdi_write_register (ZDI_BRK_CTL, debug_flags);
    zdi_print_debug_status ();
}
bool zdi_debug_breakpoint_reached ()
{
    byte status = zdi_get_cpu_status ();
    return (status & 0b10000000); // ZDI mode means breakpoint
}
void zdi_debug_breakpoint_enable (uint8_t index,uint32_t address)
{
    // set bp address
    byte bp[3] = {  (byte)(address & 0xff),
                    (byte)((address>>8) & 0xff),
                    (byte)((address>>16) & 0xff)
                 };
    zdi_write_registers (ZDI_ADDR0_L+4*index,3,bp);
    // enable bp
    debug_flags|=(0b00001000 << index);
    zdi_write_register (ZDI_BRK_CTL, debug_flags);

    zdi_print_debug_status ();
}
void zdi_debug_breakpoint_disable (uint8_t index)
{
    byte mask = (0b00001000 << index);
    debug_flags &= (~mask);
    zdi_write_register (ZDI_BRK_CTL, debug_flags);
    zdi_print_debug_status ();
}
uint8_t zdi_available_break_point()
{
    uint8_t i;
    byte mask = 0b00001000;
    for (i=0;i<4;i++)
    {
        if (!(debug_flags & (mask << i)))
            break;
    }
    if (i==4)
        return 0xff;

    return i;
}
void zdi_reset ()
{
    zdi_write_register (ZDI_MASTER_CTL, 0b10000000);
}
bool zdi_mode ()
{
    return zdi_mode_flag;
}
void zdi_enter ()
{
    IO_REG_TYPE tck_mask IO_REG_MASK_ATTR = tck_bitmask;
	__attribute__((unused)) volatile IO_REG_TYPE *tck_reg IO_REG_BASE_ATTR = tck_baseReg;
    
    ihexmode = false;
    // clear line buffer
    memset (szLine,0,sizeof(szLine));
    charcnt=0;
    upper_address = 0x00;
    
    if (zdi_debug_breakpoint_reached())
        debug_flags |= 0b10000000;

    zdi_mode_flag = true;
    DIRECT_WRITE_HIGH (tck_reg,tck_mask);
    DIRECT_MODE_OUTPUT (tck_reg,tck_mask);
    
    // get cpu identification
    uint16_t pid=zdi_get_productid ();
    uint8_t rev=zdi_get_revision();
    if (pid!=0x0007 && rev!=0xaa)
    {
        zdi_mode_flag = false;
        hal_hostpc_printf ("\r\n(invalid EZ80 response, check ZDI connection: %X.%X)",pid,rev);
        hal_hostpc_printf ("\r\n*");
        return;
    }
    hal_hostpc_printf ("\r\nZDI mode EZ80: %X.%X\r\n#",pid,rev);
}
void zdi_exit ()
{
    if (zdi_available_break_point()>0)
        hal_hostpc_printf ("\r\n(exiting, breakpoint active)");

    // back to running mode if we are at breakpoint
    if (zdi_debug_breakpoint_reached())
        hal_hostpc_printf ("\r\n(exiting, break active)");
    else
        hal_hostpc_printf ("\r\n(exiting)");

    zdi_mode_flag = false;
    hal_hostpc_printf ("\r\n*");
}
uint32_t zdi_cpu_instruction_ld_r () 
{
    // ld a, nn
    uint8_t instructions[2];
    instructions[0]=0x5f;
    instructions[1]=0xed;
    zdi_write_registers (ZDI_IS1,2,instructions);

    return zdi_read_cpu (REG_AF);
    // uint32_t af = zdi_read_cpu (REG_AF);
    // return (uint8_t) (af & 0xff);
}
uint32_t zdi_cpu_instruction_ld_i () 
{
    // ld a, nn
    uint8_t instructions[2];
    instructions[0]=0x57;
    instructions[1]=0xed;
    zdi_write_registers (ZDI_IS1,2,instructions);

    return zdi_read_cpu (REG_AF);
    // uint32_t af = zdi_read_cpu (REG_AF);
    // return (uint8_t) (af & 0xff);
}
void zdi_cpu_instruction_out0 (uint8_t regnr, uint8_t value)
{
    // ld a, nn
    uint8_t instructions[3];
    instructions[0]=value;
    instructions[1]=0x3e;
    zdi_write_registers (ZDI_IS1,2,instructions);
    // out0 (nn),a
    instructions[0]=regnr;
    instructions[1]=0x39;
    instructions[2]=0xed;
    zdi_write_registers (ZDI_IS2,3,instructions);
}
void zdi_cpu_instruction_out (uint8_t regnr, uint8_t value)
{
    // ld a, nn
    uint8_t instructions[2];
    instructions[0]=value;
    instructions[1]=0x3e;
    zdi_write_registers (ZDI_IS1,2,instructions);
    // out0 (nn),a
    instructions[0]=regnr;
    instructions[1]=0xd3;
    zdi_write_registers (ZDI_IS2,2,instructions);
}
void zdi_cpu_instruction_di ()
{
    // di
    zdi_write_register (ZDI_IS0,0xf3);
}

// ZDI interface
///////////////////////
void zdi_cmd_report (debug_state_t state)
{
    byte mem[8];
    uint32_t pc;
    uint32_t af;
    uint8_t f;
    uint8_t mbase;
    uint32_t bc;
    uint32_t de;
    uint32_t hl;
    uint32_t ix;
    uint32_t iy;
    uint32_t spl;
    uint16_t sps;
    uint8_t status;
    uint8_t i,r=0;
    bool IFF2;

    // cpu status
    status = zdi_get_cpu_status ();

    // break to properly read registers
    if (!(status&0b10000000))
        zdi_debug_break ();

    // read register contents
    zdi_read_cpu (SET_ADL);
    pc = zdi_read_cpu (REG_PC);
    
    af = zdi_read_cpu (REG_AF);
    f = (af>>8) &0xff;
    mbase = (af>>16) &0xff;
    bc = zdi_read_cpu (REG_BC);
    de = zdi_read_cpu (REG_DE);
    hl = zdi_read_cpu (REG_HL);
    ix = zdi_read_cpu (REG_IX);
    iy = zdi_read_cpu (REG_IY);
    spl = zdi_read_cpu (REG_SP);
    zdi_read_cpu (RESET_ADL);
    sps = zdi_read_cpu (REG_SP);
    if (status&0b00010000)
        zdi_read_cpu (SET_ADL);

    // reading R and I registers destroys contents of A and PC
    uint32_t tmp_af = zdi_cpu_instruction_ld_i ();
    i = tmp_af & 0xff;
    IFF2 = (tmp_af >> 8) & 0b00000100;
    r = zdi_cpu_instruction_ld_r () & 0xff;
    // restore AF+MBASE
    zdi_write_cpu (REG_AF,af);
    // restore PC
    zdi_write_cpu (REG_PC,pc);

    // continue when not in break state before
    if (!(status&0b10000000))
        zdi_debug_continue ();
    
    hal_hostpc_printf ("\r\nA=%02X BC=%06X DE=%06X HL=%06X IX=%06X IY=%06X SPL=%06X", af & 0xff,bc,de,hl,ix,iy,spl);
    hal_hostpc_printf ("\r\nF=%02X %c%c%c%c%c%c STAT=%c%c%c%c%c PC=%06X MB=%02X IFF=%c I=%02X R=%02X SPS=%04X",
                            f,
                            f&0b10000000?'P':'N',
                            f&0b01000000?'Z':'.',
                            f&0b00010000?'H':'.',
                            f&0b00000100?'P':'V',
                            f&0b00000010?'A':'S',
                            f&0b00000001?'C':'.',
                            status&0b10000000?'Z':'.',
                            status&0b00100000?'H':'.',
                            status&0b00010000?'A':'.',
                            status&0b00001000?'M':'.',
                            status&0b00001000?'I':'.',
                            pc,
                            mbase,
                            IFF2?'1':'0',
                            i,
                            r,
                            sps);        

    // show register contents
    if (state==BREAK || state==STEP) // stopped at a breakpoint
    {
        // disassembly at PC?               
        zdi_read_memory (pc,8,mem);
        hal_hostpc_printf ("\r\n%06X %02X %02X %02X %02X %02X %02X %02X %02X",
                                pc,mem[0],mem[1],mem[2],mem[3],mem[4],mem[5],mem[6],mem[7]);
        zdi_write_cpu (REG_PC,pc);
    }

    // show state
    const char* prompt;
    switch (state)
    {
        case RUN: prompt = "running";break;
        case BREAK: prompt = "break";break;
        case STEP: prompt = "step";break;
        case REGONLY: prompt = "registers";break;
    }
    hal_hostpc_printf ("\r\n(%s)\r\n#",prompt);                                                                            
}

uint8_t zdi_checksum_memory (byte* memory,uint16_t len)
{
    uint8_t checksum=0;
    for (uint16_t i=0;i<len;i++)
        checksum += memory[i];
    return (~checksum) + 1;
}
#define LINE_LENGTH 0x20
void zdi_bin_to_intel_hex (byte* memory,uint32_t start,uint16_t size,bool first, bool last)
{
    uint8_t checksum=0;
    uint8_t line_len;
    for (int i=0;i<size;i++)
    {
        if (size>=LINE_LENGTH)
            line_len = LINE_LENGTH;
        else    
            line_len = size;

        if ((i%line_len)==0)
        {
            uint8_t new_upper_address = (start>>16);
            if (new_upper_address!=upper_address)
            {
                upper_address = new_upper_address;
                checksum = (~(0x02+0x04+upper_address));
                hal_hostpc_printf (":0200000400%02X%02X\r\n",upper_address,checksum+1);
                checksum = 0;
            }
            uint16_t pc = i+(start&0xffff);
            hal_hostpc_printf (":%02X%04X00",line_len,pc);
            checksum += line_len + ((pc>>8)&0xff) + (pc&0xff);
        }
        checksum+=memory[i];
        hal_hostpc_printf ("%02X",memory[i]);
        if ((i%line_len)==(line_len-1))
        {
            checksum = (~checksum);
            hal_hostpc_printf ("%02X\r\n",(uint8_t)(checksum+1));
            checksum = 0;
        }
    }
    if (last)
        hal_hostpc_printf (":00000001FF\r\n#");
}
bool cpu_was_at_breakpoint = false;
void zdi_intel_hex_to_bin(char* szLine, uint8_t charcnt)
{
    char szLen[3],szAddress[5],szType[3],szCheckSum[3],szByte[3];
    szByte[2]='\0';
    strncpy (szLen,szLine+1,2);szLen[2]='\0';
    strncpy (szAddress,szLine+3,4);szAddress[4]='\0';
    strncpy (szType,szLine+7,2);szType[2]='\0';
    strncpy (szCheckSum,szLine+charcnt-2,2);szCheckSum[2]='\0';
    uint8_t len = strtoul (szLen,NULL,16);
    uint32_t address = (uint16_t) strtoul (szAddress,NULL,16);
    uint8_t type = strtoul (szType,NULL,16);
    uint8_t checksum = strtoul (szCheckSum,NULL,16);
    uint8_t check = (uint8_t)len + 
                    (uint8_t)((address>>8)&0xff) + 
                    (uint8_t)(address&0xff) + 
                    (uint8_t)type;
                    
    if (type==0)
    {
        // get pc
        uint32_t pc = zdi_read_cpu (REG_PC);

        // calculate long address
        address = (upper_address<<16)+address;
        
        // allocate memory
        byte* memory = (byte*) malloc (len);
        
        // parse data
        for (uint8_t i=0;i<len;i++)
        {
            strncpy (szByte,szLine+9+i*2,2);
            byte b = strtoul (szByte,NULL,16);
            memory[i] = b;
            check = check + b;
        }

        // only write to memory when no checksum error
        check = (~check);
        check++;
        if (check != checksum)
            hal_hostpc_printf ("!");
        else
        {
            hal_hostpc_printf (".");
            // write to ez80 and free memory
            zdi_write_memory (address,len,memory);
        }

        free (memory);

        // restore pc
        zdi_write_cpu (REG_PC, pc);
    }
    if (type==1)
    {
        // end of file
        if (!cpu_was_at_breakpoint)
            zdi_debug_continue ();
        hal_hostpc_printf ("\r\n#");
    }
    if (type==4 || type==2)
    {
        // start of file
        cpu_was_at_breakpoint = zdi_debug_breakpoint_reached();
        if (!cpu_was_at_breakpoint)
            zdi_debug_break ();

        char szUpper[5];
        strncpy (szUpper,szLine+9,4);szUpper[4]='\0';
        upper_address = strtoul (szUpper,NULL,16)&0xff;
    }
}

void zdi_cmd_help () 
{
    hal_hostpc_printf ("\r\nh                              - help ");
    hal_hostpc_printf ("\r\na / z                          - cpu      - switch to ADL or Z80 mode");
    hal_hostpc_printf ("\r\ni                              - cpu      - bring EZ80 to an initialized state");
    hal_hostpc_printf ("\r\nj address                      - cpu      - jump to address");
    hal_hostpc_printf ("\r\nR                              - cpu      - reset EZ80");
    hal_hostpc_printf ("\r\nb                              - debug    - break immediate");
    hal_hostpc_printf ("\r\nb address                      - debug    - set breakpoint");
    hal_hostpc_printf ("\r\nd nr                           - debug    - unset breakpoint");
    hal_hostpc_printf ("\r\nc                              - debug    - continue the program");
    hal_hostpc_printf ("\r\nr                              - debug    - show registers and status");
    hal_hostpc_printf ("\r\ns                              - debug    - step");
    hal_hostpc_printf ("\r\nw [address]                    - debug    - wait until breakpoint or we reach address");
    hal_hostpc_printf ("\r\nx address [size]               - examine  - memory from address, intelhex format");
    hal_hostpc_printf ("\r\nX address size                 - examine  - memory from address, binary format");
    hal_hostpc_printf ("\r\n:0123456789ABCD                - write    - memory, intelhex format");
    hal_hostpc_printf ("\r\ne command                      - terminal - sends command to EZ80 to be executed");
    hal_hostpc_printf ("\r\n#");
}
void zdi_cmd_mode_ADL () 
{
    zdi_debug_break ();
    zdi_read_cpu (SET_ADL);
    zdi_cmd_report (BREAK);
}
void zdi_cmd_mode_Z80 ()
{
    zdi_debug_break ();
    zdi_read_cpu (RESET_ADL);
    zdi_cmd_report (BREAK);
}
void zdi_cmd_set_breakpoint ()
{
    if (charcnt==1)
    {
        // issue BREAK next instruction
        zdi_debug_break ();
        zdi_cmd_report (BREAK);
    }
    else
    {
        // set breakpoint to address
        char* pIndex=NULL;
        uint32_t address = strtoul (szLine+1,&pIndex,16);
        uint8_t bp;
        if (*pIndex != '\0')
        {
            bp = strtoul (pIndex,NULL,16);
            bp--;
        }
        else 
            bp = zdi_available_break_point();
        if (bp!=255)
        {
            zdi_debug_breakpoint_enable (bp,address);
            hal_hostpc_printf ("\r\n(bp%d set to 0x%06X)\r\n#",bp+1,address);
        }
        else
            hal_hostpc_printf ("\r\n(error, not more than 4 hw breakpoints)\r\n#");
    }
}
void zdi_cmd_continue ()
{
    if (zdi_debug_breakpoint_reached()) // breakpoint?
    {
        zdi_debug_continue();
        
        if (zdi_available_break_point()>0)
        {
            // one or more breakpoints set
            hal_hostpc_printf ("\r\n(continue, breakpoint active)\r\n#");
        }
        else
            hal_hostpc_printf ("\r\n(continue)\r\n#");
    }
    else
        hal_hostpc_printf ("\r\n#");
}
void zdi_cmd_delete_breakpoint ()
{
    if (charcnt>1)
    {
        // delete breakpoint
        uint8_t bp = strtoul (szLine+1,NULL,10);
        bp--;
        if (bp<4)
        {
            zdi_debug_breakpoint_disable (bp);
            hal_hostpc_printf ("\r\n(bp%d disabled)\r\n#",bp+1);
        }
        else
            hal_hostpc_printf ("\r\n(error, not more than 4 hw breakpoints)\r\n#");
    }
    else
        hal_hostpc_printf ("\r\n(error, specify breakpoint)\r\n#");
}
void zdi_cmd_execute_ez80 ()
{
    // execute command in ElectronOS by sending chars to HardwareSerial
    if (charcnt>2)
    {
        bool already_breaked = false;
        if (zdi_debug_breakpoint_reached())
        {
            already_breaked = true;
            zdi_debug_continue ();
        }
        
        ez80_serial.write (szLine+2);
        ez80_serial.write ("\r\n");
        
        if (already_breaked)
            zdi_debug_break ();

        hal_hostpc_printf ("\r\n(command send to ElectronOS)\r\n#");
    }
}
void zdi_cmd_jump ()
{
    if (charcnt>2)
    {
        u32_t address=strtoul (szLine+2,NULL,16);
        bool already_breaked = false;
        if (zdi_debug_breakpoint_reached())
            already_breaked = true;
        else
            zdi_debug_break ();
        zdi_write_cpu (REG_PC,address);
        // continue if
        if (!already_breaked)
        {
            hal_hostpc_printf ("\r\n(jumping to 0x%06X)\r\n#",address);
            zdi_debug_continue();  
        }
        else
        {
            hal_hostpc_printf ("\r\n(PC set to 0x%06X)\r\n#",address);
        }
    }
    else
    {
        hal_hostpc_printf ("\r\n(error, no jump address)\r\n#");
    }
}
// return memory contents in binary chunks
void zdi_cmd_examine_binary ()
{
    if (charcnt>2)
    {
        // break cpu
        bool already_breaked = false;
        if (zdi_debug_breakpoint_reached())
            already_breaked = true;
        else
            zdi_debug_break ();
        
        // get pc
        uint32_t pc = zdi_read_cpu (REG_PC);

        // get requested address + size
        char* pStart = szLine+2;
        char* pSize;
        u32_t address=strtoul (pStart,&pSize,16);
        u32_t size=LINE_LENGTH;
        if (*pSize != '\0')
            size = strtoul (pSize,NULL,16);
        u32_t total_size = size;

        const uint8_t CHUNKSIZE = 30;
        byte* mem = (byte*) malloc (CHUNKSIZE);        
        // retrieve and send binary
        while (size>0) 
        {
            // how many bytes still to transfer?
            uint8_t chunksize = (size>CHUNKSIZE) ? CHUNKSIZE : size;
            // more then 0? let's write
            if (chunksize>0)
            {
                memset (mem,0,CHUNKSIZE);
                zdi_read_memory (address,chunksize,mem);
                host_serial.write (chunksize);
                host_serial.write (mem,CHUNKSIZE); // read memory with or without trailing zeroes
                uint8_t checksum = zdi_checksum_memory (mem,chunksize);
                host_serial.write (checksum);

                // hal_terminal_printf ("%02X - %02X%02X%02X%02X%02X%02X - %02X\r\n",chunksize,
                //                                                      mem[0],
                //                                                      mem[1],
                //                                                      mem[2],
                //                                                      mem[3],
                //                                                      mem[4],
                //                                                      mem[5],
                //                                                      checksum);
            
                // wait for response from host
                char ch;
                uint8_t times = 10;
                while ((ch=hal_hostpc_serial_read())==0);
                if (ch=='+') 
                {
                    // okay? else resend
                    size -= chunksize;
                    address += chunksize;
                    continue;
                }
                if (ch=='-') 
                {
                    hal_hostpc_printf ("\r\n(error, resending)\r\n#");
                    continue;
                }
                if (ch==0x1b)
                {
                    hal_hostpc_printf ("\r\n(error, host escapes)\r\n#");
                    break;
                }
            }
        }
        free (mem);

        // restore pc
        zdi_write_cpu (REG_PC, pc);

        // continue if
        if (!already_breaked)
            zdi_debug_continue();

        // ready prompt
        hal_hostpc_printf ("\r\n(%ld bytes transferred)\r\n#",total_size);
    }
    else
        hal_hostpc_printf ("\r\n(error, no start address)\r\n#");
}
void zdi_cmd_examine_intelhex ()
{
    if (charcnt>2)
    {
        hal_hostpc_printf ("\r\n");

        // break cpu
        bool already_breaked = false;
        if (zdi_debug_breakpoint_reached())
            already_breaked = true;
        else
            zdi_debug_break ();
        // get pc
        uint32_t pc = zdi_read_cpu (REG_PC);
        // get requested address + size
        char* pStart = szLine+2;
        char* pSize;
        u32_t address=strtoul (pStart,&pSize,16);
        u16_t size=LINE_LENGTH;
        if (*pSize != '\0')
            size = strtoul (pSize,NULL,16);
        // read memory in chunks of LINE_LENGTH
        bool first = true;
        bool last = false;
        uint16_t chunk;
        while (size>0)
        {
            chunk = (size>LINE_LENGTH?LINE_LENGTH:size);
            byte* mem = (byte*) malloc (chunk);
            
            if (size<=chunk)
                last = true;
            zdi_read_memory (address,chunk,mem);
            zdi_bin_to_intel_hex (mem,address,chunk,first,last);
            address+=chunk;
            size-=chunk;
            if (first)
                first = false;
                
            free (mem);
        }
        // restore pc
        zdi_write_cpu (REG_PC, pc);
        // continue if
        if (!already_breaked)
            zdi_debug_continue();
    }
    else
        hal_hostpc_printf ("\r\n(error, no start address)\r\n#");
}
void zdi_cmd_step ()
{
    if (zdi_debug_breakpoint_reached()) // breakpoint?
    {
        zdi_debug_step ();
        zdi_cmd_report (STEP);
    }
    else    
        hal_hostpc_printf ("\r\n#");
}
void zdi_cmd_store_intelhex ()
{
    if (charcnt>=1+2+4+2+2)
        zdi_intel_hex_to_bin (szLine,charcnt);
    else
        hal_hostpc_printf ("\r\n(wrong Intel Hex format)\r\n#");
}
void zdi_cmd_initialize_EZ80 ()
{
    zdi_debug_break ();
    zdi_read_cpu (SET_ADL);
    zdi_cpu_instruction_di ();
    // configure default GPIO
    // no interrupt, input mode, high impedance
    zdi_cpu_instruction_out (PB_DDR, 0xff);
    zdi_cpu_instruction_out (PC_DDR, 0xff);
    zdi_cpu_instruction_out (PD_DDR, 0xff);
    zdi_cpu_instruction_out (PB_ALT1, 0x0);
    zdi_cpu_instruction_out (PC_ALT1, 0x0);
    zdi_cpu_instruction_out (PD_ALT1, 0x0);
    zdi_cpu_instruction_out (PB_ALT2, 0x0);
    zdi_cpu_instruction_out (PC_ALT2, 0x0);
    zdi_cpu_instruction_out (PD_ALT2, 0x0);
    // timers
    zdi_cpu_instruction_out (TMR0_CTL, 0x0);
    zdi_cpu_instruction_out (TMR1_CTL, 0x0);
    zdi_cpu_instruction_out (TMR2_CTL, 0x0);
    zdi_cpu_instruction_out (TMR3_CTL, 0x0);
    zdi_cpu_instruction_out (TMR4_CTL, 0x0);
    zdi_cpu_instruction_out (TMR5_CTL, 0x0);
    // uart interrupts
    zdi_cpu_instruction_out (UART0_IER, 0x0);
    zdi_cpu_instruction_out (UART1_IER, 0x0);
    // I2C / Flash / SPI / RTC
    zdi_cpu_instruction_out (I2C_CTL, 0x0);
    zdi_cpu_instruction_out (FLASH_IRQ, 0x0);
    zdi_cpu_instruction_out (SPI_CTL, 0x4);
    zdi_cpu_instruction_out (RTC_CTRL, 0x0);
    // configure internal flash
    zdi_cpu_instruction_out (FLASH_ADDR_U,0x00);
    zdi_cpu_instruction_out (FLASH_CTRL,0b00101000);   // flash enabled, 1 wait state
    // configure internal RAM chip-select range
    zdi_cpu_instruction_out (RAM_ADDR_U,0xbc);         // configure internal RAM chip-select range
    zdi_cpu_instruction_out (RAM_CTL,0b10000000);      // enable
    // configure external RAM chip-select range
    zdi_cpu_instruction_out (CS0_LBR,0x04);            // lower boundary
    zdi_cpu_instruction_out (CS0_UBR,0x0b);            // upper boundary
    zdi_cpu_instruction_out (CS0_BMC,0b00000001);      // 1 wait-state, ez80 mode
    zdi_cpu_instruction_out (CS0_CTL,0b00001000);      // memory chip select, cs0 enabled
    // configure external RAM chip-select range
    zdi_cpu_instruction_out (CS1_CTL,0x00);            // memory chip select, cs1 disabled
    // configure external RAM chip-select range
    zdi_cpu_instruction_out (CS2_CTL,0x00);            // memory chip select, cs2 disabled
    // configure external RAM chip-select range
    zdi_cpu_instruction_out (CS3_CTL,0x00);            // memory chip select, cs3 disabled
    // set stack pointer
    zdi_write_cpu (REG_SP,0x0BFFFF);
    // set program counter
    zdi_write_cpu (REG_PC,0x000000);
    // show status
    zdi_cmd_report (BREAK);
}

void zdi_cmd_wait ()
{
    byte ch;
    hal_hostpc_printf ("\r\n(waiting)");
    // wait until 
    while (true)
    {
        // do normal communication with EZ80
        do_keys_ps2();
        do_serial_ez80();
        
        // check if we reached a breakpoint
        if (zdi_debug_breakpoint_reached())
        {
            hal_hostpc_printf ("\r\n(breakpoint reached)\r\n#");
            break;
        }

        // no? check if we receive ESC
        if ((ch=hal_hostpc_serial_read())!=0)
        {
            if (ch==0x1b)
            {
                hal_hostpc_printf ("\r\n(terminated)\r\n#");
                break;
            }
        }
    }
}

void zdi_process_line ()
{
    switch (szLine[0])
    {
        case 'h':
            zdi_cmd_help ();
            break;
        case 'a':
            zdi_cmd_mode_ADL ();
            break;
        case 'b':
            zdi_cmd_set_breakpoint ();
            break;
        case 'c':
            zdi_cmd_continue ();
            break;            
        case 'd':
            zdi_cmd_delete_breakpoint ();
            break;
        case 'e':
            zdi_cmd_execute_ez80 ();
            break;
        case 'j': // jump to address
            zdi_cmd_jump ();
            break;
        case 'X':
            zdi_cmd_examine_binary ();
            break;
        case 'x': // examine, memory dump
            zdi_cmd_examine_intelhex();
            break;
        case 'r':
            zdi_cmd_report (REGONLY);
            break;
        case 'R':
            zdi_reset ();
            break;
        case 's':
        case '\0':
            zdi_cmd_step ();
            break;
        case 'w':
            zdi_cmd_wait ();
            break;
        case ':':
            zdi_cmd_store_intelhex ();
            break;
        case 'z':
            zdi_cmd_mode_Z80 ();
            break;
        case 'i':
            zdi_cmd_initialize_EZ80 ();
            break;
        default:
            hal_hostpc_printf ("\r\n(unknown command)\r\n#");
            break;
    }
}
void zdi_process_cmd (uint8_t key)
{
    switch (key)
    {
        case 0x08: // backspace
            hal_hostpc_printf ("%c %c",key,key);
            szLine[--charcnt]='\0';
            break;
        case 'q':
        case ESC: // escape
            zdi_exit();
            break;
        case '\r': // carriage return
            // process line
            zdi_process_line ();
            memset (szLine,0,sizeof(szLine));
            charcnt=0;
            if (ihexmode) 
                ihexmode = false;
            break;
        case '\n': // newline
            // absorb
            break;
        case ':':
            ihexmode = true;
        default:
            // echo characters and add to line buffer
            if (!ihexmode)
                hal_hostpc_printf ("%c",key);
            if (charcnt<sizeof (szLine))
                szLine[charcnt++]=key;
    }
}

