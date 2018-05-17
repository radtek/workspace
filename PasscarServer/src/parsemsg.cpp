#include "parsemsg.h"

CTask::CTask()
{
	buf = NULL;
	memset(&data, 0, sizeof(VmsData));
	memset(response, 0, RESPONSE_SIZE);
}

CTask::~CTask()
{
	if(buf != NULL)
	{
		delete [] buf;
		buf = NULL;
	}
}

/*代入整个报文，len是总长度*/
int CTask::parseMsg()
{
	int result = normal;
	if(buf == NULL || len <= 0)
	{
		return memory_error;
	}

	char* vmsMsg = new char[len];
	int length = 0;
	vmsMsg[len] = '\0';
	memcpy(vmsMsg, buf, len);

	if(!((vmsMsg[0] & 0xFF) == 0x44 && (vmsMsg[1] & 0xFF) == 0x46))
	{	/*校验包头*/
		if(vmsMsg != NULL)
		{
			delete [] vmsMsg;
			vmsMsg = NULL;
		}
		return protocol_error;
	}

	if(!((vmsMsg[len-2] & 0xFF) == 0x44 && (vmsMsg[len-1] & 0xFF) == 0x45))
	{	/*校验包尾*/
		if(vmsMsg != NULL)
		{
			delete [] vmsMsg;
			vmsMsg = NULL;
		}
		return protocol_error;
	}

	length += 2;
	int packlen = *((short*)(vmsMsg+length));
	length += 2;
	if(packlen != len)
	{	/*校验包长*/
		if(vmsMsg != NULL)
		{
			delete [] vmsMsg;
			vmsMsg = NULL;
		}
		return protocol_error;
	}
	
	short crc16 = *((short*)(vmsMsg+len-4));
	short tempcrc = CRC16((unsigned char*)(vmsMsg+2), len-6);
	if(tempcrc != crc16)
	{
		if(vmsMsg != NULL)
		{
			delete [] vmsMsg;
			vmsMsg = NULL;
		}
		g_logs.WriteLog("CRC校验错误，原始CRC：%x; 计算CRC：%x", crc16, tempcrc);
		result = crc_error;
		goto RESPONSE;
	}

	data.type = vmsMsg[length];					
	length += 1;
	data.clientnum = *((short*)(vmsMsg+length));	
	length += 2;
	memcpy(data.onlysign, (vmsMsg+length), 6);	
	length += 6;
	data.isretry = vmsMsg[length];
	length += 1;
	data.errorcode = vmsMsg[length];
	length += 1;
	data.retrytimes = vmsMsg[length];
	length += 1;

	if(data.type == 1)
	{
		memcpy(data.data1.stationcode, vmsMsg+length, 15);
		length += 15;
		memcpy(data.data1.devicecode, vmsMsg+length, 32);
		length += 32;
		data.data1.year = *((short*)(vmsMsg+length));
		length += 2;
		data.data1.month = vmsMsg[length];
		length += 1;
		data.data1.mday = vmsMsg[length];
		length += 1;
		data.data1.hour = vmsMsg[length];
		length += 1;
		data.data1.minute = vmsMsg[length];
		length += 1;
		data.data1.second = vmsMsg[length];
		length += 1;
		data.data1.direction = vmsMsg[length];
		length += 1;
		data.data1.lane = vmsMsg[length];
		length += 1;
		data.data1.carType = vmsMsg[length];
		length += 1;
		data.data1.isBeijing = vmsMsg[length];
		length += 1;
	
/*
		char *plateNum = new char[16];
		memcpy(plateNum, vmsMsg+length, 16);
		length += 16;
		char Temp[256];
		code_convert("gb2312", "utf-8", plateNum, strlen(plateNum), Temp, 256);
		memcpy(data.data1.plateNumber, Temp, 16);
*/
		memcpy(data.data1.plateNumber, vmsMsg+length, 16);
		length += 16;

		data.data1.speed = *((short*)(vmsMsg+length));
		length += 2;
		data.data1.plateColor = vmsMsg[length];
		length += 1;
		data.data1.carColor = vmsMsg[length];
		length += 1;
		data.data1.signType = vmsMsg[length];
		length += 1;
		data.data1.plateType = vmsMsg[length];
		length += 1;
		data.data1.illegalNumber = *((short*)(vmsMsg+length));
		length += 2;
		data.data1.recordType = *((short*)(vmsMsg+length));
		length += 2;
		data.data1.dataSrc = *((short*)(vmsMsg+length));
		length += 2;
		data.data1.axlType = vmsMsg[length];
		length += 1;
		data.data1.weight = *((int*)(vmsMsg+length));
		length += 4;
		memcpy(data.data1.picName1, vmsMsg+length, 36);
		length += 36;
		memcpy(data.data1.picPath1, vmsMsg+length, 8);
		length += 8;
		memcpy(data.data1.picName2, vmsMsg+length, 36);
		length += 36;
		memcpy(data.data1.picPath2, vmsMsg+length, 8);
		length += 8;

		sprintf(data.data1.checktime, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", data.data1.year, data.data1.month, data.data1.mday,
				data.data1.hour, data.data1.minute, data.data1.second);
	}
	else if(data.type == 2)
	{
		memcpy(data.data2.stationcode, vmsMsg+length, 15);
		length += 15;
		memcpy(data.data2.devicecode, vmsMsg+length, 32);
		length += 32;
		data.data2.year = *((short*)(vmsMsg+length));
		length += 2;
		data.data2.month = vmsMsg[length];
		length += 1;
		data.data2.mday = vmsMsg[length];
		length += 1;
		data.data2.hour = vmsMsg[length];
		length += 1;
		data.data2.minute = vmsMsg[length];
		length += 1;
		data.data2.second = vmsMsg[length];
		length += 1;
		data.data2.direction = vmsMsg[length];
		length += 1;
		data.data2.lanecount = vmsMsg[length];
		length += 1;

		sprintf(data.data2.checktime, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", (int)data.data2.year, (int)data.data2.month, (int)data.data2.mday, 
				(int)data.data2.hour, (int)data.data2.minute, (int)data.data2.second);

		if(data.data2.lanecount != (len-76)/7)
		{	/*车道数校验*/
			if(vmsMsg != NULL)
			{
				delete [] vmsMsg;
				vmsMsg = NULL;
			}
			result = lanecount_error;
			goto RESPONSE;
		}

		int iTemp = 0;
		for(int i = 0; i < data.data2.lanecount; i++)
		{
			data.data2.veh[iTemp].lane = vmsMsg[length];
			length += 1;
			data.data2.veh[iTemp].volume = *((int*)(vmsMsg+length));
			length += 4;
			data.data2.veh[iTemp].speed = *((short*)(vmsMsg+length));
			length += 2;
			iTemp++;
		}
	}
	else
	{
		g_logs.WriteLog("数据解析失败，错误的数据包类型(%d).",data.type);
	}

RESPONSE:
	response[0] = 0x44;
	response[1] = 0x41;
	*(short*)(response+2) = 10;
	response[4] = data.type;
	memcpy(response+5, data.onlysign, 6);		// 数据唯一标识
	*(short*)(response+11) = DSC_SIGN;			// DSC服务器编号
	response[13] = (0xFF & result);				// 解析结果
	*(unsigned short*)(response+14) = CRC16((unsigned char*)response+2,12);
	response[16] = 0x44;
	response[17] = 0x45;

	if(vmsMsg != NULL)
	{
		delete [] vmsMsg;
		vmsMsg = NULL;
	}
	return result;
}

int CTask::Run()
{
	int result;
	result = parseMsg();
	if(result == normal)
	{
		if(!insert_passcar_message(&data))
		{
			g_logs.WriteLog("卡口数据插入数据库失败");
			result = insert_error;
		}
		else
		{
			if(data.type == 1)
				g_logs.WriteLog("卡口实时数据插入数据库成功");
			else if(data.type == 2)
				g_logs.WriteLog("卡口统计数据插入数据库成功");
		}
	}
	else
	{
		g_logs.WriteLog("卡口数据解析失败:%x",result);
	}
	return result;
}

void CTask::printMsg()
{
	cout << "Data type:" << (int)data.type << endl;
}

int CTask::code_convert(string from_charset,string to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset.c_str(),from_charset.c_str());
	if(cd == 0)
		return -1;
	memset(outbuf, 0, outlen);
	iconv(cd, pin, (size_t*)&inlen, pout, (size_t*)&outlen);
	iconv_close(cd);
	return 0;
}

