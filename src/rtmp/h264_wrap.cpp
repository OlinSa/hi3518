#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "h264_wrap.h"

using namespace std;

unsigned int Ue(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
	//计算0bit的个数
	unsigned int nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;

	//计算结果
	unsigned long dwRet = 0;
	for (unsigned int i = 0; i < nZeroNum; i++)
	{

		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

int Se(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2); //ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

unsigned long u(unsigned int BitCount, unsigned char *buf, unsigned int &nStartBit)
{
	unsigned long dwRet = 0;
	for (unsigned int i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

int H264Wrap::GetNaluFromBuffer(NaluUnit &nalu, unsigned char *buf, int bufSize)
{
	int i = 0;
	int startCodeCnt = 0;
	unsigned char *pbuf = buf;
	while (i < bufSize)
	{
		if (pbuf[i++] == 0x00 && pbuf[i++] == 0x00)
		{
			if (pbuf[i] == 0x01)
			{
				i += 1;
				startCodeCnt = 3;
			}
			else if (pbuf[i] == 0x00 && pbuf[i + 1] == 0x01)
			{
				i += 2;
				startCodeCnt = 4;
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		//find next frame
		int pos = i;
		int startCodeCnt2 = 0;

		while (i < bufSize)
		{
			if (pbuf[i++] == 0x00 && pbuf[i++] == 0x00)
			{
				if (pbuf[i] == 0x01)
				{
					i += 1;
					startCodeCnt2 = 3;
					break;
				}
				else if (pbuf[i] == 0x00 && pbuf[i + 1] == 0x01)
				{
					i += 2;
					startCodeCnt2 = 4;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		nalu.startCode = startCodeCnt;
		if (i > bufSize)
		{
			return -1;
		}
		else if (i == bufSize)
		{
			nalu.size = i - pos;
		}
		else
		{
			nalu.size = (i - startCodeCnt2) - pos;
		}
		nalu.type = pbuf[pos] & 0x1f;
		nalu.data = &pbuf[pos];
		return i - startCodeCnt2;
	}
	return -1;
}

void H264Wrap::PrintH264Info(std::string filename)
{
	unsigned char buf[10240];
	int pos = 0;
	int alreadSeen = 0;
	int bufSize = sizeof(buf);

	int nextFramePos;
	std::streamsize readCount;
	int naluCount = 0;

	NaluUnit nalu;

	ifstream inFile(filename, ios::in | ios::binary);
	if (!inFile)
	{
		LOG_DEBUG("open %s failed", filename);
		return;
	}
	while (inFile.read(reinterpret_cast<char *>(&buf[0]) + alreadSeen, bufSize - alreadSeen))
	{
		readCount = inFile.gcount();
		alreadSeen += readCount;

		while ((nextFramePos = GetNaluFromBuffer(nalu, buf + pos, alreadSeen - pos)) > 0)
		{
			naluCount++;
			PrintNalu(nalu);
			// LOG_DEBUG("%d nalu: startcode:%d,type:%d, data:%p, dataSize:%d,nextFramePos:%d",
			//   naluCount, nalu.startCode, nalu.type, nalu.data, nalu.size, nextFramePos);
			pos += nextFramePos;

			if (nalu.type == NALU_TYPE_SPS)
			{
				NaluParam naluParam;
				if (!DecodeNaluParams(naluParam, nalu.data, nalu.size))
				{
					LOG_ERR("decode NaluParams failed");
				}
				else
				{
					LOG_INFO("width:%u, height:%u, fps:%u",
							 (naluParam.rbsp.pic_width_in_mbs_minus1 + 1) * 16, (naluParam.rbsp.pic_height_in_map_units_minus1 + 1) * 16, naluParam.rbsp.uvi_parameters.time_scale / (2 * naluParam.rbsp.uvi_parameters.num_units_in_tick));
					FreeNaluParams(naluParam);
				}
			}
			//                pos          alreadSeen      bufSize
			// |  #############| ############| #############|

			// for (int i = 0; i < 38; i++)
			// {
			//     if (i % 8 == 0)
			//     {
			//         cout << endl;
			//     }
			//     cout << "0x" << setfill('0') << setw(2) << hex << (buf[i] & 0xff) << " ";
			// }
			// cout << endl;
			// sleep(1);
		}
		if (pos > 0)
		{
			memmove(buf, buf + pos, alreadSeen - pos); //memcpy会有内存重叠问题
			alreadSeen -= pos;
			pos = 0;
		}
		else
		{
			pos = 0;
			alreadSeen = 0;
		}
	}
	inFile.close();
	return;
}

void H264Wrap::KickOutNaluRaceCondition(unsigned char *data, int &dataLen)
{
	int i = 0, j = 0;
	unsigned char *tmpPtr = data;
	int tmpBufSize = dataLen;
	int val = 0;

	for (i = 0; i < (tmpBufSize - 2); i++)
	{
		//check for 0x000003
		val = (tmpPtr[i] ^ 0x00) + (tmpPtr[i + 1] ^ 0x00) + (tmpPtr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j < tmpBufSize - 1; j++)
				tmpPtr[j] = tmpPtr[j + 1];

			//and so we should devrease bufsize
			dataLen--;
		}
	}

	return;
}

void H264Wrap::PrintNalu(const NaluUnit &nalu)
{
	std::string outputString;
	NaluHeader naluheader;
	naluheader.forbiddent_zero_bit = nalu.data[0] >> 7 & 0x01;
	naluheader.nal_ref_idc = nalu.data[0] >> 5 & 0x03;
	naluheader.type = nalu.data[0] & 0x1f;

	switch (naluheader.nal_ref_idc)
	{
	case NALU_PRIORITY_DISPOSABLE:
		outputString = "DISPOSABLE";
		break;
	case NALU_PRIRITY_LOW:
		outputString = "LOW";
		break;
	case NALU_PRIORITY_HIGH:
		outputString = "HIGH";
		break;
	case NALU_PRIORITY_HIGHEST:
		outputString = "HIGHEST";
		break;
	}

	outputString += " ";
	switch (naluheader.type)
	{
	case NALU_TYPE_SLICE:
		outputString += "SLICE";
		break;
	case NALU_TYPE_DPA:
		outputString += "DPA";
		break;
	case NALU_TYPE_DPB:
		outputString += "DPB";
		break;
	case NALU_TYPE_DPC:
		outputString += "DPC";
		break;
	case NALU_TYPE_IDR:
		outputString += "IDR";
		break;
	case NALU_TYPE_SEI:
		outputString += "SEI";
		break;
	case NALU_TYPE_SPS:
		outputString += "SPS";
		break;
	case NALU_TYPE_PPS:
		outputString += "PPS";
		break;
	case NALU_TYPE_AUD:
		outputString += "AUD";
		break;
	case NALU_TYPE_EOSEQ:
		outputString += "EOSEQ";
		break;
	case NALU_TYPE_EOSTREAM:
		outputString += "EOSTREAM";
		break;
	case NALU_TYPE_FILL:
		outputString += "FILL";
		break;
	default:
		outputString += "unkown" + to_string(naluheader.type);
		break;
	}
	LOG_DEBUG(outputString.c_str());
}

bool H264Wrap::DecodeNaluParams(NaluParam &naluParam, unsigned char *data, int &dataLen)
{
	KickOutNaluRaceCondition(data, dataLen);
	unsigned int startBits = 0;
	naluParam.header.forbiddent_zero_bit = u(1, data, startBits);
	naluParam.header.nal_ref_idc = u(2, data, startBits);
	naluParam.header.type = u(5, data, startBits);

	naluParam.rbsp.profile_idc = u(8, data, startBits);
	naluParam.rbsp.constraint_set0_flag = u(1, data, startBits); //(buf[1] & 0x80)>>7;
	naluParam.rbsp.constraint_set1_flag = u(1, data, startBits); //(buf[1] & 0x40)>>6;
	naluParam.rbsp.constraint_set2_flag = u(1, data, startBits); //(buf[1] & 0x20)>>5;
	naluParam.rbsp.constraint_set3_flag = u(1, data, startBits); //(buf[1] & 0x10)>>4;
	naluParam.rbsp.reserved_zero_4bits = u(4, data, startBits);
	naluParam.rbsp.level_idc = u(8, data, startBits);

	naluParam.rbsp.seq_parameter_set_id = Ue(data, dataLen, startBits);
	if (naluParam.rbsp.profile_idc == 100 || naluParam.rbsp.profile_idc == 110 ||
		naluParam.rbsp.profile_idc == 122 || naluParam.rbsp.profile_idc == 144)
	{
		naluParam.rbsp.chroma_format_idc = Ue(data, dataLen, startBits);
		if (naluParam.rbsp.chroma_format_idc == 3)
			naluParam.rbsp.residual_colour_transform_flag = u(1, data, startBits);
		naluParam.rbsp.bit_depth_luma_minus8 = Ue(data, dataLen, startBits);
		naluParam.rbsp.bit_depth_chroma_minus8 = Ue(data, dataLen, startBits);
		naluParam.rbsp.qpprime_y_zero_transform_bypass_flag = u(1, data, startBits);
		naluParam.rbsp.seq_scaling_matrix_present_flag = u(1, data, startBits);

		if (naluParam.rbsp.seq_scaling_matrix_present_flag)
		{
			for (int i = 0; i < 8; i++)
			{
				naluParam.rbsp.seq_scaling_list_present_flag[i] = u(1, data, startBits);
			}
		}
	}
	naluParam.rbsp.log2_max_frame_num_minus4 = Ue(data, dataLen, startBits);
	naluParam.rbsp.pic_order_cnt_type = Ue(data, dataLen, startBits);
	if (naluParam.rbsp.pic_order_cnt_type == 0)
		naluParam.rbsp.log2_max_pic_order_cnt_lsb_minus4 = Ue(data, dataLen, startBits);
	else if (naluParam.rbsp.pic_order_cnt_type == 1)
	{
		naluParam.rbsp.delta_pic_order_always_zero_flag = u(1, data, startBits);
		naluParam.rbsp.offset_for_non_ref_pic = Se(data, dataLen, startBits);
		naluParam.rbsp.offset_for_top_to_bottom_field = Se(data, dataLen, startBits);
		naluParam.rbsp.num_ref_frames_in_pic_order_cnt_cycle = Ue(data, dataLen, startBits);

		naluParam.rbsp.offset_for_ref_frame = new int[naluParam.rbsp.num_ref_frames_in_pic_order_cnt_cycle];
		for (unsigned int i = 0; i < naluParam.rbsp.num_ref_frames_in_pic_order_cnt_cycle; i++)
			naluParam.rbsp.offset_for_ref_frame[i] = Se(data, dataLen, startBits);
	}
	naluParam.rbsp.num_ref_frames = Ue(data, dataLen, startBits);
	naluParam.rbsp.gaps_in_frame_num_value_allowed_flag = u(1, data, startBits);
	naluParam.rbsp.pic_width_in_mbs_minus1 = Ue(data, dataLen, startBits);
	naluParam.rbsp.pic_height_in_map_units_minus1 = Ue(data, dataLen, startBits);

	naluParam.rbsp.frame_mbs_only_flag = u(1, data, startBits);
	if (!naluParam.rbsp.frame_mbs_only_flag)
		naluParam.rbsp.mb_adaptive_frame_field_flag = u(1, data, startBits);

	naluParam.rbsp.direct_8x8_inference_flag = u(1, data, startBits);
	naluParam.rbsp.frame_cropping_flag = u(1, data, startBits);
	if (naluParam.rbsp.frame_cropping_flag)
	{
		naluParam.rbsp.frame_crop_left_offset = Ue(data, dataLen, startBits);
		naluParam.rbsp.frame_crop_right_offset = Ue(data, dataLen, startBits);
		naluParam.rbsp.frame_crop_top_offset = Ue(data, dataLen, startBits);
		naluParam.rbsp.frame_crop_bottom_offset = Ue(data, dataLen, startBits);
	}
	naluParam.rbsp.vui_parameters_present_flag = u(1, data, startBits);
	if (naluParam.rbsp.vui_parameters_present_flag)
	{
		naluParam.rbsp.uvi_parameters.aspect_ratio_info_present_flag = u(1, data, startBits);
		if (naluParam.rbsp.uvi_parameters.aspect_ratio_info_present_flag)
		{
			naluParam.rbsp.uvi_parameters.aspect_ratio_idc = u(8, data, startBits);
			if (naluParam.rbsp.uvi_parameters.aspect_ratio_idc == 255)
			{
				naluParam.rbsp.uvi_parameters.sar_width = u(16, data, startBits);
				naluParam.rbsp.uvi_parameters.sar_height = u(16, data, startBits);
			}
		}
		naluParam.rbsp.uvi_parameters.overscan_info_present_flag = u(1, data, startBits);
		if (naluParam.rbsp.uvi_parameters.overscan_info_present_flag)
			naluParam.rbsp.uvi_parameters.overscan_appropriate_flag = u(1, data, startBits);
		naluParam.rbsp.uvi_parameters.video_signal_type_present_flag = u(1, data, startBits);
		if (naluParam.rbsp.uvi_parameters.video_signal_type_present_flag)
		{
			naluParam.rbsp.uvi_parameters.video_format = u(3, data, startBits);
			naluParam.rbsp.uvi_parameters.video_full_range_flag = u(1, data, startBits);
			naluParam.rbsp.uvi_parameters.colour_description_present_flag = u(1, data, startBits);
			if (naluParam.rbsp.uvi_parameters.colour_description_present_flag)
			{
				naluParam.rbsp.uvi_parameters.colour_primaries = u(8, data, startBits);
				naluParam.rbsp.uvi_parameters.transfer_characteristics = u(8, data, startBits);
				naluParam.rbsp.uvi_parameters.matrix_coefficients = u(8, data, startBits);
			}
		}
		naluParam.rbsp.uvi_parameters.chroma_loc_info_present_flag = u(1, data, startBits);
		if (naluParam.rbsp.uvi_parameters.chroma_loc_info_present_flag)
		{
			naluParam.rbsp.uvi_parameters.chroma_sample_loc_type_top_field = Ue(data, dataLen, startBits);
			naluParam.rbsp.uvi_parameters.chroma_sample_loc_type_bottom_field = Ue(data, dataLen, startBits);
		}
		naluParam.rbsp.uvi_parameters.timing_info_present_flag = u(1, data, startBits);
		if (naluParam.rbsp.uvi_parameters.timing_info_present_flag)
		{
			naluParam.rbsp.uvi_parameters.num_units_in_tick = u(32, data, startBits);
			naluParam.rbsp.uvi_parameters.time_scale = u(32, data, startBits);
		}
	}
	return true;
}
bool H264Wrap::FreeNaluParams(NaluParam &naluParam)
{
	if (naluParam.rbsp.offset_for_ref_frame)
	{
		delete[] naluParam.rbsp.offset_for_ref_frame;
		naluParam.rbsp.offset_for_ref_frame = NULL;
	}
	return true;
}