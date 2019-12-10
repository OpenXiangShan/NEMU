/*
 * NEMU (NJU Emulator) sdhost driver.
 *
 * Author:      Zihao Yu <yuzihao@ict.ac.cn>
 *
 * Based on
 *  bcm2835.c by Phil Elwell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/time.h>
#include <linux/workqueue.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#define SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

#define SDCMD_NEW_FLAG			0x8000
#define SDCMD_FAIL_FLAG			0x4000
#define SDCMD_BUSYWAIT			0x800
#define SDCMD_NO_RESPONSE		0x400
#define SDCMD_LONG_RESPONSE		0x200
#define SDCMD_WRITE_CMD			0x80
#define SDCMD_READ_CMD			0x40
#define SDCMD_CMD_MASK			0x3f

#define SDCDIV_MAX_CDIV			0x7ff

#define SDHSTS_BUSY_IRPT		0x400
#define SDHSTS_BLOCK_IRPT		0x200
#define SDHSTS_SDIO_IRPT		0x100
#define SDHSTS_REW_TIME_OUT		0x80
#define SDHSTS_CMD_TIME_OUT		0x40
#define SDHSTS_CRC16_ERROR		0x20
#define SDHSTS_CRC7_ERROR		0x10
#define SDHSTS_FIFO_ERROR		0x08
/* Reserved */
/* Reserved */
#define SDHSTS_DATA_FLAG		0x01

#define SDHSTS_TRANSFER_ERROR_MASK	(SDHSTS_CRC7_ERROR | \
					 SDHSTS_CRC16_ERROR | \
					 SDHSTS_REW_TIME_OUT | \
					 SDHSTS_FIFO_ERROR)

#define SDHSTS_ERROR_MASK		(SDHSTS_CMD_TIME_OUT | \
					 SDHSTS_TRANSFER_ERROR_MASK)

#define SDHCFG_BUSY_IRPT_EN	BIT(10)
#define SDHCFG_BLOCK_IRPT_EN	BIT(8)
#define SDHCFG_SDIO_IRPT_EN	BIT(5)
#define SDHCFG_DATA_IRPT_EN	BIT(4)
#define SDHCFG_SLOW_CARD	BIT(3)
#define SDHCFG_WIDE_EXT_BUS	BIT(2)
#define SDHCFG_WIDE_INT_BUS	BIT(1)
#define SDHCFG_REL_CMD_LINE	BIT(0)

#define SDVDD_POWER_OFF		0
#define SDVDD_POWER_ON		1

#define SDEDM_FORCE_DATA_MODE	BIT(19)
#define SDEDM_CLOCK_PULSE	BIT(20)
#define SDEDM_BYPASS		BIT(21)

#define SDEDM_WRITE_THRESHOLD_SHIFT	9
#define SDEDM_READ_THRESHOLD_SHIFT	14
#define SDEDM_THRESHOLD_MASK		0x1f

#define SDEDM_FSM_MASK		0xf
#define SDEDM_FSM_IDENTMODE	0x0
#define SDEDM_FSM_DATAMODE	0x1
#define SDEDM_FSM_READDATA	0x2
#define SDEDM_FSM_WRITEDATA	0x3
#define SDEDM_FSM_READWAIT	0x4
#define SDEDM_FSM_READCRC	0x5
#define SDEDM_FSM_WRITECRC	0x6
#define SDEDM_FSM_WRITEWAIT1	0x7
#define SDEDM_FSM_POWERDOWN	0x8
#define SDEDM_FSM_POWERUP	0x9
#define SDEDM_FSM_WRITESTART1	0xa
#define SDEDM_FSM_WRITESTART2	0xb
#define SDEDM_FSM_GENPULSES	0xc
#define SDEDM_FSM_WRITEWAIT2	0xd
#define SDEDM_FSM_STARTPOWDOWN	0xf

#define SDDATA_FIFO_WORDS	16

#define FIFO_READ_THRESHOLD	4
#define FIFO_WRITE_THRESHOLD	4
#define SDDATA_FIFO_PIO_BURST	8

#define PIO_THRESHOLD	1  /* Maximum block count for PIO (0 = always DMA) */

struct nemu_host {
	spinlock_t		lock;
	struct mutex		mutex;

	void __iomem		*ioaddr;
	u32			phys_addr;

