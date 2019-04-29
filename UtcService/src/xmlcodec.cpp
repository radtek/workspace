// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  xmlCodec.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月16日 10时32分46秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "xmlcodec.h"

list<T_Message*> g_listWaitSendMsg;
pthread_mutex_t g_mutexWaitSendMsg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_condWaitSendMsg = PTHREAD_COND_INITIALIZER;

map<unsigned int, list<T_Message*> > g_mapRecvMessage;
map<unsigned int, pthread_mutex_t> g_mapRecvMessageLock;

map<unsigned int, string> g_mapBaseinfo;

list<string> g_listWaitDecodeXml;
pthread_mutex_t g_mutexWaitDecodeXml = PTHREAD_MUTEX_INITIALIZER;

list<string> g_listWaitSendXml;
pthread_mutex_t g_mutexWaitSendXml = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_condWaitSendXml = PTHREAD_COND_INITIALIZER;

vector<T_CcuInfo*> g_vecCcuInfo;
map<unsigned int, T_LcInfo*> g_mapCrossInfo;

// 组XML消息
bool XmlCodec::DecodeXmlMessage(string xmlStr)
{
	XMLDocument doc;
	doc.Parse(xmlStr.c_str());
	XMLElement *rootElement = doc.RootElement();

	if(doc.ErrorID() == XML_SUCCESS)
	{
		XMLElement *msgType = rootElement->FirstChildElement("messagetype");
		string strMsgType;
		if(msgType != NULL)
		{
			strMsgType = msgType->GetText();
		}
		else
		{
			return false;
		}

		if(strMsgType == "CONTROL")
		{
			XMLElement *controlNode = rootElement->FirstChildElement("control");
			string type = controlNode->FirstChildElement("type")->GetText();					// 控制类型，STARTVIP, STOPVIP
			if(type == "STARTVIP")
			{
				XMLElement *crossNode = controlNode->FirstChildElement("vip")->FirstChildElement("cross");
				string strCrossId = crossNode->FirstChildElement("lcid")->GetText();				// 路口ID
				string strDirectionIn = crossNode->FirstChildElement("directionin")->GetText();		// 入口方向
				string strDirectionOut = crossNode->FirstChildElement("directionout")->GetText();	// 出口方向
				
				// MTC2000 锁定的是车头方向，需要转换入口方向为相反方向
				if(strDirectionIn == "north")
				{
					strDirectionIn = "south";
				}
				else if(strDirectionIn == "east")
				{
					strDirectionIn = "west";
				}
				else if(strDirectionIn == "south")
				{
					strDirectionIn = "north";
				}
				else if(strDirectionIn == "west")
				{
					strDirectionIn = "east";
				}

				g_logs->WriteLog("执行特勤，路口锁定方向：入口[%s], 出口[%s]", strDirectionIn.c_str(), strDirectionOut.c_str());

				T_Message *msg = new T_Message;
				memset(msg, 0, sizeof(T_Message));
				msg->crossId = atoi(strCrossId.c_str());
				*((unsigned int*)(msg->buffer + 2)) = msg->crossId;
				msg->buffer[7] = 0x81;
				msg->buffer[8] = 0xCC;
				msg->buffer[9] = 0x00;
				msg->buffer[10] = 0x01;

				if(g_mapCrossInfo[msg->crossId]->type == 9)
				{
					// 枣庄，金通信号机
					msg->buflen = 13;
					*(unsigned short*)msg->buffer = msg->buflen;
					msg->buffer[11] = 0x02;

					for(int i = 0; i < 2; i++)
					{
						msg->buffer[i*4 + 12] = (i + 1) & 0xFF;		// 相位状态组行号
						msg->buffer[i*4 + 12 + 1] = 0xFF;			// 红,全部初始化为红灯
						msg->buffer[i*4 + 12 + 2] = 0x00;			// 黄
						msg->buffer[i*4 + 12 + 3] = 0x00;			// 绿
					}

					if(strDirectionIn == "north")
					{
						if(strDirectionOut == "east")
						{
							// 北进东出，锁左转
							// 00000000 00000000 00000000 00001001
							msg->buffer[13] = 0xF6;
							msg->buffer[15] = 0x09;
						}
						else if(strDirectionOut == "south")
						{
							// 北进南出，锁右直行
							// 00000000 00000000 00110000 00110110
							msg->buffer[13] = 0xC9;
							msg->buffer[15] = 0x36;

							msg->buffer[17] = 0xCF;
							msg->buffer[19] = 0x30;
						}
						else
						{
							// 其余默认为锁单口，北进西出
							// 00000000 00000000 00010000 00000111
							msg->buffer[13] = 0xF8;
							msg->buffer[15] = 0x07;

							msg->buffer[17] = 0xEF;
							msg->buffer[19] = 0x10;
						}
					}
					else if(strDirectionIn == "east")
					{
						if(strDirectionOut == "south")
						{
							// 东进南出，左转
							// 00000000 00000000 00000010 01000000
							msg->buffer[13] = 0xBF;
							msg->buffer[15] = 0x40;
							
							msg->buffer[17] = 0xFD;
							msg->buffer[19] = 0x02;
						}
						else if(strDirectionOut == "west")
						{
							// 东进西出，右直
							// 00000000 00000000 11001101 10000000
							msg->buffer[13] = 0x7F;
							msg->buffer[15] = 0x80;
							
							msg->buffer[17] = 0x32;
							msg->buffer[19] = 0xCD;
						}
						else
						{
							// 默认为锁单口
							// 00000000 00000000 10001110 00000000
							msg->buffer[17] = 0x71;
							msg->buffer[19] = 0x8E;
						}
					}
					else if(strDirectionIn == "south")
					{
						if(strDirectionOut == "west")
						{
							// 南进西出，左转
							// 00000000 00000000 00000000 00001001
							msg->buffer[13] = 0xF6;
							msg->buffer[15] = 0x09;
						}
						else if(strDirectionOut == "north")
						{
							// 南进北出，右直
							// 00000000 00000000 00110000 00110110
							msg->buffer[13] = 0xC9;
							msg->buffer[15] = 0x36;

							msg->buffer[17] = 0xCF;
							msg->buffer[19] = 0x30;
						}
						else
						{
							// 锁单口
							// 00000000 00000000 00100000 00111000
							msg->buffer[13] = 0xC7;
							msg->buffer[15] = 0x38;

							msg->buffer[17] = 0xDF;
							msg->buffer[19] = 0x20;
						}
					}
					else if(strDirectionIn == "west")
					{
						if(strDirectionOut == "north")
						{
							// 西进北出，左转
							// 00000000 00000000 00000010 01000000
							msg->buffer[13] = 0xBF;
							msg->buffer[15] = 0x40;
							
							msg->buffer[17] = 0xFD;
							msg->buffer[19] = 0x02;
						}
						else if(strDirectionOut == "east")
						{
							// 西进东出，右直
							// 00000000 00000000 11001101 10000000
							msg->buffer[13] = 0x7F;
							msg->buffer[15] = 0x80;
							
							msg->buffer[17] = 0x32;
							msg->buffer[19] = 0xCD;
						}
						else
						{
							// 锁单口
							// 00000000 00000000 01000001 11000000
							msg->buffer[13] = 0x3F;
							msg->buffer[15] = 0xC0;

							msg->buffer[17] = 0xBE;
							msg->buffer[19] = 0x41;
						}
					}
					else
					{
						if(msg != NULL)
						{
							delete msg;
							msg = NULL;
						}
					}

					if(msg != NULL)
					{
						pthread_mutex_lock(&g_mutexWaitSendMsg);
						g_listWaitSendMsg.push_back(msg);
						pthread_mutex_unlock(&g_mutexWaitSendMsg);
						pthread_cond_signal(&g_condWaitSendMsg);
					}
				}
				else
				{
					// 枣庄，双柏信号机相位顺序
					msg->buflen = 21;
					*(unsigned short*)msg->buffer = msg->buflen;
					msg->buffer[11] = 0x04;

					for(int i = 0; i < 4; i++)
					{
						msg->buffer[i*4 + 12] = (i + 1) & 0xFF;		// 相位状态组行号
						msg->buffer[i*4 + 12 + 1] = 0xFF;			// 全部初始化为红灯
						msg->buffer[i*4 + 12 + 2] = 0x00;
						msg->buffer[i*4 + 12 + 3] = 0x00;
					}

					if(strDirectionIn == "north")
					{
						if(strDirectionOut == "east")
						{
							// 北进东出，左转
							// 00000001 00000001 00000000 00000000
							msg->buffer[21] = 0xFE;
							msg->buffer[23] = 0x01;
							msg->buffer[25] = 0xFE;
							msg->buffer[27] = 0x01;
						}
						else if(strDirectionOut == "south")
						{
							// 北进南出，右直行
							// 01100110 01100110 00000000 00000000
							msg->buffer[21] = 0x99;
							msg->buffer[23] = 0x66;
							msg->buffer[25] = 0x99;
							msg->buffer[27] = 0x66;
						}
						else
						{
							// 默认为锁单口
							// 01101111 00000000 00000000 00000000
							msg->buffer[25] = 0x90;
							msg->buffer[26] = 0x00;
							msg->buffer[27] = 0x6F;
						}
					}
					else if(strDirectionIn == "east")
					{
						if(strDirectionOut == "south")
						{
							// 东进南出，左
							// 00000000 00000000 00000001 00000001
							msg->buffer[13] = 0xFE;
							msg->buffer[15] = 0x01;
							msg->buffer[17] = 0xFE;
							msg->buffer[19] = 0x01;
						}
						else if(strDirectionOut == "west")
						{
							// 东进西出，右直行
							// 00000000 00000000 01100110 01100110
							msg->buffer[13] = 0x99;
							msg->buffer[15] = 0x66;
							msg->buffer[17] = 0x99;
							msg->buffer[19] = 0x66;
						}
						else
						{
							// 默认为锁单口
							// 00000000 00000000 00000000 01101111
							msg->buffer[13] = 0x90;
							msg->buffer[14] = 0x00;
							msg->buffer[15] = 0x6F;
						}
					}
					else if(strDirectionIn == "south")
					{
						if(strDirectionOut == "west")
						{
							// 南进西出，左
							// 00000001 00000001 00000000 00000000
							msg->buffer[21] = 0xFE;
							msg->buffer[23] = 0x01;
							msg->buffer[25] = 0xFE;
							msg->buffer[27] = 0x01;
						}
						else if(strDirectionOut == "north")
						{
							// 南进北出，右直行
							// 01100110 01100110 00000000 00000000
							msg->buffer[21] = 0x99;
							msg->buffer[23] = 0x66;
							msg->buffer[25] = 0x99;
							msg->buffer[27] = 0x66;
						}
						else
						{
							// 默认为锁单口
							// 00000000 01101111 00000000 00000000
							msg->buffer[21] = 0x90;
							msg->buffer[22] = 0x00;
							msg->buffer[23] = 0x6F;
						}
					}
					else if(strDirectionIn == "west")
					{
						if(strDirectionOut == "north")
						{
							// 西进北出，左
							// 00000000 00000000 00000001 00000001
							msg->buffer[13] = 0xFE;
							msg->buffer[15] = 0x01;
							msg->buffer[17] = 0xFE;
							msg->buffer[19] = 0x01;
						}
						else if(strDirectionOut == "east")
						{
							// 西进东出，右直行
							// 00000000 00000000 01100110 01100110
							msg->buffer[13] = 0x99;
							msg->buffer[15] = 0x66;
							msg->buffer[17] = 0x99;
							msg->buffer[19] = 0x66;
						}
						else
						{
							// 默认为锁单口
							// 00000000 00000000 01101111 00000000
							msg->buffer[17] = 0x90;
							msg->buffer[18] = 0x00;
							msg->buffer[19] = 0x6F;
						}
					}
					else
					{
						if(msg != NULL)
						{
							delete msg;
							msg = NULL;
						}
					}

					if(msg != NULL)
					{
						pthread_mutex_lock(&g_mutexWaitSendMsg);
						g_listWaitSendMsg.push_back(msg);
						pthread_mutex_unlock(&g_mutexWaitSendMsg);
						pthread_cond_signal(&g_condWaitSendMsg);
					}
				}
			}
			else if(type == "STOPVIP")
			{
				XMLElement *crossNode = controlNode->FirstChildElement("vip")->FirstChildElement("cross");
				string strCrossId = crossNode->FirstChildElement("lcid")->GetText();		// 路口ID
				T_Message *msg = new T_Message;
				memset(msg, 0, sizeof(T_Message));
				msg->crossId = atoi(strCrossId.c_str());
				msg->buflen = 4;
				*(unsigned short*)msg->buffer = msg->buflen;
				*((unsigned int*)(msg->buffer + 2)) = msg->crossId;
				msg->buffer[7] = 0x81;
				msg->buffer[8] = 0xCC;
				msg->buffer[9] = 0x00;
				msg->buffer[10] = 0x00;

				pthread_mutex_lock(&g_mutexWaitSendMsg);
				g_listWaitSendMsg.push_back(msg);
				pthread_mutex_unlock(&g_mutexWaitSendMsg);
				pthread_cond_signal(&g_condWaitSendMsg);
			}
			else if(type == "ANALOGMANUALSTART" || type == "ANALOGMANUALNEXT" || type == "ANALOGMANUALEND")
			{
				// 中心手动，步进
				string strCrossId = controlNode->FirstChildElement("lcid")->GetText();
				T_Message *msg = new T_Message;
				memset(msg, 0, sizeof(T_Message));
				msg->crossId = atoi(strCrossId.c_str());
				msg->buflen = 6;
				*(unsigned short*)msg->buffer = msg->buflen;
				*((unsigned int*)(msg->buffer + 2)) = msg->crossId;
				if(type == "ANALOGMANUALEND")
				{
					msg->buffer[7] = 0x81;
					msg->buffer[8] = 0xDC;
					msg->buffer[9] = 0x00;
					msg->buffer[10] = 0x00;
					msg->buffer[11] = 0x00;
					msg->buffer[12] = 0x00;
				}
				else
				{
					msg->buffer[7] = 0x81;
					msg->buffer[8] = 0xDC;
					msg->buffer[9] = 0x00;
					msg->buffer[10] = 0x01;
					msg->buffer[11] = 0x02;
					msg->buffer[12] = 0x00;
				}

				pthread_mutex_lock(&g_mutexWaitSendMsg);
				g_listWaitSendMsg.push_back(msg);
				pthread_mutex_unlock(&g_mutexWaitSendMsg);
				pthread_cond_signal(&g_condWaitSendMsg);
			}
		}
		else if(strMsgType == "REQUEST")
		{
			XMLElement *requestNode = rootElement->FirstChildElement("request");
			string type = requestNode->FirstChildElement("type")->GetText();					// 请求类型，BASSINFO
			string crossId = requestNode->FirstChildElement("id")->GetText();

			T_Message *msg = new T_Message;
			memset(msg, 0, sizeof(T_Message));
			msg->crossId = atoi(crossId.c_str());
			*((unsigned int*)(msg->buffer + 2)) = msg->crossId;
			msg->buffer[6] = 0x06;

			string sourceIP = rootElement->FirstChildElement("sourceIP")->GetText();
			g_mapBaseinfo[msg->crossId] = sourceIP;
			
			if(type == "BASEINFO")
			{
				// BASEINFO需要用自定义协议,  91 01 00 01
				msg->buflen = 4;
				*(unsigned short*)msg->buffer = msg->buflen;
				msg->buffer[7] = 0x91;
				msg->buffer[8] = 0x01;
				msg->buffer[9] = 0x00;
				msg->buffer[10] = 0x01;

				pthread_mutex_lock(&g_mutexWaitSendMsg);
				g_listWaitSendMsg.push_back(msg);
				pthread_mutex_unlock(&g_mutexWaitSendMsg);
				pthread_cond_signal(&g_condWaitSendMsg);
			}
			else if(type == "SIGNALSTART")
			{
				// 91 DD 00 01  订阅路口灯态

				msg->buflen = 4;
				*(unsigned short*)msg->buffer = msg->buflen;
				msg->buffer[7] = 0x91;
				msg->buffer[8] = 0xDD;
				msg->buffer[9] = 0x00;
				msg->buffer[10] = 0x01;

				pthread_mutex_lock(&g_mutexWaitSendMsg);
				g_listWaitSendMsg.push_back(msg);
				pthread_mutex_unlock(&g_mutexWaitSendMsg);
				pthread_cond_signal(&g_condWaitSendMsg);
				g_logs->WriteLog("订阅路口灯态信息: %d", msg->crossId);
			}
			else if(type == "SIGNALEND")
			{
				// 91 DD 00 00  取消订阅
				//
				msg->buflen = 4;
				*(unsigned short*)msg->buffer = msg->buflen;
				msg->buffer[7] = 0x91;
				msg->buffer[8] = 0xDD;
				msg->buffer[9] = 0x00;
				msg->buffer[10] = 0x00;

				pthread_mutex_lock(&g_mutexWaitSendMsg);
				g_listWaitSendMsg.push_back(msg);
				pthread_mutex_unlock(&g_mutexWaitSendMsg);
				pthread_cond_signal(&g_condWaitSendMsg);
				g_logs->WriteLog("取消订阅路口灯态: %d", msg->crossId);
			}
			else
			{
				if(msg != NULL)
				{
					delete msg;
					msg = NULL;
				}
			}
		}
	}
	return true;
}

