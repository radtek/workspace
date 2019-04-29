// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  defines.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月11日 12时26分54秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

typedef unsigned char BYTE;

enum MessageType
{
	emEventLog = 10,
	emRegionDetail = 20,
	emQueryDatetime = 30,
	emControllerMemory = 40,
	emQueryControllerDetail = 41,
	emQueryControllerExtend = 50,
	emActionList = 90,
	emGreenWindow = 100,
	emGreenWindowCancel = 101,
	emAlarmDisConn = 120,
	emAlarmConn = 121,
	emAlarmAck = 122,
	emAlarmHide = 123,
	emAlarmShow = 125,
	emAlarmClear = 128,
	emAlarmDelete = 129,
//	emOpenFile = 110,
//	emReadFile = 111,
//	emCloseFile = 112,
	emRampMetering = 130,
	emRedlightTime = 131,
	emCongestion = 140,
	emCongestionIndex = 141,
	emDetectorCongestionIndex = 142,
	emRegionDetailExtend = 220,
	emLockPhaseOrder = 300,
	emIntervene = 310,
	emLaneStatus = 320,
	emStrategyMonitor = 330,
	emDetectorCount = 350,
	emRegisterService = 1500
};
/* ************************************************************************************************ */

/* *
 * scats版本
 * */
struct T_Version
{
	BYTE main;			// 主版本号	0-15
	BYTE major;			// 主修订号	0-63
	BYTE minor;			// 次修订号	0-63
};

struct T_LicenceVersion
{
	BYTE protocol;		// 协议版本 1-255,仅当主机版本不为0时有此项
	T_Version version;	// 主机版本
	BYTE hostNumber;	// 主机号	0-255
};

/* ************************************************************************************************ */
struct T_LicenceOption
{
	bool isFixedTime;
	bool isAdaptive;
	bool isCMS;
	bool isDIDO;
	bool isANTTS;
};

struct T_SiteData
{
	unsigned short id;
	BYTE subsystemId;
	BYTE spare;
};

struct T_RegionDetail
{
	BYTE regionNo;				//
	T_Version version;			//
	char regionName[8];			//
	T_LicenceOption support;	//
	int ipaddr;					//
	short port;
	// 22
	bool isHave;			// 用于标记是否含下列数据
	BYTE count;
	BYTE buildNo;
	unsigned short itsPort;
	BYTE flag;
	int siteCount;				// site数
	T_SiteData site[250];		// 
};
/* ************************************************************************************************ */

struct T_RouteRequest
{
	BYTE workstation;			// 0,未知或不需要
								// 1-100,workstation ID
								// 广播
	BYTE connFlag;				// bit0: 工作站连接标志，0，未连接，1，连接
								// bit7: 特权用户标志(message subtypes 1–3, 6–10, 17–18)
	BYTE userId;				// 用户id
	BYTE subtype;				// 消息子类型
								// 1 = Load
								// 2 = Start
								// 3 = Cancel
								// 4 = Query
								// 5 = Monitor
								// 6 = Hold
								// 7 = Release
								// 8 = Dwell
								// 9 = Free
								// 10 = Unload
								// 16 = Connect
								// 17 = Set start
								// 18 = Set end
								// 32 = Query route
								// 33 = Query site
	unsigned short routeNumber;		// 0 = All routes (message subtypes 4, 32)
									// 0 = Not needed (message subtypes 16, 33)
									// 1–32699 = Route number
	unsigned short siteId;
};
/* ************************************************************************************************ */

struct T_RouteResponseCode
{
	bool succeed;
	bool allRoute;
	bool loadedFlag;
	bool description;		// 是否包含描述信息

	bool errorFlag;			// 此位被设置则认为有错误发生,以上字段认为无效
	BYTE error;				// 128 = Database read error
							// 129 = Database format error
							// 130 = Site not found
							// 131 = Site already loaded
							// 132 = Route not found
							// 133 = Memory allocation failure
							// 134 = Route not active
							// 135 = Route not held
							// 136 = Route is active
							// 137 = Insufficient access level
							// 138 = Route conflict
							// 139 = Workstation not connected
};

struct T_RouteResponseFlag
{
	bool cancel;
	bool held;
	bool privilege;
	bool active;
};

struct T_RouteResponsePhaseFlag
{
	BYTE dwell;			// 1-7 代表A-G相位
	bool ignore;		// 是否忽略此站点
	bool startProcess;
	bool started;
	bool finished;
};

