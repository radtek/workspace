#include "jdmessage.h"

int iTimeSpace;
extern pthread_mutex_t m_infoMutex;

JdMessage::JdMessage()
{
}

JdMessage::~JdMessage()
{
}

//解析数据，参数为数据包地址，包长（整个包长度，包含包头、包尾）
int16_t JdMessage::ParseMessage(char * source_packet,int16_t length,stuOutMessage &stuMsg,void* &data)
{
	int16_t result = normal;
	unsigned char * msg;
	msg = new unsigned char[length];
	memcpy(msg,source_packet,length);

	if(length < 23)
	{/*包长度不满足要求，暂时无标准最短长度要求*/
		result = protocol_error;
		if(msg != NULL)
		{
			delete [] msg;
			msg = NULL;
		}
		return result;
	}
	
	memcpy(stuMsg.header,msg,2);				//包头,2
	if(stuMsg.header[0] != 0x44 || stuMsg.header[1] != 0x46)
	{/*校验包头，包尾是否正确*/
		result = protocol_error;
		if(msg != NULL)
		{
			delete [] msg;
			msg = NULL;
		}
		return result;
	}
	
	stuMsg.packet_length =*((short*)(msg+2));	//包长,2
	if(stuMsg.packet_length != (length-8))
	{/*检验包长*/
		result = protocol_error;
		if(msg != NULL)
		{
			delete [] msg;
			msg = NULL;
		}
		return result;
	}
	
	memcpy(stuMsg.tailer,msg+stuMsg.packet_length+8-2,2);		//包尾,2
	if(stuMsg.tailer[0] != 0x44 || stuMsg.tailer[1] != 0x45)
	{/*校验包头，包尾是否正确*/
		result = protocol_error;
		if(msg != NULL)
		{
			delete [] msg;
			msg = NULL;
		}
		return result;
	}
	
	memcpy(&stuMsg.command_type,msg+4,1);		//命令字,1
	stuMsg.dsc_id = *((short*)(msg+5));			//客户端编号,2
	stuMsg.packet_id_p1 = *((long*)(msg+7));	//UNIXtime,4
	stuMsg.packet_id_p2 = *((short*)(msg+11));	//自增号,2
	memcpy(&stuMsg.is_retried,msg+13,1);		//是否为重发数据,1
	memcpy(&stuMsg.error_code,msg+14,1);		//故障码,1
	stuMsg.retried = *((long*)(msg+15));		//重传次数,4
	memcpy(&stuMsg.crc16,msg+stuMsg.packet_length+8-4,2);		//CRC校验码,2,低位在前，高位在后

	//是否重发
	if(stuMsg.is_retried == 48)
		stuMsg.is_retried = 0x00;
	else if(stuMsg.is_retried == 49)
		stuMsg.is_retried = 0x01;

	//内层包
	unsigned char * msg_copy;
	msg_copy = new unsigned char[length-23];
	memcpy(msg_copy,msg+19,length-23);

	if(stuMsg.command_type == 1)
	{/*交调数据*/
		if(stuMsg.crc16 != CRC16(msg+2,stuMsg.packet_length+2))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(TRF)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(TRF)：%x",CRC16(msg+2,stuMsg.packet_length+2));
			result = crc_check_error;
		}
		else
		{
			stuInTraffic * jdmsg;
			jdmsg = new stuInTraffic;
			memset(jdmsg,0,sizeof(stuInTraffic));
			result = ParseTraffic(msg_copy,length-23,jdmsg);		//解析内层包
			data = jdmsg;
		}
	}
	else if(stuMsg.command_type == 57)
	{/*单车数据*/
		if(stuMsg.crc16 != CRC16(msg+2,stuMsg.packet_length+2))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(TRF)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(TRF)：%x",CRC16(msg+2,stuMsg.packet_length+2));
			result = crc_check_error;
		}
		else
		{
			stuInRealtime * dcmsg;
			dcmsg = new stuInRealtime;
			memset(dcmsg,0,sizeof(stuInRealtime));
			result = ParseAxlRealtime(msg_copy,length-23,dcmsg);		//解析内层包
			data = dcmsg;
		}
	}
	else if(stuMsg.command_type == 97)
	{/*车重数据*/
		if(stuMsg.crc16 != CRC16(msg+2,stuMsg.packet_length+2))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(TRF)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(TRF)：%x",CRC16(msg+2,stuMsg.packet_length+2));
			result = crc_check_error;
		}
		else
		{
			stuInWeight * czmsg;
			czmsg = new stuInWeight;
			memset(czmsg,0,sizeof(stuInWeight));
			result = ParseCarWeight(msg_copy,length-23,czmsg);		//解析内层包
			data = czmsg;
		}
	}
	else if(stuMsg.command_type == 98)
	{/*轴数数据*/
		if(stuMsg.crc16 != CRC16(msg+2,stuMsg.packet_length+2))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(TRF)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(TRF)：%x",CRC16(msg+2,stuMsg.packet_length+2));
			result = crc_check_error;
		}
		else
		{
			stuInAxlNumber * zsmsg;
			zsmsg = new stuInAxlNumber;
			memset(zsmsg,0,sizeof(stuInAxlNumber));
			result = ParseAxlNumber(msg_copy,length-23,zsmsg);		//解析内层包
			data = zsmsg;
		}
	}
	else if(stuMsg.command_type == 104)
	{/*轴重数据*/
		if(stuMsg.crc16 != CRC16(msg+2,stuMsg.packet_length+2))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(TRF)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(TRF)：%x",CRC16(msg+2,stuMsg.packet_length+2));
			result = crc_check_error;
		}
		else
		{
			stuInAxlWeight * zzmsg;
			zzmsg = new stuInAxlWeight;
			memset(zzmsg,0,sizeof(stuInAxlWeight));
			result = ParseAxlWeight(msg_copy,length-23,zzmsg);		//解析内层包
			data = zzmsg;
		}
	}
	else if(stuMsg.command_type == 65)
	{/*劝返数据*/
		if(stuMsg.crc16 != CRC16_OWM(msg+4,stuMsg.packet_length))
		{/*CRC校验*/
			g_logs.WriteLog("原始CRC(OWM)：%x",stuMsg.crc16);
			g_logs.WriteLog("计算CRC(OWM)：%x",CRC16_OWM(msg+4,stuMsg.packet_length));
			result = crc_check_error;
		}
		else
		{
			stuInExceed * qfmsg;
			qfmsg = new stuInExceed;
			memset(qfmsg,0,sizeof(stuInExceed));
			result = ParseExceed(msg_copy,length-23,qfmsg);
			data = qfmsg;
		}
	}
	else
	{/*无此命令字*/
		result = protocol_error;
	}

	if(msg != NULL)
	{
		delete [] msg;
		msg = NULL;
	}
	if(msg_copy != NULL)
	{/*释放内存*/
		delete [] msg_copy;
		msg_copy = NULL;
	}

	return result;
}

