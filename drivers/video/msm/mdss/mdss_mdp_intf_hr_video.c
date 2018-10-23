/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/iopoll.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/memblock.h>

#include "mdss_fb.h"
#include "mdss_mdp.h"
#include "mdss_panel.h"
#include "mdss_debug.h"
#include "mdss_mdp_trace.h"
#ifdef CONFIG_SHDISP /* CUST_ID_00028 */
#include "mdss_dsi.h"
#endif /* CONFIG_SHDISP */
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
#include <linux/pm_qos.h>
#include "mdss_shdisp.h"
#endif /* CONFIG_SHDISP */

/* wait for at least 2 vsyncs for lowest refresh rate (24hz) */
#define VSYNC_TIMEOUT_US 100000

/* Poll time to do recovery during active region */
#define POLL_TIME_USEC_FOR_LN_CNT 500

#define MDP_INTR_MASK_INTF_VSYNC(intf_num) \
	(1 << (2 * (intf_num - MDSS_MDP_INTF0) + MDSS_MDP_IRQ_INTF_VSYNC))

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
#define DSI_CLK_DELAY      2
#define REPEAT_CNT         2
#define WAIT_CNT           1
#define MFR_EO_CNT         6 /* about 100ms */
#define INT_ENABLE_PERIOD 10 /* re-enabled every 10 sec */
#define TG_OFF_LIMIT_LINE  4 /* TG_OFF limit line before vsync */

static char *state_str[] = {
	"POWER_ON",
	"UPDATE_FRAME",
	"BLANK_FRAME",
	"REPEAT_FRAME",
	"WAIT_FRAME",
	"HR_VIDEO_WAIT",
	"HR_VIDEO_PREPARE",
	"HR_VIDEO_REFRESH",
	"HR_VIDEO_MFR_WAIT",
	"HR_VIDEO_MFR_REFRESH",
	"SUSPEND",
	"COMMIT",
	"POWER_OFF",
};

enum mdss_mdp_hr_video_mode {
	POWER_ON,
	UPDATE_FRAME,
	BLANK_FRAME,
	REPEAT_FRAME,
	WAIT_FRAME,
	HR_VIDEO_WAIT,
	HR_VIDEO_PREPARE,
	HR_VIDEO_REFRESH,
	HR_VIDEO_MFR_WAIT,
	HR_VIDEO_MFR_REFRESH,
	SUSPEND,
	COMMIT,
	POWER_OFF,
};

/* Possible states for hr_video state machine i.e.ctx->hr_video_state */
enum vsync_source {
	IT_VSYNC_INT = 0,
	IT_VARIABLE_TIMER,
	IT_PERIODIC_TIMER,
	IT_PENDING,
};

static char *tg_state_str[] = {
	"HW_TG_ON",
	"SW_TG_OFF",
	"HW_TG_OFF",
};

enum mdss_mdp_hr_video_tg_state {
	HW_TG_ON,
	SW_TG_OFF,
	HW_TG_OFF,
};

enum sw_tg_off {
	SW_TG_OFF_NONE = 0,
	SW_TG_OFF_REQ,
	SW_TG_OFF_BLOCK,
	SW_TG_OFF_TIMING_1 = 1500000, /*  2500us before vsync, turn qos on */
	SW_TG_OFF_TIMING_2 =  700000, /*  1000us before vsync, turn tg off */
	SW_TG_OFF_TIMING_3 =  300000, /*   300us before vsync, turn tg off in case overlay hint is set */
	SW_TG_OFF_TIMING   = SW_TG_OFF_TIMING_1 + SW_TG_OFF_TIMING_2 + SW_TG_OFF_TIMING_3,
};

enum swt_id {
	SWT_0,
	SWT_1,
	SWT_2,
	SWT_MAX,
};

static char *swt_mode_str[] = {
	"VARIABLE",
	"PERIODIC",
	"ONESHOT",
	"ONESHOT_L",
};

enum swt_mode {
	SWT_FREE = -1,
	SWT_VARIABLE = 0,
	SWT_PERIODIC,
	SWT_ONESHOT,
	SWT_ONESHOT_L,
};

enum swt_state {
	SWT_IDLE,
	SWT_RUNNING,
	SWT_EXPIRED,
	SWT_CANCEL_PENDING,
};

enum mdss_mdp_hr_pol_state {
	POL_STATE_INIT,
	POL_STATE_POS,
	POL_STATE_NEG,
};

enum clk_state {
	CLK_STATE_NONE = 0,
	CLK_STATE_ON,
	CLK_STATE_OFF,
	CS_SHIFT = 2,
	CS_CLK_MASK = (1 << CS_SHIFT) - 1,
	CS_INT_MASK = CS_CLK_MASK << CS_SHIFT,
	CS_QOS_MASK = CS_INT_MASK << CS_SHIFT,
};

static char *clk_onoff_str[] = {
	"CLK_OFF",
	"CLK_ON",
	"INT_OFF",
	"INT_ON",
	"QOS_OFF",
	"QOS_ON",
	"ALL_ON_SYNC",
};

enum clk_onoff {
	CO_CLK_OFF = 0,
	CO_CLK_ON,
	CO_INT_OFF,
	CO_INT_ON,
	CO_QOS_OFF,
	CO_QOS_ON,
	CO_ALL_ON_SYNC,
};

enum fps_led_state {
	FPS_LED_STATE_NONE = 0,
	FPS_LED_STATE_60HZ,
	FPS_LED_STATE_30HZ,
	FPS_LED_STATE_1HZ,
};

struct mdss_mdp_hr_video_ctx;
struct state_table {
	unsigned char tg;
	unsigned char tick;
	unsigned char clk_int_ctrl;
	unsigned char next_state;
	int (*cb)(struct mdss_mdp_hr_video_ctx *);
};

struct swtimer {
	int id;
	int mode;
	int state;
	int ticks;
	int qos;
	u64 base;
	u64 expire;
	struct hrtimer hrt;
};

struct fps_led_ctx {
	bool inited;
	bool enable;
	spinlock_t lock;
	enum fps_led_state state;
	struct hrtimer timer;
	unsigned int frame_hist;
	char led_red;
	char led_green;
	char led_blue;
	struct work_struct indicate_work;
};
#endif /* CONFIG_SHDISP */

/* intf timing settings */
struct intf_timing_params {
	u32 width;
	u32 height;
	u32 xres;
	u32 yres;

	u32 h_back_porch;
	u32 h_front_porch;
	u32 v_back_porch;
	u32 v_front_porch;
	u32 hsync_pulse_width;
	u32 vsync_pulse_width;

	u32 border_clr;
	u32 underflow_clr;
	u32 hsync_skew;
};

struct mdss_mdp_hr_video_ctx {
	u32 intf_num;
	char __iomem *base;
	u32 intf_type;
	u8 ref_cnt;

	u8 timegen_en;
	bool polling_en;
	u32 poll_cnt;
	struct completion vsync_comp;
	int wait_pending;

	atomic_t vsync_ref;
	spinlock_t vsync_lock;
	spinlock_t dfps_lock;
	struct mutex vsync_mtx;
	struct list_head vsync_handlers;
	struct mdss_intf_recovery intf_recovery;

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	struct mdss_mdp_ctl *ctl;
	atomic_t vsync_handler_cnt;
	enum mdss_mdp_hr_video_tg_state tg_state;
	enum mdss_mdp_hr_video_mode video_state;
	enum mdss_mdp_hr_video_mode video_state_default;
	enum mdss_mdp_hr_video_mode video_state_save;
	int video_state_tg;
	int video_state_tick;
	int video_state_ctrl;
	struct state_table state_table[POWER_OFF + 1];
	spinlock_t hrtimer_lock;

	ktime_t swt_hint;
	int int_type;
	bool repeat_done;
	int hr_video_wait_tick;
	int hr_video_wait_tick_wait;
	enum sw_tg_off sw_tg_off_state;
	u64 sw_tg_off_limit_time;
	u64 hw_tg_off_time;
	bool need_dsi_sw_reset;

	enum mdss_mdp_hr_pol_state pol_state;
	int pol_need;
	int pol_sum;
	ktime_t pol_time;

	int mfr;
	int mfr_cnt;
	int suspend;

	struct completion commit_comp;
	struct mutex commit_mtx;
	int (*commit_cb)(struct mdss_mdp_hr_video_ctx *);
	struct task_struct *clk_task;
	spinlock_t clk_task_lock;
	wait_queue_head_t clk_task_wq;
	struct completion clk_task_comp;
	int clk_ref_cnt;
	int clk_state;
	int clk_state_req;
	int clk_state_work;
	int clk_state_clk;
	int clk_state_int;
	bool clk_state_running;
	bool clk_state_wait;
	bool clk_off_block;
	ktime_t swt_next_timing;
	bool swt_next_tg;
	bool swt_vsync;
	int swt_tick;
	int swt_total_line;
	int swt_time_of_line;
	int swt_time_of_vsync;
	int swt_time_of_frames;
	int swt_frame_rate;
	int swt_vsync_line;
	int swt_tg_off_limit_line;
	int swt_vbp_end_line;
	struct swtimer timer[SWT_MAX];
	struct pm_qos_request qos_req;
	int qos_on;
	atomic_t overlay_hint;

	struct fps_led_ctx fps_led;
#endif /* CONFIG_SHDISP */
};

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
extern int mdss_dsi_state_reset(struct mdss_panel_data *pdata);
extern void mdss_dsi_video_comp(struct mdss_mdp_ctl *ctl);

static void hr_video_clk_thread_init(struct mdss_mdp_hr_video_ctx *ctx);
static void hr_video_clk_thread_deinit(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_clk_onoff(struct mdss_mdp_hr_video_ctx *ctx, enum clk_onoff on);
static void hr_video_clk_sync(struct mdss_mdp_hr_video_ctx *ctx);
static void hr_video_clk_ctrl(struct mdss_mdp_hr_video_ctx *ctx, int onoff);
static void hr_video_start_oneshot_timer(struct mdss_mdp_hr_video_ctx *ctx, int sec);
static void hr_video_start_variable_timer(struct mdss_mdp_hr_video_ctx *ctx, int expire, int ticks);
static void hr_video_start_periodic_timer(struct mdss_mdp_hr_video_ctx *ctx, int expire);
static void hr_video_switch_periodic_timer(struct mdss_mdp_hr_video_ctx *ctx, bool periodic);
static int hr_video_get_next_tick(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_mfr(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_resume(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_power_off(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_start_repeat(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_start_prepare(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_start_refresh(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_start_wait(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_update_mfr(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_cb_set_mfr(struct mdss_mdp_hr_video_ctx *ctx);
static void hr_video_init_mfr(struct mdss_mdp_hr_video_ctx *ctx, bool enable_only);
static void hr_video_tbl_init(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_time_from_vsync(struct mdss_mdp_hr_video_ctx *ctx);
static int hr_video_time_to_vsync(struct mdss_mdp_hr_video_ctx *ctx);
static int mdss_mdp_timegen_enable(struct mdss_mdp_ctl *ctl, bool enable);
static void mdss_mdp_hr_polarity_update(struct mdss_mdp_hr_video_ctx *ctx, bool start_tg, int frms);
static void commit_state(struct mdss_mdp_hr_video_ctx *ctx, struct state_table *tbl);
static void update_state(struct mdss_mdp_hr_video_ctx *ctx);
static int vsync_adjust(struct mdss_mdp_hr_video_ctx *ctx);
static void vsync_common(struct mdss_mdp_hr_video_ctx *ctx);
static bool mdss_mdp_hr_video_need_video_comp(struct mdss_mdp_ctl *ctl);
static int is_clk_on(struct mdss_mdp_hr_video_ctx *ctx);
static int is_int_on(struct mdss_mdp_hr_video_ctx *ctx);
static void hr_video_intr_enable(struct mdss_mdp_hr_video_ctx *ctx, bool isr);
static void hr_video_intr_disable(struct mdss_mdp_hr_video_ctx *ctx);
static void mdss_mdp_hr_video_fps_led_vsync(struct mdss_mdp_hr_video_ctx *ctx);
static void set_timings(struct mdss_mdp_hr_video_ctx *ctx);

static const struct state_table init_state[] = {
	/*                           TG  TICK   CLKCTL  NEXTSTATE             CB */
	{ /* POWER_ON             */  0, 0,          0, POWER_ON,             NULL},
	{ /* UPDATE_FRAME         */  1, 1,          0, BLANK_FRAME,          NULL},
	{ /* BLANK_FRAME          */  0, -1,         0, REPEAT_FRAME,         hr_video_cb_start_repeat},
	{ /* REPEAT_FRAME         */  1, REPEAT_CNT, 0, WAIT_FRAME,           NULL},
	{ /* WAIT_FRAME           */  0, WAIT_CNT,   0, HR_VIDEO_WAIT,        hr_video_cb_start_wait},
	{ /* HR_VIDEO_WAIT        */  0, -1,         1, HR_VIDEO_PREPARE,     hr_video_cb_start_prepare},
	{ /* HR_VIDEO_PREPARE     */  0, -1,         0, HR_VIDEO_REFRESH,     hr_video_cb_start_refresh},
	{ /* HR_VIDEO_REFRESH     */  1, 1,          0, HR_VIDEO_WAIT,        hr_video_cb_start_wait},
	{ /* HR_VIDEO_MFR_WAIT    */  0, 1,          0, HR_VIDEO_MFR_REFRESH, hr_video_cb_update_mfr},
	{ /* HR_VIDEO_MFR_REFRESH */  1, 1,          0, HR_VIDEO_MFR_WAIT,    hr_video_cb_set_mfr},
	{ /* SUSPEND              */  0, 1,          0, SUSPEND,              NULL},
	{ /* COMMIT               */  0, 1,          0, COMMIT,               NULL},
	{ /* POWER_OFF            */  1, 1,          0, POWER_OFF,            NULL},
};
#endif /* CONFIG_SHDISP */

static void mdss_mdp_hr_fetch_start_config(struct mdss_mdp_hr_video_ctx *ctx,
		struct mdss_mdp_ctl *ctl);

static inline void mdp_hr_video_write(struct mdss_mdp_hr_video_ctx *ctx,
				   u32 reg, u32 val)
{
	writel_relaxed(val, ctx->base + reg);
}

static inline u32 mdp_hr_video_read(struct mdss_mdp_hr_video_ctx *ctx,
				   u32 reg)
{
	return readl_relaxed(ctx->base + reg);
}

static inline u32 mdss_mdp_hr_video_line_count(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	u32 line_cnt = 0;
	if (!ctl || !ctl->priv_data)
		goto line_count_exit;
	ctx = ctl->priv_data;
	line_cnt = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_LINE_COUNT);
line_count_exit:
	return line_cnt;
}

int mdss_mdp_hr_video_addr_setup(struct mdss_data_type *mdata,
				u32 *offsets,  u32 count)
{
	struct mdss_mdp_hr_video_ctx *head;
	u32 i;

	head = devm_kzalloc(&mdata->pdev->dev,
			sizeof(struct mdss_mdp_hr_video_ctx) * count, GFP_KERNEL);
	if (!head)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		head[i].base = mdata->mdss_io.base + offsets[i];
		pr_debug("adding Video Intf #%d offset=0x%x virt=%pK\n", i,
				offsets[i], head[i].base);
		head[i].ref_cnt = 0;
		head[i].intf_num = i + MDSS_MDP_INTF0;
		INIT_LIST_HEAD(&head[i].vsync_handlers);
	}

	mdata->video_intf = head;
	mdata->nintf = count;
	return 0;
}

static void mdss_mdp_hr_video_intf_recovery(void *data, int event)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_mdp_ctl *ctl = data;
	struct mdss_panel_info *pinfo;
	u32 line_cnt, min_ln_cnt, active_lns_cnt;
	u32 clk_rate, clk_period, time_of_line;
	u32 delay;

	if (!data) {
		pr_err("%s: invalid ctl\n", __func__);
		return;
	}

	/*
	 * Currently, only intf_fifo_overflow is
	 * supported for recovery sequence for video
	 * mode DSI interface
	 */
	if (event != MDP_INTF_DSI_VIDEO_FIFO_OVERFLOW) {
		pr_warn("%s: unsupported recovery event:%d\n",
					__func__, event);
		return;
	}

	ctx = ctl->priv_data;
	pr_debug("%s: ctl num = %d, event = %d\n",
				__func__, ctl->num, event);

	pinfo = &ctl->panel_data->panel_info;
	clk_rate = ((ctl->intf_type == MDSS_INTF_DSI) ?
			pinfo->mipi.dsi_pclk_rate :
			pinfo->clk_rate);

	clk_rate /= 1000;	/* in kHz */
	if (!clk_rate) {
		pr_err("Unable to get proper clk_rate\n");
		return;
	}
	/*
	 * calculate clk_period as pico second to maintain good
	 * accuracy with high pclk rate and this number is in 17 bit
	 * range.
	 */
	clk_period = 1000000000 / clk_rate;
	if (!clk_period) {
		pr_err("Unable to calculate clock period\n");
		return;
	}
	min_ln_cnt = pinfo->lcdc.v_back_porch + pinfo->lcdc.v_pulse_width;
	active_lns_cnt = pinfo->yres;
	time_of_line = (pinfo->lcdc.h_back_porch +
		 pinfo->lcdc.h_front_porch +
		 pinfo->lcdc.h_pulse_width +
		 pinfo->xres) * clk_period;

	/* delay in micro seconds */
	delay = (time_of_line * (min_ln_cnt +
			pinfo->lcdc.v_front_porch)) / 1000000;

	/*
	 * Wait for max delay before
	 * polling to check active region
	 */
	if (delay > POLL_TIME_USEC_FOR_LN_CNT)
		delay = POLL_TIME_USEC_FOR_LN_CNT;

	while (1) {
		line_cnt = mdss_mdp_hr_video_line_count(ctl);

		if ((line_cnt >= min_ln_cnt) && (line_cnt <
			(active_lns_cnt + min_ln_cnt))) {
			pr_debug("%s, Needed lines left line_cnt=%d\n",
						__func__, line_cnt);
			return;
		} else {
			pr_warn("line count is less. line_cnt = %d\n",
								line_cnt);
			/* Add delay so that line count is in active region */
			udelay(delay);
		}
	}
}

static int mdss_mdp_hr_video_timegen_setup(struct mdss_mdp_ctl *ctl,
					struct intf_timing_params *p)
{
	u32 hsync_period, vsync_period;
	u32 hsync_start_x, hsync_end_x, display_v_start, display_v_end;
	u32 active_h_start, active_h_end, active_v_start, active_v_end;
	u32 den_polarity, hsync_polarity, vsync_polarity;
	u32 display_hctl, active_hctl, hsync_ctl, polarity_ctl;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_data_type *mdata;

	ctx = ctl->priv_data;
	mdata = ctl->mdata;
	hsync_period = p->hsync_pulse_width + p->h_back_porch +
			p->width + p->h_front_porch;
	vsync_period = p->vsync_pulse_width + p->v_back_porch +
			p->height + p->v_front_porch;

	display_v_start = ((p->vsync_pulse_width + p->v_back_porch) *
			hsync_period) + p->hsync_skew;
	display_v_end = ((vsync_period - p->v_front_porch) * hsync_period) +
			p->hsync_skew - 1;

	if (ctx->intf_type == MDSS_INTF_EDP) {
		display_v_start += p->hsync_pulse_width + p->h_back_porch;
		display_v_end -= p->h_front_porch;
	}

	/* TIMING_2 flush bit on 8939 is BIT 31 */
	if (mdata->mdp_rev == MDSS_MDP_HW_REV_108 &&
				ctx->intf_num == MDSS_MDP_INTF2)
		ctl->flush_bits |= BIT(31);
	else
		ctl->flush_bits |= BIT(31) >>
			(ctx->intf_num - MDSS_MDP_INTF0);

	hsync_start_x = p->h_back_porch + p->hsync_pulse_width;
	hsync_end_x = hsync_period - p->h_front_porch - 1;

	if (p->width != p->xres) {
		active_h_start = hsync_start_x;
		active_h_end = active_h_start + p->xres - 1;
	} else {
		active_h_start = 0;
		active_h_end = 0;
	}

	if (p->height != p->yres) {
		active_v_start = display_v_start;
		active_v_end = active_v_start + (p->yres * hsync_period) - 1;
	} else {
		active_v_start = 0;
		active_v_end = 0;
	}


	if (active_h_end) {
		active_hctl = (active_h_end << 16) | active_h_start;
		active_hctl |= BIT(31);	/* ACTIVE_H_ENABLE */
	} else {
		active_hctl = 0;
	}

	if (active_v_end)
		active_v_start |= BIT(31); /* ACTIVE_V_ENABLE */

	hsync_ctl = (hsync_period << 16) | p->hsync_pulse_width;
	display_hctl = (hsync_end_x << 16) | hsync_start_x;

	den_polarity = 0;
	if (MDSS_INTF_HDMI == ctx->intf_type) {
		hsync_polarity = p->yres >= 720 ? 0 : 1;
		vsync_polarity = p->yres >= 720 ? 0 : 1;
	} else {
		hsync_polarity = 0;
		vsync_polarity = 0;
	}
	polarity_ctl = (den_polarity << 2)   | /*  DEN Polarity  */
		       (vsync_polarity << 1) | /* VSYNC Polarity */
		       (hsync_polarity << 0);  /* HSYNC Polarity */

	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_HSYNC_CTL, hsync_ctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			vsync_period * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PULSE_WIDTH_F0,
			   p->vsync_pulse_width * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_HCTL, display_hctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_START_F0,
			   display_v_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_END_F0, display_v_end);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_ACTIVE_HCTL, active_hctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_ACTIVE_V_START_F0,
			   active_v_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_ACTIVE_V_END_F0, active_v_end);

	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_BORDER_COLOR, p->border_clr);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_UNDERFLOW_COLOR,
			   p->underflow_clr);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_HSYNC_SKEW, p->hsync_skew);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_POLARITY_CTL, polarity_ctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_FRAME_LINE_COUNT_EN, 0x3);

	return 0;
}


static inline void hr_video_vsync_irq_enable(struct mdss_mdp_ctl *ctl, bool clear)
{
	struct mdss_mdp_hr_video_ctx *ctx = ctl->priv_data;

	mutex_lock(&ctx->vsync_mtx);
	if (atomic_inc_return(&ctx->vsync_ref) == 1)
		mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_VSYNC, ctl->intf_num);
	else if (clear)
		mdss_mdp_irq_clear(ctl->mdata, MDSS_MDP_IRQ_INTF_VSYNC,
				ctl->intf_num);
	mutex_unlock(&ctx->vsync_mtx);
}

static inline void hr_video_vsync_irq_disable(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx = ctl->priv_data;

	mutex_lock(&ctx->vsync_mtx);
	if (atomic_dec_return(&ctx->vsync_ref) == 0)
		mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_VSYNC, ctl->intf_num);
	mutex_unlock(&ctx->vsync_mtx);
}

