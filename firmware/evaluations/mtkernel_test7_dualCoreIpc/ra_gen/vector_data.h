/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (34)
#endif
/* ISR prototypes */
void adc_b_limclpi_isr(void);
void adc_b_err0_isr(void);
void adc_b_err1_isr(void);
void adc_b_resovf0_isr(void);
void adc_b_resovf1_isr(void);
void adc_b_calend0_isr(void);
void adc_b_calend1_isr(void);
void adc_b_adi0_isr(void);
void adc_b_fifoovf_isr(void);
void adc_b_fiforeq0_isr(void);
void adc_b_fiforeq1_isr(void);
void adc_b_fiforeq2_isr(void);
void adc_b_fiforeq3_isr(void);
void adc_b_fiforeq4_isr(void);
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void glcdc_line_detect_isr(void);
void usbfs_interrupt_handler(void);
void usbfs_resume_handler(void);
void usbfs_d0fifo_handler(void);
void usbfs_d1fifo_handler(void);
void usbhs_interrupt_handler(void);
void usbhs_d0fifo_handler(void);
void usbhs_d1fifo_handler(void);
void dmac_int_isr(void);
void ipc_isr(void);
void rm_ethosu_isr(void);
void vin_status_isr(void);
void mipi_csi_rx_isr(void);
void mipi_csi_dl_isr(void);
void mipi_csi_vc_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_ADC_LIMCLPI ((IRQn_Type) 0) /* ADC LIMCLPI (Limiter clip interrupt with the limit table 0 to 7) */
#define ADC_LIMCLPI_IRQn          ((IRQn_Type) 0) /* ADC LIMCLPI (Limiter clip interrupt with the limit table 0 to 7) */
#define VECTOR_NUMBER_ADC_ERR0 ((IRQn_Type) 1) /* ADC ERR0 (A/D converter unit 0 Error) */
#define ADC_ERR0_IRQn          ((IRQn_Type) 1) /* ADC ERR0 (A/D converter unit 0 Error) */
#define VECTOR_NUMBER_ADC_ERR1 ((IRQn_Type) 2) /* ADC ERR1 (A/D converter unit 1 Error) */
#define ADC_ERR1_IRQn          ((IRQn_Type) 2) /* ADC ERR1 (A/D converter unit 1 Error) */
#define VECTOR_NUMBER_ADC_RESOVF0 ((IRQn_Type) 3) /* ADC RESOVF0 (A/D conversion overflow on A/D converter unit 0) */
#define ADC_RESOVF0_IRQn          ((IRQn_Type) 3) /* ADC RESOVF0 (A/D conversion overflow on A/D converter unit 0) */
#define VECTOR_NUMBER_ADC_RESOVF1 ((IRQn_Type) 4) /* ADC RESOVF1 (A/D conversion overflow on A/D converter unit 1) */
#define ADC_RESOVF1_IRQn          ((IRQn_Type) 4) /* ADC RESOVF1 (A/D conversion overflow on A/D converter unit 1) */
#define VECTOR_NUMBER_ADC_CALEND0 ((IRQn_Type) 5) /* ADC CALEND0 (End of calibration of A/D converter unit 0) */
#define ADC_CALEND0_IRQn          ((IRQn_Type) 5) /* ADC CALEND0 (End of calibration of A/D converter unit 0) */
#define VECTOR_NUMBER_ADC_CALEND1 ((IRQn_Type) 6) /* ADC CALEND1 (End of calibration of A/D converter unit 1) */
#define ADC_CALEND1_IRQn          ((IRQn_Type) 6) /* ADC CALEND1 (End of calibration of A/D converter unit 1) */
#define VECTOR_NUMBER_ADC_ADI0 ((IRQn_Type) 7) /* ADC ADI0 (End of A/D scanning operation(Gr.0)) */
#define ADC_ADI0_IRQn          ((IRQn_Type) 7) /* ADC ADI0 (End of A/D scanning operation(Gr.0)) */
#define VECTOR_NUMBER_ADC_FIFOOVF ((IRQn_Type) 8) /* ADC FIFOOVF (FIFO data overflow) */
#define ADC_FIFOOVF_IRQn          ((IRQn_Type) 8) /* ADC FIFOOVF (FIFO data overflow) */
#define VECTOR_NUMBER_ADC_FIFOREQ0 ((IRQn_Type) 9) /* ADC FIFOREQ0 (FIFO data read request interrupt(Gr.0)) */
#define ADC_FIFOREQ0_IRQn          ((IRQn_Type) 9) /* ADC FIFOREQ0 (FIFO data read request interrupt(Gr.0)) */
#define VECTOR_NUMBER_ADC_FIFOREQ1 ((IRQn_Type) 10) /* ADC FIFOREQ1 (FIFO data read request interrupt(Gr.1)) */
#define ADC_FIFOREQ1_IRQn          ((IRQn_Type) 10) /* ADC FIFOREQ1 (FIFO data read request interrupt(Gr.1)) */
#define VECTOR_NUMBER_ADC_FIFOREQ2 ((IRQn_Type) 11) /* ADC FIFOREQ2 (FIFO data read request interrupt(Gr.2)) */
#define ADC_FIFOREQ2_IRQn          ((IRQn_Type) 11) /* ADC FIFOREQ2 (FIFO data read request interrupt(Gr.2)) */
#define VECTOR_NUMBER_ADC_FIFOREQ3 ((IRQn_Type) 12) /* ADC FIFOREQ3 (FIFO data read request interrupt(Gr.3)) */
#define ADC_FIFOREQ3_IRQn          ((IRQn_Type) 12) /* ADC FIFOREQ3 (FIFO data read request interrupt(Gr.3)) */
#define VECTOR_NUMBER_ADC_FIFOREQ4 ((IRQn_Type) 13) /* ADC FIFOREQ4 (FIFO data read request interrupt(Gr.4)) */
#define ADC_FIFOREQ4_IRQn          ((IRQn_Type) 13) /* ADC FIFOREQ4 (FIFO data read request interrupt(Gr.4)) */
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 14) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 14) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 15) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 15) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 16) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 16) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 17) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 17) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 18) /* GLCDC LINE DETECT (Specified line) */
#define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 18) /* GLCDC LINE DETECT (Specified line) */
#define VECTOR_NUMBER_USBFS_INT ((IRQn_Type) 19) /* USBFS INT (USBFS interrupt) */
#define USBFS_INT_IRQn          ((IRQn_Type) 19) /* USBFS INT (USBFS interrupt) */
#define VECTOR_NUMBER_USBFS_RESUME ((IRQn_Type) 20) /* USBFS RESUME (USBFS resume interrupt) */
#define USBFS_RESUME_IRQn          ((IRQn_Type) 20) /* USBFS RESUME (USBFS resume interrupt) */
#define VECTOR_NUMBER_USBFS_FIFO_0 ((IRQn_Type) 21) /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
#define USBFS_FIFO_0_IRQn          ((IRQn_Type) 21) /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
#define VECTOR_NUMBER_USBFS_FIFO_1 ((IRQn_Type) 22) /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
#define USBFS_FIFO_1_IRQn          ((IRQn_Type) 22) /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
#define VECTOR_NUMBER_USBHS_USB_INT_RESUME ((IRQn_Type) 23) /* USBHS USB INT RESUME (USBHS interrupt) */
#define USBHS_USB_INT_RESUME_IRQn          ((IRQn_Type) 23) /* USBHS USB INT RESUME (USBHS interrupt) */
#define VECTOR_NUMBER_USBHS_FIFO_0 ((IRQn_Type) 24) /* USBHS FIFO 0 (DMA transfer request 0) */
#define USBHS_FIFO_0_IRQn          ((IRQn_Type) 24) /* USBHS FIFO 0 (DMA transfer request 0) */
#define VECTOR_NUMBER_USBHS_FIFO_1 ((IRQn_Type) 25) /* USBHS FIFO 1 (DMA transfer request 1) */
#define USBHS_FIFO_1_IRQn          ((IRQn_Type) 25) /* USBHS FIFO 1 (DMA transfer request 1) */
#define VECTOR_NUMBER_DMAC0_INT ((IRQn_Type) 26) /* DMAC0 INT (DMAC0 transfer end) */
#define DMAC0_INT_IRQn          ((IRQn_Type) 26) /* DMAC0 INT (DMAC0 transfer end) */
#define VECTOR_NUMBER_DMAC1_INT ((IRQn_Type) 27) /* DMAC1 INT (DMAC1 transfer end) */
#define DMAC1_INT_IRQn          ((IRQn_Type) 27) /* DMAC1 INT (DMAC1 transfer end) */
#define VECTOR_NUMBER_IPC_IRQ0 ((IRQn_Type) 28) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
#define IPC_IRQ0_IRQn          ((IRQn_Type) 28) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
#define VECTOR_NUMBER_NPU_IRQ ((IRQn_Type) 29) /* NPU IRQ (NPU IRQ) */
#define NPU_IRQ_IRQn          ((IRQn_Type) 29) /* NPU IRQ (NPU IRQ) */
#define VECTOR_NUMBER_VIN_IRQ ((IRQn_Type) 30) /* VIN IRQ (Interrupt Request) */
#define VIN_IRQ_IRQn          ((IRQn_Type) 30) /* VIN IRQ (Interrupt Request) */
#define VECTOR_NUMBER_MIPICSI_RX ((IRQn_Type) 31) /* MIPICSI RX (Receive interrupt) */
#define MIPICSI_RX_IRQn          ((IRQn_Type) 31) /* MIPICSI RX (Receive interrupt) */
#define VECTOR_NUMBER_MIPICSI_DL ((IRQn_Type) 32) /* MIPICSI DL (Data Lane interrupt) */
#define MIPICSI_DL_IRQn          ((IRQn_Type) 32) /* MIPICSI DL (Data Lane interrupt) */
#define VECTOR_NUMBER_MIPICSI_VC ((IRQn_Type) 33) /* MIPICSI VC (Virtual Channel interrupt) */
#define MIPICSI_VC_IRQn          ((IRQn_Type) 33) /* MIPICSI VC (Virtual Channel interrupt) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (34)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