struct T_RouteResponseSite
{
	unsigned short siteId;
	T_RouteResponsePhaseFlag phaseFlag;
	BYTE regionName;
	BYTE dwellPhase;
	BYTE xsfFlagsStatus;
	BYTE xsfFlagsEnabled;
	short delay;
	unsigned short duration;
	BYTE currentPhase;
	BYTE phaseTime;
};

struct T_LoadRoute
{
	bool active;
	unsigned short routeNum;
};

struct T_RouteResponse
{
	BYTE workstation;
	BYTE flag;			// 1,最后一条应答; 2,第一条应答; 前两bit有效，其他未用
	T_RouteResponseCode code;
	BYTE loadRoute;
	unsigned short notUse1;		// 0
	unsigned short notUse2;		// 0

	int routeCount;
	T_LoadRoute route[15];
	char *description[128];
	BYTE siteNumber;				// siteData 数量
	T_RouteResponseFlag flags;
	unsigned short routeNumber;		// 1-32699

	T_RouteResponseSite siteData[16];
};
/* ************************************************************************************************ */

struct T_Datetime
{
	short year;			// 年
	BYTE month;			// 月
	BYTE day;			// 日
	BYTE hour;			// 时
	BYTE minute;		// 分
	BYTE second;		// 秒
};
/* ************************************************************************************************ */

struct T_EventLog
{
	// 默认message type 为16，不带时间戳，后续有需求再更改
	bool isFaultLogFlag;			
	bool isAlarmDisplayFlag;
	bool isEventLogFlag;
	bool isAlarmTypeFlag;
	bool isFaultFlag;

	BYTE alarmId;					// 当alarmtype为1时才使用
	unsigned short extendAlarmId;	// 当alarmId为255时使用
	char text[100];
};
/* ************************************************************************************************ */

struct T_AlarmRequest
{
	BYTE subType;		// 0, 断开警报器连接
						// 1, 连接警报器
						// 2, 已确认警报
						// 3, 隐藏警报?
						// 5, 展示警报
						// 8, 清除警报
						// 9, 删除警报
	BYTE userId;		// 仅当subType大于1时包含此字段

	int idCount;		// alarmId 数
	unsigned int alarmId[250];	// 仅当subType大于1时包含
};
/* ************************************************************************************************ */

struct T_ANTTSResponse
{
};
/* ************************************************************************************************ */

struct T_ANTTSRequest
{
	bool startFlag;		// 0,停止发送; 1,开始发送
	bool tagDataFlag;	// 0,没有标记数据跟随; 1,有tag data跟随
	bool extendTagFlag;	// 是否包含扩展数据
	BYTE minute;		// 分
	BYTE second;		// 秒
};
/* ************************************************************************************************ */

struct T_AlarmResponse_Flag
{
	bool firstRecord;		// 是否是第一条记录
	bool lastRecord;		// 是否是最后记录
	bool updateType;		// 是否是部分update ?
	bool isAreaName;		// 是否含区域名
};

struct T_AlarmData_Flag
{
	bool status;			// 0,off; 1,on
	bool ackStatus;			// 
	bool asFlag;			// alarm/status lag
	bool hideStatus;
	bool clearFlag;
	bool testFlag;
};

struct T_AlarmData
{
	BYTE length;
	unsigned int identifier;
	T_Datetime datetime;
	T_Datetime ackDatetime;
	T_AlarmData_Flag flags;		// 警报参数
	BYTE count;					// 警报计数？
	BYTE ackUserId;				// 0,未知警报; 1-200,确认警报的用户ID
	BYTE areaNameId;			// 0,area name 未定义; 1-64,第多少个area name
	char remark[100];			// 最多不超100字节
};

struct T_AlarmResponse
{
	T_AlarmResponse_Flag flags;
	char areaName[1024];		// 区域名，不定长，以逗号分隔,最多64个
	int datasCount;
	T_AlarmData datas[64];
};
/* ************************************************************************************************ */

struct T_ActionListRequest
{
	BYTE mode;		// 0,region name; 1,site id
	char regionname[8];		// 6字节,只有mode为0时此字段有效
	unsigned short siteId;	// 当mode为1时此字节有效
	int listCount;
	unsigned short list[250];
};

struct T_ActionListResponse
{
	BYTE mode;		// 0,region name; 1,site id
	BYTE status;
	char regionname[8];		// 6字节
	unsigned short siteId;
	int listCount;
	unsigned short list[250];
};
/* ************************************************************************************************ */

struct T_GreenWindowResponseData
{
	unsigned short siteId;
	BYTE status;
};

struct T_GreenWindowResponse
{
	int count;
	T_GreenWindowResponseData data[250];
};
/* ************************************************************************************************ */