static int mdss_mdp_hr_video_add_vsync_handler(struct mdss_mdp_ctl *ctl,
		struct mdss_mdp_vsync_handler *handle)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	unsigned long flags;
	int ret = 0;
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	bool switch_hrt = false;
#else  /* CONFIG_SHDISP */
	bool irq_en = false;
#endif /* CONFIG_SHDISP */

	if (!handle || !(handle->vsync_handler)) {
		ret = -EINVAL;
		goto exit;
	}

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx for ctl=%d\n", ctl->num);
		ret = -ENODEV;
		goto exit;
	}

	MDSS_XLOG(ctl->num, ctl->vsync_cnt, handle->enabled);

	spin_lock_irqsave(&ctx->vsync_lock, flags);
	if (!handle->enabled) {
		handle->enabled = true;
		list_add(&handle->list, &ctx->vsync_handlers);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
		atomic_inc(&ctx->vsync_handler_cnt);
		switch_hrt = true;
#else  /* CONFIG_SHDISP */
		irq_en = true;
#endif /* CONFIG_SHDISP */
	}
	spin_unlock_irqrestore(&ctx->vsync_lock, flags);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (switch_hrt) {
		hr_video_clk_ctrl(ctx, 1);
	}
#else  /* CONFIG_SHDISP */
	if (irq_en)
		hr_video_vsync_irq_enable(ctl, false);
#endif /* CONFIG_SHDISP */
exit:
	return ret;
}

static int mdss_mdp_hr_video_remove_vsync_handler(struct mdss_mdp_ctl *ctl,
		struct mdss_mdp_vsync_handler *handle)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	unsigned long flags;
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	bool switch_hrt = false;
#else  /* CONFIG_SHDISP */
	bool irq_dis = false;
#endif /* CONFIG_SHDISP */

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx for ctl=%d\n", ctl->num);
		return -ENODEV;
	}

	MDSS_XLOG(ctl->num, ctl->vsync_cnt, handle->enabled);

	spin_lock_irqsave(&ctx->vsync_lock, flags);
	if (handle->enabled) {
		handle->enabled = false;
		list_del_init(&handle->list);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
		atomic_dec(&ctx->vsync_handler_cnt);
		switch_hrt = true;
#else  /* CONFIG_SHDISP */
		irq_dis = true;
#endif /* CONFIG_SHDISP */
	}
	spin_unlock_irqrestore(&ctx->vsync_lock, flags);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (switch_hrt) {
		hr_video_clk_ctrl(ctx, 0);
	}
#else  /* CONFIG_SHDISP */
	if (irq_dis)
		hr_video_vsync_irq_disable(ctl);
#endif /* CONFIG_SHDISP */
	return 0;
}

static int mdss_mdp_hr_video_intfs_stop(struct mdss_mdp_ctl *ctl,
	struct mdss_panel_data *pdata, int inum)
{
	struct mdss_data_type *mdata;
	struct mdss_panel_info *pinfo;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_mdp_vsync_handler *tmp, *handle;
	struct mdss_mdp_ctl *sctl;
	int rc;
	u32 frame_rate = 0;
	int ret = 0;

	if (pdata == NULL)
		return 0;

	if (is_pingpong_split(ctl->mfd)) {
		ret = mdss_mdp_hr_video_intfs_stop(ctl, pdata->next, (inum + 1));
		if (IS_ERR_VALUE(ret))
			return ret;
	}

	mdata = ctl->mdata;
	pinfo = &pdata->panel_info;

	if (inum < mdata->nintf) {
		ctx = ((struct mdss_mdp_hr_video_ctx *) mdata->video_intf) + inum;
		if (!ctx->ref_cnt) {
			pr_err("Intf %d not in use\n", (inum + MDSS_MDP_INTF0));
			return -ENODEV;
		}
		pr_debug("stop ctl=%d video Intf #%d base=%pK", ctl->num,
			ctx->intf_num, ctx->base);
	} else {
		pr_err("Invalid intf number: %d\n", (inum + MDSS_MDP_INTF0));
		return -EINVAL;
	}

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (ctx->timegen_en) {
		struct state_table tbl = {0};
		tbl.next_state = POWER_OFF;
		tbl.cb = hr_video_cb_power_off;
		hr_video_clk_onoff(ctx, CO_ALL_ON_SYNC);
		commit_state(ctx, &tbl);
	}
	hr_video_vsync_irq_disable(ctl);
#endif /* CONFIG_SHDISP */
	if (ctx->timegen_en) {
		rc = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_BLANK, NULL);
		if (rc == -EBUSY) {
			pr_debug("intf #%d busy don't turn off\n",
				 ctl->intf_num);
			return rc;
		}
		WARN(rc, "intf %d blank error (%d)\n", ctl->intf_num, rc);

		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, 0);
		/* wait for at least one VSYNC for proper TG OFF */
		frame_rate = mdss_panel_get_framerate
				(&(ctl->panel_data->panel_info));
		if (!(frame_rate >= 24 && frame_rate <= 240))
			frame_rate = 24;
		msleep((1000/frame_rate) + 1);

		mdss_iommu_ctrl(0);
		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
		ctx->timegen_en = false;

		rc = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_PANEL_OFF, NULL);
		WARN(rc, "intf %d timegen off error (%d)\n", ctl->intf_num, rc);

		mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_UNDER_RUN,
			ctl->intf_num);
		sctl = mdss_mdp_get_split_ctl(ctl);
		if (sctl)
			mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_UNDER_RUN,
				sctl->intf_num);

		mdss_bus_bandwidth_ctrl(false);
	}

	list_for_each_entry_safe(handle, tmp, &ctx->vsync_handlers, list)
		mdss_mdp_hr_video_remove_vsync_handler(ctl, handle);

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	hr_video_clk_thread_deinit(ctx);
	pm_qos_remove_request(&ctx->qos_req);
#endif /* CONFIG_SHDISP */
	mdss_mdp_set_intr_callback(MDSS_MDP_IRQ_INTF_VSYNC,
		(inum + MDSS_MDP_INTF0), NULL, NULL);
	mdss_mdp_set_intr_callback(MDSS_MDP_IRQ_INTF_UNDER_RUN,
		(inum + MDSS_MDP_INTF0), NULL, NULL);

	ctx->ref_cnt--;

	return 0;
}


static int mdss_mdp_hr_video_stop(struct mdss_mdp_ctl *ctl, int panel_power_state)
{
	int intfs_num, ret = 0;

	intfs_num = ctl->intf_num - MDSS_MDP_INTF0;
	ret = mdss_mdp_hr_video_intfs_stop(ctl, ctl->panel_data, intfs_num);
	if (IS_ERR_VALUE(ret)) {
		pr_err("unable to stop video interface: %d\n", ret);
		return ret;
	}

	MDSS_XLOG(ctl->num, ctl->vsync_cnt);

	mdss_mdp_ctl_reset(ctl);
	ctl->priv_data = NULL;

	return 0;
}

static void mdss_mdp_hr_video_vsync_intr_done(void *arg)
{
	struct mdss_mdp_ctl *ctl = arg;
	struct mdss_mdp_hr_video_ctx *ctx = ctl->priv_data;
#ifndef CONFIG_SHDISP /* CUST_ID_00027 */
	struct mdss_mdp_vsync_handler *tmp;
#endif /* CONFIG_SHDISP */
	ktime_t vsync_time;
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	bool need_comp = false;
	bool need_int = false;
	bool skip_update = false;
	u32 tg_en;
	int frms;
#endif /* CONFIG_SHDISP */

	if (!ctx) {
		pr_err("invalid ctx\n");
		return;
	}

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	pr_debug("ENTER %d\n", mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_LINE_COUNT));

	tg_en = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN);
	if (tg_en) {
		mdss_mdp_hr_video_fps_led_vsync(ctx);
	}

	if (mdss_mdp_hr_video_need_video_comp(ctl)) {
		pr_debug("fake video done\n");
		mdss_dsi_video_comp(ctl);
	}

	spin_lock(&ctx->hrtimer_lock);
	if (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2) {
		pr_debug("LEAVE ignore\n");
		spin_unlock(&ctx->hrtimer_lock);
		return;
	}
	frms = vsync_adjust(ctx);
	if (ctx->int_type != IT_VSYNC_INT) {
		ctx->int_type = IT_VSYNC_INT;
		skip_update = true;
	}
#endif /* CONFIG_SHDISP */
	vsync_time = ktime_get();
	ctl->vsync_cnt++;

	MDSS_XLOG(ctl->num, ctl->vsync_cnt, ctl->vsync_cnt);

	pr_debug("intr ctl=%d vsync cnt=%u vsync_time=%d\n",
		 ctl->num, ctl->vsync_cnt, (int)ktime_to_ms(vsync_time));

	ctx->polling_en = false;
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (1) {
		u32 flush_reg = mdss_mdp_ctl_read(ctl, MDSS_MDP_REG_CTL_FLUSH);
		if (((flush_reg & BIT(17)) == 0) ||
		    ((ctx->video_state == COMMIT) &&
		     (ctx->state_table[COMMIT].next_state == SUSPEND) &&
		     (!ctx->state_table[SUSPEND].tg) &&
		     (ctx->tg_state == SW_TG_OFF))) {
			complete_all(&ctx->vsync_comp);
		} else {
			pr_debug("not flushed yet, %08x\n", flush_reg);
			need_comp = true;
		}
	}
	if (ctx->suspend) {
		goto exit_intr_done_0;
	}
	if (ctx->video_state == POWER_OFF) {
		goto exit_intr_done_0;
	}
	if (ctx->tg_state == SW_TG_OFF || ctx->sw_tg_off_state == SW_TG_OFF_REQ) {
		if (ctx->sw_tg_off_state == SW_TG_OFF_REQ) {
			ctx->sw_tg_off_state = SW_TG_OFF_NONE;
		}
		mdss_mdp_timegen_enable(ctl, false);
	}
	mdss_mdp_hr_polarity_update(ctx, skip_update, frms);
	if (!skip_update) {
		update_state(ctx);
	}
	if (ctx->video_state_ctrl >= 2) {
		if (ctx->video_state_ctrl == 2) {
			need_int = true;
		}
	} else if (ctx->video_state != COMMIT && ctx->video_state_tg && ctx->video_state_ctrl == 1) {
		if (!need_comp && !ctx->swt_vsync && !ctx->wait_pending) {
			need_int = true;
		}
		ctx->sw_tg_off_state = SW_TG_OFF_BLOCK;
	} else {
		ctx->sw_tg_off_state = SW_TG_OFF_NONE;
	}
exit_intr_done_0:
	if (!skip_update) {
		vsync_common(ctx);
	}
	if (ctx->tg_state == HW_TG_OFF || ctx->video_state_ctrl == 2) {
		int tick = hr_video_get_next_tick(ctx);
		int delay = ktime_to_ns(ktime_sub(ktime_get(), ctx->swt_hint));
		int next = tick*ctx->swt_time_of_vsync - delay;
		if (next < 0) {
			next = 0;
		}
		if (ctx->swt_vsync) {
			hr_video_start_periodic_timer(ctx, next);
		} else {
			hr_video_start_variable_timer(ctx, next, tick);
		}
		pr_debug("LEAVE next %9d (delayed %9d)\n", next, delay);
	} else {
		if (!ctx->suspend && ctx->video_state != POWER_OFF) {
			if (ctx->swt_vsync || ctx->video_state_ctrl != 3) {
				hr_video_start_oneshot_timer(ctx, need_int?INT_ENABLE_PERIOD:0);
			}
		} else {
			need_int = false;
		}
		pr_debug("LEAVE\n");
	}
	if (need_int) {
		hr_video_clk_onoff(ctx, CO_INT_OFF);
	}
	if (ctx->qos_on) {
		if (ctx->qos_on > 1) {
			hr_video_clk_onoff(ctx, CO_QOS_OFF);
		}
		ctx->qos_on = 0;
	}
	spin_unlock(&ctx->hrtimer_lock);
