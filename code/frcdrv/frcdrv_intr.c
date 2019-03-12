#include "frc.h"
#include "frc.h"


struct tasklet_struct     frc_tasklet;
uint64_t  frc_tasklet_count = 0;


/* Bottom half processing for DMA Counter Queues. */
void
frcdrv_bh(unsigned long pdev)
{
	octeon_cntq_t    *cntq;
	uint32_t            q_no, ssh_pending=0, ssh_processed=0;
	octeon_device_t  *octeon_dev = (octeon_device_t *)pdev;

	/* Call ordered list processing here to ensure that response for any
           DDOQ related operation is delivered to host. */
	process_ordered_list(octeon_dev);
	frc_tasklet_count++;

	for(q_no = 0; q_no < MAX_OCTEON_DMA_QUEUES; q_no++)  {
		cntq = (octeon_cntq_t *)octeon_dev->cntq[q_no];
		ssh_pending = cavium_atomic_read(&cntq->ssh_pending);

		if(ssh_pending) {
			cavium_spin_lock(&cntq->lock);
			ssh_processed = frc_process_ssh(octeon_dev, cntq);
			cavium_spin_unlock(&cntq->lock);
		}

		if(ssh_processed < ssh_pending)  {
			process_ordered_list(octeon_dev);
			cavium_tasklet_schedule(&frc_tasklet);
		}
	}
}

/* Interrupt Handler for DMA Counter Queue interrupts. 
   Checks the DMA queue packet count registers and atomically
   updates internal packet count. Actual packet handling happens
   in bottom half.
*/
int
frcdrv_intr_handler(void *dev, uint64_t intr)
{
	octeon_cntq_t    *cntq;
	octeon_device_t  *octeon_dev = (octeon_device_t *)dev;
	uint32_t          q_no, pkt_count, schedule=0;
	uint64_t          mask;

	if(octeon_dev->chip_id <= OCTEON_CN58XX) {
		mask = PCI_INT_DMA0_MASK;
	} else {
		if(octeon_dev->chip_id <= OCTEON_CN56XX_PASS2)
			mask = CN56XX_INTR_DMA0_DATA;
		else
			mask = CN63XX_INTR_DMA0_DATA;
	}


	for(q_no = 0; q_no < MAX_OCTEON_DMA_QUEUES; q_no++, mask <<= 1)  {
		if(intr & mask) {
			cntq = (octeon_cntq_t *)octeon_dev->cntq[q_no];
			pkt_count = OCTEON_READ32(cntq->dma_cnt_reg);
			if(pkt_count) {
				cavium_atomic_add(pkt_count, &cntq->ssh_pending);
				OCTEON_WRITE32(cntq->dma_cnt_reg, pkt_count);
				schedule=1;
			} else
				cavium_error("OCTCNTQ: Interrupt 0x%llx with no CNTQ[%d] packets\n", CVM_CAST64(intr), q_no);
		}
	}
	if(schedule)
		cavium_tasklet_schedule(&frc_tasklet);
	return 0;
}


int
frcdrv_intr_init(int octeon_id)
{
	octeon_device_t   *octeon_dev = get_octeon_device_ptr(octeon_id);

	printk("%s.%d: octeon_id=%d.\n", __func__, __LINE__, octeon_id);
	if(octeon_dev == NULL) {
		cavium_error("OCTCNTQ: Unknown Octeon device %d in %s\n",
		              octeon_id, __CVM_FUNCTION__);
		return ENODEV;
	}
	cavium_tasklet_init(&frc_tasklet, frc_bh, (unsigned long)octeon_dev);
    octeon_dev->dam_ops.bh           = frc_bh;
	octeon_dev->dma_ops.intr_handler = frc_intr_handler; 
	octeon_dev->dma_ops.read_stats   = NULL;
	octeon_dev->dma_ops.read_statsb  = NULL;

	return 0;
}


/* End of file */