struct T_SiteTime
{
	unsigned short id;
	unsigned short starttime;
	unsigned short duration;
	BYTE phase;
};

struct T_GreenWindowRequest
{
	BYTE priority;
	unsigned short count;
	T_SiteTime site[250];
};
/* ************************************************************************************************ */

struct T_LockPhaseRequest
{
	unsigned short siteId;
	BYTE preferredPhase;	// 1-7: A-G ; 0,移除

	BYTE alternatePhase;	// bit0-6: A-G, 如果alternate Phase在Preferred Phase之前到达，
							// 它将被锁定，代替preferred Phase，锁定时间为Dwell Time。
							// 如果没有改变相位的需求，这个值应该被设置为0
							
	unsigned short time;	// 0 没有锁定时间，但相位将被调用
							// 1-900 锁定时间，单位：秒
							// 65535 永远锁定
};
/* ************************************************************************************************ */

struct T_MemoryCtrlRequest
{
	unsigned short siteId;
	BYTE page;
	BYTE offset;
};
/* ************************************************************************************************ */

struct T_DetectBlockRequest
{
	int count;
	unsigned short siteIds[250];
};
/* ************************************************************************************************ */

struct T_StrategyMonitor
{
	bool isSM;
	bool noRepeat;
	char regionname[8];
	BYTE subSystem[32];
};
/* ************************************************************************************************ */

struct T_SiteData_LaneStatus_SSDT
{
	/* * 单独定制某站点的参数
	 * * Site site-specific data type
	 * Bit 0: Send site status
	 * Bit 1: Send plan numbers
	 * Bit 2: Send phase data
	 * Bit 3: Send flags data
	 * Bit 4: Send signal group data
	 * Bit 5: Send subsystem data
	 * Bit 6–7: Not used
	 * Bit 8: Send alarm details with site status
	 * Bit 9: Send phase changes
	 * Bit 10: Send phase intervals
	 * Bits 11–14: Not used
	 * Bit 15: Send all requested messages for this site immediately
	 * Note: This word is only included if site-specific data follows each site
	 * 		number, i.e. bit 2 of message subtype =1. If site-specific requests are
	 * 		included, they are added to default data types.
	 * */
	bool bHasSiteStatus;
	bool bHasPlanNumber;
	bool bHasPhaseData;
	bool bHasFlagsData;
	bool bHasSignalGroupData;
	bool bHasSubsysData;
	bool bHasAlarmDetail;
	bool bHasPhaseChanges;
	bool bHasPhaseInterval;
	bool bHasSendToAll;
};

struct T_SiteData_LaneStatus
{
	unsigned short id;
	T_SiteData_LaneStatus_SSDT stSsdt;
};

struct T_LaneStatusRequest_Msgcode
{
	/* * 消息码
	 * * Message Code
	 * Bit 0: Start/stop flag
	 * 		0 = Stop sending site status
	 * 		1 = Send site status
	 * Bit 1: Default data type follows
	 * Bit 2: Site-specific data types follow each site number
	 * Bit 3: Not used
	 * Bits 4–5: Message subtype
	 * 		0 = No sites included in this message
	 * 		1 = Replace site list for this ITS application with these new sites
	 * 		2 = Remove these sites from the site list
	 * 		3 = Add these sites to the site list
	 * Bit 6–7: Not used
	 * */
	bool bStart;				// 1,start; 0,stop
	bool bDefaultDataType;		// 是否追加默认数据类型
	bool bSiteSpecific;			// 每条site数据附带各自的数据类型
	BYTE byMsgSubtype;			// 0,不包含站点数据
								// 1,用新站点替换站点列表?
								// 2,从站点列表移除这些站点
								// 3,添加站点到站点列表
};

struct T_LaneStatusRequest_DDT
{
	/* * Default data type
	 * Bit 0: Send site status
	 * Bit 1: Send plan numbers
	 * Bit 2: Send phase data
	 * Bit 3: Send flags data for any flags that are on
	 * Bit 4: Send signal group data
	 * Bit 5: Send subsystem data
	 * Bits 6–7: Not used
	 * Bit 8: Send alarm details with site status
	 * Bit 9: Send phase changes
	 * Bit 10: Send phase intervals
	 * Bits 11–13: Not used
	 * Bit 14: Send response
	 * 		0 = Default data type only applies to the site list following
	 * 			(from site 1 onwards as shown below in this table)
	 * 		1 = Default data type applies to all sites in system
	 * Bit 15: Send all requested messages for all sites in the site list for this ITS application immediately
	 * Note: This word is only included if bit 1 of message subtype is set.
	 * */
	bool bHasSiteStatus;
	bool bHasPlanNumber;
	bool bHasPhaseData;
	bool bHasFlagsData;
	bool bHasSignalGroupData;
	bool bHasSubsysData;