#else  /* CONFIG_SHDISP */
	complete_all(&ctx->vsync_comp);
	spin_lock(&ctx->vsync_lock);
	list_for_each_entry(tmp, &ctx->vsync_handlers, list) {
		tmp->vsync_handler(ctl, vsync_time);
	}
	spin_unlock(&ctx->vsync_lock);
#endif /* CONFIG_SHDISP */
}

static int mdss_mdp_hr_video_pollwait(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx = ctl->priv_data;
	u32 mask, status;
	int rc;

	mask = MDP_INTR_MASK_INTF_VSYNC(ctl->intf_num);

	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
	rc = readl_poll_timeout(ctl->mdata->mdp_base + MDSS_MDP_REG_INTR_STATUS,
		status,
		(status & mask) || try_wait_for_completion(&ctx->vsync_comp),
		1000,
		VSYNC_TIMEOUT_US);
	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);

	if (rc == 0) {
		MDSS_XLOG(ctl->num, ctl->vsync_cnt);
		pr_debug("vsync poll successful! rc=%d status=0x%x\n",
				rc, status);
		ctx->poll_cnt++;
		if (status) {
			struct mdss_mdp_vsync_handler *tmp;
			unsigned long flags;
			ktime_t vsync_time = ktime_get();

			spin_lock_irqsave(&ctx->vsync_lock, flags);
			list_for_each_entry(tmp, &ctx->vsync_handlers, list)
				tmp->vsync_handler(ctl, vsync_time);
			spin_unlock_irqrestore(&ctx->vsync_lock, flags);
		}
	} else {
		pr_warn("vsync poll timed out! rc=%d status=0x%x mask=0x%x\n",
				rc, status, mask);
	}

	return rc;
}

static int mdss_mdp_hr_video_wait4comp(struct mdss_mdp_ctl *ctl, void *arg)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	int rc;

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

	WARN(!ctx->wait_pending, "waiting without commit! ctl=%d", ctl->num);

	if (ctx->polling_en) {
		rc = mdss_mdp_hr_video_pollwait(ctl);
	} else {
		mutex_unlock(&ctl->lock);
		rc = wait_for_completion_timeout(&ctx->vsync_comp,
				usecs_to_jiffies(VSYNC_TIMEOUT_US));
		mutex_lock(&ctl->lock);
		if (rc == 0) {
			pr_warn("vsync wait timeout %d, fallback to poll mode\n",
					ctl->num);
			ctx->polling_en++;
			rc = mdss_mdp_hr_video_pollwait(ctl);
		} else {
			rc = 0;
		}
	}
	mdss_mdp_ctl_notify(ctl,
			rc ? MDP_NOTIFY_FRAME_TIMEOUT : MDP_NOTIFY_FRAME_DONE);

	if (ctx->wait_pending) {
		ctx->wait_pending = 0;
#ifndef CONFIG_SHDISP /* CUST_ID_00027 */
		hr_video_vsync_irq_disable(ctl);
#endif /* CONFIG_SHDISP */
	}

	return rc;
}

static void hr_recover_underrun_work(struct work_struct *work)
{
	struct mdss_mdp_ctl *ctl =
		container_of(work, typeof(*ctl), recover_work);

	if (!ctl || !ctl->add_vsync_handler) {
		pr_err("ctl or vsync handler is NULL\n");
		return;
	}

	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
	ctl->add_vsync_handler(ctl, &ctl->recover_underrun_handler);
	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
}

static void mdss_mdp_hr_video_underrun_intr_done(void *arg)
{
	struct mdss_mdp_ctl *ctl = arg;
	if (unlikely(!ctl))
		return;

	ctl->underrun_cnt++;
	MDSS_XLOG(ctl->num, ctl->underrun_cnt);
	trace_mdp_video_underrun_done(ctl->num, ctl->underrun_cnt);
	pr_debug("display underrun detected for ctl=%d count=%d\n", ctl->num,
			ctl->underrun_cnt);

	if (ctl->opmode & MDSS_MDP_CTL_OP_PACK_3D_ENABLE)
		schedule_work(&ctl->recover_work);
}

static int mdss_mdp_hr_video_timegen_update(struct mdss_mdp_hr_video_ctx *ctx,
					struct mdss_panel_info *pinfo)
{
	u32 hsync_period, vsync_period;
	u32 hsync_start_x, hsync_end_x, display_v_start, display_v_end;
	u32 display_hctl, hsync_ctl;

	hsync_period = mdss_panel_get_htotal(pinfo, true);
	vsync_period = mdss_panel_get_vtotal(pinfo);

	display_v_start = ((pinfo->lcdc.v_pulse_width +
			pinfo->lcdc.v_back_porch) * hsync_period) +
					pinfo->lcdc.hsync_skew;
	display_v_end = ((vsync_period - pinfo->lcdc.v_front_porch) *
				hsync_period) + pinfo->lcdc.hsync_skew - 1;

	hsync_start_x = pinfo->lcdc.h_back_porch + pinfo->lcdc.h_pulse_width;
	hsync_end_x = hsync_period - pinfo->lcdc.h_front_porch - 1;

	hsync_ctl = (hsync_period << 16) | pinfo->lcdc.h_pulse_width;
	display_hctl = (hsync_end_x << 16) | hsync_start_x;

	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_HSYNC_CTL, hsync_ctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
				vsync_period * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PULSE_WIDTH_F0,
			pinfo->lcdc.v_pulse_width * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_HCTL, display_hctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_START_F0,
						display_v_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_END_F0, display_v_end);

	return 0;
}

static int mdss_mdp_hr_video_hfp_fps_update(struct mdss_mdp_hr_video_ctx *ctx,
			struct mdss_panel_data *pdata, int new_fps)
{
	int curr_fps;
	int add_h_pixels = 0;
	int hsync_period;
	int diff;

	hsync_period = mdss_panel_get_htotal(&pdata->panel_info, true);
	curr_fps = mdss_panel_get_framerate(&pdata->panel_info);

	diff = curr_fps - new_fps;
	add_h_pixels = mult_frac(hsync_period, diff, new_fps);
	pdata->panel_info.lcdc.h_front_porch += add_h_pixels;

	mdss_mdp_hr_video_timegen_update(ctx, &pdata->panel_info);
	return 0;
}

static int mdss_mdp_hr_video_vfp_fps_update(struct mdss_mdp_hr_video_ctx *ctx,
				 struct mdss_panel_data *pdata, int new_fps)
{
	int curr_fps;
	int add_v_lines = 0;
	u32 current_vsync_period_f0, new_vsync_period_f0;
	int vsync_period, hsync_period;
	int diff;

	vsync_period = mdss_panel_get_vtotal(&pdata->panel_info);
	hsync_period = mdss_panel_get_htotal(&pdata->panel_info, true);
	curr_fps = mdss_panel_get_framerate(&pdata->panel_info);

	diff = curr_fps - new_fps;
	add_v_lines = mult_frac(vsync_period, diff, new_fps);
	pdata->panel_info.lcdc.v_front_porch += add_v_lines;

	vsync_period = mdss_panel_get_vtotal(&pdata->panel_info);
	current_vsync_period_f0 = mdp_hr_video_read(ctx,
		MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0);
	new_vsync_period_f0 = (vsync_period * hsync_period);

	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			current_vsync_period_f0 | 0x800000);
	if (new_vsync_period_f0 & 0x800000) {
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			new_vsync_period_f0);
	} else {
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			new_vsync_period_f0 | 0x800000);
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			new_vsync_period_f0 & 0x7fffff);
	}

	return 0;
}

static int mdss_mdp_hr_video_fps_update(struct mdss_mdp_hr_video_ctx *ctx,
				 struct mdss_panel_data *pdata, int new_fps)
{
	int rc;

	if (pdata->panel_info.dfps_update ==
				DFPS_IMMEDIATE_PORCH_UPDATE_MODE_HFP)
		rc = mdss_mdp_hr_video_hfp_fps_update(ctx, pdata, new_fps);
	else
		rc = mdss_mdp_hr_video_vfp_fps_update(ctx, pdata, new_fps);

	return rc;
}

static int mdss_mdp_hr_video_dfps_wait4vsync(struct mdss_mdp_ctl *ctl)
{
	int rc = 0;
	struct mdss_mdp_hr_video_ctx *ctx;

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

#ifndef CONFIG_SHDISP /* CUST_ID_00027 */
	hr_video_vsync_irq_enable(ctl, true);
#endif /* CONFIG_SHDISP */
	INIT_COMPLETION(ctx->vsync_comp);
	rc = wait_for_completion_timeout(&ctx->vsync_comp,
		usecs_to_jiffies(VSYNC_TIMEOUT_US));
	WARN(rc <= 0, "timeout (%d) vsync interrupt on ctl=%d\n",
		rc, ctl->num);

#ifndef CONFIG_SHDISP /* CUST_ID_00027 */
	hr_video_vsync_irq_disable(ctl);
#endif /* CONFIG_SHDISP */
	if (rc <= 0)
		return -EPERM;

	return 0;
}

static int mdss_mdp_hr_video_dfps_check_line_cnt(struct mdss_mdp_ctl *ctl)
{
	struct mdss_panel_data *pdata;
	u32 line_cnt;
	pdata = ctl->panel_data;
	if (pdata == NULL) {
		pr_err("%s: Invalid panel data\n", __func__);
		return -EINVAL;
	}

	line_cnt = mdss_mdp_hr_video_line_count(ctl);
	if (line_cnt >=	pdata->panel_info.yres/2) {
		pr_err("Too few lines left line_cnt=%d yres/2=%d\n",
			line_cnt,
			pdata->panel_info.yres/2);
		return -EPERM;
	}
	return 0;
}

static void mdss_mdp_hr_video_timegen_flush(struct mdss_mdp_ctl *ctl,
					struct mdss_mdp_hr_video_ctx *sctx)
{
	u32 ctl_flush;
	struct mdss_data_type *mdata;
	mdata = ctl->mdata;
	ctl_flush = (BIT(31) >> (ctl->intf_num - MDSS_MDP_INTF0));
	if (sctx) {
		/* For 8939, sctx is always INTF2 and the flush bit is BIT 31 */
		if (mdata->mdp_rev == MDSS_MDP_HW_REV_108)
			ctl_flush |= BIT(31);
		else
			ctl_flush |= (BIT(31) >>
					(sctx->intf_num - MDSS_MDP_INTF0));
	}
	mdss_mdp_ctl_write(ctl, MDSS_MDP_REG_CTL_FLUSH, ctl_flush);
}

static int mdss_mdp_hr_video_config_fps(struct mdss_mdp_ctl *ctl,
					struct mdss_mdp_ctl *sctl, int new_fps)
{
	struct mdss_mdp_hr_video_ctx *ctx, *sctx = NULL;
	struct mdss_panel_data *pdata;
	int rc = 0;
	u32 hsync_period, vsync_period;
	struct mdss_data_type *mdata;
	u32 inum = ctl->intf_num - MDSS_MDP_INTF0;

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

	mdata = ctl->mdata;
	if (sctl) {
		sctx = (struct mdss_mdp_hr_video_ctx *) sctl->priv_data;
		if (!sctx) {
			pr_err("invalid ctx\n");
			return -ENODEV;
		}
	} else if (is_pingpong_split(ctl->mfd)) {
		/*
		 * On targets when destination split is enabled, mixer swap
		 * is not valid. So we can safely assume that ctl->intf_num
		 * is INTF1. For this case, increment inum to retrieve sctx.
		 */
		inum += 1;
		if (inum < mdata->nintf) {
			sctx = ((struct mdss_mdp_hr_video_ctx *)
						mdata->video_intf) + inum;
			if (!sctx) {
				pr_err("invalid sctx\n");
				return -ENODEV;
			}
		} else {
			pr_err("Invalid intf number: %d\n",
						(inum + MDSS_MDP_INTF0));
			return -ENODEV;
		}
	}

	pdata = ctl->panel_data;
	if (pdata == NULL) {
		pr_err("%s: Invalid panel data\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (0 <= new_fps && new_fps <= 6) {
		if (new_fps == ctx->mfr) {
			rc = -EINVAL;
		}
		if (!ctx->timegen_en) {
			rc = -EINVAL;
		}
		pr_debug("NEW_FPS=%d, MFR=%d, %d\n", new_fps, ctx->mfr, rc);
		if (rc == 0) {
			struct state_table tbl = {0};
			hr_video_clk_onoff(ctx, CO_ALL_ON_SYNC);
			if (new_fps) {
				tbl.next_state = HR_VIDEO_MFR_REFRESH;
				tbl.tick = new_fps - 1;
			} else {
				tbl.next_state = UPDATE_FRAME;
			}
			tbl.cb = hr_video_cb_mfr;
			commit_state(ctx, &tbl);
			pr_debug("done MFR=%d\n", ctx->mfr);
		}
		return rc;
	}
#endif /* CONFIG_SHDISP */
	if (!pdata->panel_info.dynamic_fps) {
		pr_err("%s: Dynamic fps not enabled for this panel\n",
						__func__);
		return -EINVAL;
	}

	vsync_period = mdss_panel_get_vtotal(&pdata->panel_info);
	hsync_period = mdss_panel_get_htotal(&pdata->panel_info, true);

	if (pdata->panel_info.dfps_update
			!= DFPS_SUSPEND_RESUME_MODE) {
		if (pdata->panel_info.dfps_update
				== DFPS_IMMEDIATE_CLK_UPDATE_MODE) {
			if (!ctx->timegen_en) {
				pr_err("TG is OFF. DFPS mode invalid\n");
				return -EINVAL;
			}
			rc = mdss_mdp_ctl_intf_event(ctl,
					MDSS_EVENT_PANEL_UPDATE_FPS,
					(void *) (unsigned long) new_fps);
			WARN(rc, "intf %d panel fps update error (%d)\n",
							ctl->intf_num, rc);
		} else if (pdata->panel_info.dfps_update
				== DFPS_IMMEDIATE_PORCH_UPDATE_MODE_VFP ||
				pdata->panel_info.dfps_update
				== DFPS_IMMEDIATE_PORCH_UPDATE_MODE_HFP) {
			unsigned long flags;
			if (!ctx->timegen_en) {
				pr_err("TG is OFF. DFPS mode invalid\n");
				return -EINVAL;
			}

			/*
			 * there is possibility that the time of mdp flush
			 * bit set and the time of dsi flush bit are cross
			 * vsync boundary. therefore wait4vsync is needed
			 * to guarantee both flush bits are set within same
			 * vsync period regardless of mdp revision.
			 */
			rc = mdss_mdp_hr_video_dfps_wait4vsync(ctl);
			if (rc < 0) {
				pr_err("Error during wait4vsync\n");
				return rc;
			}

			mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
			spin_lock_irqsave(&ctx->dfps_lock, flags);

			rc = mdss_mdp_hr_video_dfps_check_line_cnt(ctl);
			if (rc < 0)
				goto exit_dfps;

			rc = mdss_mdp_hr_video_fps_update(ctx, pdata, new_fps);
			if (rc < 0) {
				pr_err("%s: Error during DFPS\n", __func__);
				goto exit_dfps;
			}
			if (sctx) {
				rc = mdss_mdp_hr_video_fps_update(sctx,
							pdata->next, new_fps);
				if (rc < 0) {
					pr_err("%s: DFPS error\n", __func__);
					goto exit_dfps;
				}
			}
			rc = mdss_mdp_ctl_intf_event(ctl,
					MDSS_EVENT_PANEL_UPDATE_FPS,
					(void *) (unsigned long) new_fps);
			WARN(rc, "intf %d panel fps update error (%d)\n",
							ctl->intf_num, rc);

			mdss_mdp_hr_fetch_start_config(ctx, ctl);
			if (sctx)
				mdss_mdp_hr_fetch_start_config(sctx, ctl);

			/*
			 * MDP INTF registers support DB on targets
			 * starting from MDP v1.5.
			 */
			if (mdata->mdp_rev >= MDSS_MDP_HW_REV_105)
				mdss_mdp_hr_video_timegen_flush(ctl, sctx);

exit_dfps:
			spin_unlock_irqrestore(&ctx->dfps_lock, flags);
			mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
		} else {
			pr_err("intf %d panel, unknown FPS mode\n",
							ctl->intf_num);
			return -EINVAL;
		}
	} else {
		rc = mdss_mdp_ctl_intf_event(ctl,
				MDSS_EVENT_PANEL_UPDATE_FPS,
				(void *) (unsigned long) new_fps);
		WARN(rc, "intf %d panel fps update error (%d)\n",
						ctl->intf_num, rc);
	}

	return rc;
}

static int mdss_mdp_hr_video_display(struct mdss_mdp_ctl *ctl, void *arg)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_mdp_ctl *sctl;
	struct mdss_panel_data *pdata = ctl->panel_data;
	int rc;

	pr_debug("kickoff ctl=%d\n", ctl->num);

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	atomic_set(&ctx->overlay_hint, 2);
#endif /* CONFIG_SHDISP */

	if (!ctx->wait_pending) {
		ctx->wait_pending++;
#ifndef CONFIG_SHDISP /* CUST_ID_00027 */
		hr_video_vsync_irq_enable(ctl, true);
#endif /* CONFIG_SHDISP */
		INIT_COMPLETION(ctx->vsync_comp);
	} else {
		WARN(1, "commit without wait! ctl=%d", ctl->num);
	}

