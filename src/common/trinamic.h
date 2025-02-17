// trinamic.h
#pragma once
#include <stdint.h>

#include <inttypes.h>

typedef void(tmc_sg_sample_cb_t)(uint8_t axis, uint16_t sg);

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef struct {
    const char *cmd_name;
    uint8_t reg_adr;
    bool write;
    bool read;
} tmc_reg_t;

extern tmc_reg_t tmc_reg_map[]; //< Null terminated array of known registers

extern void init_tmc_bare_minimum(void);
extern void init_tmc(void);
extern void tmc_get_sgt();
extern void tmc_get_TPWMTHRS();
extern void tmc_get_tstep();
extern uint8_t tmc_sample();
extern uint16_t tmc_get_last_sg_sample(uint8_t axis);

extern uint8_t tmc_get_sg_mask();
extern uint8_t tmc_get_sg_axis();
extern tmc_sg_sample_cb_t *tmc_get_sg_sample_cb();

extern void tmc_set_sg_mask(uint8_t mask);
extern void tmc_set_sg_axis(uint8_t axis);
extern void tmc_set_sg_sample_cb(tmc_sg_sample_cb_t *cb);

extern void tmc_enable_wavetable(bool enabled, bool X, bool Y, bool Z);

#ifdef __cplusplus
}

#endif //__cplusplus