	bool bHasAlarmDetail;
	bool bHasPhaseChanges;
	bool bHasPhaseInterval;

	bool bHasResponse;
	bool bHasSendToAll;
};

struct T_LaneStatusRequest
{
	T_LaneStatusRequest_Msgcode stMsgcode;		// Byte
	T_LaneStatusRequest_DDT stDefaultDataType;	// Word
	int iCount;
	T_SiteData_LaneStatus stSitedata[250];		// Word
};
/* ************************************************************************************************ */

struct T_PhaseChange
{
	unsigned short siteid;
//	BYTE phaseData;
	BYTE phaseId;		// 相位编号 A-G
	BYTE phaseStat;		// 相位间隔？
	bool change;		// 相位改变
	bool delay;			// 相位延长
	BYTE lampStatus;	// 0正常,1黄闪,2灭灯,3,没有通讯
};

struct T_PhaseData_Flag
{
	bool bSS;				// 包含站点状态数据
	bool bPSL;				// 是否包含 plan number, special facilities, locks
	bool bPPP;				// 是否包含 phase data, phase times, phase status
	bool bFC;				// 是否包含 flag content
	bool bSP;				// 是否包含 signal group data 和 phase demands
	bool bSSNR;				// 是否包含 subsystem no,flags,nominal cycle,require cycle length
};

struct T_PhaseData_SiteStatus
{
	bool bSIDA;
	bool bFYBO;
	bool bWAlarm;
	bool bMAlarm;
	bool bHpMode;
	bool bFallback;
	bool bClloss;
	bool bDwell;
	bool bAlarmDetail;
	BYTE byPlanStat;		// 灯态计划
							// 0 = Off
							// 1 = On
							// 2 = Flashing yellow

	bool bCongested;		// 拥堵
	BYTE bySlMode;			// Stored link mode
							// 0 = Isolated
							// 1 = Flexilink
							// 2 = Master-isolated
							// 3 = Masterlink
};

struct T_PhaseData_Data
{
	unsigned short phaseTime;	// 阶段运行时间
	BYTE phaseInterval;			// 当前灯态
								// 0 = Late start
								// 1 = Minimum green
								// 2 = Variable initial green
								// 3 = Rest
								// 4 = Early cut-off green
								// 5 = Red/yellow
								// 6 = 黄灯
								// 7 = 全红

	BYTE phaseId;				//当前相位，A-G 
};

struct T_PhaseData_PlanNo
{
	BYTE bySplitPlanNo;			// Split plan number
	BYTE byOffsetPlanNo;		// Offset plan number
};

struct T_PhaseData_Special
{
	bool bY1;		// Y+
	bool bY2;		// Y-
	bool bZ1;		// Z+
	bool bZ2;		// Z-
	BYTE byCtrlMode;	// controller mode
};

struct T_PhaseData_Locks
{
	bool bSplit;	// 
	bool bOffset;	// 
	bool bSysFlag;	// 
};

struct T_PhaseData_SubSysflag
{
	BYTE byCongIndex;		// 1. If value is 14, site is in an unused subsystem.
							// 2. Otherwise, if bit 3 is set, site is on fallback.
							// 3. Otherwise, value is congestion index.
	bool bMarriage;
};

struct T_PhaseData
{
	unsigned short siteid;			// 站点Id
	T_PhaseData_Flag flags;			// Bit 0: 是否含站点状态数据
									// Bit 1: 是否含plan number, special facilities, Locks等数据
									// Bit 2: 是否含灯态数据
									// Bit 3: Flags content included
									// Bit 4: Signal group data and phase demands included
									// Bit 5: Subsystem no, subsystem flags, nominal cycle length and required cycle length included
									// Bits 6–7: Not used
	T_PhaseData_SiteStatus siteStatus;		// 站点状态数据
	unsigned int alarmFlags;
	unsigned int detectorFlags;				// 未明确何种情况附带此数据
	T_PhaseData_PlanNo planNo;				// plan number
	T_PhaseData_Special specialFac;			// special facilities
	T_PhaseData_Locks locks;				// locks
	T_PhaseData_Data data;					// 灯态数据
	unsigned short time;
	unsigned short status;
	BYTE flagsContent;
	unsigned short XSF_flags;
	unsigned short MSS_flags;
	unsigned short RSF_flags;
	BYTE zFlags;
	unsigned int signalGroupData;
	BYTE phaseDemands;
	BYTE bySubsystemNo;
	T_PhaseData_SubSysflag stSubSysflag;			// 
	unsigned short nominalCycle;
	unsigned short requireCycle;
};	// 41字节