	MDSS_XLOG(ctl->num, ctl->underrun_cnt);

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	if (ctx->suspend) {
		return 0;
	}
#endif /* CONFIG_SHDISP */
	if (!ctx->timegen_en) {
		rc = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_LINK_READY, NULL);
		if (rc) {
			pr_warn("intf #%d link ready error (%d)\n",
					ctl->intf_num, rc);
			hr_video_vsync_irq_disable(ctl);
			ctx->wait_pending = 0;
			return rc;
		}

		rc = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_UNBLANK, NULL);
		WARN(rc, "intf %d unblank error (%d)\n", ctl->intf_num, rc);

		pr_debug("enabling timing gen for intf=%d\n", ctl->intf_num);

		if (pdata->panel_info.cont_splash_enabled &&
			!ctl->mfd->splash_info.splash_logo_enabled) {
			rc = wait_for_completion_timeout(&ctx->vsync_comp,
					usecs_to_jiffies(VSYNC_TIMEOUT_US));
		}

		rc = mdss_iommu_ctrl(1);
		if (IS_ERR_VALUE(rc)) {
			pr_err("IOMMU attach failed\n");
			return rc;
		}

		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);

		mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_UNDER_RUN, ctl->intf_num);
		sctl = mdss_mdp_get_split_ctl(ctl);
		if (sctl)
			mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_UNDER_RUN,
				sctl->intf_num);

		mdss_bus_bandwidth_ctrl(true);

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
		if (ctx->video_state == POWER_OFF) {
			ctx->swt_hint = ktime_get();
			hr_video_start_periodic_timer(ctx, ctx->swt_time_of_vsync);
		}
		hr_video_clk_onoff(ctx, CO_ALL_ON_SYNC);
		ctx->timegen_en = true;
		commit_state(ctx, NULL);
#else  /* CONFIG_SHDISP */
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, 1);
		wmb();
#endif /* CONFIG_SHDISP */

		rc = wait_for_completion_timeout(&ctx->vsync_comp,
				usecs_to_jiffies(VSYNC_TIMEOUT_US));
		WARN(rc == 0, "timeout (%d) enabling timegen on ctl=%d\n",
				rc, ctl->num);

		ctx->timegen_en = true;
		rc = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_PANEL_ON, NULL);
		WARN(rc, "intf %d panel on error (%d)\n", ctl->intf_num, rc);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	} else {
		hr_video_clk_onoff(ctx, CO_ALL_ON_SYNC);
		commit_state(ctx, NULL);
#endif /* CONFIG_SHDISP */
	}

	return 0;
}

int mdss_mdp_hr_video_reconfigure_splash_done(struct mdss_mdp_ctl *ctl,
	bool handoff)
{
	struct mdss_panel_data *pdata;
	int i, ret = 0, off;
	u32 data, flush;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_mdp_ctl *sctl = mdss_mdp_get_split_ctl(ctl);

	off = 0;
	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx for ctl=%d\n", ctl->num);
		return -ENODEV;
	}

	pdata = ctl->panel_data;

	pdata->panel_info.cont_splash_enabled = 0;
	if (sctl)
		sctl->panel_data->panel_info.cont_splash_enabled = 0;
	else if (ctl->panel_data->next && is_pingpong_split(ctl->mfd))
		ctl->panel_data->next->panel_info.cont_splash_enabled = 0;

	if (!handoff) {
		ret = mdss_mdp_ctl_intf_event(ctl, MDSS_EVENT_CONT_SPLASH_BEGIN,
					      NULL);
		if (ret) {
			pr_err("%s: Failed to handle 'CONT_SPLASH_BEGIN' event\n"
				, __func__);
			return ret;
		}

		/* clear up mixer0 and mixer1 */
		flush = 0;
		for (i = 0; i < 2; i++) {
			data = mdss_mdp_ctl_read(ctl,
				MDSS_MDP_REG_CTL_LAYER(i));
			if (data) {
				mdss_mdp_ctl_write(ctl,
					MDSS_MDP_REG_CTL_LAYER(i),
					MDSS_MDP_LM_BORDER_COLOR);
				flush |= (0x40 << i);
			}
		}
		mdss_mdp_ctl_write(ctl, MDSS_MDP_REG_CTL_FLUSH, flush);

		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, 0);
		/* wait for 1 VSYNC for the pipe to be unstaged */
		msleep(20);

		ret = mdss_mdp_ctl_intf_event(ctl,
			MDSS_EVENT_CONT_SPLASH_FINISH, NULL);
	}

	return ret;
}

static bool mdss_mdp_hr_fetch_programable(struct mdss_mdp_ctl *ctl)
{
	struct mdss_panel_info *pinfo = &ctl->panel_data->panel_info;
	struct mdss_data_type *mdata;
	bool ret;

	mdata = ctl->mdata;

	if (mdata->mdp_rev >= MDSS_MDP_HW_REV_105) {
		if ((pinfo->lcdc.v_back_porch + pinfo->lcdc.v_front_porch) <
				MDP_MIN_FETCH) {
			pr_warn_once("low vbp+vfp may lead to perf issues in some cases\n");
		}
		ret = true;

		if (pinfo->lcdc.v_back_porch > MDP_MIN_FETCH)
			ret = false;
	} else {
		if (pinfo->lcdc.v_back_porch < MDP_MIN_FETCH)
			pr_warn_once("low vbp may lead to display performance issues");
		ret = false;
	}

	return ret;
}

static void mdss_mdp_hr_fetch_start_config(struct mdss_mdp_hr_video_ctx *ctx,
		struct mdss_mdp_ctl *ctl)
{
	int fetch_start, fetch_enable, v_total, h_total;
	struct mdss_data_type *mdata;
	struct mdss_panel_info *pinfo = &ctl->panel_data->panel_info;

	mdata = ctl->mdata;

	if (!mdss_mdp_hr_fetch_programable(ctl)) {
		pr_debug("programmable fetch is not needed/supported\n");
		ctl->prg_fet = false;
		return;
	}

	/*
	 * Fetch should always be outside the active lines. If the fetching
	 * is programmed within active region, hardware behavior is unknown.
	 */
	v_total = mdss_panel_get_vtotal(pinfo);
	h_total = mdss_panel_get_htotal(pinfo, true);
	fetch_start = (v_total - mdss_mdp_max_fetch_lines(pinfo)) * h_total + 1;
	fetch_enable = BIT(31);
	ctl->prg_fet = true;

	pr_debug("ctl:%d, fetch start=%d\n", ctl->num, fetch_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_PROG_FETCH_START, fetch_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_CONFIG, fetch_enable);
}

static int mdss_mdp_hr_video_intfs_setup(struct mdss_mdp_ctl *ctl,
	struct mdss_panel_data *pdata, int inum)
{
	struct mdss_data_type *mdata;
	struct mdss_panel_info *pinfo;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct intf_timing_params itp = {0};
	u32 dst_bpp;
	int ret = 0;

	if (pdata == NULL)
		return 0;

	if (is_pingpong_split(ctl->mfd)) {
		ret = mdss_mdp_hr_video_intfs_setup(ctl, pdata->next, (inum + 1));
		if (IS_ERR_VALUE(ret))
			return ret;
	}

	mdata = ctl->mdata;
	pinfo = &pdata->panel_info;

	if (inum < mdata->nintf) {
		ctx = ((struct mdss_mdp_hr_video_ctx *) mdata->video_intf) + inum;
		if (ctx->ref_cnt) {
			pr_err("Intf %d already in use\n",
				(inum + MDSS_MDP_INTF0));
			return -EBUSY;
		}
		pr_debug("video Intf #%d base=%pK", ctx->intf_num, ctx->base);
		ctx->ref_cnt++;
	} else {
		pr_err("Invalid intf number: %d\n", (inum + MDSS_MDP_INTF0));
		return -EINVAL;
	}

	ctl->priv_data = ctx;
	ctx->intf_type = ctl->intf_type;
	init_completion(&ctx->vsync_comp);
	spin_lock_init(&ctx->vsync_lock);
	spin_lock_init(&ctx->dfps_lock);
	mutex_init(&ctx->vsync_mtx);
	atomic_set(&ctx->vsync_ref, 0);
	INIT_WORK(&ctl->recover_work, hr_recover_underrun_work);
#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	atomic_set(&ctx->vsync_handler_cnt, 0);
	spin_lock_init(&ctx->hrtimer_lock);
	init_completion(&ctx->commit_comp);
	mutex_init(&ctx->commit_mtx);
	ctx->ctl = ctl;
	ctx->tg_state = HW_TG_OFF;
	ctx->video_state = POWER_OFF;
	ctx->video_state_default = UPDATE_FRAME;
	ctx->mfr = 0;
	ctx->mfr_cnt = 0;
	ctx->suspend = 0;
	ctx->pol_state = POL_STATE_INIT;
	ctx->pol_sum = 0;
	hr_video_clk_thread_init(ctx);
	hr_video_tbl_init(ctx);
	ctx->qos_on = 0;
	ctx->qos_req.type = PM_QOS_REQ_AFFINE_CORES;
	ctx->qos_req.cpus_affine.bits[0] = 0x0f; /* little cluster */
	pm_qos_add_request(&ctx->qos_req, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
	atomic_set(&ctx->overlay_hint, 0);
#endif /* CONFIG_SHDISP */

	if (ctl->intf_type == MDSS_INTF_DSI) {
		ctx->intf_recovery.fxn = mdss_mdp_hr_video_intf_recovery;
		ctx->intf_recovery.data = ctl;
		if (mdss_mdp_ctl_intf_event(ctl,
					MDSS_EVENT_REGISTER_RECOVERY_HANDLER,
					(void *)&ctx->intf_recovery)) {
			pr_err("Failed to register intf recovery handler\n");
			return -EINVAL;
		}
	} else {
		ctx->intf_recovery.fxn = NULL;
		ctx->intf_recovery.data = NULL;
	}

	mdss_mdp_set_intr_callback(MDSS_MDP_IRQ_INTF_VSYNC,
				(inum + MDSS_MDP_INTF0),
				mdss_mdp_hr_video_vsync_intr_done, ctl);
	mdss_mdp_set_intr_callback(MDSS_MDP_IRQ_INTF_UNDER_RUN,
				(inum + MDSS_MDP_INTF0),
				mdss_mdp_hr_video_underrun_intr_done, ctl);

	dst_bpp = pinfo->fbc.enabled ? (pinfo->fbc.target_bpp) : (pinfo->bpp);

	itp.width = mult_frac((pinfo->xres + pinfo->lcdc.border_left +
			pinfo->lcdc.border_right), dst_bpp, pinfo->bpp);
	itp.height = pinfo->yres + pinfo->lcdc.border_top +
					pinfo->lcdc.border_bottom;
	itp.border_clr = pinfo->lcdc.border_clr;
	itp.underflow_clr = pinfo->lcdc.underflow_clr;
	itp.hsync_skew = pinfo->lcdc.hsync_skew;

	/* tg active area is not work, hence yres should equal to height */
	itp.xres = mult_frac((pinfo->xres + pinfo->lcdc.border_left +
			pinfo->lcdc.border_right), dst_bpp, pinfo->bpp);

	itp.yres = pinfo->yres + pinfo->lcdc.border_top +
				pinfo->lcdc.border_bottom;

	itp.h_back_porch = pinfo->lcdc.h_back_porch;
	itp.h_front_porch = pinfo->lcdc.h_front_porch;
	itp.v_back_porch = pinfo->lcdc.v_back_porch;
	itp.v_front_porch = pinfo->lcdc.v_front_porch;
	itp.hsync_pulse_width = pinfo->lcdc.h_pulse_width;
	itp.vsync_pulse_width = pinfo->lcdc.v_pulse_width;

	if (!ctl->panel_data->panel_info.cont_splash_enabled) {
		if (mdss_mdp_hr_video_timegen_setup(ctl, &itp)) {
			pr_err("unable to set timing parameters intfs: %d\n",
				(inum + MDSS_MDP_INTF0));
			return -EINVAL;
		}
		mdss_mdp_hr_fetch_start_config(ctx, ctl);
	}

	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_PANEL_FORMAT, ctl->dst_format);

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
	{
		u32 tg_en = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN);
		pr_debug("TG_EN=%d\n", tg_en);
		if (tg_en) {
			ctx->swt_hint = ktime_sub_ns(ktime_get(), hr_video_time_from_vsync(ctx));
			ctx->tg_state = HW_TG_ON;
			commit_state(ctx, NULL);
		}
		hr_video_vsync_irq_enable(ctl, true);
	}
#endif /* CONFIG_SHDISP */
	return 0;
}

int mdss_mdp_hr_video_start(struct mdss_mdp_ctl *ctl)
{
	int intfs_num, ret = 0;

	intfs_num = ctl->intf_num - MDSS_MDP_INTF0;
	ret = mdss_mdp_hr_video_intfs_setup(ctl, ctl->panel_data, intfs_num);
	if (IS_ERR_VALUE(ret)) {
		pr_err("unable to set video interface: %d\n", ret);
		return ret;
	}

	ctl->stop_fnc = mdss_mdp_hr_video_stop;
	ctl->display_fnc = mdss_mdp_hr_video_display;
	ctl->wait_fnc = mdss_mdp_hr_video_wait4comp;
	ctl->read_line_cnt_fnc = mdss_mdp_hr_video_line_count;
	ctl->add_vsync_handler = mdss_mdp_hr_video_add_vsync_handler;
	ctl->remove_vsync_handler = mdss_mdp_hr_video_remove_vsync_handler;
	ctl->config_fps_fnc = mdss_mdp_hr_video_config_fps;

	return 0;
}

void *mdss_mdp_hr_get_intf_base_addr(struct mdss_data_type *mdata,
		u32 interface_id)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	ctx = ((struct mdss_mdp_hr_video_ctx *) mdata->video_intf) + interface_id;
	return (void *)(ctx->base);
}

#ifdef CONFIG_SHDISP /* CUST_ID_00028 */ /* CUST_ID_00041 */
void mdss_mdp_hr_video_transfer_ctrl(struct msm_fb_data_type *mfd, int onoff, int commit)
{
	struct mdss_overlay_private *mdp5_data = NULL;
	struct mdss_mdp_ctl *ctl;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata * mdss_dsi_ctrl;

	if (!mfd) {
		pr_err("invalid mfd\n");
		return;
	}

	mdp5_data = mfd_to_mdp5_data(mfd);
	if (!mdp5_data) {
		pr_err("invalid mdp5_data\n");
		return;
	}

	ctl = mdp5_data->ctl;
	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	ctx = (struct mdss_mdp_hr_video_ctx *)ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("invalid pdata\n");
		return;
	}

	mdss_dsi_ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (onoff) {
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, 1);
		wmb();
		mdss_dsi_ctrl->ctrl_state |= CTRL_STATE_MDP_ACTIVE;
		if (commit == true) {
			ctl->force_screen_state = MDSS_SCREEN_DEFAULT;
			mdss_mdp_display_commit(ctl, NULL, NULL);
			mdss_mdp_display_wait4comp(ctl);
		}
	} else {
		if (commit == true) {
			ctl->force_screen_state = MDSS_SCREEN_FORCE_BLANK;
			mdss_mdp_display_commit(ctl, NULL, NULL);
			mdss_mdp_display_wait4comp(ctl);
		}
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, 0);
		wmb();
		msleep(20);
		mdss_dsi_ctrl->ctrl_state &= ~CTRL_STATE_MDP_ACTIVE;
		mdss_dsi_controller_cfg(true, pdata);
	}
}

int mdss_mdp_hr_video_clkchg_mdp_update(struct mdss_mdp_ctl *ctl)
{
	int ret = 0;
	int fetch_start;
	u32 hsync_period, vsync_period;
	u32 hsync_start_x, hsync_end_x, display_v_start, display_v_end;
	u32 display_hctl, hsync_ctl;
	struct mdss_panel_info *pinfo;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_data_type *mdata;

	pinfo = &ctl->panel_data->panel_info;
	ctx = ctl->priv_data;
	if (pinfo == NULL) {
		pr_err("invalid pinfo\n");
		return -ENODEV;
	}
	if (ctx == NULL) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}
	mdata = ctl->mdata;
	if (mdata == NULL) {
		pr_err("invalid mdata\n");
		return -ENODEV;
	}
	hsync_period = pinfo->lcdc.h_pulse_width + pinfo->lcdc.h_back_porch +
			pinfo->xres + pinfo->lcdc.xres_pad + pinfo->lcdc.h_front_porch;
	vsync_period = pinfo->lcdc.v_pulse_width + pinfo->lcdc.v_back_porch +
			pinfo->yres + pinfo->lcdc.yres_pad + pinfo->lcdc.v_front_porch;

	display_v_start = ((pinfo->lcdc.v_pulse_width + pinfo->lcdc.v_back_porch) *
			hsync_period) + pinfo->lcdc.hsync_skew;
	display_v_end = ((vsync_period - pinfo->lcdc.v_front_porch) * hsync_period) +
			pinfo->lcdc.hsync_skew - 1;

	if (ctx->intf_type == MDSS_INTF_EDP) {
		display_v_start += pinfo->lcdc.h_pulse_width + pinfo->lcdc.h_back_porch;
		display_v_end -= pinfo->lcdc.h_front_porch;
	}

	/* TIMING_2 flush bit on 8939 is BIT 31 */
	if (mdata->mdp_rev == MDSS_MDP_HW_REV_108 &&
				ctx->intf_num == MDSS_MDP_INTF2)
		ctl->flush_bits |= BIT(31);
	else
		ctl->flush_bits |= BIT(31) >>
			(ctx->intf_num - MDSS_MDP_INTF0);

	hsync_start_x = pinfo->lcdc.h_back_porch + pinfo->lcdc.h_pulse_width;
	hsync_end_x = hsync_period - pinfo->lcdc.h_front_porch - 1;

	hsync_ctl = (hsync_period << 16) | pinfo->lcdc.h_pulse_width;
	display_hctl = (hsync_end_x << 16) | hsync_start_x;


	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_HSYNC_CTL, hsync_ctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0,
			vsync_period * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_VSYNC_PULSE_WIDTH_F0,
			   pinfo->lcdc.v_pulse_width * hsync_period);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_HCTL, display_hctl);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_START_F0,
			   display_v_start);
	mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_DISPLAY_V_END_F0, display_v_end);

	if (mdss_mdp_hr_fetch_programable(ctl)) {
		fetch_start = (vsync_period - mdss_mdp_max_fetch_lines(pinfo)) * hsync_period + 1;
		ctl->prg_fet = true;
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_PROG_FETCH_START, fetch_start);
		mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_CONFIG, BIT(31));
	} else {
		ctl->prg_fet = false;
	}

	set_timings(ctx);

	return ret;
}
#endif /* CONFIG_SHDISP */

