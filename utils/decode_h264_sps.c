/* read 1 bit */
static inline unsigned bits1(unsigned char* &sps, unsigned& off)
{
    unsigned i = off/8;
    off -= 8*i;
    sps += i;
    i = ((*sps)>>(7-off)) & 1;
    ++off;
    return i;
}
/* read 00001xxxx, the number of 'x' equals to the number of '0' */
static inline unsigned golomb(unsigned char* &sps, unsigned &off)
{
    unsigned num=1;
    unsigned i = off/8;
    off -= 8*i;
    sps += i;
    while( !( (*sps) & (1<<(7-off)) ) ){
        if(++off == 8) off=0, ++sps;
        ++num;
    }
    i = 0;
    do {
        i <<=1;
        i |= ( ( (*sps)>>(7-off)) & 1);
        if(++off == 8) off=0, ++sps;
        --num;
    }while(num);
    return --i;
} 
/* @TODO */
static inline void decode_scaling_list(unsigned char* &sps, unsigned &off, int size)
{
    int last=8, next=8;
    if (bits1(sps, off)) {
        assert("not support parse decode_scaling_list"&&0);
    }
}
/* read width, height from sps */
static inline void decode_h264_sps(unsigned char* nalu, int &width, int &height)
{
    /* skip nalu head */
    unsigned char* sps = nalu+1;

    /* profile_idc */
    const unsigned char profile_idc = sps[0];

    /* skip profile_idc + [constraint_set[0-5]_flag+reserved_zero_2bits]+level_idc */
    unsigned sps_bitoff = 8+8+8;

    /* seq_parameter_set_id */
    golomb(sps,sps_bitoff);

    if (profile_idc == 100 ||  // High profile
        profile_idc == 110 ||  // High10 profile
        profile_idc == 122 ||  // High422 profile
        profile_idc == 244 ||  // High444 Predictive profile
        profile_idc ==  44 ||  // Cavlc444 profile
        profile_idc ==  83 ||  // Scalable Constrained High profile (SVC)
        profile_idc ==  86 ||  // Scalable High Intra profile (SVC)
        profile_idc == 118 ||  // Stereo High profile (MVC)
        profile_idc == 128 ||  // Multiview High profile (MVC)
        profile_idc == 138 ||  // Multiview Depth High profile (MVCD)
        profile_idc == 144) {  // old High444 profile

        /* chroma_format_idc */
        unsigned chroma_format_idc = golomb(sps,sps_bitoff);
        if ( chroma_format_idc == 3) {
            /* residual_color_transform_flag */
            ++sps_bitoff;
        }
        /* skip bit_depth_luma_minus8
         *      bit_depth_chroma_minus8
         *      qpprime_y_zero_transform_bypass_flag
         */
        sps_bitoff += 3;
        /* seq_scaling_matrix_present_flag */
        if (bits1(sps, sps_bitoff) ) {
            decode_scaling_list(sps, sps_bitoff, 16); /* Intra, Y */
            decode_scaling_list(sps, sps_bitoff, 16); /* Intra, Cr */
            decode_scaling_list(sps, sps_bitoff, 16); /* Intra, Cb */
            decode_scaling_list(sps, sps_bitoff, 16); /* Inter, Y */
            decode_scaling_list(sps, sps_bitoff, 16); /* Inter, Cr */
            decode_scaling_list(sps, sps_bitoff, 16); /* Inter, Cb */

            decode_scaling_list(sps, sps_bitoff, 64); /* Intra, Y */
            decode_scaling_list(sps, sps_bitoff, 64); /* inter, Y */
            if (chroma_format_idc == 3) {
                decode_scaling_list(sps, sps_bitoff, 64); /* Intra, Cr */
                decode_scaling_list(sps, sps_bitoff, 64); /* Inter, Cr */
                decode_scaling_list(sps, sps_bitoff, 64); /* Intra, Cb */
                decode_scaling_list(sps, sps_bitoff, 64); /* Inter, Cb */
            }
        }
    }
    else  {
        /* Baseline */
    }
    /* log2_max_frame_num_minus4 */
    golomb(sps,sps_bitoff);
    /* pic_order_cnt_type  */
    unsigned pic_order_cnt_type = golomb(sps,sps_bitoff);
    if(pic_order_cnt_type == 0) {
        /* log2_max_pic_order_cnt_lsb_minus4 */
        golomb(sps,sps_bitoff);
    }
    else if (pic_order_cnt_type == 1) {
        /* delta_pic_order_always_zero_flag*/
        ++sps_bitoff;
        /* offset_for_non_ref_pic */
        golomb(sps,sps_bitoff);
        /* offset_for_top_to_bottom_field */
        golomb(sps,sps_bitoff);
        /* poc_cycle_length */
        golomb(sps,sps_bitoff);

    }
    else if (pic_order_cnt_type != 2) {
        assert("illegal POC type"&&0);
    }
    /* max_num_ref_frames*/
    golomb(sps,sps_bitoff);
    /* gaps_in_frame_num_value_allowed_flag*/
    ++sps_bitoff;
    /* pic_width_in_mbs_minus1 */
    unsigned pic_width_in_mbs_minus1  = golomb(sps,sps_bitoff);
    unsigned pic_height_in_map_units_minus1  = golomb(sps,sps_bitoff);

    width  = (pic_width_in_mbs_minus1+1)*16;
    height = (pic_height_in_map_units_minus1+1)*16;
}

