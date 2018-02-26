/*
 * 作者: 莫斌生 <mobinsheng@corp.netease.com>
 * 说明:比特流解析
 */
#ifndef BITSTREAM_PROCESS_H
#define BITSTREAM_PROCESS_H

#include <stdint.h>

typedef struct z_h264_stream_t {
    uint8_t *data;
    uint32_t size;
    int bit_pos;
    int byte_pos;
} z_h264_stream_t;

/*typedef struct z_h264_nalu_t {
    uint8_t nal_ref_idc;
    uint8_t nal_unit_type;
    void *rbsp;
} z_h264_nalu_t;*/

typedef enum {
    Z_NALU_TYPE_SLICE    = 1,
    Z_NALU_TYPE_DPA      = 2,
    Z_NALU_TYPE_DPB      = 3,
    Z_NALU_TYPE_DPC      = 4,
    Z_NALU_TYPE_IDR      = 5,
    Z_NALU_TYPE_SEI      = 6,
    Z_NALU_TYPE_SPS      = 7,
    Z_NALU_TYPE_PPS      = 8,
    Z_NALU_TYPE_AUD      = 9,
    Z_NALU_TYPE_EOSEQ    = 10,
    Z_NALU_TYPE_EOSTREAM = 11,
    Z_NALU_TYPE_FILL     = 12,
} EnNaluType;

typedef enum {
    Z_NALU_PRIORITY_DISPOSABLE = 0,
    Z_NALU_PRIRITY_LOW         = 1,
    Z_NALU_PRIORITY_HIGH       = 2,
    Z_NALU_PRIORITY_HIGHEST    = 3
} EnNaluPriority;

typedef struct z_h264_nalu_t {
    int start_code_len; ///< startcode length
    int nal_len; ///< without startcode
    unsigned char nal_header;
    EnNaluPriority nal_ref_idc; ///< 优先级
    EnNaluType nal_unit_type;///< 类型
    char* buf; ///< widthout startcode and nal>rbsp
} z_h264_nalu_t;

typedef struct z_h264_vui_t {
    int aspect_ratio_info_present_flag;
    int aspect_ratio_idc;
    int sar_width;
    int sar_height;
    int overscan_info_present_flag;
    int overscan_appropriate_flag;
    int video_signal_type_present_flag;
    int video_format;
    int video_full_range_flag;
    int colour_description_present_flag;
    int colour_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
    int chroma_loc_info_present_flag;
    int chroma_sample_loc_type_top_field;
    int chroma_sample_loc_type_bottom_field;
    int timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    int fixed_frame_rate_flag;
    int nal_hrd_parameters_present_flag;
    // hrd_parameters_t hrd_parameters;
    int vcl_hrd_parameters_present_flag;
    int low_delay_hrd_flag;
    int pic_struct_present_flag;
    int bitstream_restriction_flag;
    int motion_vectors_over_pic_boundaries_flag;
    int max_bytes_per_pic_denom;
    int max_bits_per_mb_denom;
    int log2_max_mv_length_horizontal;
    int log2_max_mv_length_vertical;
    int num_reorder_frames;
    int max_dec_frame_buffering;
} z_h264_vui_t;

typedef struct z_h264_sps_t {
    int profile_idc;
    int constraint_set0_flag;
    int constraint_set1_flag;
    int constraint_set2_flag;
    int level_idc;
    int seq_parameter_set_id;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;
    int offset_for_ref_frame[255];
    int num_ref_frames;
    int gaps_in_frame_num_value_allowed_flag;
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int mb_adaptive_frame_field_flag;
    int direct_8x8_inference_flag;
    int frame_cropping_flag;
    int frame_crop_left_offset;
    int frame_crop_right_offset;
    int frame_crop_top_offset;
    int frame_crop_bottom_offset;
    int vui_parameters_present_flag;
    z_h264_vui_t vui_parameters;
} z_h264_sps_t;

typedef struct z_h264_pps_t {
    int pic_parameter_set_id;
    int seq_parameter_set_id;
    int entropy_coding_mode_flag;
    int pic_order_present_flag;
    int num_slice_groups_minus1;
    int slice_group_map_type;
    int run_length_minus1;
    int top_left;
    int bottom_right;
    int slice_group_change_direction_flag;
    int slice_group_change_rate_minus1;
    int pic_size_in_map_units_minus1;
    int slice_group_id;
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    int deblocking_filter_control_present_flag;
    int constrained_intra_pred_flag;
    int redundant_pic_cnt_present_flag;
} z_h264_pps_t;

#define BITAT(x, n) ((x & (1 << n)) == (1 << n))


z_h264_stream_t *z_h264_stream_new();
void z_h264_stream_init(z_h264_stream_t *s,uint8_t *data, int size);
z_h264_stream_t *z_h264_stream_from_file(char *path);
void z_h264_stream_free(z_h264_stream_t *s);
uint32_t z_h264_stream_read_bits(z_h264_stream_t *s, uint32_t n);
uint32_t z_h264_stream_peek_bits(z_h264_stream_t *s, uint32_t n);
uint32_t z_h264_stream_read_bytes(z_h264_stream_t *s, uint32_t n);
uint32_t z_h264_stream_peek_bytes(z_h264_stream_t *s, uint32_t n);
int z_h264_stream_bits_remaining(z_h264_stream_t *s);
int z_h264_stream_bytes_remaining(z_h264_stream_t *s);

int z_h264_more_data_in_byte_stream(z_h264_stream_t *s);
uint32_t z_h264_next_bits(z_h264_stream_t *s, int n);
uint32_t z_h264_u(z_h264_stream_t *s, uint32_t n);
uint32_t z_h264_ue(z_h264_stream_t *s);
void z_h264_f(z_h264_stream_t *s, uint32_t n, uint32_t pattern);
int32_t z_h264_se(z_h264_stream_t *s);
void z_h264_rbsp_trailing_bits(z_h264_stream_t *s);

int z_h264_get_sps(z_h264_stream_t *s,z_h264_sps_t *sps);

int z_h264_get_pps(z_h264_stream_t *s,z_h264_pps_t *pps);

int z_h264_get_vui(z_h264_stream_t *s,z_h264_vui_t *vp);

/*
 * 1 success
 * 0 failed
 */
int z_h264_get_nal(const char* buf,const unsigned int size,z_h264_nalu_t* nalu);

int z_h264_nal2rbsp(z_h264_nalu_t* nalu,unsigned char* rbsp);

int z_h264_nal_unit(z_h264_stream_t *s,z_h264_nalu_t* nu);
int NaluStartCodeLen(unsigned char* pucBuf);
int NaluIsSPS(unsigned char* pucBuf,int startCodeLen);
int GetProfile(unsigned char* pucBuf,int startCodeLen);

#endif // BITSTREAM_PROCESS_H
