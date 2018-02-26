#include "mem_codec_api.h"
#include "mem_codec_common_prv.h"
#include "z_bitstream_process.h"
#include <assert.h>
static int FindStartCode3 (unsigned char *Buf){
   if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //0x000001?
   else return 1;
}

/**
 * 查找startcode
 */
static int FindStartCode4 (unsigned char *Buf){
   if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//0x00000001?
   else return 1;
}

int NaluStartCodeLen(unsigned char* pucBuf){
    int startCode = 3;
    if(pucBuf[0] == 0 && pucBuf[1] == 0 &&pucBuf[2] == 0 && pucBuf[3] == 1){
        startCode = 4;
    }
    else if(pucBuf[0] == 0 && pucBuf[1] == 0 &&pucBuf[2] == 1 ){
        startCode = 3;
    }
    else{
        startCode = 0;
    }
    return startCode;
}

/*
 * 判断是不是nalu
 */
int NaluIsSPS(unsigned char* pucBuf,int startCodeLen){
    if(pucBuf == NULL || startCodeLen == 0){
        return 0;
    }
    int type = pucBuf[startCodeLen] & 0x1f;
    if(type == 7){

        return 1;
    }
    return 0;
}

int GetProfile(unsigned char* pucBuf,int startCodeLen){
    if(pucBuf == NULL || startCodeLen == 0){
        return 0;
    }
    int type = pucBuf[startCodeLen] & 0x1f;
    if(type != 0x07){
        return 0;
    }

    int cur_profile = pucBuf[startCodeLen + 1];
    return cur_profile;
}

int z_h264_nal2rbsp(z_h264_nalu_t* nalu,unsigned char* rbsp){
    int zeroCount = 0;
    unsigned char* buf = nalu->buf;
    unsigned int size = nalu->nal_len;

    int j = 0;
    for (int i = 0; i < size; i++) {
        if (zeroCount == 2 && buf[i] == 0x03) {
            i++;
            zeroCount = 0;
        }
        if (buf[i] == 0)
            zeroCount++;
        else
            zeroCount = 0;
        rbsp[j++] = buf[i];
    }

    return j;
}

static int nalu2rbsp(z_h264_nalu_t* nalu){
    int zeroCount = 0;
    unsigned char* buf = nalu->buf;
    unsigned int size = nalu->nal_len;

    unsigned char* rbsp = nalu->buf;
    int j = 0;
    for (int i = 0; i < size; i++) {
        if (zeroCount == 2 && buf[i] == 0x03) {
            i++;
            zeroCount = 0;
        }
        if (buf[i] == 0)
            zeroCount++;
        else
            zeroCount = 0;
        rbsp[j++] = buf[i];
    }

    nalu->nal_len = j;

    return j;
}

int z_h264_get_nal(const char* buf,const unsigned int size,z_h264_nalu_t* nalu){
    int i = 0;
    while(i<size)
    {
        int startCodeLen = 0;
        if((i + 2 < size) && FindStartCode3(&buf[i]) ){
            startCodeLen = 3;
        }
        else if((i + 3 < size) && FindStartCode4(&buf[i])){
            startCodeLen = 4;
        }
        else{
            ++i;
            continue;
        }

        nalu->start_code_len = startCodeLen;
        i += startCodeLen;
        int begin = i;
        int end = i;
        int nextStartCodeLen = 0;
        while (end<size)
        {
            nextStartCodeLen = 0;
            if((end + 2 < size) && FindStartCode3(&buf[end]) ){
                nextStartCodeLen = 3;
                break;
            }
            else if((end + 3 < size) && FindStartCode4(&buf[end])){
                nextStartCodeLen = 4;
                break;
            }
            else{
                ++end;
                continue;
            }
        }

        /*int dataLen = end- begin;
        nalu->nal_unit_type = buf[begin]&0x1f;
        nalu->nal_ref_idc = buf[begin] & 0x60;
        nalu->start_pos = &buf[begin-startCodeLen];
        nalu->end_pos = nalu->start_pos + dataLen + startCodeLen;
        nalu->rbsp = nalu->start_pos + startCodeLen;*/
        int dataLen = end- begin;
        nalu->nal_unit_type = buf[begin]&0x1f;
        nalu->nal_ref_idc = buf[begin] & 0x60;
        nalu->nal_header = buf[begin];
        nalu->nal_len = dataLen;
        memcpy(nalu->buf,&buf[begin+1],nalu->nal_len);
        nalu2rbsp(nalu);
        return 1;
    }
    return 0;
}