	struct mmc_host		*mmc;
	struct platform_device	*pdev;

	int			clock;		/* Current clock speed */
	unsigned int		max_clk;	/* Max possible freq */
	struct work_struct	dma_work;
	struct delayed_work	timeout_work;	/* Timer for timeouts */
	struct sg_mapping_iter	sg_miter;	/* SG state for PIO */
	unsigned int		blocks;		/* remaining PIO blocks */
	int			irq;		/* Device IRQ */

	u32			ns_per_fifo_word;

	/* cached registers */
	u32			hcfg;
	u32			cdiv;

	struct mmc_request	*mrq;		/* Current request */
	struct mmc_command	*cmd;		/* Current command */
	struct mmc_data		*data;		/* Current data request */
	bool			data_complete:1;/* Data finished before cmd */
	bool			use_busy:1;	/* Wait for busy interrupt */
	bool			use_sbc:1;	/* Send CMD23 */

	/* for threaded irq handler */
	bool			irq_block;
	bool			irq_busy;
	bool			irq_data;

	/* DMA part */
	struct dma_chan		*dma_chan_rxtx;
	struct dma_chan		*dma_chan;
	struct dma_slave_config dma_cfg_rx;
	struct dma_slave_config dma_cfg_tx;
	struct dma_async_tx_descriptor	*dma_desc;
	u32			dma_dir;
	u32			drain_words;
	struct page		*drain_page;
	u32			drain_offset;
	bool			use_dma;
};

static void nemu_reset_internal(struct nemu_host *host)
{
}

static void nemu_reset(struct mmc_host *mmc)
{
	struct nemu_host *host = mmc_priv(mmc);

	if (host->dma_chan)
		dmaengine_terminate_sync(host->dma_chan);
	nemu_reset_internal(host);
}

static void nemu_finish_command(struct nemu_host *host);
static void nemu_data_irq(struct nemu_host *host, u32 intmask);

