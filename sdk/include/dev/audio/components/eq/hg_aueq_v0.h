#ifndef _HG_AUEQ_V0_H_
#define _HG_AUEQ_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_aueq_v0_init(void);
int32 hg_aueq_v0_open(struct aueq_device *aueq, struct aueq_cfg *p_cfg);
int32 hg_aueq_v0_close(struct aueq_device *aueq);
int32 hg_aueq_v0_run(struct aueq_device *aueq, void* dat_buf, uint32 bytes);
int32 hg_aueq_v0_ioctl(struct aueq_device *aueq, enum aueq_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif /* _HG_AUEQ_V0_H_ */
