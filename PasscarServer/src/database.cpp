#include "database.h"
static otl_connect db_data,db_logs;
static DbInfo dbinfo;
pthread_mutex_t m_databaseMutex;

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
		g_logs->WriteLog("原始数据库重连失败，等待30秒后重新尝试.");
			goto Retry;
	}
	g_logs->WriteLog("原始数据库重连成功,[%s].",strConn);
}

bool InitializeOtlInfo()
{
	pthread_mutex_init(&m_databaseMutex,NULL);
	
	GetConfigureString("ORACLE_DATA_CONNECT", dbinfo.strDataConnect, 256, "vroad/vroad@orcl", CONFFILE);
	GetConfigureString("ORACLE_LOGS_CONNECT", dbinfo.strLogsConnect, 256, "vroad/vroad@192.168.137.1:1521/orcl", CONFFILE);

	if(OTLConnect(dbinfo.strDataConnect))
		cout<<GetSystemTime()<<": 数据库连接成功."<<endl;
	else
		cout<<GetSystemTime()<<": 数据库连接失败."<<endl;
	if(LogConnect(dbinfo.strLogsConnect))
		cout<<GetSystemTime()<<": 日志库连接成功."<<endl;
	else
		cout<<GetSystemTime()<<": 日志库连接失败."<<endl;
}

/***************************************************************** 插入数据 **************************************************************************************/

bool insert_passcar_message(VmsData * msg)
{
	pthread_mutex_lock(&m_databaseMutex);
	bool result = true;
	try{
		ostringstream ossSqls;
		char sqls[SQLLEN] = {0},temp[SQLLEN] = {0};
		if(msg->type == 1)
		{
			char servertime[20] = {0};
			sprintf(servertime,"%s",GetSystemTime().c_str());

			sprintf(sqls,"insert into vms_passcar_param(devcode,stationcode,checktime,direction,laneno,vehicletype,isbeijing,platetcode,platetcolour,"
					"vehiclecolour,marktype,platetype,behaviorno,recordtype,datasource,axletreetype,speed,weight,servertime,imgname1,imgpath1,imgname2,"
					"imgpath2)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
					"to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s','%s')",msg->data1.devicecode,msg->data1.stationcode,msg->data1.checktime,
					(int)msg->data1.direction,(int)msg->data1.lane,(int)msg->data1.carType,(int)msg->data1.isBeijing,msg->data1.plateNumber,
					(int)msg->data1.plateColor,(int)msg->data1.carColor,(int)msg->data1.signType,(int)msg->data1.plateType,(int)msg->data1.illegalNumber,
					msg->data1.recordType,msg->data1.dataSrc,(int)msg->data1.axlType,msg->data1.speed,msg->data1.weight,servertime,msg->data1.picName1,
					msg->data1.picPath1,msg->data1.picName2,msg->data1.picPath2);
			ossSqls << sqls;
		}
		else if(msg->type == 2)
		{
			
			sprintf(sqls,"insert into vms_passcar_statistics(devcode,stationcode,statisticstime,direction,lanecount,"
					"laneno1,laneno1volume,laneno1speed,laneno2,laneno2volume,laneno2speed,laneno3,laneno3volume,laneno3speed,laneno4,laneno4volume,"
					"laneno4speed,laneno5,laneno5volume,laneno5speed,laneno6,laneno6volume,laneno6speed,laneno7,laneno7volume,laneno7speed,laneno8,"
					"laneno8volume,laneno8speed)values('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d",msg->data2.devicecode,msg->data2.stationcode,
					msg->data2.checktime,(int)msg->data2.direction,(int)msg->data2.lanecount);

			ossSqls << sqls;
			for(int i = 0; i < 8; i++)
			{
				if(i >= msg->data2.lanecount)
				{
					ossSqls << ",null,null,null";
				}
				else
				{
					ossSqls << ","<< (int)msg->data2.veh[i].lane << "," << msg->data2.veh[i].volume << "," << msg->data2.veh[i].speed;
				}
			}
			ossSqls << ")";
		}
		else
		{
			g_logs->WriteLog("Insert Passcar Message error,error type(%d)",msg->type);
			pthread_mutex_unlock(&m_databaseMutex);
			return false;
		}
#ifndef NO_DEBUG
		cout << ossSqls.str().c_str() << endl;
#else
		otl_cursor::direct_exec(db_data,ossSqls.str().c_str());
		db_logs.commit();
#endif
	}
	catch(otl_exception &p)
	{
		std::cerr<<GetSystemTime()<<": Insert Passcar Message error:"<<p.msg<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert Passcar Message error:"<<p.stm_text<<std::endl;
		std::cerr<<GetSystemTime()<<": Insert Passcar Message error:"<<p.var_info<<std::endl;
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
		std::cerr << "Close error: " << p.msg << std::endl;
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
		g_logs->WriteLog("日志数据库重连失败，等待30秒后重新尝试.");
		goto Retry;
	}
	g_logs->WriteLog("日志数据库重连成功,[%s].",strConn);
}

bool insert_recv_log(const stuLogInfo &logs)
{
	char sqls[10240] = {0};
	sprintf(sqls,"insert into tbl_recv_log(devcode,stationcode,createtime,servertime,isretried,message_type,parseresult,messagelen,remote_ip,sourcemsg,devid,"
				 "staid) values ('%s','%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),%d,%d,%d,%d,'%s','%s',%d,%d)",
				 logs.devcode,logs.stationcode,logs.createtime,logs.servertime,(int)logs.isretried,(int)logs.messagetype,
				 (int)logs.parseresult,(int)logs.msglen,logs.remote_ip,logs.sourcemsg);

#ifndef NO_DEBUG	
	g_logs->WriteLog("Print recv_log sqls:%s",sqls);
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
		g_logs->WriteLog("Insert RECV_LOG Error:%s",sqls);
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
	g_logs->WriteLog("Print error_log sqls:%s",sqls);
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
		g_logs->WriteLog("Insert ERROR_LOG Error:%s",sqls);
		LogReConnect(dbinfo.strLogsConnect);
		result = false;
	}
	pthread_mutex_unlock(&m_databaseMutex);
	return result;
}