static void nemu_transfer_block_pio(struct nemu_host *host, bool is_read)
{
	unsigned long flags;
	size_t blksize;
	unsigned long wait_max;

	blksize = host->data->blksz;

	wait_max = jiffies + msecs_to_jiffies(500);

	local_irq_save(flags);

	while (blksize) {
		int copy_words;
		u32 hsts = 0;
		size_t len;
		u32 *buf;

		if (!sg_miter_next(&host->sg_miter)) {
			host->data->error = -EINVAL;
			break;
		}

		len = min(host->sg_miter.length, blksize);
		if (len % 4) {
			host->data->error = -EINVAL;
			break;
		}

		blksize -= len;
		host->sg_miter.consumed = len;

		buf = (u32 *)host->sg_miter.addr;

		copy_words = len / 4;

		while (copy_words) {
			int burst_words, words;
			u32 edm;

			burst_words = min(SDDATA_FIFO_PIO_BURST, copy_words);
			edm = readl(host->ioaddr + SDEDM);
			if (is_read)
				words = ((edm >> 4) & 0x1f);
			else
				words = SDDATA_FIFO_WORDS - ((edm >> 4) & 0x1f);

			if (words < burst_words) {
				int fsm_state = (edm & SDEDM_FSM_MASK);
				struct device *dev = &host->pdev->dev;

				if ((is_read &&
				     (fsm_state != SDEDM_FSM_READDATA &&
				      fsm_state != SDEDM_FSM_READWAIT &&
				      fsm_state != SDEDM_FSM_READCRC)) ||
				    (!is_read &&
				     (fsm_state != SDEDM_FSM_WRITEDATA &&
				      fsm_state != SDEDM_FSM_WRITESTART1 &&
				      fsm_state != SDEDM_FSM_WRITESTART2))) {
					hsts = readl(host->ioaddr + SDHSTS);
					dev_err(dev, "fsm %x, hsts %08x\n",
						fsm_state, hsts);
					if (hsts & SDHSTS_ERROR_MASK)
						break;
				}

				if (time_after(jiffies, wait_max)) {
					dev_err(dev, "PIO %s timeout - EDM %08x\n",
						is_read ? "read" : "write",
						edm);
					hsts = SDHSTS_REW_TIME_OUT;
					break;
				}
				ndelay((burst_words - words) *
				       host->ns_per_fifo_word);
				continue;
			} else if (words > copy_words) {
				words = copy_words;
			}

			copy_words -= words;

			while (words) {
				if (is_read)
					*(buf++) = readl(host->ioaddr + SDDATA);
				else
					writel(*(buf++), host->ioaddr + SDDATA);
				words--;
			}
		}

    if (!is_read) {
      panic("can not write");
    }

		if (hsts & SDHSTS_ERROR_MASK)
			break;
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void nemu_transfer_pio(struct nemu_host *host)
{
	struct device *dev = &host->pdev->dev;
	u32 sdhsts;
	bool is_read;

	is_read = (host->data->flags & MMC_DATA_READ) != 0;
	nemu_transfer_block_pio(host, is_read);

	sdhsts = readl(host->ioaddr + SDHSTS);
	if (sdhsts & (SDHSTS_CRC16_ERROR |
		      SDHSTS_CRC7_ERROR |
		      SDHSTS_FIFO_ERROR)) {
		dev_err(dev, "%s transfer error - HSTS %08x\n",
			is_read ? "read" : "write", sdhsts);
		host->data->error = -EILSEQ;
	} else if ((sdhsts & (SDHSTS_CMD_TIME_OUT |
			      SDHSTS_REW_TIME_OUT))) {
		dev_err(dev, "%s timeout error - HSTS %08x\n",
			is_read ? "read" : "write", sdhsts);
		host->data->error = -ETIMEDOUT;
	}
}

static void nemu_start_dma(struct nemu_host *host)
{
  panic("dma not support");
}

static void nemu_set_transfer_irqs(struct nemu_host *host)
{
	u32 all_irqs = SDHCFG_DATA_IRPT_EN | SDHCFG_BLOCK_IRPT_EN |
		SDHCFG_BUSY_IRPT_EN;

	if (host->dma_desc) {
		host->hcfg = (host->hcfg & ~all_irqs) |
			SDHCFG_BUSY_IRPT_EN;
	} else {
		host->hcfg = (host->hcfg & ~all_irqs) |
			SDHCFG_DATA_IRPT_EN |
			SDHCFG_BUSY_IRPT_EN;
	}

	//writel(host->hcfg, host->ioaddr + SDHCFG);
}

static
void nemu_prepare_data(struct nemu_host *host, struct mmc_command *cmd)
{
	struct mmc_data *data = cmd->data;

	WARN_ON(host->data);

	host->data = data;
	if (!data)
		return;

	host->data_complete = false;
	host->data->bytes_xfered = 0;

	if (!host->dma_desc) {
		/* Use PIO */
		int flags = SG_MITER_ATOMIC;

		if (data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;
		sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
		host->blocks = data->blocks;
	}

	nemu_set_transfer_irqs(host);

	writel(data->blksz, host->ioaddr + SDHBCT);
	writel(data->blocks, host->ioaddr + SDHBLC);
}

static void nemu_finish_request(struct nemu_host *host)
{
	struct dma_chan *terminate_chan = NULL;
	struct mmc_request *mrq;

	cancel_delayed_work(&host->timeout_work);

	mrq = host->mrq;

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;

	host->dma_desc = NULL;
	terminate_chan = host->dma_chan;
	host->dma_chan = NULL;

	if (terminate_chan) {
		int err = dmaengine_terminate_all(terminate_chan);

		if (err)
			dev_err(&host->pdev->dev,
				"failed to terminate DMA (%d)\n", err);
	}

	mmc_request_done(host->mmc, mrq);
}

static
bool nemu_send_command(struct nemu_host *host, struct mmc_command *cmd)
{
	//struct device *dev = &host->pdev->dev;
	u32 sdcmd; //, sdhsts;
	//unsigned long timeout;

	WARN_ON(host->cmd);

#if 0
	sdcmd = nemu_read_wait_sdcmd(host, 100);
	if (sdcmd & SDCMD_NEW_FLAG) {
		dev_err(dev, "previous command never completed.\n");
		nemu_dumpregs(host);
		cmd->error = -EILSEQ;
		nemu_finish_request(host);
		return false;
	}

	if (!cmd->data && cmd->busy_timeout > 9000)
		timeout = DIV_ROUND_UP(cmd->busy_timeout, 1000) * HZ + HZ;
	else
		timeout = 10 * HZ;
	schedule_delayed_work(&host->timeout_work, timeout);
#endif

	host->cmd = cmd;

	/* Clear any error flags */
#if 0
	sdhsts = readl(host->ioaddr + SDHSTS);
	if (sdhsts & SDHSTS_ERROR_MASK)
		writel(sdhsts, host->ioaddr + SDHSTS);

	if ((cmd->flags & MMC_RSP_136) && (cmd->flags & MMC_RSP_BUSY)) {
		dev_err(dev, "unsupported response type!\n");
		cmd->error = -EINVAL;
		nemu_finish_request(host);
		return false;
	}
#endif

	nemu_prepare_data(host, cmd);

	writel(cmd->arg, host->ioaddr + SDARG);

	sdcmd = cmd->opcode & SDCMD_CMD_MASK;

	host->use_busy = false;
	if (!(cmd->flags & MMC_RSP_PRESENT)) {
		sdcmd |= SDCMD_NO_RESPONSE;
	} else {
		if (cmd->flags & MMC_RSP_136)
			sdcmd |= SDCMD_LONG_RESPONSE;
		if (cmd->flags & MMC_RSP_BUSY) {
			sdcmd |= SDCMD_BUSYWAIT;
			host->use_busy = true;
		}
	}

	if (cmd->data) {
		if (cmd->data->flags & MMC_DATA_WRITE) {
			sdcmd |= SDCMD_WRITE_CMD;
    }
		if (cmd->data->flags & MMC_DATA_READ)
			sdcmd |= SDCMD_READ_CMD;
	}

	writel(sdcmd | SDCMD_NEW_FLAG, host->ioaddr + SDCMD);

	return true;
}

static void nemu_transfer_complete(struct nemu_host *host)
{
	struct mmc_data *data;

	WARN_ON(!host->data_complete);

	data = host->data;
	host->data = NULL;

	/* Need to send CMD12 if -
	 * a) open-ended multiblock transfer (no CMD23)
	 * b) error in multiblock transfer
	 */
	if (host->mrq->stop && (data->error || !host->use_sbc)) {
		if (nemu_send_command(host, host->mrq->stop)) {
			/* No busy, so poll for completion */
			if (!host->use_busy)
				nemu_finish_command(host);
		}
	} else {
		//nemu_wait_transfer_complete(host);
		nemu_finish_request(host);
	}
}

static void nemu_finish_data(struct nemu_host *host)
{
	struct device *dev = &host->pdev->dev;
	struct mmc_data *data;

	data = host->data;

	host->hcfg &= ~(SDHCFG_DATA_IRPT_EN | SDHCFG_BLOCK_IRPT_EN);
	writel(host->hcfg, host->ioaddr + SDHCFG);

	data->bytes_xfered = data->error ? 0 : (data->blksz * data->blocks);

	host->data_complete = true;

	if (host->cmd) {
		/* Data managed to finish before the
		 * command completed. Make sure we do
		 * things in the proper order.
		 */
		dev_dbg(dev, "Finished early - HSTS %08x\n",
			readl(host->ioaddr + SDHSTS));
	} else {
		nemu_transfer_complete(host);
	}
}

static void nemu_finish_command(struct nemu_host *host)
{
	//struct device *dev = &host->pdev->dev;
	struct mmc_command *cmd = host->cmd;
	//u32 sdcmd;
  int i;

#if 0
	sdcmd = nemu_read_wait_sdcmd(host, 100);

	/* Check for errors */
	if (sdcmd & SDCMD_NEW_FLAG) {
		dev_err(dev, "command never completed.\n");
		nemu_dumpregs(host);
		host->cmd->error = -EIO;
		nemu_finish_request(host);
		return;
	} else if (sdcmd & SDCMD_FAIL_FLAG) {
		u32 sdhsts = readl(host->ioaddr + SDHSTS);

		/* Clear the errors */
		writel(SDHSTS_ERROR_MASK, host->ioaddr + SDHSTS);

		if (!(sdhsts & SDHSTS_CRC7_ERROR) ||
		    (host->cmd->opcode != MMC_SEND_OP_COND)) {
			if (sdhsts & SDHSTS_CMD_TIME_OUT) {
				host->cmd->error = -ETIMEDOUT;
			} else {
				dev_err(dev, "unexpected command %d error\n",
					host->cmd->opcode);
				nemu_dumpregs(host);
				host->cmd->error = -EILSEQ;
			}
			nemu_finish_request(host);
			return;
		}
	}
#endif

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				cmd->resp[3 - i] =
					readl(host->ioaddr + SDRSP0 + i * 4);
			}
		} else {
			cmd->resp[0] = readl(host->ioaddr + SDRSP0);
		}
	}

	if (cmd == host->mrq->sbc) {
		/* Finished CMD23, now send actual command. */
		host->cmd = NULL;
		if (nemu_send_command(host, host->mrq->cmd)) {
			if (host->data) {
        if (host->dma_desc) {
				/* DMA transfer starts now, PIO starts
				 * after irq
				 */
				nemu_start_dma(host);
        } else {
          // start PIO right now
          for (i = 0; i < host->data->blocks; i ++) {
            nemu_data_irq(host, 0);
          }
          nemu_finish_data(host);
        }
      }

			if (!host->use_busy) {
				nemu_finish_command(host);
      }
		}
	} else if (cmd == host->mrq->stop) {
		/* Finished CMD12 */
		nemu_finish_request(host);
	} else
  {
		/* Processed actual command. */
		host->cmd = NULL;
		if (!host->data) {
			nemu_finish_request(host);
    }
		else if (host->data_complete) {
			nemu_transfer_complete(host);
    }
	}
}