//==============================
z_h264_stream_t *z_h264_stream_new()
{
    z_h264_stream_t *s = malloc(sizeof(z_h264_stream_t));
    s->bit_pos = 7;
    s->byte_pos = 0;
    return s;
}

void z_h264_stream_init(z_h264_stream_t *s,uint8_t *data, int size){
    s->data = data;
    s->size = size;
}

z_h264_stream_t *z_h264_stream_from_file(char *path)
{
    FILE *fp;
    long file_size;
    uint8_t *buffer;
    size_t result;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp , 0 , SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    buffer = (uint8_t *)malloc(file_size);
    assert(buffer);

    result = fread(buffer, 1, file_size, fp);
    assert(result == file_size);

    fclose (fp);

    z_h264_stream_t * s = z_h264_stream_new();
    assert(s != NULL);
    z_h264_stream_init(s,buffer,file_size);
    return s;
}

void z_h264_stream_free(z_h264_stream_t *s)
{
    s->size = 0;
    s->bit_pos = 7;
    s->byte_pos = 0;
    free(s);
}

uint32_t z_h264_stream_read_bits(z_h264_stream_t *s, uint32_t n)
{
    uint32_t ret = 0;

    if (n == 0) {
        return 0;
    }

    int i;
    for (i = 0; i < n; ++i) {
        if (z_h264_stream_bits_remaining(s) == 0) {
            ret <<= n - i - 1;
        }
        uint8_t b = s->data[s->byte_pos];
        if (n - i <= 32) {
            ret = ret << 1 | BITAT(b, s->bit_pos);
        }
        if (s->bit_pos == 0) {
            s->bit_pos = 7;
            s->byte_pos++;
        } else {
            s->bit_pos--;
        }
    }
    return ret;
}

uint32_t z_h264_stream_peek_bits(z_h264_stream_t *s, uint32_t n)
{
    int prev_bit_pos = s->bit_pos;
    int prev_byte_pos = s->byte_pos;
    uint32_t ret = z_h264_stream_read_bits(s, n);
    s->bit_pos = prev_bit_pos;
    s->byte_pos = prev_byte_pos;
    return ret;
}

uint32_t z_h264_stream_read_bytes(z_h264_stream_t *s, uint32_t n)
{
    uint32_t ret = 0;

    if (n == 0) {
        return 0;
    }

    int i;
    for (i = 0; i < n; ++i) {
        if (z_h264_stream_bytes_remaining(s) == 0) {
            ret <<= (n - i - 1) * 8;
            break;
        }
        if (n - i <= 4) {
            ret = ret << 8 | s->data[s->byte_pos];
        }
        s->byte_pos++;
    }

    return ret;
}

uint32_t z_h264_stream_peek_bytes(z_h264_stream_t *s, uint32_t n)
{
    int prev_byte_pos = s->byte_pos;
    uint32_t ret = z_h264_stream_read_bytes(s, n);
    s->byte_pos = prev_byte_pos;
    return ret;
}

int z_h264_stream_bits_remaining(z_h264_stream_t *s)
{
    return (s->size - s->byte_pos) * 8 + s->bit_pos;
}

int z_h264_stream_bytes_remaining(z_h264_stream_t *s)
{
    return s->size - s->byte_pos;
}


//=================================

int z_h264_more_data_in_byte_stream(z_h264_stream_t *s)
{
    return z_h264_stream_bytes_remaining(s) > 0;
}

uint32_t z_h264_next_bits(z_h264_stream_t *s, int n)
{
    if (n % 8 == 0) {
        return z_h264_stream_peek_bytes(s, n / 8);
    }
    return z_h264_stream_peek_bits(s, n);
}

uint32_t z_h264_u(z_h264_stream_t *s, uint32_t n)
{
    //if (n % 8 == 0) {
    //    return z_h264_stream_read_bytes(s, n / 8);
    //}
    return z_h264_stream_read_bits(s, n);
}

static uint8_t h264_exp_golomb_bits[256] = {
8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0,
};

uint32_t z_h264_ue(z_h264_stream_t *s) {
    uint32_t bits, read;
    uint8_t coded;
    bits = 0;
    while (1) {
        if (z_h264_stream_bytes_remaining(s) < 1) {
            read = z_h264_stream_peek_bits(s, s->bit_pos) << (8 - s->bit_pos);
            break;
        } else {
            read = z_h264_stream_peek_bits(s, 8);
            if (bits > 16) {
                break;
            }
            if (read == 0) {
                z_h264_stream_read_bits(s, 8);
                bits += 8;
            } else {
                break;
            }
        }
    }
    coded = h264_exp_golomb_bits[read];
    z_h264_stream_read_bits(s, coded);
    bits += coded;
    return z_h264_stream_read_bits(s, bits + 1) - 1;
}