#ifdef CONFIG_SHDISP /* CUST_ID_00027 */
static void mdss_mdp_hr_polarity_update(struct mdss_mdp_hr_video_ctx *ctx, bool start_tg, int tick)
{
	if (ctx->tg_state != HW_TG_OFF) {
		int state = ctx->pol_state;
		ktime_t vsync = ctx->swt_hint;
		if (state == POL_STATE_INIT) {
			state = POL_STATE_POS;
			ctx->pol_sum = 0;
		} else {
			int diff = ktime_to_us(ktime_sub(vsync, ctx->pol_time));
			int sum = ctx->pol_sum;
			int need = 0;
			if (!start_tg && tick > 1) {
				int us = ctx->swt_time_of_vsync/1000;
				if ((tick & 1) == 0) {
					if (state == POL_STATE_POS) {
						sum += us;
						state = POL_STATE_NEG;
					} else {
						sum -= us;
						state = POL_STATE_POS;
					}
				}
				diff = us;
			}
			if (state == POL_STATE_POS) {
				sum += diff;
				state = POL_STATE_NEG;
				if (sum < 0) {
					need = 1;
				}
			} else {
				sum -= diff;
				state = POL_STATE_POS;
				if (sum > 0) {
					need = 1;
				}
			}
			ctx->pol_sum = sum;
			ctx->pol_need = need;

			pr_debug("POLARITY: %s, need=%d, sum=%d, diff=%d\n",
				 (state == POL_STATE_POS)?"POS":"NEG", need, sum, diff);
		}
		ctx->pol_time = vsync;
		ctx->pol_state = state;
	}
	return;
}

static int mdss_dsi_clk_lane(struct mdss_panel_data *pdata, int hs)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	u32 tmp;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	tmp = MIPI_INP((ctrl_pdata->ctrl_base) + 0xac);
	if (hs) {
		if (hs < 0) {
			return tmp & (1 << 28);
		}
		tmp |= (1 << 28);
	} else {
		tmp &= ~(1 << 28);
	}

	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp);
	wmb();

	return 0;
}

static void mdss_dsi_hs(struct mdss_panel_data *pdata)
{
	mdss_dsi_clk_lane(pdata, 1);
	return;
}

static void mdss_dsi_lp(struct mdss_panel_data *pdata)
{
	mdss_dsi_clk_lane(pdata, 0);
	return;
}

static void mdss_mdp_dsi_clk_ctl(struct mdss_mdp_hr_video_ctx *ctx, int onoff)
{
	mdss_mdp_ctl_intf_event(ctx->ctl, MDSS_EVENT_PANEL_CLK_CTRL, (void *)((long int)onoff));
	return;
}

static inline void mdss_mdp_hr_video_clk_on(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (ctx->timegen_en) {
		int rc;
		mdss_bus_bandwidth_ctrl(true);
		rc = mdss_iommu_ctrl(1);
		if (IS_ERR_VALUE(rc)) {
			pr_err("IOMMU attach failed\n");
		}
		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
		mdss_mdp_dsi_clk_ctl(ctx, 1);
		hr_video_intr_enable(ctx, false);
	}
}

static inline void mdss_mdp_hr_video_clk_off(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (ctx->timegen_en) {
		hr_video_intr_disable(ctx);
		mdss_mdp_dsi_clk_ctl(ctx, 0);
		mdss_iommu_ctrl(0);
		mdss_bus_bandwidth_ctrl(false);
		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
	}
}

static int mdss_mdp_timegen_enable(struct mdss_mdp_ctl *ctl, bool enable)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	int state;
	char *clk = "";

	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

	state = ctx->tg_state;
	if (enable) {
		if (state != HW_TG_ON) {
			state = HW_TG_ON;
		}
	} else {
		if (state == HW_TG_ON) {
			state = SW_TG_OFF;
		} else if (state == SW_TG_OFF) {
			state = HW_TG_OFF;
		}
	}

	if (ctx->tg_state != state) {
		if (ctx->tg_state == HW_TG_OFF) {
			mdss_dsi_hs(ctx->ctl->panel_data);
			udelay(6);
			clk = "hs";
		}

		ctx->tg_state = state;

		if (state == HW_TG_OFF) {
			mdss_dsi_lp(ctx->ctl->panel_data);
			mdss_dsi_state_reset(ctx->ctl->panel_data);
			ctx->need_dsi_sw_reset = false;
			clk = "lp";
		} else {
			if (state == HW_TG_ON) {
				state = 1;
			} else {
				state = 0;
			}
			mdp_hr_video_write(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN, state);
			wmb();
			if (state) {
				ctx->need_dsi_sw_reset = false;
				ctx->hw_tg_off_time = 0;
			} else {
				ctx->need_dsi_sw_reset = true;
				ctx->hw_tg_off_time =
						local_clock() + hr_video_time_to_vsync(ctx) +
						ctx->swt_time_of_line * (ctx->swt_total_line - ctx->swt_vsync_line + 1);
			}
		}
		pr_debug("TG=%d, tg_state=%d %s\n", enable, ctx->tg_state, clk);
	}

	return 0;
}

static int mdss_mdp_get_timegen_state(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (ctx->hw_tg_off_time) {
		if (local_clock() > ctx->hw_tg_off_time) {
			return HW_TG_OFF;
		} else {
			return SW_TG_OFF;
		}
	} else {
		return ctx->tg_state;
	}
}

static void commit_sw_tg_off(struct mdss_mdp_hr_video_ctx *ctx, int flg)
{
	int line, diff;
	bool line_ok, diff_ok;

	diff = (int)(ctx->sw_tg_off_limit_time - local_clock());
	diff_ok = (diff > 0);
	line = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_LINE_COUNT);
	line_ok = (1 < line && line <= ctx->swt_tg_off_limit_line);
	if (line_ok && diff_ok) {
		mdss_mdp_timegen_enable(ctx->ctl, false);
	} else {
		ctx->sw_tg_off_state = SW_TG_OFF_REQ;
	}
	pr_debug("flg=%d, line_ok=%d, line= %4d, diff_ok=%d, diff= %9d\n", flg, line_ok, line, diff_ok, diff);
	return;
}

static int commit_state_cb(struct mdss_mdp_hr_video_ctx *ctx)
{
	int ret = 1;

	if (ctx->commit_cb) {
		ret = ctx->commit_cb(ctx);
	}
	if (ret == 1) {
		ctx->state_table[COMMIT].cb = NULL;
		complete(&ctx->commit_comp);
	}
	return ret;
}

static void commit_state(struct mdss_mdp_hr_video_ctx *ctx, struct state_table *tbl)
{
	unsigned long flags;
	int state;

	if (tbl) {
		mutex_lock(&ctx->commit_mtx);
		INIT_COMPLETION(ctx->commit_comp);
	}
	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (tbl == NULL) {
		if (ctx->video_state == COMMIT) {
			pr_debug("already COMMIT state\n");
			goto done;
		}
		state = ctx->video_state_default;
	} else {
		if (tbl->cb == hr_video_cb_resume) {
			tbl->next_state = ctx->video_state_save;
		} else if (tbl->next_state == SUSPEND) {
			ctx->video_state_save = ctx->video_state_default;
		} else if (tbl->next_state == UPDATE_FRAME ||
		           tbl->next_state == HR_VIDEO_MFR_REFRESH) {
			ctx->video_state_save = tbl->next_state;
		}
		state = tbl->next_state;
		ctx->commit_cb = tbl->cb;
		ctx->state_table[COMMIT].cb = commit_state_cb;
		ctx->state_table[COMMIT].tick = tbl->tick;
		ctx->video_state_default = state;
	}
	if (ctx->tg_state == HW_TG_ON && !ctx->state_table[state].tg) {
		commit_sw_tg_off(ctx, 1);
	}
	pr_debug("commit <%s>\n", state_str[state]);
	ctx->state_table[COMMIT].next_state = state;
	if (state == HR_VIDEO_MFR_REFRESH && ctx->mfr) {
		hr_video_init_mfr(ctx, false);
	}
	if (ctx->video_state_ctrl > 2) {
		if (ctx->video_state_ctrl == 3) {
			hr_video_start_oneshot_timer(ctx, 0);
		}
		if (ctx->qos_on == 1) {
			ctx->qos_on = 2;
			hr_video_clk_onoff(ctx, CO_QOS_ON);
		}
	}
	ctx->video_state = COMMIT;
	ctx->video_state_tick = 1;
 done:
	atomic_set(&ctx->overlay_hint, 0);
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
	hr_video_clk_sync(ctx);
	if (tbl) {
		if (wait_for_completion_timeout(&ctx->commit_comp, usecs_to_jiffies(VSYNC_TIMEOUT_US)) <= 0) {
			pr_err("commit_comp timeout\n");
		}
		mutex_unlock(&ctx->commit_mtx);
	}
	return;
}

static int hr_video_time_from_vsync(struct mdss_mdp_hr_video_ctx *ctx)
{
	s32 line, lorg = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_LINE_COUNT);
	line = lorg - ctx->swt_vsync_line;
	if (line < 0) {
		line += ctx->swt_total_line;
	}
	if (line > ctx->swt_total_line) {
		line = ctx->swt_total_line;
	}
	return line*ctx->swt_time_of_line;
}

static int hr_video_time_to_vsync(struct mdss_mdp_hr_video_ctx *ctx)
{
	return ctx->swt_time_of_vsync - hr_video_time_from_vsync(ctx);
}

static void hr_video_push_frames(struct mdss_mdp_hr_video_ctx *ctx, int frames)
{
	if (ctx->tg_state != HW_TG_ON) {
		mdss_mdp_timegen_enable(ctx->ctl, true);
	}
	return;
}

static void update_state(struct mdss_mdp_hr_video_ctx *ctx)
{
	int state = ctx->video_state;
	int tick = ctx->video_state_tick;

	if (ctx->int_type == IT_VSYNC_INT) {
		tick -= 1;
	} else {
		tick -= ctx->swt_tick;
	}
	if (tick < 0) {
		tick = 0;
	}
	ctx->video_state_tick = tick;
	if (tick) {
		pr_debug("keep <%s>, tick=%d\n", state_str[state], tick);
	} else {
		struct state_table *tab = &ctx->state_table[state], *next = tab;
		int prev_state = state;
		do {
			state = next->next_state;
			next = &ctx->state_table[state];
		} while (!next->tick);
		if ((ctx->tg_state == HW_TG_OFF && next->tg && ctx->int_type == IT_VSYNC_INT) ||
		    (ctx->tg_state != HW_TG_OFF && !next->tg)) {
			pr_debug("stay <%s>, need 1 vsync , tg_state=<%s>, next_tg=%d <%s>, int=%d\n", state_str[prev_state],
				 tg_state_str[ctx->tg_state], next->tg, state_str[state], ctx->int_type);
		} else if (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2) {
			pr_debug("stay <%s>, wait mfr refresh\n", state_str[prev_state]);
		} else if (tab->cb && !tab->cb(ctx)) {
			pr_debug("stay <%s>, wait condition\n", state_str[prev_state]);
		} else {
			ctx->video_state = state;
			ctx->video_state_tg = next->tg;
			ctx->video_state_tick = next->tick;
			ctx->video_state_ctrl = next->clk_int_ctrl;
			tick = next->tick;
			pr_debug("set  <%s>, tick=%d, ctrl=%d\n", state_str[state], tick, ctx->video_state_ctrl);
			ctx->swt_next_timing = ktime_add_ns(ctx->swt_hint, tick*ctx->swt_time_of_vsync);
			ctx->swt_next_tg = ctx->state_table[next->next_state].tg;
			if (next->tg) {
				if (ctx->int_type == IT_VSYNC_INT) {
					/* already pushed, clear */
					tick = 0;
				}
				if (ctx->swt_next_tg) {
					/* in case MFR is 1 */
					tick++;
				}
				if (tick && !ctx->suspend) {
					hr_video_push_frames(ctx, tick);
				}
			}
		}
	}
	return;
}

static int vsync_adjust(struct mdss_mdp_hr_video_ctx *ctx)
{
	s64 diff = ktime_to_ns(ktime_sub(ktime_get(), ctx->swt_hint));
	int frms = 0, d;
	if (diff > 0) {
		if (diff >= ctx->swt_time_of_frames) {
			d = do_div(diff, ctx->swt_time_of_frames);
			frms = diff;
			frms *= ctx->swt_frame_rate;
			diff *= ctx->swt_time_of_frames;
		} else {
			d = diff;
			diff = 0;
		}
		if (d >= ctx->swt_time_of_vsync) {
			int frm = d/ctx->swt_time_of_vsync;
			d -= frm*ctx->swt_time_of_vsync;
			diff += frm*ctx->swt_time_of_vsync;
			frms += frm;
		}
		if (frms) {
			ctx->swt_hint = ktime_add_ns(ctx->swt_hint, diff);
			pr_debug("swt_hint += %3d (%12lld +%9d)\n", frms, diff, d);
		}
	}
	return frms;
}

static void vsync_common(struct mdss_mdp_hr_video_ctx *ctx)
{
	ktime_t curr = ktime_get();
	struct mdss_mdp_vsync_handler *tmp;

	if (ctx->int_type == IT_VSYNC_INT && ctx->tg_state == HW_TG_OFF) {
		ktime_sub_ns(curr, ctx->swt_time_of_line * (ctx->swt_total_line - ctx->swt_vsync_line));
	}
	pr_debug("isr=%d handler=%d\n", ctx->int_type, atomic_read(&ctx->vsync_handler_cnt));
	if (atomic_read(&ctx->vsync_handler_cnt) > 0) {
		spin_lock(&ctx->vsync_lock);
		list_for_each_entry(tmp, &ctx->vsync_handlers, list) {
			tmp->vsync_handler(ctx->ctl, curr);
		}
		spin_unlock(&ctx->vsync_lock);
	}
}

