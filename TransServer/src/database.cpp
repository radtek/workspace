#include "database.h"
static otl_connect db_data,db_logs;
static DbInfo dbinfo;
vector<StationInfo*> vecStation;
pthread_mutex_t m_databaseMutex,m_infoMutex;

bool OTLConnect(const char * strConn)
{
	bool result = true;
	pthread_mutex_lock(&m_databaseMutex);
	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		db_data.rlogon(strConn,1);
	}
	catch(otl_exception &p){
		std::cerr<<"Open error: "<<p.msg<<std::endl;
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool OTLDisConnect()
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		db_data.logoff();
	}
	catch(otl_exception &p){
		std::cerr<<"Close error: "<<p.msg<<std::endl;
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

void OTLReConnect(const char* strConn)
{
Retry:
	if(db_data.connected == 1)
	{
		try
		{
			db_data.logoff();
		}
		catch(otl_exception &p)
		{
			std::cerr<<"Close error: "<<p.msg<<std::endl;
		}
	}
	sleep(1);
	
	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		db_data.rlogon(strConn,1);
	}
	catch(otl_exception &p){
		g_logs.WriteLog("原始数据库重连失败，等待30秒后重新尝试.");
			goto Retry;
	}
	g_logs.WriteLog("原始数据库重连成功,[%s].",strConn);
}

bool InitializeOtlInfo()
{
	pthread_mutex_init(&m_databaseMutex,NULL);	// 数据库锁,日志与数据两库共用一个锁
	pthread_mutex_init(&m_infoMutex,NULL);		// 还未使用
	
	GetConfigureString("ORACLE_DATA_CONNECT",dbinfo.strDataConnect,256,"vroad/vroad@orcl");
	GetConfigureString("ORACLE_LOGS_CONNECT",dbinfo.strLogsConnect,256,"vroad/vroad@192.168.137.1:1521/orcl");

	if(OTLConnect(dbinfo.strDataConnect))
		cout<<GetSystemTime()<<": 数据库连接成功."<<endl;
	if(LogConnect(dbinfo.strLogsConnect))
		cout<<GetSystemTime()<<": 日志库连接成功."<<endl;
}

/***************************************************************** 插入数据 **************************************************************************************/

