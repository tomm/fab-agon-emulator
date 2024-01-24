#ifndef __ZDI_H_
#define __ZDI_H_

#define ZDI_TCK 26
#define ZDI_TDI 27

#define ZDI_READ 1
#define ZDI_WRITE 0
#define ZDI_CMD_CONTINUE 0
#define ZDI_CMD_DONE 1

// nr of microseconds to wait for next bit
// documentation says 4Mhz ZCLK speed is possible, so 0.25 usecs
// we stay a bit on the safe side with 10 usecs
#define ZDI_WAIT_MICRO 1

// ZDI write registers
#define ZDI_ADDR0_L     0x00
#define ZDI_ADDR0_H     0x01
#define ZDI_ADDR0_U     0x02
#define ZDI_ADDR1_L     0x04
#define ZDI_ADDR1_H     0x05
#define ZDI_ADDR1_U     0x06
#define ZDI_ADDR2_L     0x08
#define ZDI_ADDR2_H     0x09
#define ZDI_ADDR2_U     0x0A
#define ZDI_ADDR3_L     0x0c
#define ZDI_ADDR3_H     0x0d
#define ZDI_ADDR3_U     0x0e
#define ZDI_BRK_CTL     0x10
#define ZDI_MASTER_CTL  0x11
#define ZDI_WR_DATA_L   0x13
#define ZDI_WR_DATA_H   0x14
#define ZDI_WR_DATA_U   0x15
#define ZDI_RW_CTL      0x16
#define ZDI_BUS_CTL     0x17
#define ZDI_IS4         0x21
#define ZDI_IS3         0x22
#define ZDI_IS2         0x23
#define ZDI_IS1         0x24
#define ZDI_IS0         0x25
#define ZDI_WR_MEM      0x30

// ZDI read registers
#define ZDI_ID_L        0x00
#define ZDI_ID_H        0x01
#define ZDI_ID_REV      0x02
#define ZDI_STAT        0x03
#define ZDI_RD_L        0x10
#define ZDI_RD_H        0x11
#define ZDI_RD_U        0x12
#define ZDI_BUS_STAT    0x17
#define ZDI_RD_MEM      0x20

// EZ80 registers

/* Programmable Reload Counter/Timers */
#define TMR0_CTL        0x80
#define TMR0_DR_L       0x81
#define TMR0_RR_L       0x81
#define TMR0_DR_H       0x82
#define TMR0_RR_H       0x82
#define TMR1_CTL        0x83
#define TMR1_DR_L       0x84
#define TMR1_RR_L       0x84
#define TMR1_DR_H       0x85
#define TMR1_RR_H       0x85
#define TMR2_CTL        0x86
#define TMR2_DR_L       0x87
#define TMR2_RR_L       0x87
#define TMR2_DR_H       0x88
#define TMR2_RR_H       0x88
#define TMR3_CTL        0x89
#define TMR3_DR_L       0x8A
#define TMR3_RR_L       0x8A
#define TMR3_DR_H       0x8B
#define TMR3_RR_H       0x8B
#define TMR4_CTL        0x8C
#define TMR4_DR_L       0x8D
#define TMR4_RR_L       0x8D
#define TMR4_DR_H       0x8E
#define TMR4_RR_H       0x8E
#define TMR5_CTL        0x8F
#define TMR5_DR_L       0x90
#define TMR5_RR_L       0x90
#define TMR5_DR_H       0x91
#define TMR5_RR_H       0x91
#define TMR_ISS         0x92

/* Watch-Dog Timer */
#define WDT_CTL         0x93
#define WDT_RR          0x94

/* General-Purpose Input/Output Ports */
#define PB_DR           0x9A
#define PB_DDR          0x9B
#define PB_ALT1         0x9C
#define PB_ALT2         0x9D
#define PC_DR           0x9E
#define PC_DDR          0x9F
#define PC_ALT1         0xA0
#define PC_ALT2         0xA1
#define PD_DR           0xA2
#define PD_DDR          0xA3
#define PD_ALT1         0xA4
#define PD_ALT2         0xA5

/* Chip Select/Wait State Generator */
#define CS0_LBR         0xA8
#define CS0_UBR         0xA9
#define CS0_CTL         0xAA
#define CS1_LBR         0xAB
#define CS1_UBR         0xAC
#define CS1_CTL         0xAD
#define CS2_LBR         0xAE
#define CS2_UBR         0xAF
#define CS2_CTL         0xB0
#define CS3_LBR         0xB1
#define CS3_UBR         0xB2
#define CS3_CTL         0xB3

/* On-Chip Random Access Memory Control */
#define RAM_CTL         0xB4
#define RAM_ADDR_U      0xB5