static enum hrtimer_restart hrt_vsync_cb(struct hrtimer *timer)
{
	unsigned long flags;
	int ret = HRTIMER_NORESTART, res = 0, adj = 0, qos = 0;
	struct swtimer *swt = container_of(timer, typeof(*swt), hrt);
	struct mdss_mdp_hr_video_ctx *ctx = container_of(swt, typeof(*ctx), timer[swt->id]);
	s64 left = 0;

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	pr_debug("ENTER\n");
	if (swt->state != SWT_RUNNING) {
		goto exit_cb_1;
	}
	left = swt->base + swt->expire - local_clock();
	qos = swt->qos;
	if (qos) {
		bool tg_check = false;
		if (qos == 1) {
			res = SW_TG_OFF_TIMING_1;
			if (swt->mode != SWT_ONESHOT || left < SW_TG_OFF_TIMING_2 + SW_TG_OFF_TIMING_3) {
				qos = 2;
			} else {
				swt->qos = 2;
			}
		}
		if (qos == 2) {
			res += SW_TG_OFF_TIMING_2 + SW_TG_OFF_TIMING_3;
			swt->qos = 0;
			if (swt->mode == SWT_ONESHOT) {
				tg_check = true;
			} else if (swt->mode == SWT_ONESHOT_L) {
				res = 0;
			}
		} else if (qos == 3) {
			res = SW_TG_OFF_TIMING_3;
			qos = 0;
			swt->qos = 0;
			tg_check = true;
		}
		if (res && left < 0) {
			pr_debug("too late\n");
			adj = res;
			res = 0;
		} else if (tg_check) {
			if (ctx->video_state != COMMIT &&
			    ctx->tg_state == HW_TG_ON &&
			    ctx->sw_tg_off_state == SW_TG_OFF_NONE &&
			    ctx->video_state_tick <= 1) {
				int hint = atomic_read(&ctx->overlay_hint);
				if (hint == 1 && qos == 2 && left > (SW_TG_OFF_TIMING_2 >> 1) + SW_TG_OFF_TIMING_3) {
					res -= SW_TG_OFF_TIMING_3;
					swt->qos = 3;
				} else if (hint != 2) {
					commit_sw_tg_off(ctx, qos);
				}
			}
			if (swt->qos == 0) {
				res = 0;
			}
		}
		if (res) {
			ret = HRTIMER_RESTART;
			goto exit_cb_1;
		}
	}
	if (swt->mode >= SWT_ONESHOT) {
		goto exit_cb_1;
	}
	ctx->swt_hint = ktime_add_ns(ctx->swt_hint, swt->ticks*ctx->swt_time_of_vsync);
	pr_debug("swt_hint += %3d\n", swt->ticks);
	if (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2) {
		ctx->mfr_cnt += swt->ticks;
		if (ctx->mfr_cnt >= 2) {
			ctx->mfr_cnt = 0;
			mdss_mdp_timegen_enable(ctx->ctl, false);
		}
	}
	if (ctx->video_state != POWER_OFF) {
		bool need_vsync = false;
		ctx->swt_tick = swt->ticks;
		if (!ctx->suspend) {
			need_vsync = true;
		} else if (ctx->state_table[SUSPEND].tg == 0) {
			need_vsync = true;
			complete_all(&ctx->vsync_comp);
		}
		update_state(ctx);
		if (need_vsync) {
			vsync_common(ctx);
		}
		if (ctx->suspend) {
			ret = HRTIMER_RESTART;
			goto exit_cb_1;
		}
	}
	if (ctx->tg_state == HW_TG_OFF ||
	    (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2)) {
		ret = HRTIMER_RESTART;
	}
exit_cb_1:
	if (ret == HRTIMER_RESTART) {
		if (res == 0) {
			if (swt->mode == SWT_PERIODIC) {
				swt->ticks = 1;
			} else {
				swt->ticks = hr_video_get_next_tick(ctx);
			}
			res = swt->ticks*ctx->swt_time_of_vsync;
			swt->base += swt->expire;
			swt->expire = res;
			if (ctx->swt_vsync || ctx->swt_next_tg) {
				res -= SW_TG_OFF_TIMING;
				swt->qos = 1;
			}
			qos = -1;
		}
		hrtimer_forward_now(timer, ns_to_ktime(res + adj));
		pr_debug("LEAVE RESTART %10d (left %10lld)\n", res + adj, left);
	} else {
		if (swt->state == SWT_RUNNING &&
		    (swt->mode == SWT_VARIABLE || swt->mode == SWT_PERIODIC)) {
			ctx->int_type = IT_PENDING;
		}
		swt->state = SWT_EXPIRED;
		pr_debug("LEAVE NORESTART (left %10lld)\n", left);
	}
	if (qos) {
		if (qos < 0) {
			if (ctx->qos_on > 1) {
				hr_video_clk_onoff(ctx, CO_QOS_OFF);
			}
			ctx->qos_on = 0;
		} else {
			bool need_int = false;
			if (ctx->video_state_ctrl == 2 &&
			    (swt->mode == SWT_VARIABLE || ctx->video_state_tick == 1)) {
				need_int = true;
				if (swt->mode == SWT_VARIABLE) {
					swt->mode = SWT_PERIODIC;
					ctx->int_type = IT_PERIODIC_TIMER;
				}
			}
			if (swt->mode == SWT_ONESHOT_L || need_int) {
				if (is_int_on(ctx) == 0) {
					hr_video_clk_onoff(ctx, CO_INT_ON);
				}
			}
			if (ctx->qos_on == 0) {
				if (ctx->video_state_ctrl == 2 && !ctx->swt_vsync && ctx->video_state != COMMIT) {
					ctx->qos_on = 1;
				} else if (ctx->video_state_ctrl < 3 || ctx->swt_vsync || ctx->video_state == COMMIT) {
					ctx->qos_on = 2;
					hr_video_clk_onoff(ctx, CO_QOS_ON);
				} else {
					ctx->qos_on = 1;
				}
			}
		}
	}
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);

	return ret;
}

static void hr_video_init_mfr(struct mdss_mdp_hr_video_ctx *ctx, bool enable_only)
{
	int ref = 1;
	int state = HR_VIDEO_MFR_WAIT;
	int ctrl = 0;
	int tick = ctx->mfr;
	
	if (enable_only || tick == 1) {
		ref = tick;
		state = HR_VIDEO_MFR_REFRESH;
		if (tick == 1) {
			ctrl = 1;
		} else {
			ctrl = 2;
		}
	}
	if (ctx->state_table[HR_VIDEO_MFR_REFRESH].clk_int_ctrl != (ctrl & 15)) {
		ctx->state_table[HR_VIDEO_MFR_REFRESH].tick = ref;
		ctx->state_table[HR_VIDEO_MFR_REFRESH].next_state = state;
		ctx->state_table[HR_VIDEO_MFR_REFRESH].clk_int_ctrl = ctrl & 15;
		ctx->state_table[HR_VIDEO_MFR_WAIT].tick = tick - ref;
		ctx->state_table[HR_VIDEO_MFR_WAIT].clk_int_ctrl = (ctrl >> 4) & 15;
	}
	if (ctx->video_state_ctrl != 2) {
		ctx->mfr_cnt = 0;
	}
	pr_debug("eo=%d stt=%d tic=%d ref=%d mfr=%d cnt=%d ctr=%d:%d\n",
		 enable_only, state, tick, ref, ctx->mfr, ctx->mfr_cnt, ctrl, ctx->video_state_ctrl);
	return;
}

static int hr_video_cb_set_mfr(struct mdss_mdp_hr_video_ctx *ctx)
{
	int ret = is_int_on(ctx);
	
	if (ret == 0) {
		pr_debug("int not on\n");
		hr_video_clk_onoff(ctx, CO_INT_ON);
	} else if (ctx->video_state_ctrl >= 2) {
		ctx->sw_tg_off_state = SW_TG_OFF_REQ;
		ctx->mfr_cnt = 0;
	}
	return ret;
}

static int hr_video_cb_update_mfr(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (ctx->video_state_ctrl < 2) {
		int cnt = ctx->mfr_cnt + ctx->mfr;
		if (cnt < MFR_EO_CNT) {
			ctx->mfr_cnt = cnt;
		} else {
			hr_video_init_mfr(ctx, true);
			ctx->sw_tg_off_state = SW_TG_OFF_REQ;
			ctx->mfr_cnt = 0;
		}
	}
	return hr_video_cb_set_mfr(ctx);
}

static int hr_video_cb_mfr(struct mdss_mdp_hr_video_ctx *ctx)
{
	struct state_table *tab = &ctx->state_table[COMMIT];

	if (ctx->video_state_default == HR_VIDEO_MFR_REFRESH) {
		ctx->mfr = tab->tick + 1;
		ctx->state_table[HR_VIDEO_MFR_REFRESH].clk_int_ctrl = -1;
		hr_video_init_mfr(ctx, false);
	} else {
		ctx->mfr = 0;
	}
	return 1;
}

static int hr_video_cb_suspend(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (ctx->state_table[SUSPEND].tg) {
		mdss_mdp_timegen_enable(ctx->ctl, true);
		if (ctx->int_type == IT_VSYNC_INT) {
			hr_video_start_periodic_timer(ctx, ctx->swt_time_of_vsync);
		}
	}
	ctx->suspend = 1;
	return 1;
}

static int hr_video_cb_resume(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN)) {
		ctx->tg_state = HW_TG_ON;
	} else {
		ctx->tg_state = HW_TG_OFF;
	}
	ctx->suspend = 0;
	return 1;
}

static int hr_video_cb_power_off(struct mdss_mdp_hr_video_ctx *ctx)
{
	return 1;
}

static int hr_video_cb_start_repeat(struct mdss_mdp_hr_video_ctx *ctx)
{
	struct state_table *tab = &ctx->state_table[REPEAT_FRAME];
	int tick = REPEAT_CNT;

	if (ctx->pol_need) {
		tick++;
	}
	if (tab->tick != tick) {
		tab->tick = tick;
	}
	ctx->repeat_done = true;
	return 1;
}

static int hr_video_cb_start_wait(struct mdss_mdp_hr_video_ctx *ctx)
{
	int tick = ctx->hr_video_wait_tick_wait;

	if (!ctx->swt_vsync) {
		if (hr_video_clk_onoff(ctx, CO_CLK_OFF) != 0) {
			return 0;
		}
	}
	if (ctx->repeat_done) {
		ctx->repeat_done = false;
		tick = ctx->hr_video_wait_tick;
	}
	ctx->state_table[HR_VIDEO_WAIT].tick = tick;
	return 1;
}

static int hr_video_cb_start_prepare(struct mdss_mdp_hr_video_ctx *ctx)
{
	if (!ctx->swt_vsync) {
		hr_video_clk_onoff(ctx, CO_CLK_ON);
	}
	return 1;
}

static int hr_video_cb_start_refresh(struct mdss_mdp_hr_video_ctx *ctx)
{
	int ret = is_clk_on(ctx);

	if (ret == 0) {
		pr_debug("clock not on\n");
		hr_video_clk_onoff(ctx, CO_CLK_ON);
	}
	return ret;
}

static int hr_video_get_next_tick(struct mdss_mdp_hr_video_ctx *ctx)
{
	int cnt = 1;

	if (ctx->video_state != COMMIT && !ctx->swt_vsync) {
		if ((!ctx->video_state_tg || ctx->video_state_ctrl == 2) && ctx->video_state_tick > 1) {
			cnt = ctx->video_state_tick;
		}
	}
	return cnt;
}

static void hr_video_sw_vsync(struct mdss_mdp_hr_video_ctx *ctx, bool enable)
{
	pr_debug("vsync=%d, enable=%d <%s> tg=%d tick=%d clk_ctrl=%d\n",
		 ctx->swt_vsync, enable, state_str[ctx->video_state], ctx->video_state_tg, ctx->video_state_tick, ctx->video_state_ctrl);
	if ((ctx->swt_vsync && !enable) ||
	    (!ctx->swt_vsync && enable)) {
		ctx->swt_vsync = enable;
		if (!enable && ctx->video_state != COMMIT && ctx->video_state_ctrl) {
			if (!ctx->video_state_tg || ctx->video_state_ctrl >= 2) {
				if (ctx->video_state_tick > 1 && ctx->int_type == IT_PERIODIC_TIMER) {
					if (ctx->video_state_tg || hr_video_clk_onoff(ctx, CO_CLK_OFF) == 0) {
						hr_video_switch_periodic_timer(ctx, false);
					}
				}
			} else {
				int sec = INT_ENABLE_PERIOD;
				hr_video_start_oneshot_timer(ctx, sec);
				hr_video_clk_onoff(ctx, CO_INT_OFF);
			}
		}
	}
	return;
}

static void hr_video_clk_ctrl(struct mdss_mdp_hr_video_ctx *ctx, int onoff)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (onoff) {
		if (ctx->clk_ref_cnt == 0) {
			hr_video_sw_vsync(ctx, true);
		}
		ctx->clk_ref_cnt++;
	} else if (ctx->clk_ref_cnt) {
		if (ctx->clk_ref_cnt == 1) {
			hr_video_sw_vsync(ctx, false);
		}
		ctx->clk_ref_cnt--;
	}
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
	if (onoff) {
		hr_video_clk_onoff(ctx, CO_ALL_ON_SYNC);
		hr_video_clk_sync(ctx);
	}
	return;
}

static void hr_video_clk_sync(struct mdss_mdp_hr_video_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->clk_task_lock, flags);
	ctx->clk_off_block = false;
	spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
	return;
}

static int is_clk_on_sub(struct mdss_mdp_hr_video_ctx *ctx, int shift)
{
	int rc = 0;
	unsigned long flags;

	spin_lock_irqsave(&ctx->clk_task_lock, flags);
	if (shift == 0) {
		if (ctx->clk_state_clk == CLK_STATE_ON) {
			rc = 1;
		}
	} else {
		if (ctx->clk_state_int == CLK_STATE_ON) {
			rc = 1;
			shift = CS_SHIFT;
		} else {
			if (ctx->clk_state_work == CLK_STATE_NONE &&
			    (CS_CLK_MASK & (ctx->clk_state_req >> CS_SHIFT)) == CLK_STATE_OFF &&
			    (CS_CLK_MASK & (ctx->clk_state >> CS_SHIFT)) == CLK_STATE_OFF) {
				pr_debug("Call INT_ON\n");
				hr_video_intr_enable(ctx, true);
				ctx->clk_state_int = CLK_STATE_ON;
				ctx->clk_state_req &= ~(CS_CLK_MASK << CS_SHIFT);
				ctx->clk_state_req |= CLK_STATE_ON << CS_SHIFT;
				ctx->clk_state &= ~(CS_CLK_MASK << CS_SHIFT);
				ctx->clk_state |= CLK_STATE_ON << CS_SHIFT;
				pr_debug("Done INT_ON\n");
				rc = 1;
			}
		}
	}
	if (rc) {
		int state_work = CS_CLK_MASK & (ctx->clk_state_work >> shift);
		if (state_work != CLK_STATE_NONE) {
			int state_req = CS_CLK_MASK & (ctx->clk_state_req >> shift);
			if (state_work == CLK_STATE_OFF || state_req == CLK_STATE_OFF) {
				rc = 0;
			}
		}
	}
	spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
	return rc;
}

static int is_clk_on(struct mdss_mdp_hr_video_ctx *ctx)
{
	return is_clk_on_sub(ctx, 0);
}

static int is_int_on(struct mdss_mdp_hr_video_ctx *ctx)
{
	return is_clk_on_sub(ctx, 1);
}

#define NEED_WAKE (1 << 0)
#define NEED_WAIT (1 << 1)
#define NEED_INT  (1 << 2)
#define NEED_LOCK (1 << 3)
#define NEED_BLOCKED (1 << 4)

static int hr_video_clk_onoff(struct mdss_mdp_hr_video_ctx *ctx, enum clk_onoff onoff)
{
	unsigned long flags;
	int cur_state, new_state, need = 0;

	spin_lock_irqsave(&ctx->clk_task_lock, flags);
	if (ctx->clk_task == NULL) {
		pr_debug("clk_task is NULL\n");
		goto exit_onoff;
	}
	cur_state = ctx->clk_state_req;
	new_state = cur_state;
	switch (onoff) {
	case CO_ALL_ON_SYNC:
		ctx->clk_off_block = true;
		new_state = CLK_STATE_ON | (CLK_STATE_ON << CS_SHIFT) | (cur_state & CS_QOS_MASK);
		need = NEED_INT | NEED_LOCK;
		if (new_state == cur_state &&
		    ((new_state ^ ctx->clk_state) & (CS_CLK_MASK | CS_INT_MASK)) == 0) {
			if (ctx->clk_state_work == CLK_STATE_NONE ||
			    ((new_state ^ ctx->clk_state_work) & (CS_CLK_MASK | CS_INT_MASK)) == 0) {
				goto exit_onoff;
			}
		}
		need |= NEED_WAIT;
		break;
	case CO_QOS_ON:
		new_state = (cur_state & (CS_CLK_MASK|CS_INT_MASK)) | (CLK_STATE_ON << CS_SHIFT*2);
		break;
	case CO_QOS_OFF:
		if (ctx->clk_off_block) {
			need |= NEED_BLOCKED;
			goto exit_onoff;
		}
		new_state = (cur_state & (CS_CLK_MASK|CS_INT_MASK)) | (CLK_STATE_OFF << CS_SHIFT*2);
		break;
	case CO_INT_ON:
		new_state = (cur_state & (CS_CLK_MASK|CS_QOS_MASK)) | (CLK_STATE_ON << CS_SHIFT);
		need = NEED_INT;
		break;
	case CO_INT_OFF:
		if (ctx->clk_off_block) {
			need |= NEED_BLOCKED;
			goto exit_onoff;
		}
		new_state = (cur_state & (CS_CLK_MASK|CS_QOS_MASK)) | (CLK_STATE_OFF << CS_SHIFT);
		break;
	case CO_CLK_ON:
		new_state = CLK_STATE_ON | (cur_state & (CS_INT_MASK|CS_QOS_MASK));
		break;
	case CO_CLK_OFF:
		if (ctx->clk_off_block) {
			need |= NEED_BLOCKED;
			goto exit_onoff;
		}
		new_state = CLK_STATE_OFF | (cur_state & (CS_INT_MASK|CS_QOS_MASK));
		break;
	}
	if (cur_state == new_state) {
		if (ctx->clk_state == new_state && ctx->clk_state_work == CLK_STATE_NONE) {
			goto exit_onoff;
		}
	} else {
		ctx->clk_state_req = new_state;
	}
	if (!ctx->clk_state_running) {
		ctx->clk_state_running = true;
		need |= NEED_WAKE;
	}
	if (need & NEED_WAIT) {
		if (!ctx->clk_state_wait) {
			INIT_COMPLETION(ctx->clk_task_comp);
			ctx->clk_state_wait = true;
		}
	}
 exit_onoff:
	spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
	if (need & NEED_INT) {
		if (need & NEED_LOCK) {
			spin_lock_irqsave(&ctx->hrtimer_lock, flags);
		}
		if (ctx->int_type == IT_VARIABLE_TIMER) {
			hr_video_switch_periodic_timer(ctx, true);
		}
		if (need & NEED_LOCK) {
			spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
		}
	}
	if (need & NEED_WAKE) {
		wake_up_interruptible(&ctx->clk_task_wq);
	}
	if (need & NEED_WAIT) {
		wait_for_completion(&ctx->clk_task_comp);
	}
	pr_debug("<%s>, need=0x%x (wake=%d, wait=%d, int=%d, lock=%d, blocked=%d)\n", clk_onoff_str[onoff], need,
		 (need & NEED_WAKE)/NEED_WAKE,
		 (need & NEED_WAIT)/NEED_WAIT,
		 (need & NEED_INT)/NEED_INT,
		 (need & NEED_LOCK)/NEED_LOCK,
		 (need & NEED_BLOCKED)/NEED_BLOCKED);
	return need & NEED_BLOCKED;
}