void z_h264_f(z_h264_stream_t *s, uint32_t n, uint32_t pattern) {
    uint32_t val = z_h264_u(s, n);
    if (val != pattern) {
        fprintf(stderr, "z_h264_f Error: fixed-pattern doesn't match. \nexpected: %x \nactual: %x \n", pattern, (unsigned int)val);
        exit(0);
    }
}

int32_t h264_se(z_h264_stream_t *s)
{
    uint32_t ret;
    ret = z_h264_ue(s);
    if (!(ret & 0x1)) {
        ret >>= 1;
        return (int32_t)(- ret);
    }
    return (ret + 1) >> 1;
}

void z_h264_rbsp_trailing_bits(z_h264_stream_t *s) {
    z_h264_f(s, 1, 1);
    z_h264_f(s, s->bit_pos, 0);
}

//=================================
int z_h264_get_sps(z_h264_stream_t *s,
                                                           z_h264_sps_t *sps)
{
    int i;
    sps->profile_idc = z_h264_u(s, 8);
    sps->constraint_set0_flag = z_h264_u(s, 1);
    sps->constraint_set1_flag = z_h264_u(s, 1);
    sps->constraint_set2_flag = z_h264_u(s, 1);
    z_h264_u(s, 5); // reserved_zero_5bits
    sps->level_idc = z_h264_u(s, 8);
    sps->seq_parameter_set_id = z_h264_ue(s);
    sps->log2_max_frame_num_minus4 = z_h264_ue(s);
    sps->pic_order_cnt_type = z_h264_ue(s);
    if (sps->pic_order_cnt_type == 0) {
        sps->log2_max_pic_order_cnt_lsb_minus4 = z_h264_ue(s);
    } else if (sps->pic_order_cnt_type == 1) {
        sps->delta_pic_order_always_zero_flag = h264_se(s);
        sps->offset_for_non_ref_pic = h264_se(s);
        sps->offset_for_top_to_bottom_field = h264_se(s);
        sps->num_ref_frames_in_pic_order_cnt_cycle = h264_se(s);
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            sps->offset_for_ref_frame[i] = h264_se(s);
        }
    }
    sps->num_ref_frames = z_h264_ue(s);
    sps->gaps_in_frame_num_value_allowed_flag = z_h264_u(s, 1);
    sps->pic_width_in_mbs_minus1 = z_h264_ue(s);
    sps->pic_height_in_map_units_minus1 = z_h264_ue(s);
    sps->frame_mbs_only_flag = z_h264_u(s, 1);
    if (!sps->frame_mbs_only_flag) {
        sps->mb_adaptive_frame_field_flag = z_h264_u(s, 1);
    }
    sps->direct_8x8_inference_flag = z_h264_u(s, 1);
    sps->frame_cropping_flag = z_h264_u(s, 1);
    if (sps->frame_cropping_flag) {
        sps->frame_crop_left_offset = z_h264_ue(s);
        sps->frame_crop_right_offset = z_h264_ue(s);
        sps->frame_crop_top_offset = z_h264_ue(s);
        sps->frame_crop_bottom_offset = z_h264_ue(s);
    }
    sps->vui_parameters_present_flag = z_h264_u(s, 1);
    if (sps->vui_parameters_present_flag) {
         z_h264_get_vui(s,&sps->vui_parameters);
    }
    //z_h264_rbsp_trailing_bits(s);
    return 0;
}

