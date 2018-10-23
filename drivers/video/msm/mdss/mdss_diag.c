/* drivers/video/msm/mdss/mdss_diag.c  (Display Driver)
 *
 * Copyright (C) 2014 SHARP CORPORATION
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <mdss_shdisp.h>
#include <mdss_diag.h>
#include <linux/types.h>
#include <sharp/shdisp_kerl.h>
#include <sharp/shdisp_dsi.h>
#include "mdss_fb.h"
#include <mdss_dsi.h>
#include <mdss_mdp.h>
#include <linux/iopoll.h>

#ifndef CONFIG_USES_SHLCDC
#define MDSS_DIAG_MIPI_CHECK_AMP_OFF		(0x0780)
#define MDSS_DIAG_WAIT_1FRAME_US		(16666)

static uint8_t mdss_diag_mipi_check_amp_data;
static uint8_t mdss_diag_mipi_check_rec_sens_data;
static int mdss_diag_mipi_check_exec_state = false;

static int mdss_diag_mipi_check_exec(uint8_t flame_cnt, uint8_t amp, uint8_t sensitiv, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_mipi_check_manual(struct mdp_mipi_check_param *mipi_check_param, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_mipi_check_auto(struct mdp_mipi_check_param *mipi_check_param, struct mdss_dsi_ctrl_pdata *ctrl);
static void mdss_diag_mipi_check_set_param(uint8_t amp, uint8_t sensitiv, struct mdss_dsi_ctrl_pdata *ctrl);
static void mdss_diag_mipi_check_get_param(uint8_t *amp, uint8_t *sensitiv, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_mipi_check_test(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_mipi_check_test_video(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_mipi_check_test_cmd(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_read_sensitiv(uint8_t *read_data);
static int mdss_diag_dsi_cmd_bta_sw_trigger(struct mdss_dsi_ctrl_pdata *ctrl);
static int mdss_diag_dsi_ack_err_status(struct mdss_dsi_ctrl_pdata *ctrl);
static void mdss_diag_mipi_check_result_convert(
			uint8_t befoe[MDSS_MIPICHK_SENSITIV_NUM][MDSS_MIPICHK_AMP_NUM],
			struct mdp_mipi_check_param *mipi_check_param);
static int mdss_diag_mipi_clkchg_setparam(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl);
static void mdss_diag_mipi_clkchg_host_data(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_panel_data *pdata);
static int mdss_diag_mipi_clkchg_host(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl);
static int mdss_diag_mipi_clkchg_panel_clk_update(struct mdss_panel_data *pdata);
static int mdss_diag_mipi_clkchg_panel_clk_data(struct mdss_panel_data *pdata);
static int mdss_diag_mipi_clkchg_panel_porch_update(struct mdss_panel_data *pdata);
static int mdss_diag_mipi_clkchg_panel(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl);
static void mdss_diag_mipi_clkchg_param_log(struct mdp_mipi_clkchg_param *mdp_mipi_clkchg_param);

extern int shdisp_api_set_freq_param(mdp_mipi_clkchg_panel_t *freq);
#ifndef SHDISP_DET_DSI_MIPI_ERROR
extern void mdss_dsi_err_intr_ctrl(struct mdss_dsi_ctrl_pdata *ctrl, u32 mask,int enable);
#endif /* SHDISP_DET_DSI_MIPI_ERROR */
extern void mdss_shdisp_video_transfer_ctrl(int onoff, int commit);
extern int mdss_shdisp_host_dsi_tx(int commit,
		struct shdisp_dsi_cmd_desc *shdisp_cmds, int size);
extern int mdss_shdisp_host_dsi_rx(struct shdisp_dsi_cmd_desc *cmds,
		unsigned char *rx_data, int rx_size);
