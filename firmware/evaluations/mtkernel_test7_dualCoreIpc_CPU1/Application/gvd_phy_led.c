#include "hal_data.h"
#include "gvd_phy_led.h"

/* EK-RA8P1 v1: LED6 = GPY111 LED2, LED7 = GPY111 LED0.
 * P414/P415 are already ET1_MDIO/ET1_MDC and P708 releases PHY reset in
 * the CPU0 pin configuration. RMAC1 manages only the PHY registers here;
 * no Ethernet frames, generated pin changes, or extra IPC are needed.
 * MDIO setup follows Renesas FSP 6.5.1 r_layer3_switch/r_rmac_phy;
 * LED register 0x1b and bits follow the GPY111 hardware description.
 */
#define LED67_PHY_LED_REG       (0x1bU)
#define LED67_PHY_ID1_REG       (2U)
#define LED67_PHY_ID2_REG       (3U)
#define LED67_PHY_ID1          (0xd565U)
#define LED67_PHY_ID2_MASK     (0xfff0U)
#define LED67_PHY_ID2_VALUE    (0xa400U)
#define LED67_EN_MASK          ((1U << 10) | (1U << 8))
#define LED67_DATA_MASK        ((1U << 2) | (1U << 0))
#define LED67_MDC_HZ           (2500000U)
#define LED67_WAIT_POLLS       (100000U)

volatile uint32_t g_cpu1_led67_ready;
volatile uint32_t g_cpu1_led67_error;
volatile uint32_t g_cpu1_led67_phy_address = UINT32_MAX;
volatile uint32_t g_cpu1_led67_mask;
static uint16_t s_led_control;

static uint32_t mdio_wait(void)
{
    for (uint32_t i = 0; i < LED67_WAIT_POLLS; i++) {
        if (!R_RMAC1->MPSM_b.PSME) { return 1U; }
    }
    g_cpu1_led67_error = 6U;
    return 0U;
}

static uint32_t mdio_read(uint32_t phy, uint32_t reg, uint16_t *value)
{
    if (!mdio_wait()) { return 0U; }
    R_RMAC1->MPSM = (2U << R_RMAC0_MPSM_POP_Pos) |
                    (phy << R_RMAC0_MPSM_PDA_Pos) |
                    (reg << R_RMAC0_MPSM_PRA_Pos);
    R_RMAC1->MPSM_b.PSME = 1U;
    if (!mdio_wait()) { return 0U; }
    *value = (uint16_t) R_RMAC1->MPSM_b.PRD;
    return 1U;
}

static uint32_t mdio_write(uint32_t phy, uint32_t reg, uint16_t value)
{
    if (!mdio_wait()) { return 0U; }
    R_RMAC1->MPSM = (1U << R_RMAC0_MPSM_POP_Pos) |
                    (phy << R_RMAC0_MPSM_PDA_Pos) |
                    (reg << R_RMAC0_MPSM_PRA_Pos) |
                    ((uint32_t) value << R_RMAC0_MPSM_PRD_Pos);
    R_RMAC1->MPSM_b.PSME = 1U;
    return mdio_wait();
}

static uint32_t wait_etha(uint32_t mode)
{
    for (uint32_t i = 0; i < LED67_WAIT_POLLS; i++) {
        if (R_ETHA1->EAMS_b.OPS == mode) { return 1U; }
    }
    g_cpu1_led67_error = 4U;
    return 0U;
}