static void nemu_check_data_error(struct nemu_host *host, u32 intmask)
{
	if (!host->data)
		return;
	if (intmask & (SDHSTS_CRC16_ERROR | SDHSTS_FIFO_ERROR))
		host->data->error = -EILSEQ;
	if (intmask & SDHSTS_REW_TIME_OUT)
		host->data->error = -ETIMEDOUT;
}

static void nemu_data_irq(struct nemu_host *host, u32 intmask)
{
	/* There are no dedicated data/space available interrupt
	 * status bits, so it is necessary to use the single shared
	 * data/space available FIFO status bits. It is therefore not
	 * an error to get here when there is no data transfer in
	 * progress.
	 */
	if (!host->data)
		return;

	nemu_check_data_error(host, intmask);
	if (host->data->error)
		goto finished;

	if (host->data->flags & MMC_DATA_WRITE) {
		/* Use the block interrupt for writes after the first block */
		host->hcfg &= ~(SDHCFG_DATA_IRPT_EN);
		host->hcfg |= SDHCFG_BLOCK_IRPT_EN;
		writel(host->hcfg, host->ioaddr + SDHCFG);
		nemu_transfer_pio(host);
	} else {
		nemu_transfer_pio(host);
		host->blocks--;
		if ((host->blocks == 0) || host->data->error)
			goto finished;
	}
	return;

finished:
	host->hcfg &= ~(SDHCFG_DATA_IRPT_EN | SDHCFG_BLOCK_IRPT_EN);
	writel(host->hcfg, host->ioaddr + SDHCFG);
}