/*劝返解析*/
int16_t JdMessage::ParseExceed(unsigned char * packet,int length,stuInExceed * exceed)
{
	time_t timer = time(NULL);
	struct tm* local;
	local = localtime(&timer);
	sprintf(exceed->servertime,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
		local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);
	
	int16_t result = normal;
	memcpy(&exceed->createtime,packet,8);
	memcpy(exceed->stationcode,packet+8,15);
	memcpy(exceed->devcode,packet+23,16);
	memcpy(&exceed->deverr,packet+39,1);
	exceed->vehicle = *((long*)(packet+40));
	memcpy(&exceed->lane_num,packet+44,1);
	memcpy(&exceed->lane_code,packet+45,1);
	memcpy(&exceed->vehicle_type,packet+46,1);
	memcpy(&exceed->axletree_type,packet+47,1);
	exceed->vehicle_length = *((long*)(packet+48));
	exceed->vehicle_width = *((long*)(packet+52));
	exceed->vehicle_high = *((long*)(packet+56));
	memcpy(exceed->platetno,packet+60,12);
	memcpy(exceed->platet_color,packet+72,2);
	memcpy(exceed->exceed_type,packet+74,4);
	
	local = localtime(&exceed->checktime);
	sprintf(exceed->recordtime,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
		local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(exceed->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(exceed->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			exceed->t_id = vecStation[i]->deviceid;
			exceed->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	exceed->weight = *((long*)(packet+78));
	exceed->exceed_sign = *((long*)(packet+82));
	exceed->equivalentaxle = *((long*)(packet+86));
	memcpy(&exceed->checktime,packet+90,8);
	exceed->speed = *((long*)(packet+98));
	exceed->acceleration = *((long*)(packet+102));
	exceed->leftwheelwt1 = *((long*)(packet+106));
	exceed->leftwheelwt2 = *((long*)(packet+110));
	exceed->leftwheelwt3 = *((long*)(packet+114));
	exceed->leftwheelwt4 = *((long*)(packet+118));
	exceed->leftwheelwt5 = *((long*)(packet+122));
	exceed->leftwheelwt6 = *((long*)(packet+126));
	exceed->leftwheelwt7 = *((long*)(packet+130));
	exceed->leftwheelwt8 = *((long*)(packet+134));
	exceed->rightwheelwt1 = *((long*)(packet+138));
	exceed->rightwheelwt2 = *((long*)(packet+142));
	exceed->rightwheelwt3 = *((long*)(packet+146));
	exceed->rightwheelwt4 = *((long*)(packet+150));
	exceed->rightwheelwt5 = *((long*)(packet+154));
    exceed->rightwheelwt6 = *((long*)(packet+158));
    exceed->rightwheelwt7 = *((long*)(packet+162));
    exceed->rightwheelwt8 = *((long*)(packet+166));
	exceed->wheelbase1 = *((long*)(packet+170));
    exceed->wheelbase2 = *((long*)(packet+174));
    exceed->wheelbase3 = *((long*)(packet+178));
    exceed->wheelbase4 = *((long*)(packet+182));
    exceed->wheelbase5 = *((long*)(packet+186));
    exceed->wheelbase6 = *((long*)(packet+190));
    exceed->wheelbase7 = *((long*)(packet+194));
	memcpy(exceed->imagesource,packet+198,100);
	exceed->imagelen = *((long*)(packet+298));

	memcpy(exceed->imagepath,exceed->imagesource,24);
	int len;
	for(int i=24;i<50;i++)
	{
		if(exceed->imagesource[i] == 0x20)
		{
			len = i;
			break;
		}
		else if(i == 99)
			len = 99;
	}
	memcpy(exceed->imagename,exceed->imagesource+24,len-24);
	memcpy(exceed->imgpath1,exceed->imagesource,24);
	memcpy(exceed->imgpath2,exceed->imagesource,24);
	memcpy(exceed->imgpath3,exceed->imagesource,24);
	sprintf(exceed->imgname1,"%s_VehImage.jpg",exceed->imagename);
	sprintf(exceed->imgname2,"%s_PlateImage.jpg",exceed->imagename);
	sprintf(exceed->imgname3,"%s_FarImage.jpg",exceed->imagename);

	return result;
}

/*单车数据解析*/
int16_t JdMessage::ParseAxlRealtime(unsigned char * packet,int length,stuInRealtime * realtime)
{
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(realtime->receive_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",local->tm_year+1900,
		local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	int16_t result = normal;
	memcpy(realtime->i_header,packet,2);									//内包头
	realtime->i_packet_length = *((short*)(packet+2));						//内包长
	memcpy(realtime->i_tailer,packet+realtime->i_packet_length+8-2,2);		//内包尾

	if(realtime->i_header[0] != 0xAA || realtime->i_header[1] != 0xAA || realtime->i_tailer[0] != 0xEE || realtime->i_tailer[1] != 0xEE)
	{	/*包头包尾校验*/
		result = protocol_error;
		return result;
	}
	if(realtime->i_packet_length != (length-8))
	{
		/*内包长校验*/
		result = protocol_error;
		return result;
	}
	realtime->i_crc16 = *((short*)(packet+realtime->i_packet_length+8-4));
	if(realtime->i_crc16 != CRC16(packet+2,length-6))
	{
		/*CRC校验*/
		result = crc_check_error;
		return result;
	}

	memcpy(&realtime->i_command_type,packet+4,1);
	memcpy(realtime->stationcode,packet+5,15);
	memcpy(realtime->devcode,packet+20,16);
	memcpy(&realtime->deverr,packet+36,1);
	realtime->vehicleno = *((long*)(packet+37));
	realtime->year = *((short*)(packet+41));
	memcpy(&realtime->month,packet+43,1);
	memcpy(&realtime->day,packet+44,1);
	memcpy(&realtime->lanenumb,packet+45,1);
	memcpy(&realtime->lanecode,packet+46,1);
	memcpy(&realtime->hour,packet+47,1);
	memcpy(&realtime->minute,packet+48,1);
	memcpy(&realtime->second,packet+49,1);
	memcpy(&realtime->vehtype,packet+50,1);
	memcpy(&realtime->axltype,packet+51,1);
	realtime->weight =*((long*)(packet+52));
	realtime->speed =*((short*)(packet+56));
	
	sprintf(realtime->record_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",(int)realtime->year,(int)realtime->month,(int)realtime->day,
			(int)realtime->hour,(int)realtime->minute,(int)realtime->second);
	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(realtime->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(realtime->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			realtime->t_id = vecStation[i]->deviceid;
			realtime->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	int axlenumber = (int)realtime->axltype;

	if(axlenumber>10)
	{
		result = axle_number_error;
		return result;
	}
	memset(realtime->axlweight,0,sizeof(int16_t)*10);
	memset(realtime->distance,0,sizeof(int16_t)*9);

	for(int i = 0;i < axlenumber;i++)
	{
		realtime->axlweight[i] = *((short*)(packet+58+i*2));
	}
	for(int j = 0;j < axlenumber-1;j++)
	{
		realtime->distance[j] = *((short*)(packet+58+axlenumber*2+j*2));
	}
	realtime->wheelbase = *((short*)(packet+58+axlenumber*4-2));

	return result;
}

/*轴数数据解析*/
int16_t JdMessage::ParseAxlNumber(unsigned char * packet,int length,stuInAxlNumber * axlnumber)
{
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(axlnumber->receive_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",local->tm_year+1900,
		local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	int16_t result = normal;
	memcpy(axlnumber->i_header,packet,2);									//内包头
	axlnumber->i_packet_length = *((short*)(packet+2));						//内包长
	memcpy(axlnumber->i_tailer,packet+axlnumber->i_packet_length+8-2,2);	//内包尾
	if(axlnumber->i_header[0] != 0xAA || axlnumber->i_header[1] != 0xAA || axlnumber->i_tailer[0] != 0xEE || axlnumber->i_tailer[1] != 0xEE)
	{	/*包头包尾校验*/
		result = protocol_error;
		return result;
	}
	if(axlnumber->i_packet_length != (length-8))
	{
		/*内包长校验*/
		result = protocol_error;
		return result;
	}
	axlnumber->i_crc16 = *((short*)(packet+axlnumber->i_packet_length+8-4));
	if(axlnumber->i_crc16 != CRC16(packet+2,length-6))
	{
		/*CRC校验*/
		result = crc_check_error;
		return result;
	}
	
	memcpy(&axlnumber->i_command_type,packet+4,1);
	memcpy(axlnumber->devcode,packet+5,16);
	memcpy(axlnumber->stationcode,packet+21,15);
	memcpy(&axlnumber->deverr,packet+36,1);
	axlnumber->year = *((short*)(packet+37));
	memcpy(&axlnumber->month,packet+39,1);
	memcpy(&axlnumber->day,packet+40,1);
	memcpy(&axlnumber->period,packet+41,1);
	axlnumber->time_id = *((short*)(packet+42));
	memcpy(&axlnumber->lanecount,packet+44,1);
	
	CreateTime(axlnumber->record_time,axlnumber->year,axlnumber->month,axlnumber->day,axlnumber->time_id);	//数据产生时间
	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(axlnumber->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(axlnumber->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			axlnumber->t_id = vecStation[i]->deviceid;
			axlnumber->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	if(axlnumber->period != 5)
	{
		result = period_error;
		return result;
	}
	if(axlnumber->time_id < 0 || axlnumber->time_id > 288)
	{
		result = time_id_error;
		return result;
	}

	int len = 0;
	for(int i=0;i<axlnumber->lanecount;i++)
	{
		struct lane_axle temp_lane;
		memcpy(&temp_lane.lane_id,packet+45+len,1);
		len++;
		temp_lane.levelcount = *((short*)(packet+45+len));
		len+=2;
		for(int j=0;j<temp_lane.levelcount;j++)
		{
			struct axlgrade_data temp_data;
			memcpy(&temp_data.grade,packet+45+len,1);
			len++;
			temp_data.volume = *((short*)(packet+45+len));
			len+=2;
			temp_data.weight = *((long*)(packet+45+len));
			len+=4;
			temp_data.equivalentnumber = 0;
			temp_lane.axldata.push_back(temp_data);
		}
		axlnumber->lanes.push_back(temp_lane);
	}
	
	return result;
}

/*轴重数据解析*/
int16_t JdMessage::ParseAxlWeight(unsigned char * packet,int length,stuInAxlWeight * axlweight)
{
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(axlweight->receive_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",local->tm_year+1900,
		local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);
	
	int16_t result = normal;
	memcpy(axlweight->i_header,packet,2);									//内包头
	axlweight->i_packet_length = *((short*)(packet+2));						//内包长
	memcpy(axlweight->i_tailer,packet+axlweight->i_packet_length+8-2,2);	//内包尾

	if(axlweight->i_header[0] != 0xAA || axlweight->i_header[1] != 0xAA || axlweight->i_tailer[0] != 0xEE || axlweight->i_tailer[1] != 0xEE)
	{	/*包头包尾校验*/
		result = protocol_error;
		return result;
	}
	if(axlweight->i_packet_length != (length-8))
	{
		/*内包长校验*/
		result = protocol_error;
		return result;
	}
	axlweight->i_crc16 = *((short*)(packet+axlweight->i_packet_length+8-4));
	if(axlweight->i_crc16 != CRC16(packet+2,length-6))
	{
		/*CRC校验*/
		result = crc_check_error;
		return result;
	}

	memcpy(&axlweight->i_command_type,packet+4,1);
	memcpy(axlweight->devcode,packet+5,16);
	memcpy(axlweight->stationcode,packet+21,15);
	memcpy(&axlweight->deverr,packet+36,1);
	axlweight->year = *((short*)(packet+37));
	memcpy(&axlweight->month,packet+39,1);
	memcpy(&axlweight->day,packet+40,1);
	memcpy(&axlweight->period,packet+41,1);
	axlweight->time_id = *((short*)(packet+42));
	memcpy(&axlweight->lanecount,packet+44,1);
	CreateTime(axlweight->record_time,axlweight->year,axlweight->month,axlweight->day,axlweight->time_id);	//数据产生时间
	
	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(axlweight->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(axlweight->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			axlweight->t_id = vecStation[i]->deviceid;
			axlweight->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	if(axlweight->period != 5)
	{
		result = period_error;
		return result;
	}
	if(axlweight->time_id < 0 || axlweight->time_id > 288)
	{
		result = time_id_error;
		return result;
	}

	int len = 0;
	for(int i=0;i<axlweight->lanecount;i++)
	{
		struct lane_axle temp_lane;
		memcpy(&temp_lane.lane_id,packet+45+len,1);
		len++;
		temp_lane.levelcount = *((short*)(packet+45+len));
		len+=2;
		for(int j=0;j<temp_lane.levelcount;j++)
		{
			struct axlgrade_data temp_data;
			memcpy(&temp_data.grade,packet+45+len,1);
			len++;
			temp_data.volume = *((long*)(packet+45+len));
			len+=4;
			temp_data.weight = *((long*)(packet+45+len));
			len+=4;
			temp_data.equivalentnumber = *((long*)(packet+45+len));
			len+=4;
			temp_lane.axldata.push_back(temp_data);
		}
		axlweight->lanes.push_back(temp_lane);
	}
	return result;
}

/*车重数据解析*/
int16_t JdMessage::ParseCarWeight(unsigned char * packet,int length,stuInWeight * weight)
{
	int16_t result = normal;
	memcpy(weight->i_header,packet,2);										//内包头
	weight->i_packet_length = *((short*)(packet+2));						//内包长
	memcpy(weight->i_tailer,packet+weight->i_packet_length+8-2,2);			//内包尾

	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(weight->receive_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",local->tm_year+1900,
			local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	if(weight->i_header[0] != 0xAA || weight->i_header[1] != 0xAA || weight->i_tailer[0] != 0xEE || weight->i_tailer[1] != 0xEE)
	{	/*包头包尾校验*/
		result = protocol_error;
		return result;
	}
	if(weight->i_packet_length != (length-8))
	{
		/*内包长校验*/
		result = protocol_error;
		return result;
	}
	weight->i_crc16 = *((short*)(packet+weight->i_packet_length+8-4));
	if(weight->i_crc16 != CRC16(packet+2,length-6))
	{
		/*CRC校验*/
		result = crc_check_error;
		return result;
	}
	
	memcpy(&weight->i_command_type,packet+4,1);
	memcpy(weight->devcode,packet+5,16);
	memcpy(weight->stationcode,packet+21,15);
	memcpy(&weight->deverr,packet+36,1);
	weight->year = *((short*)(packet+37));
	memcpy(&weight->month,packet+39,1);
	memcpy(&weight->day,packet+40,1);
	memcpy(&weight->period,packet+41,1);
	weight->time_id = *((short*)(packet+42));
	memcpy(&weight->lanecount,packet+44,1);
	memcpy(&weight->devtype,weight->devcode+4,1);
	CreateTime(weight->record_time,weight->year,weight->month,weight->day,weight->time_id);	//数据产生时间
	
	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(weight->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(weight->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			weight->t_id = vecStation[i]->deviceid;
			weight->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	if(weight->period != 5)
	{
		result = period_error;
		return result;
	}
	if(weight->time_id < 0 || weight->time_id > 288)
	{
		result = time_id_error;
		return result;
	}

	int len = 0;
	for(int i=0;i<weight->lanecount;i++)
	{
		if(weight->lanecount > 20 || weight->lanecount < 0)
		{
			result = lane_id_scope_error;
			break;
		}
		struct lane_axle temp_lane;
		memcpy(&temp_lane.lane_id,packet+45+len,1);
		len++;
		temp_lane.levelcount = *((short*)(packet+45+len));
		len+=2;
		for(int j=0;j<temp_lane.levelcount;j++)
		{
			if(temp_lane.levelcount > 10 ||temp_lane.levelcount < 0)
			{
				result = level_count_error;
				return result;;
			}
			struct axlgrade_data temp_data;
			memcpy(&temp_data.grade,packet+45+len,1);
			len++;
			temp_data.volume = *((short*)(packet+45+len));
			len+=2;
			temp_data.weight = *((long*)(packet+45+len));
			len+=4;
			temp_data.equivalentnumber = *((long*)(packet+45+len));
			len+=4;
			temp_lane.axldata.push_back(temp_data);
		}
		weight->lanes.push_back(temp_lane);
	}
	return result;
}

/*交调解析*/
int16_t JdMessage::ParseTraffic(unsigned char * packet,int length,stuInTraffic * traffic)
{
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(traffic->receive_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",local->tm_year+1900,
		local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	int16_t result = normal;
	memcpy(traffic->i_header,packet,2);										//内包头
	traffic->i_packet_length = *((short*)(packet+2));						//内包长
	memcpy(traffic->i_tailer,packet+traffic->i_packet_length+8-2,2);		//内包尾
	
	if(traffic->i_header[0] != 0xAA || traffic->i_header[1] != 0xAA || traffic->i_tailer[0] != 0xEE || traffic->i_tailer[1] != 0xEE)
	{	/*包头包尾校验*/
		result = protocol_error;
		return result;
	}
	if(traffic->i_packet_length != (length-8))
	{
		/*内包长校验*/
		result = protocol_error;
		return result;
	}

	traffic->i_crc16 = *((short*)(packet+traffic->i_packet_length+8-4));
	if(traffic->i_crc16 != CRC16(packet+2,length-6))
	{
		/*CRC校验*/
		result = crc_check_error;
		return result;
	}
	
	memcpy(&traffic->i_command_type,packet+4,1);	//内包命令字
	if(traffic->i_command_type != 0x01)
	{
		result = protocol_error;
		return result;
	}
	memcpy(traffic->devcode,packet+5,16);
	memcpy(traffic->stationcode,packet+21,15);
	if(!strlen(traffic->stationcode))
	{/*站点编号为空*/
		result = stationcode_error;
		return result;
	}

	memcpy(&traffic->deverr,packet+36,1);
	memcpy(&traffic->content_type,packet+37,1);				//调查内容，01 无预留，02 有预留, 取设备ID第4位
	memcpy(&traffic->content_type2,traffic->devcode+4,1);	//设备级别，一级、二级、三级
	memcpy(&traffic->year,packet+38,2);			//年
	memcpy(&traffic->month,packet+40,1);		//月
	memcpy(&traffic->day,packet+41,1);			//日
	memcpy(&traffic->period,packet+42,1);		//周期
	traffic->time_id = *((short*)(packet+43));	//时间序号
	if(traffic->period != 5)
	{/*周期校验*/
		result = period_error;
		return result;
	}
	if(traffic->time_id < 0 || traffic->time_id > 288)
	{/*包序号校验*/
		result = time_id_error;
		return result;
	}
	CreateTime(traffic->record_time,traffic->year,traffic->month,traffic->day,traffic->time_id);	//数据产生时间

	// 校验站点编号,设备编号
	pthread_mutex_lock(&m_infoMutex);
	for(int i=0;i<vecStation.size();i++)
	{
		if(memcmp(traffic->stationcode,vecStation[i]->stationcode,15)==0 && memcmp(traffic->devcode,vecStation[i]->devicecode,16)== 0){
			result = normal;
			traffic->t_id = vecStation[i]->deviceid;
			traffic->s_id = vecStation[i]->stationid;
			break;
		}
		else
			result = stationcode_not_found;
	}
	pthread_mutex_unlock(&m_infoMutex);
	if(result != normal)
		return result;

	memcpy(&traffic->lane_num,packet+45,1);		//车道数
	if((int)(traffic->lane_num) > 20)
	{
		result = lane_id_scope_error;
		g_logs.WriteLog("Error Info 车道号：%d",(int)(traffic->lane_num));
		return result;
	}
	
	if(CompareTime(traffic->record_time,traffic->receive_time) == false)
	{// 超过截止时间
		result = record_time_late_error;
		return result;
	}

	int temp_lane_id = 0;

	if(traffic->content_type2 == '1')
	{/*一类设备*/
		traffic->device_type = 1;
		if(traffic->content_type == 1)
		{/*无保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_11[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;
			
			for(int i=0;i<(int)traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+32*i,1);					//车道号
				memcpy(&temp_lane.percentage,packet+47+32*i,1);					//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+32*i));			//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+32*i,1);			//时间占比
				if(temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}
				for(int k=0;k<9;k++)
				{
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+3*k+32*i));
					memcpy(&veh.avg_speed,packet+51+3*k+32*i+2,1);
					temp_lane.vehicles.push_back(veh);			//压入lane内

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal)
					{
#ifndef NO_DEBUG
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
#endif
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else if(traffic->content_type == 2)
		{/*有保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_12[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;
			
			for(int i=0;i<traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+68*i,1);				//车道号
				memcpy(&temp_lane.percentage,packet+47+68*i,1);				//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+68*i));		//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+68*i,1);		//时间占比

				if(temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}

				for(int k=0;k<9;k++)
				{
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+7*k+68*i));
					memcpy(&veh.avg_speed,packet+51+7*k+68*i+2,1);
					veh.reserved_1 = *((short*)(packet+51+7*k+68*i+3));
					veh.reserved_2 = *((short*)(packet+51+7*k+68*i+5));
					temp_lane.vehicles.push_back(veh);	//压入lane内					

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal){	
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else
		{
			result = content_type_error;
		}
	}
	else if(traffic->content_type2 == '2')
	{/*二类设备*/
		traffic->device_type = 2;
		if(traffic->content_type == 1)
		{/*无保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_21[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;
			
			for(int i=0;i<traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+23*i,1);				//车道号
				memcpy(&temp_lane.percentage,packet+47+23*i,1);				//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+23*i));		//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+23*i,1);		//时间占比
				if(temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}
				for(int k=0;k<6;k++)
				{
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+3*k+23*i));
					memcpy(&veh.avg_speed,packet+51+3*k+23*i+2,1);
					temp_lane.vehicles.push_back(veh);		//压入lane内

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal){	
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else if(traffic->content_type == 2)
		{/*有保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_22[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;

			for(int i=0;i<traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+47*i,1);			//车道号
				memcpy(&temp_lane.percentage,packet+47+47*i,1);		//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+47*i));	//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+47*i,1);		//时间占比
				if(temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}
				for(int k=0;k<6;k++)
				{
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+7*k+47*i));
					memcpy(&veh.avg_speed,packet+51+7*k+47*i+2,1);
					veh.reserved_1 = *((short*)(packet+51+7*k+47*i+3));
					veh.reserved_2 = *((short*)(packet+51+7*k+47*i+5));
					temp_lane.vehicles.push_back(veh);	//压入lane内

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal){	
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else
		{
			result = content_type_error;
		}
	}
	else if(traffic->content_type2 == '3')
	{/*三类设备*/
		traffic->device_type = 3;
		if(traffic->content_type == 1)
		{/*无保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_31[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;
			
			for(int i=0;i<traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+11*i,1);			//车道号
				memcpy(&temp_lane.percentage,packet+47+11*i,1);			//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+11*i));		//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+11*i,1);		//时间占比
				if((int)temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}
				for(int k=0;k<2;k++)
				{	
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+3*k+11*i));
					memcpy(&veh.avg_speed,packet+51+3*k+11*i+2,1);
					temp_lane.vehicles.push_back(veh);	//压入lane内

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal){	
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else if(traffic->content_type == 2)
		{/*有保留字段*/
			for(int j=0;j<20;j++)
			{
				if(length == valid_32[j]-23){
					temp_lane_id = j+1;
					if(temp_lane_id != (int)(traffic->lane_num))
						result = lane_id_numb_error;
					else
						result = normal;
					break;
				}
				else
					result = content_type_error;
			}
			if(result != normal)
				return result;
			
			for(int i=0;i<traffic->lane_num;i++)
			{
				struct lane_data temp_lane;
				memcpy(&temp_lane.lane_id,packet+46+19*i,1);			//车道号
				memcpy(&temp_lane.percentage,packet+47+19*i,1);			//跟车百分比
				temp_lane.avg_distance = *((short*)(packet+48+19*i));	//平均车头距离
				memcpy(&temp_lane.time_percentage,packet+50+19*i,1);	//时间占比
				
				if(temp_lane.lane_id > 40){
					result = lane_id_scope_error;
					return result;
				}
				for(int k=0;k<2;k++)
				{
					struct vehicle_data veh;
					veh.v_numbers =*((short*)(packet+51+7*k+19*i));
					memcpy(&veh.avg_speed,packet+51+7*k+19*i+2,1);
					veh.reserved_1 = *((short*)(packet+51+7*k+19*i+3));
					veh.reserved_2 = *((short*)(packet+51+7*k+19*i+5));
					temp_lane.vehicles.push_back(veh);			//压入lane内

					if(veh.v_numbers < 0)
					{
						result = car_volume_error;
					}
					else if(veh.v_numbers > 0 && veh.avg_speed <= 0)
					{
						result = speed_logic_error;
					}
					else if(veh.avg_speed < 0)
					{
						result = car_speed_error;
					}
					else if(veh.avg_speed > 0 && veh.v_numbers <= 0)
					{
						result = volume_logic_error;
					}

					if(result != normal){	
						g_logs.WriteLog("Car volume error:%d",veh.v_numbers);
						g_logs.WriteLog("Car speed error:%d",veh.avg_speed);
						return result;
					}
				}
				traffic->lanes.push_back(temp_lane);
			}
		}
		else
		{
			result = content_type_error;
		}
	}
	else
	{
		result = content_type_error;
	}
	return result;
}

void JdMessage::PrintMessage(void * message,int type)
{
	if(type == 1)
	{
		stuInTraffic * traffic = (stuInTraffic*)message;
		std::cout << "包头：" << traffic->i_header <<std::endl;
		std::cout << "包长：" << traffic->i_packet_length <<std::endl;
		std::cout << "命令字：" << (int)traffic->i_command_type <<std::endl;
		std::cout << "设备编码：" << traffic->devcode <<std::endl;
		std::cout << "站点编号：" << traffic->stationcode <<std::endl;
		std::cout << "错误码：" << (int)traffic->deverr <<std::endl;
		std::cout << "调查内容1：" << (int)traffic->content_type <<std::endl;
		std::cout << "调查内容2：" << traffic->content_type2 <<std::endl;
		std::cout << "采集时间：" << traffic->record_time <<std::endl;
		std::cout << "接收时间：" << traffic->receive_time <<std::endl;	
		std::cout << "年：" << traffic->year << std::endl;
		std::cout << "月：" << (int)traffic->month << std::endl;
		std::cout << "日：" << (int)traffic->day << std::endl;
		std::cout << "车道数：" << (int)traffic->lane_num << std::endl;
		for(int i=0;i < traffic->lane_num;i++)
        {
			std::cout <<"车道号："<< (int)traffic->lanes[i].lane_id << std::endl;
			std::cout <<"跟车百分比："<< (int)traffic->lanes[i].percentage << std::endl;
			std::cout <<"平均车头距离："<< (int)traffic->lanes[i].avg_distance << std::endl;
			std::cout <<"时间占比："<< (int)traffic->lanes[i].time_percentage << std::endl;

			if(traffic->content_type2 == '1')
			{
				for(int j=0;j<9;j++)
				{
					std::cout <<"vehicle_num_"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].v_numbers << std::endl;
					std::cout <<"vehicle_speed_"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].avg_speed << std::endl;
				}
			}
			else if(traffic->content_type2 == '2')
			{
				for(int j=0;j<6;j++)
				{
					std::cout <<"vehicle_num_"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].v_numbers << std::endl;
					std::cout <<"vehicle_speed_"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].avg_speed << std::endl;
				}
			}
			else if(traffic->content_type2 == '3')
			{
				for(int j=0;j<2;j++)
				{
					std::cout <<"vehicle_num_"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].v_numbers << std::endl;
					std::cout <<"vehicle_speed"<<j+1<<"："<< (int)traffic->lanes[i].vehicles[j].avg_speed << std::endl;
				}
			}
			std::cout <<"周期："<< (int)traffic->period << std::endl;
			std::cout <<"时间序号："<< traffic->time_id << std::endl;
		}
		std::cout << "包尾：" << traffic->i_tailer <<std::endl;
	}
	else if(type == 65)
	{
		stuInExceed * exceed = (stuInExceed*)message;
		cout<<"设备编号："<<exceed->devcode<<endl;
		cout<<"站点编号："<<exceed->stationcode<<endl;
		cout<<"采集时间："<<exceed->recordtime<<endl;
		cout<<"接收时间："<<exceed->servertime<<endl;
		cout<<"车辆编号："<<exceed->vehicle<<endl;
		cout<<"车 牌 号："<<exceed->platetno<<endl;
		cout<<"车牌颜色："<<exceed->platet_color<<endl;
		cout<<"车 道 数："<<(int)exceed->lane_num<<endl;
		cout<<"车 道 号："<<(int)exceed->lane_code<<endl;
		cout<<"轴    型："<<(int)exceed->axletree_type<<endl;
		cout<<"车    重："<<exceed->weight<<endl;
		cout<<"车    型："<<(int)exceed->vehicle_type<<endl;
		cout<<"左轮1 重："<<exceed->leftwheelwt1<<endl;
		cout<<"左轮2 重："<<exceed->leftwheelwt2<<endl;
		cout<<"左轮3 重："<<exceed->leftwheelwt3<<endl;
		cout<<"左轮4 重："<<exceed->leftwheelwt4<<endl;
		cout<<"左轮5 重："<<exceed->leftwheelwt5<<endl;
		cout<<"左轮6 重："<<exceed->leftwheelwt6<<endl;
		cout<<"左轮7 重："<<exceed->leftwheelwt7<<endl;
		cout<<"左轮8 重："<<exceed->leftwheelwt8<<endl;
		cout<<"右轮1 重："<<exceed->rightwheelwt1<<endl;
		cout<<"右轮2 重："<<exceed->rightwheelwt2<<endl;
		cout<<"右轮3 重："<<exceed->rightwheelwt3<<endl;
		cout<<"右轮4 重："<<exceed->rightwheelwt4<<endl;
		cout<<"右轮5 重："<<exceed->rightwheelwt5<<endl;
		cout<<"右轮6 重："<<exceed->rightwheelwt6<<endl;
		cout<<"右轮7 重："<<exceed->rightwheelwt7<<endl;
		cout<<"右轮8 重："<<exceed->rightwheelwt8<<endl;
		cout<<"轴 距 1 ："<<exceed->wheelbase1<<endl;
		cout<<"轴 距 2 ："<<exceed->wheelbase2<<endl;
		cout<<"轴 距 3 ："<<exceed->wheelbase3<<endl;
		cout<<"轴 距 4 ："<<exceed->wheelbase4<<endl;
		cout<<"轴 距 5 ："<<exceed->wheelbase5<<endl;
		cout<<"轴 距 6 ："<<exceed->wheelbase6<<endl;
		cout<<"轴 距 7 ："<<exceed->wheelbase7<<endl;
		cout<<"违 例 码："<<atoi(exceed->exceed_type)<<endl;
		cout<<"超限标识："<<exceed->exceed_sign<<endl;
		cout<<"速    度："<<exceed->speed<<endl;
		cout<<"加 速 度："<<exceed->acceleration<<endl;
		cout<<"车    长："<<exceed->vehicle_length<<endl;
		cout<<"车    宽："<<exceed->vehicle_width<<endl;
		cout<<"车    高："<<exceed->vehicle_high<<endl;
		cout<<"当量轴次："<<exceed->equivalentaxle<<endl;
		cout<<"大    图："<<exceed->imgname1<<endl;
		cout<<"大图路径："<<exceed->imgpath1<<endl;
		cout<<"车 牌 图："<<exceed->imgname2<<endl;
		cout<<"车牌路径："<<exceed->imgpath2<<endl;
		cout<<"全 景 图："<<exceed->imgname3<<endl;
		cout<<"全景路径："<<exceed->imgpath3<<endl;
	}
	else if(type == 57)
	{
	}
	else if(type == 97)
	{
	}
	else if(type == 98)
	{
	}
	else if(type == 104)
	{
	}
	else if(type == 255)
	{
		stuLogInfo* log = (stuLogInfo*)message;
		cout<<"设备编号："<<log->devcode<<endl;
		cout<<"站点编号："<<log->stationcode<<endl;
		cout<<"采集时间："<<log->createtime<<endl;
		cout<<"接收时间："<<log->servertime<<endl;
		cout<<"是否重发："<<log->isretried<<endl;
		cout<<"数据类型："<<log->messagetype<<endl;
		cout<<"解析结果："<<log->parseresult<<endl;
		cout<<"原始报文："<<log->sourcemsg<<endl;
	}
}

void JdMessage::AnswerMessage(char * answer_msg,char * source_packet,int16_t code)
{

	answer_msg[0] = 0x44;	
	answer_msg[1] = 0x41;	
	
	*(short*)(answer_msg+2) = 10;			//包长
	answer_msg[4] = source_packet[4];		//命令字
	memcpy(answer_msg+5,source_packet+7,6);		//数据唯一标识
	memcpy(answer_msg+11,source_packet+5,2);	//DSC客户端编号
	*(unsigned char*)(answer_msg+13) = code;
	*(unsigned short*)(answer_msg+14) = CRC16((unsigned char *)answer_msg+2,12);
	answer_msg[16] = 0x44;
	answer_msg[17] = 0x45;
}

void JdMessage::CreateTime(char * record_time,int16_t yy,unsigned char mm,unsigned char dd,int16_t time_id)
{
	char createtime[32] = {0};
	int year,month,day,hour,minute,mi_count;
	mi_count = time_id*5;
	year = yy;
	month = (int)mm;
	day = (int)dd;

	if(mi_count == 1440)
	{
		if(year%400 == 0 || (year%100 != 0 && year%4 == 0))
		{/*闰年*/
			if(month == 2)
			{
				if(day == 29)
				{
					month += 1;
					day = 1;
				}
				else
				{
					day += 1;
				}
			}
			else if(month == 4 ||month == 6 || month == 9 ||month == 11 )
			{
				if(day == 30)
				{
					month += 1;
					day = 1;
				}
				else
				{
					day += 1;
				}
			}
			else
			{
				if(day == 31)
				{
					if(month == 12)
					{
						year += 1;
						month = 1;
						day = 1;
					}
					else
					{
						month += 1;
						day = 1;
					}
				}
				else
				{
					day += 1;
				}
			}
		}
		else
		{/*非闰年*/
			if(month == 2)
			{
				if(day == 28)
				{
					month += 1;
					day = 1;
				}
				else
				{
					day += 1;
				}
			}
			else if(month == 4 ||month == 6 || month == 9 ||month == 11 )
			{
				if(day == 30)
				{
					month += 1;
					day = 1;
				}
				else
				{
					day += 1;
				}
			}
			else
			{
				if(day == 31)
				{
					if(month == 12)
					{
						year += 1;
						month = 1;
						day = 1;
					}
					else
					{
						month += 1;
						day = 1;
					}
				}
				else
				{
					day += 1;
				}
			}
		}
		hour = 0;
		minute = 0;
	}
	else
	{
		hour = mi_count/60;
		minute = mi_count%60;
	}
	
	sprintf(createtime,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:00",year,month,day,hour,minute);
	memcpy(record_time,createtime,32);
}

bool JdMessage::CompareTime(char * createtime,char * servertime)
{
	bool result;
	struct tm * local1;
	struct tm * local2;
	time_t timer1,timer2;
	local1 = new struct tm;
	local2 = new struct tm;
	sscanf(createtime,"%4d-%2d-%2d %2d:%2d:%2d",&local1->tm_year,&local1->tm_mon,&local1->tm_mday,&local1->tm_hour,&local1->tm_min,&local1->tm_sec);
	sscanf(servertime,"%4d-%2d-%2d %2d:%2d:%2d",&local2->tm_year,&local2->tm_mon,&local2->tm_mday,&local2->tm_hour,&local2->tm_min,&local2->tm_sec);
	
	local1->tm_year -= 1900;
	local1->tm_mon -= 1;
	local2->tm_year -= 1900;
	local2->tm_mon -= 1;

	timer1 = mktime(local1);
	timer2 = mktime(local2);
	pthread_mutex_lock(&m_infoMutex);
	result = ((((timer2-timer1)/60/60/24) < iTimeSpace )?true:false);
	pthread_mutex_unlock(&m_infoMutex);
	if(local1 != NULL){
		delete local1;
		local1 = NULL;
	}
	if(local2 != NULL){
		delete local1;
		local2 = NULL;
	}
	return result;
}