static int is_clk_req(struct mdss_mdp_hr_video_ctx *ctx)
{
	int rc = 0;
	unsigned long flags;

	spin_lock_irqsave(&ctx->clk_task_lock, flags);
	if (ctx->clk_state_work != CLK_STATE_NONE) {
		ctx->clk_state = ctx->clk_state_work;
		ctx->clk_state_work = CLK_STATE_NONE;
	}
	if (ctx->clk_state != ctx->clk_state_req) {
		ctx->clk_state_work = ctx->clk_state_req;
		rc = 1;
	} else {
		ctx->clk_state_running = false;
	}
	if (ctx->clk_state_wait &&
	    ((ctx->clk_state & (CS_INT_MASK | CS_CLK_MASK)) == (CLK_STATE_ON | (CLK_STATE_ON << CS_SHIFT)))) {
		complete_all(&ctx->clk_task_comp);
		ctx->clk_state_wait = false;
	}
	spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
	return rc;
}

static int clk_thread(void *data)
{
	struct mdss_mdp_hr_video_ctx *ctx = data;
	struct sched_param param;
	int ret;

	pr_debug("START\n");
	param.sched_priority = 16;
	ret = sched_setscheduler_nocheck(current, SCHED_FIFO, &param);
	if (ret) {
		pr_err("set priority failed (%d)\n", ret);
	}
	while (!kthread_should_stop()) {
		int req, dif, lpm = 0;
		if (wait_event_interruptible(ctx->clk_task_wq, is_clk_req(ctx))) {
			continue;
		}
		req = ctx->clk_state_work;
		if (req == CLK_STATE_NONE) {
			break;
		}
		dif = req ^ ctx->clk_state;
		if (dif & CS_CLK_MASK) {
			if ((req & CS_CLK_MASK) == CLK_STATE_ON) {
				pr_debug("Call CLK_ON\n");
				mdss_mdp_hr_video_clk_on(ctx);
				ctx->clk_state_clk = CLK_STATE_ON;
				pr_debug("Done CLK_ON\n");
			} else {
				pr_debug("Call CLK_OFF\n");
				ctx->clk_state_clk = CLK_STATE_OFF;
				mdss_mdp_hr_video_clk_off(ctx);
				pr_debug("Done CLK_OFF\n");
			}
		}
		if (dif & CS_INT_MASK) {
			if ((req & CS_INT_MASK) == (CLK_STATE_ON << CS_SHIFT)) {
				pr_debug("Call INT_ON\n");
				hr_video_intr_enable(ctx, false);
				ctx->clk_state_int = CLK_STATE_ON;
				pr_debug("Done INT_ON\n");
			} else {
				pr_debug("Call INT_OFF\n");
				ctx->clk_state_int = CLK_STATE_OFF;
				hr_video_intr_disable(ctx);
				pr_debug("Done INT_OFF\n");
				lpm = 1;
			}
		}
		if (dif & CS_QOS_MASK) {
			if ((req & CS_QOS_MASK) == (CLK_STATE_ON << CS_SHIFT*2)) {
				pr_debug("Call QOS_ON\n");
				pm_qos_update_request(&ctx->qos_req, 800);
				pr_debug("Done QOS_ON\n");
			} else {
				pr_debug("Call QOS_OFF\n");
				pm_qos_update_request(&ctx->qos_req, -1);
				pr_debug("Done QOS_OFF\n");
			}
		}
		if (lpm) {
			unsigned long flags;
			spin_lock_irqsave(&ctx->hrtimer_lock, flags);
			if (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2) {
				if (mdss_dsi_clk_lane(ctx->ctl->panel_data, -1) > 0) {
					u32 line_cnt;
					int try = 0;
					do {
						line_cnt = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_LINE_COUNT);
						try++;
					} while (try < 200 && (line_cnt <= ctx->swt_vbp_end_line || line_cnt == ctx->swt_total_line));
					mdss_dsi_lp(ctx->ctl->panel_data);
					pr_debug("lp, line_cnt= %3d, try= %2d\n", line_cnt, try);
				}
			}
			spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
		}
	}
	pr_debug("END\n");
	return 0;
}

static void hr_video_clk_thread_start(struct mdss_mdp_hr_video_ctx *ctx)
{
	pr_debug("ENTER\n");
	ctx->clk_task = kthread_run(clk_thread, (void *)ctx, "clk_thread");
	if (IS_ERR(ctx->clk_task)) {
		pr_debug("kthread_run returns %d\n", (int)PTR_ERR(ctx->clk_task));
		ctx->clk_task = NULL;
	}
	pr_debug("LEAVE\n");
	return;
}

static void hr_video_clk_thread_stop(struct mdss_mdp_hr_video_ctx *ctx)
{
	pr_debug("ENTER\n");
	if (ctx->clk_task) {
		unsigned long flags;
		spin_lock_irqsave(&ctx->clk_task_lock, flags);
		ctx->clk_state_req = CLK_STATE_NONE;
		wake_up_interruptible(&ctx->clk_task_wq);
		spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
		kthread_stop(ctx->clk_task);
		spin_lock_irqsave(&ctx->clk_task_lock, flags);
		if (ctx->clk_state_wait) {
			complete_all(&ctx->clk_task_comp);
		}
		ctx->clk_task = NULL;
		spin_unlock_irqrestore(&ctx->clk_task_lock, flags);
	}
	pr_debug("LEAVE\n");
	return;
}

static void switch_swt(struct mdss_mdp_hr_video_ctx *ctx, bool periodic)
{
	if (ctx->video_state_tick > 1) {
		ktime_t curr = ktime_get();
		if (periodic) {
			int diff = ktime_to_ns(ktime_sub(curr, ctx->swt_hint));
			int tick = diff/ctx->swt_time_of_vsync;
			int period = tick*ctx->swt_time_of_vsync;
			int left = ctx->swt_time_of_vsync - (diff - period);
			if (tick) {
				ctx->video_state_tick -= tick;
				ctx->swt_hint = ktime_add_ns(ctx->swt_hint, period);
				pr_debug("  swt_hint += %3d\n", tick);
				if (ctx->tg_state == SW_TG_OFF && ctx->video_state_ctrl == 2) {
					ctx->mfr_cnt += tick;
				}
			}
			hr_video_start_periodic_timer(ctx, left);
		} else {
			int left = ktime_to_ns(ktime_sub(ctx->swt_next_timing, curr));
			hr_video_start_variable_timer(ctx, left, ctx->video_state_tick);
		}
	}
	return;
}

static void cancel_swt(struct mdss_mdp_hr_video_ctx *ctx, struct swtimer *swt_cancel)
{
	struct swtimer *swt;

	for (swt = &ctx->timer[0]; swt != &ctx->timer[SWT_MAX]; swt++) {
		if (swt == swt_cancel || swt->state == SWT_EXPIRED || swt->state == SWT_CANCEL_PENDING) {
			int ret = hrtimer_try_to_cancel(&swt->hrt);
			int state = (ret < 0)?SWT_CANCEL_PENDING:SWT_IDLE;
			swt->state = state;
		}
	}
	return;
}

static struct swtimer *find_swt(struct mdss_mdp_hr_video_ctx *ctx, int mode)
{
	struct swtimer *swt;
	bool retry = true;
 retry:
	for (swt = &ctx->timer[0]; swt != &ctx->timer[SWT_MAX]; swt++) {
		if (mode == SWT_FREE) {
			if (swt->state == SWT_IDLE) {
				return swt;
			}
		} else {
			if (swt->state == SWT_RUNNING) {
				int m = swt->mode;
				if (m > SWT_ONESHOT) {
					m = SWT_ONESHOT;
				}
				if (m == mode) {
					return swt;
				}
			}
		}
	}
	if (mode == SWT_FREE && retry) {
		retry = false;
		cancel_swt(ctx, NULL);
		goto retry;
	}
	return NULL;
}

static void start_swt(struct mdss_mdp_hr_video_ctx *ctx, int mode, int expire, int ticks)
{
	struct swtimer *swt, *swt_cancel;
	u64 ns;

	swt = find_swt(ctx, SWT_FREE);
	if (mode == SWT_ONESHOT) {
		swt_cancel = find_swt(ctx, SWT_ONESHOT);
		ns = local_clock();
		expire = hr_video_time_to_vsync(ctx);
		if (ticks) {
			if (swt_cancel && swt_cancel->mode == SWT_ONESHOT_L) {
				swt_cancel = NULL;
				pr_debug("still running\n");
				goto exit_start;
			}
			ns = ticks;
			ns *= ctx->swt_time_of_frames;
			ns += expire - ctx->swt_time_of_vsync;
			mode = SWT_ONESHOT_L;
		} else {
			ctx->sw_tg_off_limit_time = ns + expire - ctx->swt_time_of_line * TG_OFF_LIMIT_LINE;
			ns = expire;
		}
		ticks = 0;
	} else {
		if (mode == SWT_VARIABLE) {
			swt_cancel = find_swt(ctx, SWT_PERIODIC);
			ns = expire;
		} else {
			swt_cancel = find_swt(ctx, SWT_VARIABLE);
			ns = expire;
			ticks = 1;
		}
		if (swt_cancel == NULL) {
			swt_cancel = find_swt(ctx, SWT_ONESHOT);
		}
	}
	if (swt) {
		int qos = 0;
		swt->ticks = ticks;
		swt->mode = mode;
		swt->state = SWT_RUNNING;
		swt->expire = ns;
		if (ns > SW_TG_OFF_TIMING) {
			ns -= SW_TG_OFF_TIMING;
			swt->qos = 1;
		} else if (ns > SW_TG_OFF_TIMING_2 + SW_TG_OFF_TIMING_3) {
			ns -= SW_TG_OFF_TIMING_2 + SW_TG_OFF_TIMING_3;
			swt->qos = 2;
			qos = 1;
		} else {
			swt->qos = 0;
			qos = 2;
		}
		swt->base = local_clock();
		hrtimer_start(&swt->hrt, ns_to_ktime(ns), HRTIMER_MODE_REL);
		pr_debug("<%d:%s> %lld, qos=%d\n", swt->id, swt_mode_str[swt->mode], ns, qos);
		if (qos && ctx->qos_on < 2) {
			ctx->qos_on = 2;
			hr_video_clk_onoff(ctx, CO_QOS_ON);
		}
	} else {
		pr_err("no timer available <%s> %lld\n", swt_mode_str[mode], ns);
	}
 exit_start:
	cancel_swt(ctx, swt_cancel);
	return;
}

static void hr_video_start_variable_timer(struct mdss_mdp_hr_video_ctx *ctx, int expire, int ticks)
{
	ctx->int_type = IT_VARIABLE_TIMER;
	start_swt(ctx, SWT_VARIABLE, expire, ticks);
}

static void hr_video_start_periodic_timer(struct mdss_mdp_hr_video_ctx *ctx, int expire)
{
	ctx->int_type = IT_PERIODIC_TIMER;
	start_swt(ctx, SWT_PERIODIC, expire, 1);
}

static void hr_video_start_oneshot_timer(struct mdss_mdp_hr_video_ctx *ctx, int sec)
{
	start_swt(ctx, SWT_ONESHOT, 0, sec);
}

static void hr_video_switch_periodic_timer(struct mdss_mdp_hr_video_ctx *ctx, bool periodic)
{
	switch_swt(ctx, periodic);
}

static void set_timings(struct mdss_mdp_hr_video_ctx *ctx)
{
	struct mdss_panel_info *pinfo;
	u32 clk_rate, total_clk, total_width, total_line;
	unsigned long long total_period = 1000000000;

	ctx->swt_total_line = 1940;
	ctx->swt_time_of_line = 16666667/1940;
	ctx->swt_time_of_vsync = 16666667;
	ctx->swt_time_of_frames = 16666667*60;
	ctx->swt_frame_rate = 60;
	ctx->swt_vsync_line = ctx->swt_total_line - 12 + 1;
	ctx->swt_tg_off_limit_line = ctx->swt_vsync_line - TG_OFF_LIMIT_LINE;
	ctx->swt_vbp_end_line = 8;

	pinfo = &ctx->ctl->panel_data->panel_info;
	if (!pinfo || !pinfo->mipi.dsi_pclk_rate || !pinfo->mipi.frame_rate) {
		goto skip;
	}
	clk_rate = pinfo->mipi.dsi_pclk_rate;
	ctx->swt_frame_rate = pinfo->mipi.frame_rate;
	total_width = (pinfo->lcdc.h_back_porch +
		       pinfo->lcdc.h_front_porch +
		       pinfo->lcdc.h_pulse_width +
		       pinfo->xres);
	total_line = (pinfo->lcdc.v_back_porch +
		      pinfo->lcdc.v_front_porch +
		      pinfo->lcdc.v_pulse_width +
		      pinfo->yres);
	total_clk = total_width*total_line*ctx->swt_frame_rate;
	total_period *= total_clk;
	do_div(total_period, clk_rate);
	ctx->swt_time_of_frames = (int)total_period;
	ctx->swt_time_of_vsync = ctx->swt_time_of_frames/ctx->swt_frame_rate;
	ctx->swt_time_of_line = ctx->swt_time_of_frames/(total_line*ctx->swt_frame_rate);
	ctx->swt_total_line = total_line;
	ctx->swt_vsync_line = ctx->swt_total_line - mdss_mdp_max_fetch_lines(pinfo) + 1;
	ctx->swt_tg_off_limit_line = ctx->swt_vsync_line - TG_OFF_LIMIT_LINE;
	ctx->swt_vbp_end_line = pinfo->lcdc.v_pulse_width + pinfo->lcdc.v_back_porch;
	pr_debug("dsi_pclk_rate:  %9d Hz\n", clk_rate);
	pr_debug("Total width:    %9d pixels\n", total_width);
 skip:
	pr_debug("Total line:     %9d lines\n", ctx->swt_total_line);
	pr_debug("Line of vsync:  %9d lines\n", ctx->swt_vsync_line);
	pr_debug("Line of TG_OFF: %9d lines\n", ctx->swt_tg_off_limit_line);
	pr_debug("Line of VBP end: %8d lines\n", ctx->swt_vbp_end_line);
	pr_debug("Time of line:   %9d ns\n", ctx->swt_time_of_line);
	pr_debug("Time of vsync:  %9d ns\n", ctx->swt_time_of_vsync);
	pr_debug("Time of frames: %9d ns\n", ctx->swt_time_of_frames);
	pr_debug("Frame rate:     %9d fps\n", ctx->swt_frame_rate);
	return;
}

static void hr_video_tbl_init(struct mdss_mdp_hr_video_ctx *ctx)
{
	int tick;
	memcpy(&ctx->state_table, &init_state, sizeof(init_state));
	tick = ctx->swt_frame_rate - 1 - DSI_CLK_DELAY - WAIT_CNT;
	if (tick < 1) {
		tick = 1;
	}
	ctx->hr_video_wait_tick = tick;
	ctx->hr_video_wait_tick_wait = tick + WAIT_CNT;
	tick = ctx->swt_frame_rate/15 - 1;
	if (tick < 1) {
		tick = 1;
	}
	ctx->state_table[BLANK_FRAME].tick = tick;

	tick = DSI_CLK_DELAY;
	if (tick == 0) {
		tick = 1;
	}
	ctx->state_table[HR_VIDEO_PREPARE].tick = tick;
	return;
}

static void hr_video_intr_enable(struct mdss_mdp_hr_video_ctx *ctx, bool isr)
{
	struct mdss_mdp_ctl *sctl = mdss_mdp_get_split_ctl(ctx->ctl);
	if (!isr) {
		mutex_lock(&ctx->vsync_mtx);
	}
	mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_VSYNC, ctx->ctl->intf_num);
	mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_UNDER_RUN, ctx->ctl->intf_num);
	if (sctl)
		mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_UNDER_RUN, sctl->intf_num);
	if (!isr) {
		mutex_unlock(&ctx->vsync_mtx);
	}
}

static void hr_video_intr_disable(struct mdss_mdp_hr_video_ctx *ctx)
{
	struct mdss_mdp_ctl *sctl = mdss_mdp_get_split_ctl(ctx->ctl);
	mutex_lock(&ctx->vsync_mtx);
	mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_UNDER_RUN, ctx->ctl->intf_num);
	if (sctl)
		mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_UNDER_RUN, sctl->intf_num);
	mdss_mdp_irq_disable(MDSS_MDP_IRQ_INTF_VSYNC, ctx->ctl->intf_num);
	mutex_unlock(&ctx->vsync_mtx);
}