// 解析报文到XML
bool XmlCodec::EncodeXmlMessage(T_Message *msg)
{
	char declaration[] = "<?xml version=\"1.0\" encoding=\"GB2312\"?>";
	tinyxml2::XMLDocument doc;
	doc.Parse(declaration);

	XMLElement* root = doc.NewElement("message");
	root->SetAttribute("System", "ATMS");
	root->SetAttribute("Ver", "1.0");
	doc.InsertEndChild(root);

	XMLElement* secondElement = doc.NewElement("systemtype");
	secondElement->SetText("UTC");
	root->InsertEndChild(secondElement);

	if(((unsigned char)msg->buffer[0] == 0x83 || (unsigned char)msg->buffer[0] == 0x93) && (unsigned char)msg->buffer[1] == 0x96)
	{
		// 灯态数据
		XMLElement* second1Element = doc.NewElement("messagetype");
		second1Element->SetText("NOTICE");
		root->InsertEndChild(second1Element);

		string targetIP = "LC2099_" + ToString(msg->crossId);
		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		XMLElement* second3Element = doc.NewElement("targetIP");
		second3Element->SetText(targetIP.c_str());
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);
		XMLElement* second6Element = doc.NewElement("notice");
		root->InsertEndChild(second6Element);

		XMLElement* third1Element = doc.NewElement("type");
		third1Element->SetText("SIGNALSTATUS");
		second6Element->InsertEndChild(third1Element);
		XMLElement* third2Element = doc.NewElement("signalstatus");
		second6Element->InsertEndChild(third2Element);

		XMLElement* forth1Element = doc.NewElement("lcid");
		forth1Element->SetText(msg->crossId);
		third2Element->InsertEndChild(forth1Element);
		XMLElement* forth2Element = doc.NewElement("controlmodel");
		if((unsigned char)msg->buffer[0] == 0x83)
		{
			forth2Element->SetAttribute("check","true");
			char inbuf[] = "本地时间表";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth2Element->SetText(outbuf);
		}
		else
		{
			forth2Element->SetAttribute("check","false");
			char inbuf[] = "特勤控制开始";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth2Element->SetText(outbuf);
		}
		third2Element->InsertEndChild(forth2Element);
		XMLElement* forth3Element = doc.NewElement("cycle");
		forth3Element->SetText("0");
		third2Element->InsertEndChild(forth3Element);
		XMLElement* forth4Element = doc.NewElement("offset");
		forth4Element->SetText("0");
		third2Element->InsertEndChild(forth4Element);
		XMLElement* forth5Element = doc.NewElement("step");
		forth5Element->SetText("0");
		third2Element->InsertEndChild(forth5Element);
		XMLElement* forth6Element = doc.NewElement("steplength");
		forth6Element->SetText("0");
		third2Element->InsertEndChild(forth6Element);
		XMLElement* forth7Element = doc.NewElement("starttime");
		third2Element->InsertEndChild(forth7Element);
		XMLElement* forth8Element = doc.NewElement("curphase");
		third2Element->InsertEndChild(forth8Element);
		XMLElement* forth9Element = doc.NewElement("islock");
		forth9Element->SetText("F");
		third2Element->InsertEndChild(forth9Element);
		XMLElement* forth10Element = doc.NewElement("phasetime");
		third2Element->InsertEndChild(forth10Element);
		XMLElement* forth11Element = doc.NewElement("nextphase");
		third2Element->InsertEndChild(forth11Element);

		// 0 灭, 1 红,2 黄,3 绿,4 异常
		int n = msg->buffer[3] & 0xFF;
		int szLight[32] = { 0 };
		int red = 0, ylw = 0,grn = 0;

		if(g_mapCrossInfo[msg->crossId]->type == 9)
		{
			for(int i = 0; i < n; i++)
			{
				for(int j = 0; j < 8; j++)
				{
					red = (msg->buffer[4 + i * 4 + 1] >> j) & 0x01;
					ylw = (msg->buffer[4 + i * 4 + 2] >> j) & 0x01;
					grn = (msg->buffer[4 + i * 4 + 3] >> j) & 0x01;
					if(red + ylw + grn > 1)
					{
						szLight[Shape_Table_JT[i * 8 + j] - 1] = 4;
					}
					else
					{
						szLight[Shape_Table_JT[i * 8 + j] - 1] = red + 2 * ylw + 3 * grn;
					}
				}
			}
		}
		else
		{
			for(int i = 0; i < n; i++)
			{
				for(int j = 0; j < 8; j++)
				{
					red = (msg->buffer[4 + i * 4 + 1] >> j) & 0x01;
					ylw = (msg->buffer[4 + i * 4 + 2] >> j) & 0x01;
					grn = (msg->buffer[4 + i * 4 + 3] >> j) & 0x01;
					if(red + ylw + grn > 1)
					{
						szLight[Shape_Table_SB[i * 8 + j] - 1] = 4;
					}
					else
					{
						szLight[Shape_Table_SB[i * 8 + j] - 1] = red + 2 * ylw + 3 * grn;
					}
				}
			}
		}

		for(int i = 0; i < 12; i++)
		{
			XMLElement* vehLamp = doc.NewElement("vehlamp");
			vehLamp->SetAttribute("index", Shape_Table_XML[i]);
			switch(szLight[i])
			{
			case 0:
				vehLamp->SetText("OFF");break;
			case 1:
				vehLamp->SetText("RED");break;
			case 2:
				vehLamp->SetText("YELLOW");break;
			case 3:
				vehLamp->SetText("GREEN");break;
			case 4:
				vehLamp->SetText("ERROR");break;
			default:
				vehLamp->SetText("ERROR");
			}
			third2Element->InsertEndChild(vehLamp);
		}

		for(int i = 12; i < 16; i++)
		{
			XMLElement* pedLamp = doc.NewElement("pedlamp");
			pedLamp->SetAttribute("index", Shape_Table_XML[i]);
			switch(szLight[i])
			{
			case 0:
				pedLamp->SetText("OFF");break;
			case 1:
				pedLamp->SetText("RED");break;
			case 2:
				pedLamp->SetText("YELLOW");break;
			case 3:
				pedLamp->SetText("GREEN");break;
			case 4:
				pedLamp->SetText("ERROR");break;
			default:
				pedLamp->SetText("ERROR");
			}
			third2Element->InsertEndChild(pedLamp);
		}
	}
	else if((unsigned char)msg->buffer[0] == 0x85 && (unsigned char)msg->buffer[1] == 0xCC)
	{
		// 特勤反馈
		// 85 CC 00 01 01		原有协议基础上添加2字节
		XMLElement* second1Element = doc.NewElement("messagetype");
		second1Element->SetText("FEEDBACK");
		root->InsertEndChild(second1Element);

		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		XMLElement* second3Element = doc.NewElement("targetIP");
		char targetIP[20] = { 0 };
		time_t timer = time(NULL);
		struct tm local;
		localtime_r(&timer, &local);
		sprintf(targetIP, "%0.4d%0.2d%0.2d%0.2d%0.2d%0.2d0000", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
		second3Element->SetText(targetIP);
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);

		XMLElement* second6Element = doc.NewElement("feedback");
		root->InsertEndChild(second6Element);
		XMLElement* third1Element = doc.NewElement("type");
		if((unsigned char)msg->buffer[3] == 0x01)
		{
			third1Element->SetText("STARTVIP");
		}
		else
		{
			third1Element->SetText("STOPVIP");
		}
		second6Element->InsertEndChild(third1Element);
		XMLElement* third2Element = doc.NewElement("control");
		third2Element->SetAttribute("lcid", msg->crossId);
		if((unsigned char)msg->buffer[4] == 0x01)
		{
			third2Element->SetText("SUCCESS");
		}
		else
		{
			third2Element->SetText("FAIL");
		}
		second6Element->InsertEndChild(third2Element);
	}
	else if((unsigned char)msg->buffer[0] == 0x85 && (unsigned char)msg->buffer[1] == 0xDC)
	{
		// 中心手动反馈
		// 85 DC 00 01 01 01		原有协议基础上添加3字节,第4字节:0,中心手动,1,步进 第5字节:0,解除,1,执行 第6字节:1,成功,0,失败
		XMLElement* second1Element = doc.NewElement("messagetype");
		second1Element->SetText("FEEDBACK");
		root->InsertEndChild(second1Element);

		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		XMLElement* second3Element = doc.NewElement("targetIP");
		char targetIP[20] = { 0 };
		time_t timer = time(NULL);
		struct tm local;
		localtime_r(&timer, &local);
		sprintf(targetIP, "%0.4d%0.2d%0.2d%0.2d%0.2d%0.2d0000", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
		second3Element->SetText(targetIP);
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);

		XMLElement* second6Element = doc.NewElement("feedback");
		root->InsertEndChild(second6Element);
		XMLElement* third1Element = doc.NewElement("type");

		if((unsigned char)msg->buffer[3] == 0x00 && (unsigned char)msg->buffer[4] == 0x01)
		{
			third1Element->SetText("ANALOGMANUALSTART");
		}
		else if((unsigned char)msg->buffer[3] == 0x00 && (unsigned char)msg->buffer[4] == 0x00)
		{
			third1Element->SetText("ANALOGMANUALEND");
		}
		else if((unsigned char)msg->buffer[3] == 0x01)
		{
			third1Element->SetText("ANALOGMANUALNEXT");
		}
		second6Element->InsertEndChild(third1Element);

		XMLElement* third2Element = doc.NewElement("control");
		third2Element->SetAttribute("lcid", msg->crossId);
		if((unsigned char)msg->buffer[4] == 0x01)
		{
			third2Element->SetText("SUCCESS");
		}
		else
		{
			third2Element->SetText("FAIL");
		}
		second6Element->InsertEndChild(third2Element);
	}
	else if((unsigned char)msg->buffer[0] == 0x93 && (unsigned char)msg->buffer[1] == 0xDD)
	{
		// 灯态订阅反馈
		// 93 DD 00 01 01		第4位: 1,订阅 0,取消; 第5位: 1,成功  0,失败
		XMLElement *second1Element = doc.NewElement("messagetype");
		second1Element->SetText("FEEDBACK");
		root->InsertEndChild(second1Element);

		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		XMLElement* second3Element = doc.NewElement("targetIP");
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);
		XMLElement* second6Element = doc.NewElement("feedback");
		root->InsertEndChild(second6Element);

		XMLElement* third1Element = doc.NewElement("type");
		if((unsigned char)msg->buffer[3] == 0x01)
		{
			third1Element->SetText("SIGNALSTART");
		}
		else
		{
			third1Element->SetText("SIGNALEND");
		}
		second6Element->InsertEndChild(third1Element);
		XMLElement* third2Element = doc.NewElement("result");
		second6Element->InsertEndChild(third2Element);

		XMLElement *forth1Element = doc.NewElement("id");
		forth1Element->SetText(msg->crossId);
		third2Element->InsertEndChild(forth1Element);
		XMLElement *forth2Element = doc.NewElement("status");
		if((unsigned char)msg->buffer[4] == 0x01)
		{
			forth2Element->SetText("READY");
		}
		else
		{
			forth2Element->SetText("FAIL");
		}
		third2Element->InsertEndChild(forth2Element);
	}
	else if((unsigned char)msg->buffer[0] == 0x93 && (unsigned char)msg->buffer[1] == 0x01)
	{
		// 查询BASEINFO的应答消息
		// 1:0x93,0x94 4: BASEINFO 5,6,7,8,9,10,11,12: 8字节时间,time(NULL); 13: 01,联机 00,脱机 14: 00,通讯中断 01,通讯正常  15: 00,多时段 01,特勤
		// 93 01 00 01 xx xx xx xx xx xx xx xx 01 01 01		查询回报

		XMLElement *second1Element = doc.NewElement("messagetype");
		second1Element->SetText("FEEDBACK");
		root->InsertEndChild(second1Element);
		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		XMLElement* second3Element = doc.NewElement("targetIP");
		if(g_mapBaseinfo.find(msg->crossId) != g_mapBaseinfo.end())
		{
			second3Element->SetText(g_mapBaseinfo[msg->crossId].c_str());
		}
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);
		XMLElement* second6Element;
		second6Element = doc.NewElement("feedback");
		root->InsertEndChild(second6Element);

		XMLElement* third1Node = doc.NewElement("type");
		if((unsigned char)msg->buffer[3] == 0x01)
		{
			third1Node->SetText("BASEINFO");
		}
		second6Element->InsertEndChild(third1Node);
		XMLElement* third2Node = doc.NewElement("baseinfo");
		second6Element->InsertEndChild(third2Node);

		XMLElement* forth1Node = doc.NewElement("lcid");
		forth1Node->SetText(msg->crossId);
		third2Node->InsertEndChild(forth1Node);
		XMLElement* forth6Node = doc.NewElement("name");
		third2Node->InsertEndChild(forth6Node);
		XMLElement* forth2Node = doc.NewElement("location");
		third2Node->InsertEndChild(forth2Node);
		XMLElement* forth7Node = doc.NewElement("lctype");
		forth7Node->SetText("LC2099");
		third2Node->InsertEndChild(forth7Node);
		XMLElement* forth8Node = doc.NewElement("installdate");
		third2Node->InsertEndChild(forth8Node);
		XMLElement* forth9Node = doc.NewElement("crosstype");
		third2Node->InsertEndChild(forth9Node);
		XMLElement* forth3Node = doc.NewElement("commstatus");
		if((unsigned char)msg->buffer[13] == 0x00)
		{
			char inbuf[] = "通讯中断";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth3Node->SetText(outbuf);
		}
		else if((unsigned char)msg->buffer[13] == 0x01)
		{
			char inbuf[] = "通讯正常";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth3Node->SetText(outbuf);
		}
		third2Node->InsertEndChild(forth3Node);
		XMLElement* forth4Node = doc.NewElement("controlmode");
		if((unsigned char)msg->buffer[12] == 0x00)
		{
			char inbuf[] = "脱机";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth4Node->SetText(outbuf);
		}
		else if((unsigned char)msg->buffer[12] == 0x01)
		{
			char inbuf[] = "联机";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth4Node->SetText(outbuf);
		}
		third2Node->InsertEndChild(forth4Node);
		XMLElement* forth5Node = doc.NewElement("operatemode");
		if((unsigned char)msg->buffer[14] == 0x00)
		{
			char inbuf[] = "多时段";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth5Node->SetText(outbuf);
		}
		else if((unsigned char)msg->buffer[14] == 0x01)
		{
			char inbuf[] = "特勤控制";
			char outbuf[20] = { 0 };
			Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
			forth5Node->SetText(outbuf);
		}
		third2Node->InsertEndChild(forth5Node);
	}
	else if((unsigned char)msg->buffer[0] == 0x94 && (unsigned char)msg->buffer[1] == 0x01)
	{
		// 主动上报控制模式
		// 94 01 00 01 xx xx xx xx xx xx xx xx 00	主动上报
		//
		// 1: 0x94 
		// 4: CONTROLMODEL 
		// 5,6,7,8,9,10,11,12: 8字节时间,time(NULL); 
		// 13: 0x00,本地时间表、0x01,特勤开始、0x02,特勤结束、0x03,中心手动开始、0x04,中心手动结束

		XMLElement *second1Element = doc.NewElement("messagetype");
		second1Element->SetText("NOTICE");
		root->InsertEndChild(second1Element);
		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		string targetIP = "LC2099_" + ToString(msg->crossId);
		XMLElement* second3Element = doc.NewElement("targetIP");
		second3Element->SetText(targetIP.c_str());
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);
		XMLElement* second6Element;
		second6Element = doc.NewElement("notice");
		root->InsertEndChild(second6Element);

		XMLElement* third1Node = doc.NewElement("type");
		if((unsigned char)msg->buffer[3] == 0x01)
		{
			third1Node->SetText("CONTROLMODEL");
		}
		second6Element->InsertEndChild(third1Node);
		XMLElement* third2Node = doc.NewElement("lcid");
		third2Node->SetText(msg->crossId);
		second6Element->InsertEndChild(third2Node);
		XMLElement* third3Node = doc.NewElement("controlmodel");
		if((unsigned char)msg->buffer[12] == 0x00)
		{
			third3Node->SetAttribute("check", "true");
			third3Node->SetText("本地时间表");
		}
		else if((unsigned char)msg->buffer[13] == 0x01)
		{
			third3Node->SetAttribute("check", "false");
			third3Node->SetText("特勤控制开始");
		}
		else if((unsigned char)msg->buffer[13] == 0x02)
		{
			third3Node->SetAttribute("check", "true");
			third3Node->SetText("特勤控制结束");
		}
		else if((unsigned char)msg->buffer[13] == 0x03)
		{
			third3Node->SetAttribute("check", "true");
			third3Node->SetText("中心手动开始");
		}
		else if((unsigned char)msg->buffer[13] == 0x04)
		{
			third3Node->SetAttribute("check", "true");
			third3Node->SetText("中心手动结束");
		}
		second6Element->InsertEndChild(third3Node);
	}
	else if((unsigned char)msg->buffer[0] == 0x83 && (unsigned char)msg->buffer[1] == 0x92)
	{
		// 主动上报的报警信息,15字节,前5字节固定
		// 83 92 40 01 01 
		// 事件类型添加类型7, 连接 中断，恢复

		XMLElement *second1Element = doc.NewElement("messagetype");
		second1Element->SetText("ALARM");
		root->InsertEndChild(second1Element);
		XMLElement* second2Element = doc.NewElement("sourceIP");
		root->InsertEndChild(second2Element);
		string targetIP = "LC2099_" + ToString(msg->crossId);
		XMLElement* second3Element = doc.NewElement("targetIP");
		second3Element->SetText(targetIP.c_str());
		root->InsertEndChild(second3Element);
		XMLElement* second4Element = doc.NewElement("user");
		second4Element->SetText("utcmaster");
		root->InsertEndChild(second4Element);
		XMLElement* second5Element = doc.NewElement("password");
		root->InsertEndChild(second5Element);
		XMLElement* second6Element = doc.NewElement("alarm");
		root->InsertEndChild(second6Element);

		XMLElement* third1Node = doc.NewElement("type");
		third1Node->SetText("SINGLE");
		second6Element->InsertEndChild(third1Node);

		XMLElement* third2Node = doc.NewElement("device");
		second6Element->InsertEndChild(third2Node);
		XMLElement* forth1Node = doc.NewElement("id");
		forth1Node->SetText(ToString(msg->crossId).c_str());
		third2Node->InsertEndChild(forth1Node);
		XMLElement* forth2Node = doc.NewElement("name");
		third2Node->InsertEndChild(forth2Node);
		XMLElement* forth3Node = doc.NewElement("location");
		third2Node->InsertEndChild(forth3Node);
		XMLElement* forth4Node = doc.NewElement("errortype");

		XMLElement* forth5Node = doc.NewElement("occurtime");
		char strStartTime[20] = { 0 };
		{
			time_t timer = *(unsigned int*)(msg->buffer + 7);
			struct tm local;
			localtime_r(&timer, &local);
			sprintf(strStartTime, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
		}
		forth5Node->SetText(strStartTime);

		char strTime[20] = { 0 };
		time_t timer = time(NULL);
		struct tm local;
		localtime_r(&timer, &local);
		sprintf(strTime, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
		XMLElement* forth6Node = doc.NewElement("releasetime");
		XMLElement* forth7Node = doc.NewElement("desc");
		if((unsigned char)msg->buffer[5] == 0x01)
		{
			forth4Node->SetText("signalheaderErr");
			forth7Node->SetText("signalheaderErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
			else
			{//故障开始，releasetime为空
				forth6Node->SetText("");
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x02)
		{
			forth4Node->SetText("planErr");
			forth7Node->SetText("planErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x03)
		{
			forth4Node->SetText("signalheaderErr");
			forth7Node->SetText("signalheaderErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x04)
		{
			forth4Node->SetText("dooropen");
			forth7Node->SetText("dooropen");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x05)
		{
			forth4Node->SetText("yellowflashErr");
			forth7Node->SetText("yellowflashErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x06)
		{
			forth4Node->SetText("sysErr");
			forth7Node->SetText("sysErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		else if((unsigned char)msg->buffer[5] == 0x07)
		{
			forth4Node->SetText("commErr");
			forth7Node->SetText("commErr");
			if((unsigned char)msg->buffer[12]  == 0x00)
			{// 此字节为0认为是故障恢复				
				forth6Node->SetText(strTime);
			}
		}
		third2Node->InsertEndChild(forth4Node);
		third2Node->InsertEndChild(forth5Node);
		third2Node->InsertEndChild(forth6Node);
		third2Node->InsertEndChild(forth7Node);
	}

	XMLPrinter printer;
	doc.Print(&printer);
	string str = printer.CStr();

	pthread_mutex_lock(&g_mutexWaitSendXml);
	g_listWaitSendXml.push_back(str);
	pthread_mutex_unlock(&g_mutexWaitSendXml);
	pthread_cond_signal(&g_condWaitSendXml);

/*
	if(((unsigned char)msg->buffer[0] == 0x93 || (unsigned char)msg->buffer[0] == 0x94)&& (unsigned char)msg->buffer[1] == 0x01)
	{
			// 用此XML消息暂时代替BASEINFO功能
			char declaration[] = "<?xml version=\"1.0\" encoding=\"GB2312\"?>";
			tinyxml2::XMLDocument doc;
			doc.Parse(declaration);

			XMLElement* root = doc.NewElement("message");
			root->SetAttribute("System", "ATMS");
			root->SetAttribute("Ver", "1.0");
			doc.InsertEndChild(root);

			XMLElement* secondElement = doc.NewElement("systemtype");
			secondElement->SetText("UTC");
			root->InsertEndChild(secondElement);

			XMLElement* second1Element = doc.NewElement("messagetype");
			second1Element->SetText("NOTICE");
			root->InsertEndChild(second1Element);

			XMLElement* second2Element = doc.NewElement("sourceIP");
			root->InsertEndChild(second2Element);
			string targetIP = "LC2099_" + ToString(msg->crossId);
			XMLElement* second3Element = doc.NewElement("targetIP");
			second3Element->SetText(targetIP.c_str());
			root->InsertEndChild(second3Element);
			XMLElement* second4Element = doc.NewElement("user");
			second4Element->SetText("utc_ccu");
			root->InsertEndChild(second4Element);
			XMLElement* second5Element = doc.NewElement("password");
			root->InsertEndChild(second5Element);

			XMLElement* second6Element = doc.NewElement("notice");
			root->InsertEndChild(second6Element);
			XMLElement* third1Element = doc.NewElement("type");
			third1Element->SetText("CONTROLMODEL");
			second6Element->InsertEndChild(third1Element);
			XMLElement* third2Element = doc.NewElement("lcid");
			third2Element->SetText(msg->crossId);
			second6Element->InsertEndChild(third2Element);
			XMLElement* third3Element = doc.NewElement("controlmodel");
			if((unsigned char)msg->buffer[14] == 0x01)
			{
				third3Element->SetAttribute("check", "false");
				char inbuf[] = "特勤控制开始";
				char outbuf[20] = { 0 };
				Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
				third3Element->SetText(outbuf);
			}
			else
			{
				third3Element->SetAttribute("check", "true");
				char inbuf[] = "本地时间表";
				char outbuf[20] = { 0 };
				Utf8ToGb2312(inbuf, strlen(inbuf), outbuf, 20);
				third3Element->SetText(outbuf);
			}
			second6Element->InsertEndChild(third3Element);

			XMLPrinter printer;
			doc.Print(&printer);
			string str = printer.CStr();

			pthread_mutex_lock(&g_mutexWaitSendXml);
			g_listWaitSendXml.push_back(str);
			pthread_mutex_unlock(&g_mutexWaitSendXml);
	}
*/
	return true;
}

int XmlCodec::SubscribeMessage(char *buffer, int type = 1)
{
	if(buffer == NULL)
	{
		return false;
	}

	char declaration[] = "<?xml version=\"1.0\" encoding=\"GB2312\"?>";
	tinyxml2::XMLDocument doc;
	doc.Parse(declaration);

	XMLElement *root = doc.NewElement("subscription");
	root->SetAttribute("System", "ATMS");
	root->SetAttribute("Ver", "1.0");
	doc.InsertEndChild(root);

	XMLElement* second_1_Element = doc.NewElement("systemtype");
	second_1_Element->SetText("UTC");
	root->InsertEndChild(second_1_Element);

	XMLElement* second_2_Element = doc.NewElement("messagetype");
	switch(type)
	{
		case 1:
			second_2_Element->SetText("CONTROL");break;
		case 2:
			second_2_Element->SetText("REQUEST");break;
		case 5:
			second_2_Element->SetText("NOTICE");break;
		default:
			second_2_Element->SetText("FEEDBACK");
	}
	root->InsertEndChild(second_2_Element);

	XMLElement* second_3_Element = doc.NewElement("sourceIP");
	root->InsertEndChild(second_3_Element);

	XMLElement* second_4_Element = doc.NewElement("targetIP");
	second_4_Element->SetText("LC2099");
	root->InsertEndChild(second_4_Element);

	XMLElement* second_5_Element = doc.NewElement("user");
	second_5_Element->SetText("utcmaster");
	root->InsertEndChild(second_5_Element);

	XMLElement* second_6_Element = doc.NewElement("password");
	root->InsertEndChild(second_6_Element);

	XMLPrinter printer;
	doc.Print(&printer);
	string str = printer.CStr();
	memcpy(buffer, str.c_str(), str.length());

	return str.length();
}

