#ifndef H264_WRAP_HH
#define H264_WRAP_HH

#include <string>
#include "log.h"

typedef enum
{
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,
} NaluType;

typedef enum
{
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} NaluPriority;

typedef struct
{
    int startCode; // 00 00 01 or 00 00 00 01
    int type;
    int size;
    unsigned char *data;
} NaluUnit;

typedef struct
{
    unsigned char forbiddent_zero_bit : 1; //must be zero
    unsigned char nal_ref_idc : 2;
    unsigned char type : 5;
} NaluHeader;

typedef struct
{
    unsigned char aspect_ratio_info_present_flag;
    unsigned char aspect_ratio_idc; // if aspect_ratio_info_present_flag == 1
    unsigned short sar_width;       //if aspect_radio_idc == Extendted_SAR
    unsigned short sar_height;
    unsigned char overscan_info_present_flag;
    unsigned char overscan_appropriate_flag;
    unsigned char video_signal_type_present_flag;
    unsigned char video_format;
    unsigned char video_full_range_flag;
    unsigned char colour_description_present_flag;
    unsigned char colour_primaries;
    unsigned char transfer_characteristics;
    unsigned char matrix_coefficients;
    unsigned char chroma_loc_info_present_flag;
    unsigned char chroma_sample_loc_type_top_field;
    unsigned char chroma_sample_loc_type_bottom_field;
    unsigned char timing_info_present_flag;
    unsigned int num_units_in_tick;
    unsigned int time_scale;
} NaluRbspVui;

typedef struct
{
    unsigned char profile_idc;
    unsigned char constraint_set0_flag : 1;
    unsigned char constraint_set1_flag : 1;
    unsigned char constraint_set2_flag : 1;
    unsigned char constraint_set3_flag : 1;
    unsigned char reserved_zero_4bits : 4;
    unsigned char level_idc;
    unsigned int seq_parameter_set_id;
    //if idc = 100 || 110 || 122 | 144  start
    unsigned int chroma_format_idc;
    unsigned char residual_colour_transform_flag; // valid if idc == 3
    unsigned int bit_depth_luma_minus8;
    unsigned int bit_depth_chroma_minus8;
    unsigned char qpprime_y_zero_transform_bypass_flag;
    unsigned char seq_scaling_matrix_present_flag;
    unsigned char seq_scaling_list_present_flag[8];
    //end

    unsigned int log2_max_frame_num_minus4;
    unsigned int pic_order_cnt_type;
    //if pic_order_cnt_type == 0
    unsigned int log2_max_pic_order_cnt_lsb_minus4;
    //else if pic_order_cnt_type == 1
    unsigned char delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    int *offset_for_ref_frame;
    //end

    unsigned int num_ref_frames;
    unsigned char gaps_in_frame_num_value_allowed_flag;
    unsigned int pic_width_in_mbs_minus1;
    unsigned int pic_height_in_map_units_minus1;
    unsigned char frame_mbs_only_flag;
    unsigned char mb_adaptive_frame_field_flag; //valid if pic_height_in_map_units_minus1
    unsigned char direct_8x8_inference_flag;
    unsigned char frame_cropping_flag;

    //if frame_cropping_flag
    unsigned int frame_crop_left_offset;
    unsigned int frame_crop_right_offset;
    unsigned int frame_crop_top_offset;
    unsigned int frame_crop_bottom_offset;
    //end

    unsigned char vui_parameters_present_flag;
    NaluRbspVui uvi_parameters;
} NaluRbsp;

typedef struct
{
    NaluHeader header;
    NaluRbsp rbsp;
} NaluParam;
/***
 *  NALU = Header + RBSP
 *  NALU Header = forbidden_zero_bit(1bit)+nal_ref_idc(2bits) +nalu__unit_type(5bit)
 *  RBSP = SODB(原始编码数据) + rbsp_stop_one_bit+rbsp_align_zero_bit
 */

class H264Wrap
{
  public:
    /**
    * @brief 从buffer中获取nalu
    *
    * @param [out] nalu: @n nalu 信息
    * @param [in] buf: @n buffer起始地址
    * @param [in] buf_size: @n buffer长度
    * @return 返回下一个nalu位置
    * @see None.
    * @note None.
    */
    int GetNaluFromBuffer(NaluUnit &nalu, unsigned char *buf, int bufSize);

    /**
    * @brief 解析sps
    *
    * @param [in] data: @n sps data
    * @param [in] dataLen: @n sps data len
    * @param [out] width: @n 图像宽度
    * @param [out] height: @n 图像高度
    * @param [out] fps: @n 图像帧率
    * @return true or false
    * @see None.
    * @note None.
    */
    bool DecodeSPS(unsigned char *data, int dataLen, int &width, int &height, int &fps);
    void KickOutNaluRaceCondition(unsigned char *data, int &dataLen);
    void PrintH264Info(std::string filename);
    void PrintNalu(const NaluUnit &nalu);
    bool DecodeNaluParams(NaluParam &naluParam, unsigned char *data, int &dataLen);
    bool FreeNaluParams(NaluParam &naluParam);
};

#endif