bool insert_exceed_msg(stuInExceed* msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	
	bool result = true;
	try{
		char sqls[SQLLEN] = {0};

		sprintf(sqls,"insert into owm_exceedcar_param (devcode,stationcode,checktime,servertime,platetno,platetcode,platetcolour,laneno,"
			"lanecode,axletreenum,axlgroupnum,weight,vehicletype,leftwheelwt1,leftwheelwt2,leftwheelwt3,leftwheelwt4,leftwheelwt5,"
			"leftwheelwt6,leftwheelwt7,leftwheelwt8,rightwheelwt1,rightwheelwt2,rightwheelwt3,rightwheelwt4,rightwheelwt5,"
			"rightwheelwt6,rightwheelwt7,rightwheelwt8,wheelbase1,wheelbase2,wheelbase3,wheelbase4,wheelbase5,wheelbase6,"
			"wheelbase7,violatecode,overrunmark,speed,acceleration,vehiclelen,vehiclewidth,vehiclehigh,equivalentaxle,"
			"imgname1,imgpath1,imgname2,imgpath2,imgname3,imgpath3,devid,staid)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),"
			"to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,'%s','%s',%d,%d,%d,0,%d,%d, %d,%d,%d,%d,%d,%d,%d,%d, %d,%d,%d,%d,%d,%d,%d,%d ,%d,%d,%d,%d,%d,%d,%d,"
			"%d,%d,%d,%d,%d,%d,%d,%d, '%s','%s','%s','%s','%s','%s', %d,%d)",msg->devcode,msg->stationcode,msg->recordtime,msg->servertime,msg->vehicle
			,msg->platetno,msg->platet_color,(int)msg->lane_num,(int)msg->lane_code,(int)msg->axletree_type,msg->weight,(int)msg->vehicle_type
			,msg->leftwheelwt1,msg->leftwheelwt2,msg->leftwheelwt3,msg->leftwheelwt4,msg->leftwheelwt5,msg->leftwheelwt6,msg->leftwheelwt7,msg->leftwheelwt8
			,msg->rightwheelwt1,msg->rightwheelwt2,msg->rightwheelwt3,msg->rightwheelwt4,msg->rightwheelwt5,msg->rightwheelwt6,msg->rightwheelwt7
			,msg->rightwheelwt8,msg->wheelbase1,msg->wheelbase2,msg->wheelbase3,msg->wheelbase4,msg->wheelbase5,msg->wheelbase6,msg->wheelbase7
			,atoi(msg->exceed_type),msg->exceed_sign,msg->speed,msg->acceleration,msg->vehicle_length,msg->vehicle_width,msg->vehicle_high,msg->equivalentaxle
			,msg->imgname1,msg->imgpath1,msg->imgname2,msg->imgpath2,msg->imgname3,msg->imgpath3,(int)msg->t_id,(int)msg->s_id);

			otl_cursor::direct_exec(db_data,sqls);
			db_logs.commit();
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert exceed error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert exceed error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert exceed error:"<<p.var_info<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_traffic_msg(stuInTraffic * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		char sqls[SQLLEN] = {0},temp[SQLLEN] = {0};
		ostringstream ossSqls;

		sprintf(sqls, "insert into tbl_traffic_param(devcode,stationcode,createtime,servertime,devtype,errorcode,kind,lanecode,followpercent,headway,"
				"timeoccupation,zxkc_volume,zxkc_speed,xxhc_volume,xxhc_speed,dkc_volume,dkc_speed,zxhc_volume,zxhc_speed,dxhc_volume,dxhc_speed,"
				"tdxhc_volume,tdxhc_speed,jzxc_volume,jzxc_speed, xxc_volume,xxc_speed,zxc_volume,zxc_speed,dxc_volume,dxc_speed,tdxc_volume,tdxc_speed,"
				"ybjdc_volume,ybjdc_speed,tlj_volume,tlj_speed,mtc_volume,mtc_speed,handleflag,period,serialno,devid,staid) values ('%s','%s',"
				"to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,",msg->devcode,msg->stationcode,msg->record_time,
				msg->receive_time,(int)msg->device_type,(int)msg->deverr,(int)msg->content_type);

		for(int i = 0;i < (int)msg->lane_num; i++)
		{
			ossSqls.str("");
			memset(temp,0,SQLLEN);

			if(msg->content_type2 == '1')
			{
				sprintf(temp,"%d,%d,%d,%d, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, 0,0,0,0,0,0,0,0, 0,0, %d,%d,%d,%d, 0,%d,%d,%d,%d)",
						(int)msg->lanes[i].lane_id, (int)msg->lanes[i].percentage, (int)msg->lanes[i].avg_distance,(int)msg->lanes[i].time_percentage,
						(int)msg->lanes[i].vehicles[0].v_numbers,(int)msg->lanes[i].vehicles[0].avg_speed,
						(int)msg->lanes[i].vehicles[1].v_numbers,(int)msg->lanes[i].vehicles[1].avg_speed,
						(int)msg->lanes[i].vehicles[2].v_numbers,(int)msg->lanes[i].vehicles[2].avg_speed,
						(int)msg->lanes[i].vehicles[3].v_numbers,(int)msg->lanes[i].vehicles[3].avg_speed,
						(int)msg->lanes[i].vehicles[4].v_numbers,(int)msg->lanes[i].vehicles[4].avg_speed,
						(int)msg->lanes[i].vehicles[5].v_numbers,(int)msg->lanes[i].vehicles[5].avg_speed,
						(int)msg->lanes[i].vehicles[6].v_numbers,(int)msg->lanes[i].vehicles[6].avg_speed,
						(int)msg->lanes[i].vehicles[7].v_numbers,(int)msg->lanes[i].vehicles[7].avg_speed,
						(int)msg->lanes[i].vehicles[8].v_numbers,(int)msg->lanes[i].vehicles[8].avg_speed,
						(int)msg->period,(int)msg->time_id,(int)msg->t_id,(int)msg->s_id);
		
				ossSqls << sqls << temp;			
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
			else if(msg->content_type2 == '2')
			{		
				sprintf(temp,"%d,%d,%d,%d,0,0,0,0,0,0,0,0,0,0,0,0,0,0, %d,%d,%d,%d,%d,%d,%d,%d, 0,0, %d,%d,%d,%d, 0,%d,%d,%d,%d)",
						(int)msg->lanes[i].lane_id, (int)msg->lanes[i].percentage, (int)msg->lanes[i].avg_distance,(int)msg->lanes[i].time_percentage,
						(int)msg->lanes[i].vehicles[0].v_numbers,(int)msg->lanes[i].vehicles[0].avg_speed,
						(int)msg->lanes[i].vehicles[1].v_numbers,(int)msg->lanes[i].vehicles[1].avg_speed,
						(int)msg->lanes[i].vehicles[2].v_numbers,(int)msg->lanes[i].vehicles[2].avg_speed,
						(int)msg->lanes[i].vehicles[3].v_numbers,(int)msg->lanes[i].vehicles[3].avg_speed,
						(int)msg->lanes[i].vehicles[4].v_numbers,(int)msg->lanes[i].vehicles[4].avg_speed,
						(int)msg->lanes[i].vehicles[5].v_numbers,(int)msg->lanes[i].vehicles[5].avg_speed,
						(int)msg->period,(int)msg->time_id,(int)msg->t_id,(int)msg->s_id);
				
				ossSqls << sqls << temp;
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
			else if(msg->content_type2 == '3')
			{		
				sprintf(temp,"%d,%d,%d,%d, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, %d,%d, 0,0,%d,%d, 0,%d,%d,%d,%d)",
						(int)msg->lanes[i].lane_id, (int)msg->lanes[i].percentage, (int)msg->lanes[i].avg_distance,(int)msg->lanes[i].time_percentage,
						(int)msg->lanes[i].vehicles[0].v_numbers,(int)msg->lanes[i].vehicles[0].avg_speed,
						(int)msg->lanes[i].vehicles[1].v_numbers,(int)msg->lanes[i].vehicles[1].avg_speed,
						(int)msg->period,(int)msg->time_id,(int)msg->t_id,(int)msg->s_id);
				
				ossSqls << sqls << temp;
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
			else
			{
				pthread_mutex_unlock(&m_databaseMutex);
				return content_type_error;
			}
		}
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert Traffic error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert Traffic error:"<<p.stm_text<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_realtime_msg(stuInRealtime * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		char sqls[SQLLEN] = {0},temp[SQLLEN] = {0};
		sprintf(sqls,"insert into tbl_axletree_data_realtime(devcode,stationcode,createtime,servertime,errorcode,serialno,lanecount,lanecode,"
			"vehicletype,axletreetype,weight,speed,weight1,weight2,weight3,weight4,weight5,weight6,weight7,weight8,weight9,weight10,distance,"
			"distance1,distance2,distance3,distance4,distance5,distance6,distance7,distance8,distance9,devid,staid)values('%s','%s',"
			"to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"
			",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",msg->devcode,msg->stationcode,msg->record_time,msg->receive_time,(int)msg->deverr,msg->vehicleno
			,(int)msg->lanenumb,(int)msg->lanecode,(int)msg->vehtype,(int)msg->axltype,msg->weight,(int)msg->speed,(int)msg->axlweight[0],(int)msg->axlweight[1]
			,(int)msg->axlweight[2],(int)msg->axlweight[3],(int)msg->axlweight[4],(int)msg->axlweight[5],(int)msg->axlweight[6],(int)msg->axlweight[7]
			,(int)msg->axlweight[8],(int)msg->axlweight[9],(int)msg->wheelbase,(int)msg->distance[0],(int)msg->distance[1],(int)msg->distance[2]
			,(int)msg->distance[3],(int)msg->distance[4],(int)msg->distance[5],(int)msg->distance[6],(int)msg->distance[7],(int)msg->distance[8]
			,(int)msg->t_id,(int)msg->s_id);
	
		otl_cursor::direct_exec(db_data,sqls);
		db_logs.commit();
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert AxletreeRealtime error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeRealtime error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeRealtime error:"<<p.var_info<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_weight_msg(stuInWeight * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		char sqls[SQLLEN] = {0},temp[SQLLEN] = {0};
		ostringstream ossSqls;
		sprintf(sqls,"insert into tbl_weight_statistics(devcode,stationcode,createtime,servertime,devtype,errorcode,period,serialno,lanecount,lanecode,"
				"weightgradecount,weightgrade,volume,weight,equivalentnumber,devid,staid)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),"
				"to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,%d,",msg->devcode,msg->stationcode,msg->record_time,msg->receive_time,(int)msg->devtype
				,(int)msg->deverr,(int)msg->period,(int)msg->time_id);
		
		for(int i=0;i<(int)msg->lanecount;i++)
		{
			for(int j=0;j<(int)msg->lanes[i].levelcount;j++)
			{
				ossSqls.str("");
				memset(temp,0,SQLLEN);
				sprintf(temp,"%d,%d,%d,%d,%d,%d,%d,%d,%d)",(int)msg->lanecount,(int)msg->lanes[i].lane_id,(int)msg->lanes[i].levelcount
				,(int)msg->lanes[i].axldata[j].grade,(int)msg->lanes[i].axldata[j].volume,(int)msg->lanes[i].axldata[j].weight
				,(int)msg->lanes[i].axldata[j].equivalentnumber,(int)msg->t_id,(int)msg->s_id);
				
				ossSqls << sqls << temp;
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
		}
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert Weight error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert Weight error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert Weight error:"<<p.var_info<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_axlnumber_msg(stuInAxlNumber * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		char sqls[SQLLEN] = {0},temp[2048] = {0};
		ostringstream ossSqls;
		sprintf(sqls,"insert into tbl_axletree_number_statistics(devcode,stationcode,createtime,servertime,errorcode,period,serialno,lanecount,lanecode,"
				"numbergradecount,numbergrade,volume,weight,devid,staid)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),"
				"to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,",msg->devcode,msg->stationcode,msg->record_time,msg->receive_time,(int)msg->deverr
				,(int)msg->period,(int)msg->time_id);
		
		for(int i=0;i<(int)msg->lanecount;i++)
		{
			for(int j=0;j<(int)msg->lanes[i].levelcount;j++)
			{
				ossSqls.str("");
				memset(temp,0,SQLLEN);
			
				sprintf(temp,"%d,%d,%d,%d,%d,%d,%d,%d)",(int)msg->lanecount,(int)msg->lanes[i].lane_id,(int)msg->lanes[i].levelcount
						,(int)msg->lanes[i].axldata[j].grade,(int)msg->lanes[i].axldata[j].volume,(int)msg->lanes[i].axldata[j].weight,(int)msg->t_id
						,(int)msg->s_id);
				
				ossSqls << sqls << temp;
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
		}
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert AxletreeNumber error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeNumber error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeNumber error:"<<p.var_info<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_axlweight_msg(stuInAxlWeight * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		char sqls[SQLLEN] = {0},temp[SQLLEN] = {0};
		ostringstream ossSqls;
		sprintf(sqls,"insert into tbl_axletree_weight_statistics(devcode,stationcode,createtime,servertime,errorcode,period,serialno,lanecount,lanecode,"
				"weightgradecount,weightgrade,axletreenumber,axletreeweight,equivalentnumber,devid,staid)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),"
				"to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,",msg->devcode,msg->stationcode,msg->record_time,msg->receive_time,(int)msg->deverr
				,(int)msg->period,(int)msg->time_id);

		for(int i=0;i<(int)msg->lanecount;i++)
		{
			for(int j=0;j<(int)msg->lanes[i].levelcount;j++)
			{
				ossSqls.str("");
				memset(temp,0,SQLLEN);
				sprintf(temp,"%d,%d,%d,%d,%d,%d,%d,%d,%d)",(int)msg->lanecount,(int)msg->lanes[i].lane_id,(int)msg->lanes[i].levelcount
						,(int)msg->lanes[i].axldata[j].grade,(int)msg->lanes[i].axldata[j].volume,(int)msg->lanes[i].axldata[j].weight
						,(int)msg->lanes[i].axldata[j].equivalentnumber,(int)msg->t_id,(int)msg->s_id);
				
				ossSqls << sqls << temp;
				otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
				db_logs.commit();
			}
		}
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert AxletreeWeight error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeWeight error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert AxletreeWeight error:"<<p.var_info<<std::endl;
		OTLReConnect(dbinfo.strDataConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

/**************************************************************** 日志 *******************************************************************************************/

bool LogConnect(const char * strConn)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		db_logs.rlogon(strConn,1);
	}
	catch(otl_exception &p){
		std::cerr<<"Open error: "<<p.msg<<std::endl;
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool LogDisConnect()
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		db_logs.logoff();
	}
	catch(otl_exception &p){
		std::cerr<<"Log Connection Close error: "<<p.msg<<std::endl;
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

void LogReConnect(const char* strConn)
{
Retry:
	if(db_logs.connected == 1)
	{
		try{
			db_logs.logoff();
		}
		catch(otl_exception &p){
			std::cerr<<"Close error: "<<p.msg<<std::endl;
		}
	}
	sleep(1);

	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		db_logs.rlogon(strConn,1);
	}
	catch(otl_exception &p){
		g_logs.WriteLog("日志数据库重连失败，等待30秒后重新尝试.");
		goto Retry;
	}
	g_logs.WriteLog("日志数据库重连成功,[%s].",strConn);
}

bool insert_recv_log(const stuLogInfo &logs)
{
	char sqls[10240] = {0};
	sprintf(sqls,"insert into tbl_recv_log(devcode,stationcode,createtime,servertime,isretried,message_type,parseresult,messagelen,remote_ip,sourcemsg,devid,"
				 "staid) values ('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,%d,'%s','%s',%d,%d)",
				 logs.devcode,logs.stationcode,logs.createtime,logs.servertime,(int)logs.isretried,(int)logs.messagetype,
				 (int)logs.parseresult,(int)logs.msglen,logs.remote_ip,logs.sourcemsg,logs.t_id,logs.s_id);

#ifndef NO_DEBUG	
	g_logs.WriteLog("Print recv_log sqls:%s",sqls);
#endif

	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		otl_cursor::direct_exec(db_logs,sqls);
		db_logs.commit();
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert RECV_LOG error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert RECV_LOG error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert RECV_LOG error:"<<p.var_info<<std::endl;
		g_logs.WriteLog("Insert RECV_LOG Error:%s",sqls);
		LogReConnect(dbinfo.strLogsConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

bool insert_error_log(const stuLogInfo &logs)
{
	char sqls[10240] = {0};
	sprintf(sqls,"insert into tbl_error_package (msglen, parseresult, servertime, remote_ip, sourcemsg) values "
				 "(%d, %d, to_date('%s','yyyy-mm-dd hh24:mi:ss'), '%s','%s')", 
				 (int)logs.msglen, (int)logs.parseresult, logs.servertime, logs.remote_ip, logs.sourcemsg);
#ifndef NO_DEBUG	
	g_logs.WriteLog("Print error_log sqls:%s",sqls);
#endif
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		otl_cursor::direct_exec(db_logs,sqls);
		db_logs.commit();
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert ERROR_LOG error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert ERROR_LOG error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert ERROR_LOG error:"<<p.var_info<<std::endl;
		g_logs.WriteLog("Insert ERROR_LOG Error:%s",sqls);
		LogReConnect(dbinfo.strLogsConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

/***************************************************************** 获取站点信息 **********************************************************************************/

int GetConfigureFromDatabase()
{
	int result = 0;
	char strConnect[64] = {0};
	GetConfigureString("DATABASECONFIGURECONNECTSTRING",strConnect,64,"vroad/vroad@RACDB");
	otl_connect db;
	otl_connect::otl_initialize();
	try{
		db.rlogon(strConnect,1);
#ifndef NO_DEBUG
		cout<<GetSystemTime()<<": Oracle 数据库打开成功！"<<endl;
#endif
	}
	catch(otl_exception &p){
		g_logs.WriteLog("Oracle 数据库打开失败！ 连接字符串：%s",strConnect);
		result = -1;
	}

	if(result == -1)
	{
		db.logoff();
		return result;
	}

	try{
		char days[10] = {0};
		otl_stream select(1,"select configdesc from TBL_CONFIG where id = 1 and rownum = 1",db);
		select >> days;
		select.close();
		result = atoi(days);
	}
	catch(otl_exception &p){
		g_logs.WriteLog("Select Tbl_Config Error:%s",p.msg);
		result = -1;
	}
	do{
		try{
			char sqls[] = "select s.gczbh,t.devicecode,s.id,t.id from tbl_stations s "
						  "inner join tbl_stationstodevice std on s.id = std.stationid "
						  "inner join tbl_device_trf t on t.id = std.deviceid "
						  "where std.devicetype = 1 and t.isenabled in (1,-2) and gczbh is not null "
						  "and t.devicecode is not null";

			otl_stream select(1,sqls,db);
			while(!select.eof())
			{
				StationInfo * temp = new StationInfo;
				memset(temp,0,sizeof(StationInfo));
				select >> temp->stationcode;
				select >> temp->devicecode;
				select >> temp->stationid;
				select >> temp->deviceid;
				vecStation.push_back(temp);
			}
			select.close();
		}
		catch(otl_exception &p){
			std::cerr<<"Select Stationcode Error:"<<p.msg<<std::endl;
			g_logs.WriteLog("重新建立数据库连接");
Retry:
			g_logs.WriteLog("主动释放数据库连接");
			sleep(1);
			if(db.connected == 1)
			{
				try{
					db.logoff();    //logoff释放数据库连接
				}
				catch(otl_exception &p)
				{
					g_logs.WriteLog("Oracle 数据库释放失败 %s",p.msg);
				}
			}

			otl_connect::otl_initialize();  //初始化OTL数据库环境
			try
			{
				db.rlogon(strConnect,1);  //建立与数据库的连接
				g_logs.WriteLog("Oracle 数据库连接成功");
			}
			catch(otl_exception &p)     //用于描述OTL操作数据时抛出的异常
			{
				g_logs.WriteLog("Oracle 数据库连接失败 %s",p.msg);      //存异常的具体错误信息
				g_logs.WriteLog("10s后重新连接数据库");
				goto Retry;
			}
			continue;
		}
	}while(0);
	db.logoff();
	return result;
}