/* Serial Peripheral Interface */
#define SPI_BRG_L       0xB8
#define SPI_BRG_H       0xB9
#define SPI_CTL         0xBA
#define SPI_SR          0xBB
#define SPI_TSR         0xBC
#define SPI_RBR         0xBC

/* Infrared Encoder/Decoder */
#define IR_CTL          0xBF

/* Universal Asynchronous Receiver/Transmitter 0 (UART0 */
#define UART0_RBR       0xC0
#define UART0_THR       0xC0
#define UART0_BRG_L     0xC0
#define UART0_IER       0xC1
#define UART0_BRG_H     0xC1
#define UART0_IIR       0xC2
#define UART0_FCTL      0xC2
#define UART0_LCTL      0xC3
#define UART0_MCTL      0xC4
#define UART0_LSR       0xC5
#define UART0_MSR       0xC6
#define UART0_SPR       0xC7

/* Inter-Integrated Circuit Bus Control (I2C */
#define I2C_SAR         0xC8
#define I2C_XSAR        0xC9
#define I2C_DR          0xCA
#define I2C_CTL         0xCB
#define I2C_SR          0xCC
#define I2C_CCR         0xCC
#define I2C_SRR         0xCD

/* Universal Asynchronous Receiver/Transmitter 1 (UART1 */
#define UART1_RBR       0xD0
#define UART1_THR       0xD0
#define UART1_BRG_L     0xD0
#define UART1_IER       0xD1
#define UART1_BRG_H     0xD1
#define UART1_IIR       0xD2
#define UART1_FCTL      0xD2
#define UART1_LCTL      0xD3
#define UART1_MCTL      0xD4
#define UART1_LSR       0xD5
#define UART1_MSR       0xD6
#define UART1_SPR       0xD7

/* Low-Power Control */
#define CLK_PPD1        0xDB
#define CLK_PPD2        0xDC

/* Real-Time Clock */
#define RTC_SEC         0xE0
#define RTC_MIN         0xE1
#define RTC_HRS         0xE2
#define RTC_DOW         0xE3
#define RTC_DOM         0xE4
#define RTC_MON         0xE5
#define RTC_YR          0xE6
#define RTC_CEN         0xE7
#define RTC_ASEC        0xE8
#define RTC_AMIN        0xE9
#define RTC_AHRS        0xEA
#define RTC_ADOW        0xEB
#define RTC_ACTRL       0xEC
#define RTC_CTRL        0xED

/* Chip Select Bus Mode Control */
#define CS0_BMC         0xF0
#define CS1_BMC         0xF1
#define CS2_BMC         0xF2
#define CS3_BMC         0xF3

/* On-Chip Flash Memory Control */
#define FLASH_KEY       0xF5
#define FLASH_DATA      0xF6
#define FLASH_ADDR_U    0xF7
#define FLASH_CTRL      0xF8
#define FLASH_FDIV      0xF9
#define FLASH_PROT      0xFA
#define FLASH_IRQ       0xFB
#define FLASH_PAGE      0xFC
#define FLASH_ROW       0xFD
#define FLASH_COL       0xFE
#define FLASH_PGCTL     0xFF

/* Unspecified register definitions, retained for compatibility */
#define RAM_CTL0        RAM_CTL

// CPU read/write values
typedef enum 
{
    REG_AF,
    REG_BC,
    REG_DE,
    REG_HL,
    REG_IX,
    REG_IY,
    REG_SP,
    REG_PC,
    SET_ADL,
    RESET_ADL,
    EXX,
    MEM
} rw_control_t;

typedef enum
{
    BREAK,
    STEP,
    RUN,
    REGONLY
} debug_state_t;

// low-level bit stream
void zdi_delay (uint8_t);
void zdi_start ();
void zdi_write_bit (bool bit);
bool zdi_read_bit ();
void zdi_register (byte regnr,bool read);
void zdi_separator (bool done_or_continue);
void zdi_write (byte value);
byte zdi_read ();

// medium-level register read and writes
byte zdi_read_register (byte regnr);
void zdi_write_register (byte regnr, byte value);
void zdi_read_registers (byte startregnr, byte count, byte* values);
void zdi_write_registers (byte startregnr, byte count, byte* values);

// high-level debugging, register and memory read functions
bool zdi_mode ();
void zdi_enter ();
void zdi_exit ();
uint8_t zdi_available_break_point();

// zdi interface functions
void zdi_debug_status (debug_state_t state);
void zdi_intel_hex (byte* memory,uint32_t start,uint16_t size);
void zdi_process_line ();
void zdi_process_cmd (uint8_t key);


#endif