extern void mdss_dsi_pll_relock(struct mdss_dsi_ctrl_pdata *ctrl);
#ifndef SHDISP_DISABLE_HR_VIDEO
extern int mdss_mdp_hr_video_suspend(struct mdss_mdp_ctl *ctl, int tg_en_flg);
extern int mdss_mdp_hr_video_resume(struct mdss_mdp_ctl *ctl, int tg_en_flg);
extern int mdss_mdp_hr_video_clkchg_mdp_update(struct mdss_mdp_ctl *ctl);
#else  /* SHDISP_DISABLE_HR_VIDEO */
extern int mdss_mdp_video_clkchg_mdp_update(struct mdss_mdp_ctl *ctl);
#endif /* SHDISP_DISABLE_HR_VIDEO */
#endif /* CONFIG_USES_SHLCDC */

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int mdss_diag_mipi_check_get_exec_state(void)
{
#ifdef CONFIG_USES_SHLCDC
	return false;
#else  /* CONFIG_USES_SHLCDC */
	return mdss_diag_mipi_check_exec_state;
#endif /* CONFIG_USES_SHLCDC */
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int mdss_diag_mipi_check(struct mdp_mipi_check_param *mipi_check_param,
			 struct mdss_panel_data *pdata)
{
#ifdef CONFIG_USES_SHLCDC
	return 0;
#else  /* CONFIG_USES_SHLCDC */
	int ret;
	u32 isr;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_debug("%s: called\n", __func__);
	if (!ctrl_pdata) {
		pr_err("LCDERR: %s ctrl_pdata=0x%pK", __func__, ctrl_pdata);
		return -ENXIO;
	}

#ifndef SHDISP_DET_DSI_MIPI_ERROR
	/* disable dsi error interrupt */
	mdss_dsi_err_intr_ctrl(ctrl_pdata, DSI_INTR_ERROR_MASK, 0);
#endif /* SHDISP_DET_DSI_MIPI_ERROR */
	mdss_diag_mipi_check_exec_state = true;

	mdss_diag_dsi_cmd_bta_sw_trigger(ctrl_pdata);

	mdss_diag_mipi_check_get_param(&mdss_diag_mipi_check_amp_data, &mdss_diag_mipi_check_rec_sens_data, ctrl_pdata);

	if (ctrl_pdata->panel_mode == DSI_VIDEO_MODE) {
		mdss_shdisp_video_transfer_ctrl(false, false);
	}

	if (mipi_check_param->mode == MDSS_MIPICHK_MANUAL) {
		ret = mdss_diag_mipi_check_manual(mipi_check_param, ctrl_pdata);
	} else if (mipi_check_param->mode == MDSS_MIPICHK_AUTO) {
		ret = mdss_diag_mipi_check_auto(mipi_check_param, ctrl_pdata);
	} else {
		pr_err("%s:mode=%d\n", __func__, mipi_check_param->mode);
		return -EINVAL;
	}

	mdss_diag_mipi_check_set_param(mdss_diag_mipi_check_amp_data, mdss_diag_mipi_check_rec_sens_data, ctrl_pdata);

	if (ctrl_pdata->panel_mode == DSI_VIDEO_MODE) {
		mdss_shdisp_video_transfer_ctrl(true, true);
	}

	mdss_diag_mipi_check_exec_state = false;

	/* MMSS_DSI_0_INT_CTRL */
	isr = MIPI_INP(ctrl_pdata->ctrl_base + 0x0110);
	MIPI_OUTP(ctrl_pdata->ctrl_base + 0x0110, isr);

#ifndef SHDISP_DET_DSI_MIPI_ERROR
	/* enable dsi error interrupt */
	mdss_dsi_err_intr_ctrl(ctrl_pdata, DSI_INTR_ERROR_MASK, 1);
#endif /* SHDISP_DET_DSI_MIPI_ERROR */

	pr_debug("%s: end", __func__);

	return ret;
#endif /* CONFIG_USES_SHLCDC */
}

#ifndef CONFIG_USES_SHLCDC
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_manual(struct mdp_mipi_check_param *mipi_check_param, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;

	pr_debug("%s: called\n", __func__);

	if ((mipi_check_param->amp & ~0x07) != 0) {
		pr_err("LCDERR: %s AMP=0x%X Out of range", __func__, mipi_check_param->amp);
		return -ENXIO;
	}

	if ((mipi_check_param->sensitiv & ~0x0F) != 0) {
		pr_err("LCDERR: %s SENSITIV=0x%X Out of range", __func__, mipi_check_param->amp);
		return -ENXIO;
	}

	ret = mdss_diag_mipi_check_exec(mipi_check_param->flame_cnt, mipi_check_param->amp, mipi_check_param->sensitiv, ctrl);

	mipi_check_param->result[0] = ret;

	pr_debug("%s: end", __func__);

	return 0;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_auto(struct mdp_mipi_check_param *mipi_check_param, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	int ret2 = 0;
	uint8_t i,j;
	uint8_t result_temp[MDSS_MIPICHK_SENSITIV_NUM][MDSS_MIPICHK_AMP_NUM]={{0}};
	uint8_t set_flame		= 0x01;
	uint8_t max_amp			= 0x07;
	uint8_t max_sensitiv	= 0x0F;

	pr_debug("%s: called\n", __func__);


	for (i = 0; i < MDSS_MIPICHK_SENSITIV_NUM; i++) {
		for (j = 0; j < MDSS_MIPICHK_AMP_NUM; j++) {
			ret = mdss_diag_mipi_check_exec(mipi_check_param->flame_cnt, j, i, ctrl);
			if (ret == MDSS_MIPICHK_RESULT_NG) {
				ret2 = mdss_diag_mipi_check_exec(set_flame, max_amp, max_sensitiv, ctrl);
				if (ret2 == MDSS_MIPICHK_RESULT_NG) {
					pr_err("LCDERR: %s mdss_diag_mipi_check_exec ret=%d", __func__, ret);
				}
			}
			result_temp[i][j] = ret;
		}
	}

	mdss_diag_mipi_check_result_convert(result_temp, mipi_check_param);

	pr_debug("%s: end", __func__);

	return 0;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void mdss_diag_mipi_check_result_convert(
			uint8_t befoe_result[MDSS_MIPICHK_SENSITIV_NUM][MDSS_MIPICHK_AMP_NUM],
			struct mdp_mipi_check_param *mipi_check_param)
{
	uint8_t i,j,x,y;
	uint8_t after_result[MDSS_MIPICHK_SENSITIV_NUM] = {0};

	for (j = 0; j < MDSS_MIPICHK_AMP_NUM; j++) {
		for (i = 0; i < MDSS_MIPICHK_SENSITIV_NUM; i++) {
			if(befoe_result[i][j] == MDSS_MIPICHK_RESULT_OK) {
				x = j * 2;
				if (i >= 8) {
					y = i - 8;
					x++;
				} else {
					y = i;
				}
				after_result[x] |= (1 << (7-y));
			}
		}
	}
	memcpy(mipi_check_param->result, after_result, sizeof(after_result));
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_exec(uint8_t flame_cnt, uint8_t amp, uint8_t sensitiv, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	u32 amp_reg;
	u32 amp_tmp;
	uint8_t set_sensitiv = 0;

	uint8_t amp_tbl[MDSS_MIPICHK_AMP_NUM] = {
		0x03,
		0x02,
		0x00,
		0x01,
		0x04,
		0x05,
		0x06,
		0x07
	};

	pr_debug("%s: called flame_cnt=0x%02X amp=0x%02X sensitiv=0x%02X\n", __func__, flame_cnt, amp, sensitiv);

	amp_tmp = amp_tbl[amp];
	amp_reg = (amp_tmp << 1) | 1;

#if defined(CONFIG_SHDISP_PANEL_ANDY)
		set_sensitiv = (sensitiv << 4);
#elif defined(CONFIG_SHDISP_PANEL_ARIA)
		set_sensitiv = (sensitiv << 3);
#endif  /* CONFIG_SHDISP_PANEL_ANDY || CONFIG_SHDISP_PANEL_ARIA */

	mdss_diag_mipi_check_set_param(amp_reg, set_sensitiv, ctrl);

	ret = mdss_diag_mipi_check_test(flame_cnt, ctrl);

	pr_debug("%s: end ret=%d \n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void mdss_diag_mipi_check_set_param(uint8_t amp, uint8_t sensitiv, struct mdss_dsi_ctrl_pdata *ctrl)
{
	char payload_sensitiv[2][2] = {
		{0xFF, 0x00},
		{0x45, 0x00},
	};

	struct shdisp_dsi_cmd_desc cmds_sensitiv[] = {
		{SHDISP_DTYPE_DCS_WRITE1, 2, payload_sensitiv[0]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, payload_sensitiv[1]},
	};

	/* MMSS_DSI_0_PHY_REG_DSIPHY_REGULATOR_CTRL_0 */
	MIPI_OUTP((ctrl->ctrl_base) + MDSS_DIAG_MIPI_CHECK_AMP_OFF, amp);
	wmb();

#if defined(CONFIG_SHDISP_PANEL_ANDY)
	payload_sensitiv[0][1] = 0xEE;
	sensitiv |= (mdss_diag_mipi_check_rec_sens_data & 0x0F);
#elif defined(CONFIG_SHDISP_PANEL_ARIA)
	payload_sensitiv[0][1] = 0xE0;
	sensitiv |= (mdss_diag_mipi_check_rec_sens_data & 0x87);
#endif  /* CONFIG_SHDISP_PANEL_ANDY || CONFIG_SHDISP_PANEL_ARIA */
	payload_sensitiv[1][1] = sensitiv;

	mdss_shdisp_host_dsi_tx(1, cmds_sensitiv, ARRAY_SIZE(cmds_sensitiv));
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void mdss_diag_mipi_check_get_param(uint8_t *amp, uint8_t *sensitiv, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	/* MMSS_DSI_0_PHY_REG_DSIPHY_REGULATOR_CTRL_0 */
	*amp = MIPI_INP((ctrl->ctrl_base) + MDSS_DIAG_MIPI_CHECK_AMP_OFF);

	ret = mdss_diag_read_sensitiv(sensitiv);

	pr_debug("%s: amp=0x%02X sensitiv=0x%02X\n", __func__, *amp, *sensitiv);

}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_test(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	char mode;

	mode = ctrl->panel_mode;

	if (mode == DSI_VIDEO_MODE) {
		ret = mdss_diag_mipi_check_test_video(flame_cnt, ctrl);
	} else if (mode == DSI_CMD_MODE) {
		ret = mdss_diag_mipi_check_test_cmd(flame_cnt, ctrl);
	} else {
		pr_err("LCDERR: %s paneltype=%d\n", __func__, mode);
		ret = -EINVAL;
	}

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_test_video(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	uint32_t sleep;

	sleep = flame_cnt * MDSS_DIAG_WAIT_1FRAME_US;
	pr_debug("%s: flame=%d sleep time=%d\n", __func__, flame_cnt, sleep);

	mdss_shdisp_video_transfer_ctrl(true, true);

	usleep(sleep);

	ret = mdss_diag_dsi_cmd_bta_sw_trigger(ctrl);
	if (ret) {
		ret = MDSS_MIPICHK_RESULT_NG;
	} else {
		ret = MDSS_MIPICHK_RESULT_OK;
	}

	mdss_shdisp_video_transfer_ctrl(false, false);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_check_test_cmd(uint8_t flame_cnt, struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	int ret2 = 0;
	int i;
	struct mdss_mdp_ctl *pctl;

	pctl = mdss_shdisp_get_mdpctrl(0);
	if (!pctl) {
		pr_err("LCDERR:[%s] mdpctrl is NULL.\n", __func__);
		return MDSS_MIPICHK_RESULT_NG;
	}

	if (!pctl->display_fnc) {
		pr_err("LCDERR:[%s] display_fnc is NULL.\n", __func__);
		return MDSS_MIPICHK_RESULT_NG;
	}

	for (i = 0; i < flame_cnt; i++) {
		mutex_lock(&pctl->lock);
		mdss_mdp_ctl_perf_set_transaction_status(pctl, PERF_SW_COMMIT_STATE, PERF_STATUS_BUSY);

		mdss_mdp_ctl_perf_update_ctl(pctl, 1);

		if (pctl->wait_pingpong) {
			ret2 = pctl->wait_pingpong(pctl, NULL);
			if(ret2){
				pr_err("LCDERR:[%s] failed to wait_pingpong(). (ret=%d)\n", __func__, ret2);
			}
		}

		ret = pctl->display_fnc(pctl, NULL);
		if (ret) {
			pr_err("LCDERR:[%s] failed to display_fnc(). (ret=%d)\n", __func__, ret);
			mutex_unlock(&pctl->lock);
			return MDSS_MIPICHK_RESULT_NG;
		}
		mutex_unlock(&pctl->lock);
	}

	if (pctl->wait_pingpong) {
		ret2 = pctl->wait_pingpong(pctl, NULL);
		if(ret2){
			pr_err("LCDERR:[%s] failed to wait_pingpong(). (ret=%d)\n", __func__, ret2);
		}
	}

	ret = mdss_diag_dsi_cmd_bta_sw_trigger(ctrl);
	if (ret) {
		ret = MDSS_MIPICHK_RESULT_NG;
	} else {
		ret = MDSS_MIPICHK_RESULT_OK;
	}

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_read_sensitiv(uint8_t *read_data)
{
	int ret = 0;
	struct shdisp_dsi_cmd_desc cmd[1];
	char cmd_buf[1 + 2];
	char payload_page_ee[1][2] = {
		{0xFF, 0xEE}
	};
	struct shdisp_dsi_cmd_desc cmds_sensitiv[] = {
		{SHDISP_DTYPE_DCS_WRITE1, 2, payload_page_ee[0]},
	};

#if defined(CONFIG_SHDISP_PANEL_ANDY)
		payload_page_ee[0][1] = 0xEE;
#elif defined(CONFIG_SHDISP_PANEL_ARIA)
		payload_page_ee[0][1] = 0xE0;
#endif  /* CONFIG_SHDISP_PANEL_ANDY || CONFIG_SHDISP_PANEL_ARIA */

	mdss_shdisp_host_dsi_tx(1, cmds_sensitiv, ARRAY_SIZE(cmds_sensitiv));

	cmd_buf[0] = 0x45;
	cmd_buf[1] = 0x00;

	cmd[0].dtype = SHDISP_DTYPE_DCS_READ;
	cmd[0].wait = 0x00;
	cmd[0].dlen = 1;
	cmd[0].payload = cmd_buf;

	ret = mdss_shdisp_host_dsi_rx(cmd, read_data, 1);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_dsi_cmd_bta_sw_trigger(struct mdss_dsi_ctrl_pdata *ctrl)
{
	u32 status;
	int timeout_us = 35000, ret = 0;

	if (ctrl == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	/* CMD_MODE_BTA_SW_TRIGGER */
	MIPI_OUTP((ctrl->ctrl_base) + 0x098, 0x01);	/* trigger */
	wmb();

	/* Check for CMD_MODE_DMA_BUSY */
	if (readl_poll_timeout(((ctrl->ctrl_base) + 0x0008),
				status, ((status & 0x0010) == 0),
				0, timeout_us))
	{
		pr_info("%s: DSI status=%x failed\n", __func__, status);
		return -EIO;
	}

	ret = mdss_diag_dsi_ack_err_status(ctrl);

	pr_debug("%s: BTA done, status = %d\n", __func__, status);
	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_dsi_ack_err_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
	u32 status;
	unsigned char *base;
	u32 ack = 0x10000000;

	base = ctrl->ctrl_base;

	status = MIPI_INP(base + 0x0068);/* DSI_ACK_ERR_STATUS */

	if (status) {
		MIPI_OUTP(base + 0x0068, status);
		/* Writing of an extra 0 needed to clear error bits */
		MIPI_OUTP(base + 0x0068, 0);

		status &= ~(ack);
		if(status){
			pr_err("%s: status=%x\n", __func__, status);
			return -EIO;
		}
	}
	return 0;
}
#endif /* CONFIG_USES_SHLCDC */

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int mdss_diag_mipi_clkchg(struct mdp_mipi_clkchg_param *mipi_clkchg_param)
{
#ifdef CONFIG_USES_SHLCDC
	return 0;
#else  /* CONFIG_USES_SHLCDC */
	int ret = 0;
	struct mdss_mdp_ctl *pctl;
	struct mdss_panel_data *pdata;

	pr_debug("%s: called\n", __func__);
	pctl = mdss_shdisp_get_mdpctrl(0);
	if (!pctl) {
		pr_err("LCDERR:[%s] mdpctrl is NULL.\n", __func__);
		return -EIO;
	}
	pdata = pctl->panel_data;

	mdss_shdisp_lock_recovery();

	mdss_diag_mipi_clkchg_param_log(mipi_clkchg_param);

	mdss_diag_mipi_clkchg_host_data(mipi_clkchg_param, pdata);

	shdisp_api_set_freq_param(&mipi_clkchg_param->panel);

	if (mdss_shdisp_is_disp_on()) {
		ret = mdss_diag_mipi_clkchg_setparam(mipi_clkchg_param, pctl);
	} else {
		ret = mdss_diag_mipi_clkchg_panel_clk_data(pdata);
	}

	mdss_shdisp_unlock_recovery();

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
#endif /* CONFIG_USES_SHLCDC */
}

#ifndef CONFIG_USES_SHLCDC
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_setparam(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl)
{
	int ret = 0;

	pr_debug("%s: called\n", __func__);
	if (pctl->is_video_mode) {
#ifndef SHDISP_DISABLE_HR_VIDEO
		ret |= mdss_mdp_hr_video_suspend(pctl, false);
#else  /* SHDISP_DISABLE_HR_VIDEO */
		mdss_shdisp_video_transfer_ctrl(false, false);
#endif /* SHDISP_DISABLE_HR_VIDEO */
	} else {
		mdss_shdisp_clk_ctrl(true);
	}

	ret |= mdss_diag_mipi_clkchg_panel(mipi_clkchg_param, pctl);

	ret |= mdss_diag_mipi_clkchg_host(mipi_clkchg_param, pctl);

	if (pctl->is_video_mode) {
#ifndef SHDISP_DISABLE_HR_VIDEO
		ret |= mdss_mdp_hr_video_resume(pctl, false);
#else  /* SHDISP_DISABLE_HR_VIDEO */
		mdss_shdisp_video_transfer_ctrl(true, false);
#endif /* SHDISP_DISABLE_HR_VIDEO */
	} else {
		mdss_shdisp_clk_ctrl(false);
	}

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void mdss_diag_mipi_clkchg_host_data(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_panel_data *pdata)
{
	int i;
	struct mdss_panel_info *pinfo = &(pdata->panel_info);

	pr_debug("%s: called\n", __func__);

	pinfo->clk_rate = mipi_clkchg_param->host.clock_rate;
	pinfo->xres = mipi_clkchg_param->host.display_width;
	pinfo->yres = mipi_clkchg_param->host.display_height;
	pinfo->lcdc.h_pulse_width = mipi_clkchg_param->host.hsync_pulse_width;
	pinfo->lcdc.h_back_porch = mipi_clkchg_param->host.h_back_porch;
	pinfo->lcdc.h_front_porch = mipi_clkchg_param->host.h_front_porch;
	pinfo->lcdc.v_pulse_width = mipi_clkchg_param->host.vsync_pulse_width;
	pinfo->lcdc.v_back_porch = mipi_clkchg_param->host.v_back_porch;
	pinfo->lcdc.v_front_porch = mipi_clkchg_param->host.v_front_porch;
	pinfo->mipi.t_clk_post = mipi_clkchg_param->host.t_clk_post;
	pinfo->mipi.t_clk_pre = mipi_clkchg_param->host.t_clk_pre;
	for ( i=0; i<12; i++ ) {
		pinfo->mipi.dsi_phy_db.timing[i] = mipi_clkchg_param->host.timing_ctrl[i];
	}

	pr_debug("%s: end\n", __func__);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_host(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl)
{
	int ret = 0;
	u32 ctl_flush;
	struct mdss_panel_data *pdata = pctl->panel_data;

	pr_debug("%s: called\n", __func__);
	ret |= mdss_diag_mipi_clkchg_panel_clk_update(pdata);
	if (pctl->is_video_mode) {
#ifndef SHDISP_DISABLE_HR_VIDEO
		ret |= mdss_mdp_hr_video_clkchg_mdp_update(pctl);
#else  /* SHDISP_DISABLE_HR_VIDEO */
		ret |= mdss_mdp_video_clkchg_mdp_update(pctl);
#endif /* SHDISP_DISABLE_HR_VIDEO */
	}
	ret |= mdss_diag_mipi_clkchg_panel_porch_update(pdata);

	ctl_flush = (BIT(31) >> (pctl->intf_num - MDSS_MDP_INTF0));
	mdss_mdp_ctl_write(pctl, MDSS_MDP_REG_CTL_FLUSH, ctl_flush);

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_panel_clk_update(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	pr_debug("%s: called\n", __func__);
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	ret = mdss_diag_mipi_clkchg_panel_clk_data(pdata);

	mdss_dsi_pll_relock(ctrl_pdata);

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_panel_clk_data(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_panel_info *pinfo = &(pdata->panel_info);
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	pr_debug("%s: called\n", __func__);
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	ret = mdss_dsi_clk_div_config(pinfo, pinfo->mipi.frame_rate);
	if (ret) {
		pr_err("LCDERR:[%s] mdss_dsi_clk_div_config err.\n", __func__);
		return ret;
	}
	pr_debug("%s: clk_rate = %d\n", __func__, pinfo->mipi.dsi_pclk_rate);
	ctrl_pdata->pclk_rate =
		pinfo->mipi.dsi_pclk_rate;
	ctrl_pdata->byte_clk_rate =
		pinfo->clk_rate / 8;

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_panel_porch_update(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = &(pdata->panel_info);
	struct mipi_panel_info *mipi;
	u32 hbp, hfp, vbp, vfp, hspw, vspw, width, height;
	u32 ystride, bpp, data, dst_bpp;
	u32 dummy_xres, dummy_yres;
	u32 hsync_period, vsync_period;

	pr_debug("%s: called\n", __func__);
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	mdss_dsi_phy_init(ctrl_pdata);

	dst_bpp = pdata->panel_info.fbc.enabled ?
		(pdata->panel_info.fbc.target_bpp) : (pinfo->bpp);

	hbp = mult_frac(pdata->panel_info.lcdc.h_back_porch, dst_bpp,
			pdata->panel_info.bpp);
	hfp = mult_frac(pdata->panel_info.lcdc.h_front_porch, dst_bpp,
			pdata->panel_info.bpp);
	vbp = mult_frac(pdata->panel_info.lcdc.v_back_porch, dst_bpp,
			pdata->panel_info.bpp);
	vfp = mult_frac(pdata->panel_info.lcdc.v_front_porch, dst_bpp,
			pdata->panel_info.bpp);
	hspw = mult_frac(pdata->panel_info.lcdc.h_pulse_width, dst_bpp,
			pdata->panel_info.bpp);
	vspw = pdata->panel_info.lcdc.v_pulse_width;
	width = mult_frac(pdata->panel_info.xres, dst_bpp,
			pdata->panel_info.bpp);
	height = pdata->panel_info.yres;

	if (pdata->panel_info.type == MIPI_VIDEO_PANEL) {
		dummy_xres = pdata->panel_info.lcdc.xres_pad;
		dummy_yres = pdata->panel_info.lcdc.yres_pad;
	}

	vsync_period = vspw + vbp + height + dummy_yres + vfp;
	hsync_period = hspw + hbp + width + dummy_xres + hfp;

	mipi  = &pdata->panel_info.mipi;
	if (pdata->panel_info.type == MIPI_VIDEO_PANEL) {
		if (ctrl_pdata->timing_db_mode)
			MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x1e8, 0x1);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x24,
			((hspw + hbp + width + dummy_xres) << 16 |
			(hspw + hbp)));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x28,
			((vspw + vbp + height + dummy_yres) << 16 |
			(vspw + vbp)));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
				((vsync_period - 1) << 16)
				| (hsync_period - 1));

		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x30, (hspw << 16));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x34, 0);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x38, (vspw << 16));
		if (ctrl_pdata->timing_db_mode)
			MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x1e4, 0x1);

	} else {		/* command mode */
		if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB888)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB666)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB565)
			bpp = 2;
		else
			bpp = 3;	/* Default format set to RGB888 */

		ystride = width * bpp + 1;

		/* DSI_COMMAND_MODE_MDP_STREAM_CTRL */
		data = (ystride << 16) | (mipi->vc << 8) | DTYPE_DCS_LWRITE;
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x60, data);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x58, data);

		/* DSI_COMMAND_MODE_MDP_STREAM_TOTAL */
		data = height << 16 | width;
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x64, data);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x5C, data);
	}
	mdss_dsi_sw_reset(ctrl_pdata, false);
	mdss_dsi_host_init(pdata);
	mdss_dsi_op_mode_config(mipi->mode, pdata);

	pr_debug("%s: end ret(%d)\n", __func__, ret);

	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static int mdss_diag_mipi_clkchg_panel(struct mdp_mipi_clkchg_param *mipi_clkchg_param, struct mdss_mdp_ctl *pctl)
{
	int ret = 0;
#if defined(CONFIG_SHDISP_PANEL_ANDY)
	static char mipi_sh_andy_cmds_clkchgSetting[6][2] = {
		{0xFF, 0x05 },
		{0x90, 0x00 },
		{0x9B, 0x00 },
		{0xFF, 0x00 },
		{0xD3, 0x00 },
		{0xD4, 0x00 }
	};
	static struct shdisp_dsi_cmd_desc mipi_sh_andy_cmds_clkchg[] = {
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[0]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[1]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[2]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[3]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[4]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmds_clkchgSetting[5]},
	};

	pr_debug("%s: called\n", __func__);

	mipi_sh_andy_cmds_clkchgSetting[1][1] = mipi_clkchg_param->panel.andy.rtn;

	mipi_sh_andy_cmds_clkchgSetting[2][1] = mipi_clkchg_param->panel.andy.gip;

	mipi_sh_andy_cmds_clkchgSetting[4][1] = mipi_clkchg_param->panel.andy.vbp;

	mipi_sh_andy_cmds_clkchgSetting[5][1] = mipi_clkchg_param->panel.andy.vfp;

	ret = mdss_shdisp_host_dsi_tx(1, mipi_sh_andy_cmds_clkchg, ARRAY_SIZE(mipi_sh_andy_cmds_clkchg));

	pr_debug("%s: end ret(%d)\n", __func__, ret);

#elif defined(CONFIG_SHDISP_PANEL_ARIA)
	static char mipi_sh_aria_cmds_clkchgSetting[11][2] = {
		{0xFF, 0x24 },
		{0x34, 0x00 },
		{0x35, 0x00 },
		{0x7E, 0x00 },
		{0x7F, 0x00 },
		{0x81, 0x00 },
		{0x91, 0x00 },
		{0x92, 0x00 },
		{0x93, 0x00 },
		{0x94, 0x00 },
		{0xFF, 0x10 }
	};
	static struct shdisp_dsi_cmd_desc mipi_sh_aria_cmds_clkchg[] = {
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[0]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[1]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[2]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[3]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[4]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[5]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[6]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[7]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[8]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[9]},
		{SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_aria_cmds_clkchgSetting[10]},
	};

	pr_debug("%s: called\n", __func__);

	mipi_sh_aria_cmds_clkchgSetting[1][1] = mipi_clkchg_param->panel.aria.sti;
	mipi_sh_aria_cmds_clkchgSetting[2][1] = mipi_clkchg_param->panel.aria.swi;
	mipi_sh_aria_cmds_clkchgSetting[3][1] = mipi_clkchg_param->panel.aria.muxs;
	mipi_sh_aria_cmds_clkchgSetting[4][1] = mipi_clkchg_param->panel.aria.muxw;
	mipi_sh_aria_cmds_clkchgSetting[5][1] = mipi_clkchg_param->panel.aria.muxg1;
	mipi_sh_aria_cmds_clkchgSetting[6][1] = mipi_clkchg_param->panel.aria.rtn_h;
	mipi_sh_aria_cmds_clkchgSetting[7][1] = mipi_clkchg_param->panel.aria.rtna;
	mipi_sh_aria_cmds_clkchgSetting[8][1] = mipi_clkchg_param->panel.aria.fp;
	mipi_sh_aria_cmds_clkchgSetting[9][1] = mipi_clkchg_param->panel.aria.bp;

	ret = mdss_shdisp_host_dsi_tx(1, mipi_sh_aria_cmds_clkchg, ARRAY_SIZE(mipi_sh_aria_cmds_clkchg));

	pr_debug("%s: end ret(%d)\n", __func__, ret);

#endif	/* CONFIG_SHDISP_PANEL_ANDY || CONFIG_SHDISP_PANEL_ARIA */
	return ret;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void mdss_diag_mipi_clkchg_param_log(struct mdp_mipi_clkchg_param *mdp_mipi_clkchg_param)
{
	
    pr_debug("[%s]param->host.clock_rate         = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.clock_rate          );
    pr_debug("[%s]param->host.display_width      = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.display_width       );
    pr_debug("[%s]param->host.display_height     = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.display_height      );
    pr_debug("[%s]param->host.hsync_pulse_width  = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.hsync_pulse_width   );
    pr_debug("[%s]param->host.h_back_porch       = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.h_back_porch        );
    pr_debug("[%s]param->host.h_front_porch      = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.h_front_porch       );
    pr_debug("[%s]param->host.vsync_pulse_width  = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.vsync_pulse_width   );
    pr_debug("[%s]param->host.v_back_porch       = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.v_back_porch        );
    pr_debug("[%s]param->host.v_front_porch      = %10d\n"  , __func__, mdp_mipi_clkchg_param->host.v_front_porch       );
    pr_debug("[%s]param->host.t_clk_post         = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.t_clk_post          );
    pr_debug("[%s]param->host.t_clk_pre          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.t_clk_pre           );
    pr_debug("[%s]param->host.timing_ctrl[ 0]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 0]     );
    pr_debug("[%s]param->host.timing_ctrl[ 1]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 1]     );
    pr_debug("[%s]param->host.timing_ctrl[ 2]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 2]     );
    pr_debug("[%s]param->host.timing_ctrl[ 3]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 3]     );
    pr_debug("[%s]param->host.timing_ctrl[ 4]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 4]     );
    pr_debug("[%s]param->host.timing_ctrl[ 5]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 5]     );
    pr_debug("[%s]param->host.timing_ctrl[ 6]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 6]     );
    pr_debug("[%s]param->host.timing_ctrl[ 7]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 7]     );
    pr_debug("[%s]param->host.timing_ctrl[ 8]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 8]     );
    pr_debug("[%s]param->host.timing_ctrl[ 9]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[ 9]     );
    pr_debug("[%s]param->host.timing_ctrl[10]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[10]     );
    pr_debug("[%s]param->host.timing_ctrl[11]    = 0x%02X\n", __func__, mdp_mipi_clkchg_param->host.timing_ctrl[11]     );
#if defined(CONFIG_SHDISP_PANEL_ANDY)
    pr_debug("[%s]param->panel.andy.rtn          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.rtn           );
    pr_debug("[%s]param->panel.andy.gip          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.gip           );
    pr_debug("[%s]param->panel.andy.vbp          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.vbp           );
    pr_debug("[%s]param->panel.andy.vfp          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.vfp           );
#elif defined(CONFIG_SHDISP_PANEL_ARIA)
    pr_debug("[%s]param->panel.aria.sti          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.sti           );
    pr_debug("[%s]param->panel.aria.swi          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.swi           );
    pr_debug("[%s]param->panel.aria.muxs         = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.muxs          );
    pr_debug("[%s]param->panel.aria.muxw         = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.muxw          );
    pr_debug("[%s]param->panel.aria.muxg1        = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.muxg1         );
    pr_debug("[%s]param->panel.aria.rtn_h        = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.rtn_h         );
    pr_debug("[%s]param->panel.aria.rtna         = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.rtna          );
    pr_debug("[%s]param->panel.aria.fp           = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.fp            );
    pr_debug("[%s]param->panel.aria.bp           = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.aria.bp            );
#else
    pr_debug("[%s]param->panel.andy.rtn          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.rtn           );
    pr_debug("[%s]param->panel.andy.gip          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.gip           );
    pr_debug("[%s]param->panel.andy.vbp          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.vbp           );
    pr_debug("[%s]param->panel.andy.vfp          = 0x%02X\n", __func__, mdp_mipi_clkchg_param->panel.andy.vfp           );
#endif

	return;
}
#endif /* CONFIG_USES_SHLCDC */