//===========================
int z_h264_get_pps(z_h264_stream_t *s,
                                                 z_h264_pps_t *pps)
{
    pps->pic_parameter_set_id = z_h264_ue(s);
    pps->seq_parameter_set_id = z_h264_ue(s);
    pps->entropy_coding_mode_flag = z_h264_u(s, 1);
    pps->pic_order_present_flag = z_h264_u(s, 1);
    pps->num_slice_groups_minus1 = z_h264_ue(s);
    if (pps->num_slice_groups_minus1 > 0) {
        pps->slice_group_map_type = z_h264_ue(s);
        if (pps->slice_group_map_type == 0) {
            pps->run_length_minus1 = z_h264_ue(s); // FIXME: Not Correct. Really an array.
        } else if (pps->slice_group_map_type == 2) {
            pps->top_left = z_h264_ue(s); // FIXME: Not Correct. Really an array.
            pps->bottom_right = z_h264_ue(s); // FIXME: Not Correct. Really an array.
        } else if (pps->slice_group_map_type == 3 ||
            pps->slice_group_map_type == 4 ||
            pps->slice_group_map_type == 5) {
            pps->slice_group_change_direction_flag = z_h264_u(s, 1);
            pps->slice_group_change_rate_minus1 = z_h264_ue(s);
        } else if (pps->slice_group_map_type == 6) {
            pps->pic_size_in_map_units_minus1 = z_h264_ue(s);
            pps->slice_group_id = z_h264_ue(s); // FIXME: Not Correct. Really an array.
        }
    }
    pps->num_ref_idx_l0_active_minus1 = z_h264_ue(s);
    pps->num_ref_idx_l1_active_minus1 = z_h264_ue(s);
    pps->weighted_pred_flag = z_h264_u(s, 1);
    pps->weighted_bipred_idc = z_h264_u(s, 2);
    pps->pic_init_qp_minus26 = h264_se(s);
    pps->pic_init_qs_minus26 = h264_se(s);
    pps->chroma_qp_index_offset = h264_se(s);
    pps->deblocking_filter_control_present_flag = z_h264_u(s, 1);
    pps->constrained_intra_pred_flag = z_h264_u(s, 1);
    pps->redundant_pic_cnt_present_flag = z_h264_u(s, 1);
    //z_h264_rbsp_trailing_bits(s);
    return 0;
}

//=================================

int z_h264_get_vui(z_h264_stream_t *s,z_h264_vui_t *vp)
{
    vp->aspect_ratio_info_present_flag = z_h264_u(s, 1);
    if (vp->aspect_ratio_info_present_flag) {
        vp->aspect_ratio_idc = z_h264_u(s, 8);
        if (vp->aspect_ratio_idc == 255) { // Extended_SAR
            vp->sar_width = z_h264_u(s, 8);
            vp->sar_height = z_h264_u(s, 8);
        }
    }
    vp->overscan_info_present_flag = z_h264_u(s, 1);
    if (vp->overscan_info_present_flag) {
        vp->overscan_appropriate_flag = z_h264_u(s, 1);
    }
    vp->video_signal_type_present_flag = z_h264_u(s, 1);
    if (vp->video_signal_type_present_flag) {
        vp->video_format = z_h264_u(s, 3);
        vp->video_full_range_flag = z_h264_u(s, 1);
        vp->colour_description_present_flag = z_h264_u(s, 1);
        if (vp->colour_description_present_flag) {
            vp->colour_primaries = z_h264_u(s, 8);
            vp->transfer_characteristics = z_h264_u(s, 8);
            vp->matrix_coefficients = z_h264_u(s, 8);
        }
    }
    vp->chroma_loc_info_present_flag = z_h264_u(s, 1);
    if (vp->chroma_loc_info_present_flag) {
        vp->chroma_sample_loc_type_top_field = z_h264_ue(s);
        vp->chroma_sample_loc_type_bottom_field = z_h264_ue(s);
    }
    vp->nal_hrd_parameters_present_flag = z_h264_u(s, 1);
    //if (vp->nal_hrd_parameters_present_flag) {
    //    vp->hrd_parameters = h264_read_hrd_parameters(s);
    //}
    vp->vcl_hrd_parameters_present_flag  = z_h264_u(s, 1);
    //if (vp->nal_hrd_parameters_present_flag) {
    //    vp->hrd_parameters = h264_read_hrd_parameters(s);
    //}
    if (vp->nal_hrd_parameters_present_flag || vp->vcl_hrd_parameters_present_flag) {
        vp->low_delay_hrd_flag = z_h264_u(s, 1);
    }
    vp->pic_struct_present_flag = z_h264_u(s, 1);
    vp->bitstream_restriction_flag = z_h264_u(s, 1);
    if (vp->bitstream_restriction_flag) {
        vp->bitstream_restriction_flag = z_h264_u(s, 1);
        vp->motion_vectors_over_pic_boundaries_flag = z_h264_u(s, 1);
        vp->max_bytes_per_pic_denom = z_h264_ue(s);
        vp->max_bits_per_mb_denom = z_h264_ue(s);
        vp->log2_max_mv_length_horizontal = z_h264_ue(s);
        vp->log2_max_mv_length_vertical = z_h264_ue(s);
        vp->num_reorder_frames = z_h264_ue(s);
        vp->max_dec_frame_buffering = z_h264_ue(s);
    }
    return 0;
}