static void nemu_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct nemu_host *host = mmc_priv(mmc);
	struct device *dev = &host->pdev->dev;
	//u32 edm, fsm;

	/* Reset the error statuses in case this is a retry */
	if (mrq->sbc)
		mrq->sbc->error = 0;
	if (mrq->cmd)
		mrq->cmd->error = 0;
	if (mrq->data)
		mrq->data->error = 0;
	if (mrq->stop)
		mrq->stop->error = 0;

	if (mrq->data && !is_power_of_2(mrq->data->blksz)) {
		dev_err(dev, "unsupported block size (%d bytes)\n",
			mrq->data->blksz);

		if (mrq->cmd)
			mrq->cmd->error = -EINVAL;

		mmc_request_done(mmc, mrq);
		return;
	}

	//if (host->use_dma && mrq->data && (mrq->data->blocks > PIO_THRESHOLD))
	//	nemu_prepare_dma(host, mrq->data);

	mutex_lock(&host->mutex);

	WARN_ON(host->mrq);
	host->mrq = mrq;

#if 0
	edm = readl(host->ioaddr + SDEDM);
	fsm = edm & SDEDM_FSM_MASK;

	if ((fsm != SDEDM_FSM_IDENTMODE) &&
	    (fsm != SDEDM_FSM_DATAMODE)) {
		dev_err(dev, "previous command (%d) not complete (EDM %08x)\n",
			readl(host->ioaddr + SDCMD) & SDCMD_CMD_MASK,
			edm);
		nemu_dumpregs(host);

		if (mrq->cmd)
			mrq->cmd->error = -EILSEQ;

		nemu_finish_request(host);
		mutex_unlock(&host->mutex);
		return;
	}
