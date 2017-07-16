/**
 * @file install_driver.h
 * @brief A header file contains gate structures, function declarations for
 *        install_handler.c
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_INSTALL_DRIVER
#define H_INSTALL_DRIVER

#define IDT_ENTRY_LENGTH 8

/* Iterrupt descriptor table gate constants definition */
/* Fild length definition */
#define SEGMENT_OFFSET_BITS             16
#define SEGMENT_SELECTOR_BITS           16
#define RESERVED_BITS                   5
#define ZEROS_BITS                      3
#define GATE_TYPE_BITS                  3
#define SIZE_BITS                       1
#define ZERO_BITS                       1
#define DESCRIPTOR_PRIVILEGE_LEVEL_BITS 2
#define SEGEMENT_PRESENT_FLAG_BITS      1

/* Fixed values */
#define RESERVED    0 /* 00000 */
#define ZEROS       0 /* 000 */
#define GATE_TYPE   7 /* 111 for trap gate */
#define D           1 /* 1 for 32 bit size */
#define DPL         0 /* 0 for 0 ring */
#define P           1 /* 1 for a working gate */

struct idt_gates {
    /*!< the lower 16bit address of interrupt handler */
unsigned int lsb_offset:    SEGMENT_OFFSET_BITS;
unsigned int seg_sel:       SEGMENT_SELECTOR_BITS; /*!< segement selector */
unsigned int reserved:      RESERVED_BITS; /*!< reserved for future use */
unsigned int zeros:         ZEROS_BITS; /*!< all zeros */
unsigned int gate_type:     GATE_TYPE_BITS; /*!< use trap gate */
unsigned int d:             SIZE_BITS; /*!< 1 size of gate */
unsigned int zero:          ZERO_BITS; /*!< zero */
    /*!< descriptor privilege level */
unsigned int dpl:           DESCRIPTOR_PRIVILEGE_LEVEL_BITS;
unsigned int p:             SEGEMENT_PRESENT_FLAG_BITS; /*!< zero */
    /*!< the higher 16bit address of interrupt handler */
unsigned int msb_offset:    SEGMENT_OFFSET_BITS;
};


/* function declarations */
/**
 * @brief Install the timer driver and keyboard driver.
 * @param tickback the address of callback function.
 */
int handler_install(void (*tickback)(unsigned int));

/**
 * @brief Add trap gate to the corresponding IDT entery.
 * @param idt_gate_index the index of the corresponding IDT entery.
 * @param handler_addr   the address of interrupt handler.
 */
void add_to_idt_gate(int idt_gate_index, void *handler_addr);

/**
 * @brief Fill data (like handler address) to the gate entry.
 * @param idt_gate     the pointer to the trap gate structure.
 * @param handler_addr the address of the handler.
 */
void fill_gate(struct idt_gates *idt_gate, void *handler_addr);

#endif