uint32_t gvd_phy_led_init(void)
{
    uint16_t id1, id2, check;
    g_cpu1_led67_ready = 0U;
    g_cpu1_led67_error = 0U;
    g_cpu1_led67_phy_address = UINT32_MAX;
    g_cpu1_led67_mask = 0U;

    /* Minimum ESWM power/clock sequence from FSP r_layer3_switch. */
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
    if (!R_SYSTEM->PDCTRESWM_b.PDCSF && R_SYSTEM->PDCTRESWM_b.PDPGSF) {
        R_SYSTEM->PDCTRESWM_b.PDDE = 0U;
    }
    uint32_t powered = 0U;
    for (uint32_t i = 0; i < LED67_WAIT_POLLS; i++) {
        if (!R_SYSTEM->PDCTRESWM_b.PDCSF && !R_SYSTEM->PDCTRESWM_b.PDPGSF) {
            powered = 1U; break;
        }
    }
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);
    if (!powered) { g_cpu1_led67_error = 1U; return 0U; }

    R_MSTP->MSTPCRC_b.MSTPC28 = 0U;
    R_MSTP->MSTPCRC_b.MSTPC30 = 0U;
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
    R_COMA->RRC_b.RR = 1U;
    R_COMA->RRC_b.RR = 0U;
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    R_COMA->RCEC_b.RCE = 1U;
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    R_COMA->CABPIRM_b.BPIOG = 1U;
    uint32_t buffer_ready = 0U;
    for (uint32_t i = 0; i < LED67_WAIT_POLLS; i++) {
        if (R_COMA->CABPIRM_b.BPR) { buffer_ready = 1U; break; }
    }
    if (!buffer_ready) { g_cpu1_led67_error = 2U; return 0U; }
    R_COMA->RCEC = R_COMA_RCEC_RCE_Msk | R_COMA_RCEC_ACE_Msk;

    R_ETHA1->EAMC_b.OPC = 1U; /* DISABLE, then CONFIG. */
    if (!wait_etha(1U)) { return 0U; }
    R_ETHA1->EAMC_b.OPC = 2U;
    if (!wait_etha(2U)) { return 0U; }

    uint32_t esw_hz = R_BSP_SourceClockHzGet((fsp_priv_source_clock_t) R_SYSTEM->ESWCKCR_b.CKSEL) /
                      R_FSP_ClockDividerGet(R_SYSTEM->ESWCKDIVCR_b.CKDIV);
    uint32_t half_cycles = (esw_hz / LED67_MDC_HZ) / 2U;
    if (half_cycles < 2U || half_cycles > 128U) {
        g_cpu1_led67_error = 3U; return 0U;
    }
    R_ESWM->MIICR1 = R_ESWM_MIICR1_TXCIDE_Msk | 1U; /* Existing RGMII1 pins. */
    R_ESWM->MIIRR |= (1U << 1);
    R_RMAC1->MPIC = ((half_cycles - 1U) << R_RMAC0_MPIC_PSMCS_Pos) |
                    (2U << R_RMAC0_MPIC_PIS_Pos) |
                    (2U << R_RMAC0_MPIC_LSC_Pos);
    R_ETHA1->EAMC_b.OPC = 1U;
    if (!wait_etha(1U)) { return 0U; }
    R_ETHA1->EAMC_b.OPC = 3U; /* OPERATION permits MDIO writes. */
    if (!wait_etha(3U)) { return 0U; }

    /* The board has one GPY111. Discover its strapped MDIO address. */
    for (uint32_t phy = 0U; phy < 32U; phy++) {
        if (!mdio_read(phy, LED67_PHY_ID1_REG, &id1)) { return 0U; }
        if (id1 != LED67_PHY_ID1) { continue; }
        if (!mdio_read(phy, LED67_PHY_ID2_REG, &id2)) { return 0U; }
        if ((id2 & LED67_PHY_ID2_MASK) == LED67_PHY_ID2_VALUE) {
            g_cpu1_led67_phy_address = phy; break;
        }
    }
    if (g_cpu1_led67_phy_address == UINT32_MAX) {
        g_cpu1_led67_error = 5U; return 0U;
    }

    if (!mdio_read(g_cpu1_led67_phy_address, LED67_PHY_LED_REG, &s_led_control)) { return 0U; }
    s_led_control = (uint16_t) (s_led_control & ~(LED67_EN_MASK | LED67_DATA_MASK));
    if (!mdio_write(g_cpu1_led67_phy_address, LED67_PHY_LED_REG, s_led_control) ||
        !mdio_read(g_cpu1_led67_phy_address, LED67_PHY_LED_REG, &check) ||
        (check & (LED67_EN_MASK | LED67_DATA_MASK))) {
        if (!g_cpu1_led67_error) { g_cpu1_led67_error = 7U; }
        return 0U;
    }
    g_cpu1_led67_ready = 1U;
    return 1U;
}

void gvd_phy_led_set(gvd_cue_t cue)
{
    if (!g_cpu1_led67_ready) { return; }
    uint32_t mask = (cue == GVD_CUE_LEFT) ? (1U << 2) :
                    (cue == GVD_CUE_RIGHT) ? (1U << 0) : 0U;
    if (mask == g_cpu1_led67_mask) { return; }
    uint16_t value = (uint16_t) ((s_led_control & ~LED67_DATA_MASK) | mask);
    if (!mdio_write(g_cpu1_led67_phy_address, LED67_PHY_LED_REG, value)) {
        g_cpu1_led67_ready = 0U;
        return;
    }
    s_led_control = value;
    g_cpu1_led67_mask = mask;
}
