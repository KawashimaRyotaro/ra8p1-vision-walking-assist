/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = adc_b_limclpi_isr, /* ADC LIMCLPI (Limiter clip interrupt with the limit table 0 to 7) */
            [1] = adc_b_err0_isr, /* ADC ERR0 (A/D converter unit 0 Error) */
            [2] = adc_b_err1_isr, /* ADC ERR1 (A/D converter unit 1 Error) */
            [3] = adc_b_resovf0_isr, /* ADC RESOVF0 (A/D conversion overflow on A/D converter unit 0) */
            [4] = adc_b_resovf1_isr, /* ADC RESOVF1 (A/D conversion overflow on A/D converter unit 1) */
            [5] = adc_b_calend0_isr, /* ADC CALEND0 (End of calibration of A/D converter unit 0) */
            [6] = adc_b_calend1_isr, /* ADC CALEND1 (End of calibration of A/D converter unit 1) */
            [7] = adc_b_adi0_isr, /* ADC ADI0 (End of A/D scanning operation(Gr.0)) */
            [8] = adc_b_fifoovf_isr, /* ADC FIFOOVF (FIFO data overflow) */
            [9] = adc_b_fiforeq0_isr, /* ADC FIFOREQ0 (FIFO data read request interrupt(Gr.0)) */
            [10] = adc_b_fiforeq1_isr, /* ADC FIFOREQ1 (FIFO data read request interrupt(Gr.1)) */
            [11] = adc_b_fiforeq2_isr, /* ADC FIFOREQ2 (FIFO data read request interrupt(Gr.2)) */
            [12] = adc_b_fiforeq3_isr, /* ADC FIFOREQ3 (FIFO data read request interrupt(Gr.3)) */
            [13] = adc_b_fiforeq4_isr, /* ADC FIFOREQ4 (FIFO data read request interrupt(Gr.4)) */
            [14] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [15] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [16] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [17] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [18] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [19] = usbfs_interrupt_handler, /* USBFS INT (USBFS interrupt) */
            [20] = usbfs_resume_handler, /* USBFS RESUME (USBFS resume interrupt) */
            [21] = usbfs_d0fifo_handler, /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
            [22] = usbfs_d1fifo_handler, /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
            [23] = usbhs_interrupt_handler, /* USBHS USB INT RESUME (USBHS interrupt) */
            [24] = usbhs_d0fifo_handler, /* USBHS FIFO 0 (DMA transfer request 0) */
            [25] = usbhs_d1fifo_handler, /* USBHS FIFO 1 (DMA transfer request 1) */
            [26] = dmac_int_isr, /* DMAC0 INT (DMAC0 transfer end) */
            [27] = dmac_int_isr, /* DMAC1 INT (DMAC1 transfer end) */
            [28] = ipc_isr, /* IPC IRQ0 (CPU Mutual Interrupt 0) */
            [29] = rm_ethosu_isr, /* NPU IRQ (NPU IRQ) */
            [30] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [31] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [32] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [33] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_ADC_LIMCLPI,GROUP0), /* ADC LIMCLPI (Limiter clip interrupt with the limit table 0 to 7) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_ADC_ERR0,GROUP1), /* ADC ERR0 (A/D converter unit 0 Error) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_ADC_ERR1,GROUP2), /* ADC ERR1 (A/D converter unit 1 Error) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_ADC_RESOVF0,GROUP3), /* ADC RESOVF0 (A/D conversion overflow on A/D converter unit 0) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_ADC_RESOVF1,GROUP4), /* ADC RESOVF1 (A/D conversion overflow on A/D converter unit 1) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_ADC_CALEND0,GROUP5), /* ADC CALEND0 (End of calibration of A/D converter unit 0) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_ADC_CALEND1,GROUP6), /* ADC CALEND1 (End of calibration of A/D converter unit 1) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_ADC_ADI0,GROUP7), /* ADC ADI0 (End of A/D scanning operation(Gr.0)) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOOVF,GROUP0), /* ADC FIFOOVF (FIFO data overflow) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOREQ0,GROUP1), /* ADC FIFOREQ0 (FIFO data read request interrupt(Gr.0)) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOREQ1,GROUP2), /* ADC FIFOREQ1 (FIFO data read request interrupt(Gr.1)) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOREQ2,GROUP3), /* ADC FIFOREQ2 (FIFO data read request interrupt(Gr.2)) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOREQ3,GROUP4), /* ADC FIFOREQ3 (FIFO data read request interrupt(Gr.3)) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_ADC_FIFOREQ4,GROUP5), /* ADC FIFOREQ4 (FIFO data read request interrupt(Gr.4)) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP6), /* IIC1 RXI (Receive data full) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP7), /* IIC1 TXI (Transmit data empty) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP0), /* IIC1 TEI (Transmit end) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP1), /* IIC1 ERI (Transfer error) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP2), /* GLCDC LINE DETECT (Specified line) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_USBFS_INT,GROUP3), /* USBFS INT (USBFS interrupt) */
            [20] = BSP_PRV_VECT_ENUM(EVENT_USBFS_RESUME,GROUP4), /* USBFS RESUME (USBFS resume interrupt) */
            [21] = BSP_PRV_VECT_ENUM(EVENT_USBFS_FIFO_0,GROUP5), /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
            [22] = BSP_PRV_VECT_ENUM(EVENT_USBFS_FIFO_1,GROUP6), /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
            [23] = BSP_PRV_VECT_ENUM(EVENT_USBHS_USB_INT_RESUME,GROUP7), /* USBHS USB INT RESUME (USBHS interrupt) */
            [24] = BSP_PRV_VECT_ENUM(EVENT_USBHS_FIFO_0,GROUP0), /* USBHS FIFO 0 (DMA transfer request 0) */
            [25] = BSP_PRV_VECT_ENUM(EVENT_USBHS_FIFO_1,GROUP1), /* USBHS FIFO 1 (DMA transfer request 1) */
            [26] = BSP_PRV_VECT_ENUM(EVENT_DMAC0_INT,GROUP2), /* DMAC0 INT (DMAC0 transfer end) */
            [27] = BSP_PRV_VECT_ENUM(EVENT_DMAC1_INT,GROUP3), /* DMAC1 INT (DMAC1 transfer end) */
            [28] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ0,GROUP4), /* IPC IRQ0 (CPU Mutual Interrupt 0) */
            [29] = BSP_PRV_VECT_ENUM(EVENT_NPU_IRQ,GROUP5), /* NPU IRQ (NPU IRQ) */
            [30] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP6), /* VIN IRQ (Interrupt Request) */
            [31] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP7), /* MIPICSI RX (Receive interrupt) */
            [32] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,FIXED), /* MIPICSI DL (Data Lane interrupt) */
            [33] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,FIXED), /* MIPICSI VC (Virtual Channel interrupt) */
        };
        #endif
        #endif