static void hr_video_clk_thread_init(struct mdss_mdp_hr_video_ctx *ctx)
{
	int i;

	pr_debug("ENTER\n");
	spin_lock_init(&ctx->clk_task_lock);
	init_waitqueue_head(&ctx->clk_task_wq);
	init_completion(&ctx->clk_task_comp);
	ctx->clk_state_clk = CLK_STATE_ON;
	ctx->clk_state_int = CLK_STATE_ON;
	ctx->clk_state = ctx->clk_state_clk | (ctx->clk_state_int << CS_SHIFT) | (CLK_STATE_OFF << CS_SHIFT*2);
	ctx->clk_state_req = ctx->clk_state;
	ctx->clk_state_work = CLK_STATE_NONE;
	ctx->clk_state_running = false;
	ctx->clk_state_wait = false;
	ctx->clk_off_block = false;
	for (i = 0; i < SWT_MAX; i++) {
		hrtimer_init(&ctx->timer[i].hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ctx->timer[i].hrt.function = hrt_vsync_cb;
		ctx->timer[i].id = i;
		ctx->timer[i].state = SWT_IDLE;
	}
	set_timings(ctx);
	hr_video_clk_thread_start(ctx);
	pr_debug("LEAVE\n");
	return;
}

static void hr_video_clk_thread_deinit(struct mdss_mdp_hr_video_ctx *ctx)
{
	int i;

	pr_debug("ENTER\n");
	for (i = 0; i < SWT_MAX; i++) {
		hrtimer_cancel(&ctx->timer[i].hrt);
	}
	hr_video_clk_thread_stop(ctx);
	pr_debug("LEAVE\n");
	return;
}

static bool mdss_mdp_hr_video_need_video_comp(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata *ctrl;
	u32 tg_en;
	u32 int_ctrl;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return false;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return false;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("invalid pdata\n");
		return false;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	tg_en = mdp_hr_video_read(ctx, MDSS_MDP_REG_INTF_TIMING_ENGINE_EN);
	int_ctrl = readl_relaxed(ctrl->ctrl_base + 0x0110); /* DSI_INT_CTRL */

	return (!tg_en && (int_ctrl & DSI_INTR_VIDEO_DONE_MASK));
}

void mdss_mdp_hr_video_wait_vsync(int us)
{
	struct fb_info * fbi;
	struct msm_fb_data_type *mfd;
	struct mdss_overlay_private *mdp5_data;
	struct mdss_mdp_ctl *ctl;
	struct mdss_mdp_hr_video_ctx *ctx;
	unsigned long flags;
	int remain_us = 0;
	struct fb_info *mdss_fb_get_fbinfo(int);

	fbi = mdss_fb_get_fbinfo(0);
	if (!fbi) {
		return;
	}
	mfd = (struct msm_fb_data_type *)fbi->par;
	if (!mfd) {
		return;
	}
	mdp5_data = mfd_to_mdp5_data(mfd);
	if (!mdp5_data) {
		return;
	}
	ctl = mdp5_data->ctl;
	if (!ctl) {
		return;
	}
	ctx = (struct mdss_mdp_hr_video_ctx *)ctl->priv_data;
	if (!ctx) {
		return;
	}
	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (local_clock() < ctx->sw_tg_off_limit_time) {
		remain_us = ((int)(ctx->sw_tg_off_limit_time - local_clock()))/1000;
	}
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
	if (remain_us > 0 && (remain_us - us) < 100) {
		struct timespec tu;
		tu.tv_sec = 0;
		tu.tv_nsec = remain_us*1000;
		pr_debug("need wait %d us\n", remain_us);
		hrtimer_nanosleep(&tu, NULL, HRTIMER_MODE_REL, CLOCK_MONOTONIC);
		pr_debug("done wait %d us\n", remain_us);
	}
	return;
}

int mdss_mdp_hr_video_clk_ctrl(struct mdss_mdp_ctl *ctl, int onoff)
{
	struct mdss_mdp_hr_video_ctx *ctx;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return -ENODEV;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

	hr_video_clk_ctrl(ctx, (onoff & 1));
	return 0;
}

static int mdss_mdp_hr_video_get_tg_state(struct mdss_mdp_ctl *ctl)
{
	unsigned long flags;
	enum mdss_mdp_hr_video_tg_state ret;
	struct mdss_mdp_hr_video_ctx *ctx;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return -ENODEV;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_debug("invalid ctx\n");
		return HW_TG_OFF;
	}

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	ret = mdss_mdp_get_timegen_state(ctx);
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);

	return ret;
}

int mdss_mdp_hr_video_is_hw_tg_off(void)
{
	struct mdss_mdp_ctl *ctl;

	ctl = mdss_shdisp_get_mdpctrl(0);
	if (!ctl) {
		pr_err("invalid ctl\n");
		return 0;
	}

	return mdss_mdp_hr_video_get_tg_state(ctl) == HW_TG_OFF;
}

static void mdss_mdp_hr_video_wait_sw_tg_off(struct mdss_mdp_ctl *ctl)
{
	unsigned long flags;
	struct mdss_mdp_hr_video_ctx *ctx;
	int remain;
	s64 diff;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_debug("invalid ctx\n");
		return;
	}

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (ctx->hw_tg_off_time == 0) {
		spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
		return;
	}
	diff = ctx->hw_tg_off_time - local_clock();
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);

	if (diff > 0) {
		remain = ((int)diff + 999) / 1000;
		pr_debug("wait start.\n");
		usleep(remain);
		pr_debug("wait end.\n");
	}
}

static void mdss_mdp_hr_video_avoid_vsync(struct mdss_mdp_ctl *ctl, int before_us, int after_us)
{
	s64 tmp;
	struct timespec ts;
	unsigned long flags;
	int proj, subs, remain;
	struct mdss_mdp_hr_video_ctx *ctx;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return;
	}

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (ctx->video_state == POWER_OFF || ctx->suspend) {
		spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
		return;
	}
	tmp = ktime_to_ns(ktime_sub(ktime_get(), ctx->swt_hint));
	tmp -= div64_s64(tmp, ctx->swt_time_of_vsync) * ctx->swt_time_of_vsync;
	proj = (int)tmp / 1000;
	subs = (int)(ctx->swt_time_of_vsync - tmp) / 1000;
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);

	if (proj < after_us) {
		remain = after_us - proj;
	} else if (subs < before_us) {
		remain = subs + after_us;
	} else {
		remain = 0;
	}

	if (remain > 0) {
		ts.tv_sec = 0;
		ts.tv_nsec = remain * 1000;
		pr_debug("need wait %d us\n", remain);
		hrtimer_nanosleep(&ts, NULL, HRTIMER_MODE_REL, CLOCK_MONOTONIC);
		pr_debug("done wait %d us\n", remain);
	}
}

void mdss_mdp_hr_video_avoid_toggle_tg(int margin_us)
{
	struct mdss_mdp_ctl *ctl;

	ctl = mdss_shdisp_get_mdpctrl(0);
	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	mdss_mdp_hr_video_wait_sw_tg_off(ctl);
	mdss_mdp_hr_video_avoid_vsync(ctl, 1500 + margin_us, 3500);
}

void mdss_mdp_hr_video_dsi_sw_reset(void)
{
	unsigned long flags;
	struct mdss_mdp_ctl *ctl;
	struct mdss_mdp_hr_video_ctx *ctx;
	int tg_state;

	ctl = mdss_shdisp_get_mdpctrl(0);
	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	ctx = ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return;
	}

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (ctx->need_dsi_sw_reset) {
		tg_state = mdss_mdp_get_timegen_state(ctx);
		if (tg_state == HW_TG_OFF) {
			mdss_dsi_state_reset(ctx->ctl->panel_data);
			ctx->need_dsi_sw_reset = false;
		}
	}
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);
}

int mdss_mdp_hr_video_suspend(struct mdss_mdp_ctl *ctl, int tg_en_flg)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct state_table tbl = {0};

	pr_debug("ENTER(%d)\n", tg_en_flg);
	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}
	if (!ctx->timegen_en) {
		pr_debug("panel power off\n");
		return -EPERM;
	}
	if (ctx->suspend) {
		pr_err("already suspended\n");
		return -EPERM;
	}
	hr_video_clk_ctrl(ctx, 1);
	ctx->state_table[SUSPEND].tg = tg_en_flg;
	tbl.next_state = SUSPEND;
	tbl.cb = hr_video_cb_suspend;
	commit_state(ctx, &tbl);
	pr_debug("LEAVE\n");
	return 0;
}

int mdss_mdp_hr_video_resume(struct mdss_mdp_ctl *ctl, int tg_en_flg)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct state_table tbl = {0};

	pr_debug("ENTER(%d)\n", tg_en_flg);
	ctx = (struct mdss_mdp_hr_video_ctx *) ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}
	if (!ctx->timegen_en) {
		pr_debug("panel power off\n");
		return -EPERM;
	}
	if (!ctx->suspend) {
		pr_err("not suspended\n");
		return -EPERM;
	}
	tbl.cb = hr_video_cb_resume;
	commit_state(ctx, &tbl);
	hr_video_clk_ctrl(ctx, 0);
	pr_debug("LEAVE\n");
	return 0;
}

void mdss_mdp_hr_video_overlay_hint(struct msm_fb_data_type *mfd)
{
	struct mdss_overlay_private *mdp5_data;
	struct mdss_mdp_ctl *ctl;
	struct mdss_mdp_hr_video_ctx *ctx;
	struct fb_info *mdss_fb_get_fbinfo(int);

	if (mfd->fbi == mdss_fb_get_fbinfo(0)) {
		if ((mdp5_data = mfd_to_mdp5_data(mfd)) != NULL) {
			if ((ctl = mdp5_data->ctl) != NULL) {
				if ((ctx = (struct mdss_mdp_hr_video_ctx *)ctl->priv_data) != NULL) {
					atomic_set(&ctx->overlay_hint, 1);
				}
			}
		}
	}
	return;
}

static void mdss_mdp_hr_video_fps_led_indicate_worker(struct work_struct *work)
{
	struct fps_led_ctx *fps_led;
	unsigned long flags;
	char r, g, b;

	fps_led = container_of(work, struct fps_led_ctx, indicate_work);

	spin_lock_irqsave(&fps_led->lock, flags);
	r = fps_led->led_red;
	g = fps_led->led_green;
	b = fps_led->led_blue;
	spin_unlock_irqrestore(&fps_led->lock, flags);

	pr_debug("FPS_LED: RGBLED=%d%d%d\n", r, g, b);
	mdss_shdisp_tri_led_set_color(r, g, b);

	return;
}

static void mdss_mdp_hr_video_fps_led_process(struct fps_led_ctx *fps_led, bool output)
{
	unsigned long flags;
	enum fps_led_state new_state;

	spin_lock_irqsave(&fps_led->lock, flags);

	fps_led->frame_hist <<= 1;
	if (output) {
		fps_led->frame_hist |= 1;
	}

	pr_debug("FPS_LED: frame_hist=%d%d%d%d%d%d%d%d\n",
			((fps_led->frame_hist >> 7) & 1),
			((fps_led->frame_hist >> 6) & 1),
			((fps_led->frame_hist >> 5) & 1),
			((fps_led->frame_hist >> 4) & 1),
			((fps_led->frame_hist >> 3) & 1),
			((fps_led->frame_hist >> 2) & 1),
			((fps_led->frame_hist >> 1) & 1),
			((fps_led->frame_hist >> 0) & 1));

	new_state = fps_led->state;

	if ((fps_led->frame_hist & 0x03) == 0x03) {
		new_state = FPS_LED_STATE_60HZ;
	} else if ((fps_led->frame_hist & 0x07) == 0x05) {
		new_state = FPS_LED_STATE_30HZ;
	} else if ((fps_led->frame_hist & 0x03) == 0x00) {
		new_state = FPS_LED_STATE_1HZ;
	}

	if (fps_led->state != new_state) {
		switch (new_state) {
		case FPS_LED_STATE_60HZ:
			pr_debug("FPS_LED: 60Hz\n");
			fps_led->led_red = 1;
			fps_led->led_green = 0;
			fps_led->led_blue = 0;
			schedule_work(&fps_led->indicate_work);
			break;
		case FPS_LED_STATE_30HZ:
			pr_debug("FPS_LED: 30Hz\n");
			fps_led->led_red = 0;
			fps_led->led_green = 1;
			fps_led->led_blue = 0;
			schedule_work(&fps_led->indicate_work);
			break;
		case FPS_LED_STATE_1HZ:
			pr_debug("FPS_LED: 1Hz\n");
			fps_led->led_red = 0;
			fps_led->led_green = 0;
			fps_led->led_blue = 1;
			schedule_work(&fps_led->indicate_work);
			break;
		default:
			;
		}
	}

	fps_led->state = new_state;

	spin_unlock_irqrestore(&fps_led->lock, flags);

	return;
}

static void mdss_mdp_hr_video_fps_led_vsync(struct mdss_mdp_hr_video_ctx *ctx)
{
	unsigned long flags;
	int frame_time;
	ktime_t next;
	bool enable;

	if (!ctx->fps_led.inited) {
		return;
	}

	spin_lock_irqsave(&ctx->fps_led.lock, flags);
	enable = ctx->fps_led.enable;
	spin_unlock_irqrestore(&ctx->fps_led.lock, flags);

	if (!enable) {
		return;
	}

	hrtimer_cancel(&ctx->fps_led.timer);
	mdss_mdp_hr_video_fps_led_process(&ctx->fps_led, true);
	if (ctx->swt_time_of_vsync > 0) {
		frame_time = ctx->swt_time_of_vsync;
	} else {
		frame_time = 16666667;
	}
	next = ns_to_ktime(frame_time * 3 / 2);
	hrtimer_start(&ctx->fps_led.timer, next, HRTIMER_MODE_REL);

	return;
}

static enum hrtimer_restart mdss_mdp_hr_video_fps_led_timer_cb(struct hrtimer *timer)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	struct fps_led_ctx *fps_led;
	unsigned long flags;
	bool output;
	int frame_time;
	int tg_state;

	fps_led = container_of(timer, struct fps_led_ctx, timer);
	ctx = container_of(fps_led, struct mdss_mdp_hr_video_ctx, fps_led);

	spin_lock_irqsave(&fps_led->lock, flags);
	if (!fps_led->enable) {
		spin_unlock_irqrestore(&fps_led->lock, flags);
		return HRTIMER_NORESTART;
	}
	spin_unlock_irqrestore(&fps_led->lock, flags);

	spin_lock_irqsave(&ctx->hrtimer_lock, flags);
	if (ctx->swt_time_of_vsync > 0) {
		frame_time = ctx->swt_time_of_vsync;
	} else {
		frame_time = 16666667;
	}
	tg_state = mdss_mdp_get_timegen_state(ctx);
	output = (ctx->video_state != POWER_OFF) &&
	         (!ctx->suspend && (tg_state != HW_TG_OFF));
	spin_unlock_irqrestore(&ctx->hrtimer_lock, flags);

	mdss_mdp_hr_video_fps_led_process(fps_led, output);
	hrtimer_forward_now(timer, ns_to_ktime(frame_time));

	return HRTIMER_RESTART;
}

int mdss_mdp_hr_video_fps_led_start(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	unsigned long flags;
	int frame_time;
	ktime_t next;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return -ENODEV;
	}

	ctx = (struct mdss_mdp_hr_video_ctx *)ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return -ENODEV;
	}

	if (!ctx->fps_led.inited) {
		spin_lock_init(&ctx->fps_led.lock);
		INIT_WORK(&ctx->fps_led.indicate_work, mdss_mdp_hr_video_fps_led_indicate_worker);
		hrtimer_init(&ctx->fps_led.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ctx->fps_led.timer.function = mdss_mdp_hr_video_fps_led_timer_cb;
		ctx->fps_led.inited = true;
	}

	spin_lock_irqsave(&ctx->fps_led.lock, flags);
	if (ctx->fps_led.enable) {
		spin_unlock_irqrestore(&ctx->fps_led.lock, flags);
		return 0;
	}
	ctx->fps_led.enable = true;
	ctx->fps_led.state = FPS_LED_STATE_NONE;
	ctx->fps_led.frame_hist = 0;
	if (ctx->swt_time_of_vsync > 0) {
		frame_time = ctx->swt_time_of_vsync;
	} else {
		frame_time = 16666667;
	}
	spin_unlock_irqrestore(&ctx->fps_led.lock, flags);

	next = ns_to_ktime(frame_time * 3 / 2);
	hrtimer_start(&ctx->fps_led.timer, next, HRTIMER_MODE_REL);

	return 0;
}

void mdss_mdp_hr_video_fps_led_stop(struct mdss_mdp_ctl *ctl)
{
	struct mdss_mdp_hr_video_ctx *ctx;
	unsigned long flags;

	if (!ctl) {
		pr_err("invalid ctl\n");
		return;
	}

	ctx = (struct mdss_mdp_hr_video_ctx *)ctl->priv_data;
	if (!ctx) {
		pr_err("invalid ctx\n");
		return;
	}

	if (!ctx->fps_led.inited) {
		return;
	}

	spin_lock_irqsave(&ctx->fps_led.lock, flags);
	if (!ctx->fps_led.enable) {
		spin_unlock_irqrestore(&ctx->fps_led.lock, flags);
		return;
	}
	ctx->fps_led.enable = false;
	ctx->fps_led.led_red = 0;
	ctx->fps_led.led_green = 0;
	ctx->fps_led.led_blue = 0;
	spin_unlock_irqrestore(&ctx->fps_led.lock, flags);

	hrtimer_cancel(&ctx->fps_led.timer);
	cancel_work_sync(&ctx->fps_led.indicate_work);
	mdss_shdisp_tri_led_set_color(0, 0, 0);

	return;
}
#endif /* CONFIG_SHDISP */
