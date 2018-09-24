#ifndef SRC_BLOCKS_BLOCKS_H_
#define SRC_BLOCKS_BLOCKS_H_

#ifdef __cplusplus
extern "C" {
#endif

const unsigned char *get_blocks_dat_start(int testnet, int stagenet);
size_t get_blocks_dat_size(int testnet, int stagenet);

#ifdef __cplusplus
}
#endif


#endif /* SRC_BLOCKS_BLOCKS_H_ */