#endif

	host->use_sbc = !!mrq->sbc && host->mrq->data &&
			(host->mrq->data->flags & MMC_DATA_READ);
	if (host->use_sbc) {
		if (nemu_send_command(host, mrq->sbc)) {
			if (!host->use_busy)
				nemu_finish_command(host);
		}
	} else if (mrq->cmd && nemu_send_command(host, mrq->cmd)) {
		if (host->data) {
      if (host->dma_desc) {
        /* DMA transfer starts now, PIO starts after irq */
        nemu_start_dma(host);
      } else {
        int i;
        // start PIO right now
        for (i = 0; i < host->data->blocks; i ++) {
          nemu_data_irq(host, 0);
        }
        nemu_finish_data(host);
      }
    }

		if (!host->use_busy) {
			nemu_finish_command(host);
    }
	}

	mutex_unlock(&host->mutex);
}

static void nemu_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct nemu_host *host = mmc_priv(mmc);

	mutex_lock(&host->mutex);

	if (!ios->clock || ios->clock != host->clock) {
		//nemu_set_clock(host, ios->clock);
		host->clock = ios->clock;
	}

	/* set bus width */
	host->hcfg &= ~SDHCFG_WIDE_EXT_BUS;
	if (ios->bus_width == MMC_BUS_WIDTH_4)
		host->hcfg |= SDHCFG_WIDE_EXT_BUS;

	host->hcfg |= SDHCFG_WIDE_INT_BUS;

	/* Disable clever clock switching, to cope with fast core clocks */
	host->hcfg |= SDHCFG_SLOW_CARD;

	//writel(host->hcfg, host->ioaddr + SDHCFG);

	mutex_unlock(&host->mutex);
}

static const struct mmc_host_ops nemu_ops = {
	.request = nemu_request,
	.set_ios = nemu_set_ios,
	.hw_reset = nemu_reset,
};

static int nemu_add_host(struct nemu_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct device *dev = &host->pdev->dev;
	char pio_limit_string[20];
	int ret;

	if (!mmc->f_max || mmc->f_max > host->max_clk)
		mmc->f_max = host->max_clk;
	mmc->f_min = host->max_clk / SDCDIV_MAX_CDIV;

	mmc->max_busy_timeout = ~0 / (mmc->f_max / 1000);

	printk("f_max %d, f_min %d, max_busy_timeout %d\n",
		mmc->f_max, mmc->f_min, mmc->max_busy_timeout);
	dev_dbg(dev, "f_max %d, f_min %d, max_busy_timeout %d\n",
		mmc->f_max, mmc->f_min, mmc->max_busy_timeout);

	/* host controller capabilities */
	mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED |
		     MMC_CAP_NEEDS_POLL | MMC_CAP_HW_RESET | MMC_CAP_ERASE |
		     MMC_CAP_CMD23;

	spin_lock_init(&host->lock);
	mutex_init(&host->mutex);

	if (IS_ERR_OR_NULL(host->dma_chan_rxtx)) {
		dev_warn(dev, "unable to initialise DMA channel. Falling back to PIO\n");
		host->use_dma = false;
	} else {
		host->use_dma = true;

		host->dma_cfg_tx.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		host->dma_cfg_tx.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		host->dma_cfg_tx.slave_id = 13;		/* DREQ channel */
		host->dma_cfg_tx.direction = DMA_MEM_TO_DEV;
		host->dma_cfg_tx.src_addr = 0;
		host->dma_cfg_tx.dst_addr = host->phys_addr + SDDATA;

		host->dma_cfg_rx.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		host->dma_cfg_rx.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		host->dma_cfg_rx.slave_id = 13;		/* DREQ channel */
		host->dma_cfg_rx.direction = DMA_DEV_TO_MEM;
		host->dma_cfg_rx.src_addr = host->phys_addr + SDDATA;
		host->dma_cfg_rx.dst_addr = 0;

		if (dmaengine_slave_config(host->dma_chan_rxtx,
					   &host->dma_cfg_tx) != 0 ||
		    dmaengine_slave_config(host->dma_chan_rxtx,
					   &host->dma_cfg_rx) != 0)
			host->use_dma = false;
	}

	mmc->max_segs = 128;
	mmc->max_req_size = 524288;
	mmc->max_seg_size = mmc->max_req_size;
	mmc->max_blk_size = 1024;
	mmc->max_blk_count =  65535;

	/* report supported voltage ranges */
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	//INIT_WORK(&host->dma_work, nemu_dma_complete_work);
	//INIT_DELAYED_WORK(&host->timeout_work, nemu_timeout);

	/* Set interrupt enables */
	host->hcfg = SDHCFG_BUSY_IRPT_EN;

	//nemu_reset_internal(host);

	ret = mmc_add_host(mmc);
	if (ret) {
		free_irq(host->irq, host);
		return ret;
	}

	pio_limit_string[0] = '\0';
	if (host->use_dma && (PIO_THRESHOLD > 0))
		sprintf(pio_limit_string, " (>%d)", PIO_THRESHOLD);
	dev_info(dev, "loaded - DMA %s%s\n",
		 host->use_dma ? "enabled" : "disabled", pio_limit_string);

	return 0;
}