struct T_LaneStatusResponse
{
	int dataType;
	unsigned short second;
	// 1
	BYTE unknownSiteCount;
	unsigned short unknownSiteIds[250];
	BYTE inaccessSiteCount;
	unsigned short inaccessSiteIds[250];
	// 2
	int phaseCount;
	T_PhaseChange phaseInfo[250];
	// 3
	int dataCount;
	T_PhaseData phaseData[250];
};

/* ************************************************************************************************ */

struct T_RegisterService
{
	BYTE dataId1;	// 0,不需要; 151-250,要注册的新消息类型
	BYTE dataId2;	// as above
	BYTE dataId3;	// as above
	BYTE dataId4;	// as above
};
/* ************************************************************************************************ */

struct T_Intervene
{
	BYTE mode;
	BYTE userId;
	char regionname[8];
	BYTE subsysNo;
	unsigned short siteId;

	BYTE cFlag;
	unsigned short cycleLen;
	unsigned short cLocktime;

	BYTE sFlag;
	BYTE subsystemPlan;
	unsigned short sLocktime;

	BYTE lFlag;
	BYTE linkPlan;
	unsigned short lLocktime;

	BYTE mdFlag;		// marriage/divorce
	BYTE mdSelection;
	unsigned short mdLocktime;
};
/* ************************************************************************************************ */

struct T_Controller
{
	BYTE type;	// 区分 1,2
	unsigned short siteId;
	// 1
	BYTE page;
	BYTE offset;
	BYTE value;
	// 2
	BYTE version;
	BYTE controllerType;
	BYTE releaseNo;
};

struct T_ControllerExtend
{
	unsigned short siteId;
	bool lastMsgFlag;
	bool errorFlag;
	BYTE controlId;
	bool codeFlag;					// 0,binary;1,ascii
	BYTE version;
	BYTE type;
	BYTE releaseNo;
	BYTE serialNo;
	BYTE serialCodeFlag;			// 0,binary; 1,ascii
	unsigned short serialNumber;	// not use
	BYTE versionNo;
	BYTE versionCodeFlag;			// 0,binary; 1,ascii 
	unsigned short versionNumber;	// not use
};
/* ************************************************************************************************ */

struct T_RedlightTime_RampData
{
	BYTE rampNo;
	unsigned int siteId;
	unsigned short redtime;
};

struct T_RedlightTime_Request
{
	int count;
	T_RedlightTime_RampData ramp[250];
};

/* ************************************************************************************************ */

struct T_DetectorData_Station_Detector
{
	unsigned short occupancy;
	BYTE count;		// ?什么作用
};

struct T_DetectorData_Station
{
	unsigned long stationid;
	BYTE category;
	BYTE rampNumber;
	unsigned short siteId;
	BYTE detectorCount;	// 1-15,
	T_DetectorData_Station_Detector detector[16];
};

struct T_DetectorData
{
	BYTE subtype;
	// 1
	int stationCount;
	T_DetectorData_Station station[250];
};
/* ************************************************************************************************ */

struct T_BlockData_Congestion
{
	unsigned short id;
	unsigned long detectors;	// 0-23bit 代表1-24个检测器状态
};

struct T_BlockData_CongestionIndex
{
	unsigned short id;
	BYTE index;
};

struct T_BlockData
{
	BYTE dataType;		// 1,2,3
	// 1
	int noCongestionCount;
	unsigned short siteId[250];
	int congestionCount;
	T_BlockData_Congestion detects[250];
	// 2
	int indexCount;
	T_BlockData_CongestionIndex indexs[250];
	// 3
	int siteCount;
	//unsigned short siteId[250];	// 共用1的id;
};
/* ************************************************************************************************ */

struct T_DetectorCount_Site_Detect
{
	BYTE id;
	unsigned short data;	// 65534, no data; 65535, detector alarm
};

struct T_DetectorCount_Site
{
	unsigned short siteId;
	BYTE detectCount;
	T_DetectorCount_Site_Detect detector[24];
};

struct T_DetectorCountResponse
{
	BYTE period;
	T_Datetime time;
	int count;
	T_DetectorCount_Site siteData[250];
};
/* ************************************************************************************************ */