static int nemu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	//struct clk *clk;
	struct resource *iomem;
	struct nemu_host *host;
	struct mmc_host *mmc;
	const __be32 *regaddr_p;
	int ret;

	dev_dbg(dev, "%s\n", __func__);
	mmc = mmc_alloc_host(sizeof(*host), dev);
	if (!mmc)
		return -ENOMEM;

	mmc->ops = &nemu_ops;
	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->pdev = pdev;
	spin_lock_init(&host->lock);

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->ioaddr = devm_ioremap_resource(dev, iomem);
	if (IS_ERR(host->ioaddr)) {
		ret = PTR_ERR(host->ioaddr);
		goto err;
	}

	/* Parse OF address directly to get the physical address for
	 * DMA to our registers.
	 */
	regaddr_p = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!regaddr_p) {
		dev_err(dev, "Can't get phys address\n");
		ret = -EINVAL;
		goto err;
	}

	host->phys_addr = be32_to_cpup(regaddr_p);

	host->dma_chan = NULL;
	host->dma_desc = NULL;

	host->dma_chan_rxtx = dma_request_slave_channel(dev, "rx-tx");

	//clk = devm_clk_get(dev, NULL);
	//if (IS_ERR(clk)) {
	//	ret = PTR_ERR(clk);
	//	if (ret != -EPROBE_DEFER)
	//		dev_err(dev, "could not get clk: %d\n", ret);
	//	goto err;
	//}

	host->max_clk = 1000000; //clk_get_rate(clk);

	//host->irq = platform_get_irq(pdev, 0);
	//if (host->irq <= 0) {
	//	dev_err(dev, "get IRQ failed\n");
	//	ret = -EINVAL;
	//	goto err;
	//}

	ret = mmc_of_parse(mmc);
	if (ret)
		goto err;

	ret = nemu_add_host(host);
	if (ret)
		goto err;

	platform_set_drvdata(pdev, host);

	dev_dbg(dev, "%s -> OK\n", __func__);

	return 0;

err:
	dev_dbg(dev, "%s -> err %d\n", __func__, ret);
	mmc_free_host(mmc);

	return ret;
}

static int nemu_remove(struct platform_device *pdev)
{
	struct nemu_host *host = platform_get_drvdata(pdev);

	mmc_remove_host(host->mmc);

	writel(SDVDD_POWER_OFF, host->ioaddr + SDVDD);

	//free_irq(host->irq, host);

	cancel_work_sync(&host->dma_work);
	cancel_delayed_work_sync(&host->timeout_work);

	mmc_free_host(host->mmc);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id nemu_match[] = {
	{ .compatible = "nemu-sdhost" },
	{ }
};
MODULE_DEVICE_TABLE(of, nemu_match);

static struct platform_driver nemu_driver = {
	.probe      = nemu_probe,
	.remove     = nemu_remove,
	.driver     = {
		.name		= "sdhost-nemu",
		.of_match_table	= nemu_match,
	},
};
module_platform_driver(nemu_driver);

MODULE_ALIAS("platform:sdhost-nemu");
MODULE_DESCRIPTION("NEMU SDHost driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Zihao Yu");
