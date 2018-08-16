#include "mdSpi.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <numeric>
#include <cstdio>
#include <atomic>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "traderSpi.h"

std::atomic_bool processing = false;

MdSpi::MdSpi(CThostFtdcMdApi * pMdApi)
	:m_api(pMdApi)
{
	//logCurrentTime();

	for (int i = 0; i < 10; ++i)
	{
		m_main._datas.push_back(0);
		m_submain._datas.push_back(0);
	}

	m_mode = MARKET;
	m_state = EMPTY;
	/*m_gapLen = 180;
	m_limitRatio = 0.002;

	m_u1_squ = 29.77165356018525;
	m_e2 = 206.66970840196723;
	m_u2_squ = 28.601563860001693;

	m_main._name = "rb1710";
	m_submain._name = "rb1801";*/
}

MdSpi::~MdSpi()
{
	// 
	boost::property_tree::ptree root;
	boost::property_tree::ptree items;
	root.put("use", "0");
	for (auto x : m_gap)
	{
		items.add("gap", x);
	}
	root.put_child("data", items);
	boost::property_tree::write_json(m_main._name+"-"+m_submain._name+".json", root);

	///< �ο� �ۺϽ���ƽ̨API����FAQ
	if (m_api)
	{
		m_api->RegisterSpi(nullptr);
		m_api->Release();
		m_api = nullptr;
	}
	
}

void MdSpi::OnFrontConnected()
{
	std::cout << "front connected" << std::endl;
	CThostFtdcReqUserLoginField userLoginReq;
	std::memset(&userLoginReq, 0, sizeof(userLoginReq));
	std::strcpy(userLoginReq.BrokerID, "0");//m_brokerID.c_str()
	std::strcpy(userLoginReq.UserID, "0");//m_userID.c_str()
	std::strcpy(userLoginReq.Password, "0");
	m_api->ReqUserLogin(&userLoginReq, 0);
}

void MdSpi::OnFrontDisconnected(int nReason)
{
	std::cout << "front disconnected: " << nReason << std::endl;
}

void MdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0)
	{
		logCurrentTime();
		std::cout << "user login success: " << pRspUserLogin->LoginTime << " " << pRspUserLogin->SHFETime << std::endl;


		char *insArray[] = { (char*)m_main._name.c_str(), (char*)m_submain._name.c_str(), NULL };
		m_api->SubscribeMarketData(insArray, 2);
	} 
	else
	{
		std::cout << "user login failed: " << pRspInfo->ErrorID << "\r\n" << pRspInfo->ErrorMsg << std::endl;
	}
}

void MdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cout << "user login out" << std::endl;
}

void MdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0)
	{
		std::cout << "sub market data ok" << std::endl;
	}
	else
	{
		std::cout << "sub market data failed: " << pRspInfo->ErrorID << "\r\n" << pRspInfo->ErrorMsg << std::endl;
	}
}

void MdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cout << "cancel sub market data" << std::endl;
}

void MdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	///< p1 ��ӡ��־  
	logCurrentTime();
	std::cout << pDepthMarketData->UpdateTime << "." << pDepthMarketData->UpdateMillisec << "  price:" << pDepthMarketData->LastPrice << "\r\n"; // << std::endl;
	
	///< p2 �ж�����/������
	FUTURE* curFuture = nullptr;
	std::string instrument = pDepthMarketData->InstrumentID;
	if (instrument == m_main._name)
		curFuture = &m_main;
	else if (instrument == m_submain._name)
		curFuture = &m_submain;
	else
		return;

	///< p3 ���º�Լ����
	curFuture->_datas.pop_front();
	curFuture->_datas.push_back(pDepthMarketData->LastPrice);
	curFuture->_buy = pDepthMarketData->BidPrice1;
	curFuture->_vol_buy = pDepthMarketData->BidVolume1;
	curFuture->_sell = pDepthMarketData->AskPrice1; 
	curFuture->_vol_sell = pDepthMarketData->AskVolume1;
	sscanf(pDepthMarketData->UpdateTime, "%d:%d:%d",&(curFuture->_hour),&(curFuture->_min),&(curFuture->_sec));
	if (pDepthMarketData->UpdateMillisec < 500)
		curFuture->_minisec = 0;
	else
		curFuture->_minisec = 500;

	///< p4 ��Ժ�Լ�ĸ���ʱ���Ƿ�һ��
	if (m_main._minisec != m_submain._minisec || 
		m_main._sec != m_submain._sec || 
		m_main._min != m_submain._min || 
		m_main._hour != m_submain._hour)
	{
		return;
	}
	///< p5 GAP����
	m_gap.push_back(m_main._datas.back() - m_submain._datas.back());
	if (m_gap.size() <= m_gapLen)
		return;
	m_gap.pop_front();

	///< p6 ����ģʽ�������ź�
	if (TRADER != m_mode)
		return;

	///< p �����ڽ����еĺ�Լ�����ز��账��
	if (processing)
	{

		return;
	}

	///< p8 �źż���
	auto expect = computeExpectDiff();

	if (m_state == EMPTY)
	{
		if ((m_main._buy - m_submain._sell - expect > m_main._datas.back()*m_limitRatio) && zeroCheck(POSTIVE, true))
		{
			///< p ֻ����һ����Ժ�Լ���ڿ�����
			if (processing.exchange(true))
				return;

			auto vol = std::min(m_max_order_num, std::min(m_main._vol_buy, m_submain._vol_sell));
			TREND t = trend();
			bool deal = false;

			if (UP == t)
				deal = firstSecondOrder(POSTIVE, false, vol ,THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_submain._name, m_submain._sell, 
													THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_main._name, m_main._buy);
			else if (DOWN == t)
				deal = firstSecondOrder(POSTIVE, false, vol, THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_main._name, m_main._buy,
													THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_submain._name, m_submain._sell);
			else
				deal = firstSecondOrder(POSTIVE, true, vol, THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_submain._name, m_submain._sell,
													THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_main._name, m_main._buy);

			processing.exchange(deal);
		}
		else if ((m_main._sell - m_submain._buy - expect < -m_main._datas.back()*m_limitRatio) && zeroCheck(NEGTIVE, true))
		{
			///< p ֻ����һ����Ժ�Լ���ڿ�����
			if (processing.exchange(true))
				return;

			auto vol = std::min(m_max_order_num, std::min(m_main._vol_sell, m_submain._vol_buy));
			TREND t = trend();
			bool deal = false;

			if (UP == t)
				deal = firstSecondOrder(NEGTIVE, false, vol, THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_main._name, m_main._sell,
													THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_submain._name, m_submain._buy);
			else if (DOWN == t)
				deal = firstSecondOrder(NEGTIVE, false, vol, THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_submain._name, m_submain._buy,
													THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_main._name, m_main._sell);
			else
				deal = firstSecondOrder(NEGTIVE, true, vol, THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, m_submain._name, m_submain._buy,
													THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_main._name, m_main._sell);

			processing.exchange(deal);
		}
	}
	else if (m_state == POSTIVE)
	{
		if (((m_main._sell - m_submain._buy) <= expect) && zeroCheck(m_state, 0))
		{
			firstSecondOrder(EMPTY, true, 0, THOST_FTDC_D_Sell, THOST_FTDC_OF_CloseToday, m_submain._name, m_submain._buy,
											THOST_FTDC_D_Buy, THOST_FTDC_OF_CloseToday, m_main._name, m_main._sell);
		}
	}
	else if (m_state == NEGTIVE)
	{
		if (((m_main._buy - m_submain._sell) >= expect) && zeroCheck(m_state, 0))
		{
			firstSecondOrder(EMPTY, true, 0, THOST_FTDC_D_Buy, THOST_FTDC_OF_CloseToday, m_submain._name, m_submain._sell,
											THOST_FTDC_D_Sell, THOST_FTDC_OF_CloseToday, m_main._name, m_main._buy);
		}
	}

	///< p ��־��¼

}

void MdSpi::logCurrentTime()
{
//	auto tn = std::chrono::steady_clock::now();
//	std::cout << "current clock: " << tn.time_since_epoch().count() << "      ";
//	return;
	// Because c-style date&time utilities don't support microsecond precison,
	// we have to handle it on our own.
	auto time_now = std::chrono::system_clock::now();
	auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
	auto ms_part = duration_in_ms - std::chrono::duration_cast<std::chrono::seconds>(duration_in_ms);

	//tm local_time_now;
	std::time_t raw_time = std::chrono::system_clock::to_time_t(time_now);
	//_localtime64_s(&local_time_now, &raw_time);
	std::cout << std::put_time(std::localtime(&raw_time), "%H:%M:%S.") << std::setfill('0') << std::setw(3) << ms_part.count() << "   ";  //%Y%m%d 
}

double MdSpi::computeExpectDiff()
{
	// �������Ż��ռ�
	int n = m_gap.size();
	double A = n / m_u1_squ + 1 / m_u2_squ;
	//double sum = std::accumulate(std::begin(resultSet), std::end(resultSet), 0.0);
	double mean = std::accumulate(m_gap.begin(), m_gap.end(), 0.0) / n;
	double B = mean*n / m_u1_squ + m_e2 / m_u2_squ;
	return B / A;
}

bool MdSpi::zeroCheck(STATE s, bool b)
{
	// �ǵ�ͣʱ��������Ӱ�죬Ŀǰ����Ϊ�ǵ�ͣ�����5���Ӳ�����
	if (m_main._buy == 0 || m_main._sell == 0 || m_submain._buy == 0 || m_submain._sell == 0)
		return false;
	return true;

	if (s == POSTIVE)
	{
		if (b && (m_main._buy == 0 || m_submain._sell == 0))
			return false;
		else if (!b && (m_main._sell == 0 || m_submain._buy == 0))
			return false;
	}
	else if (s == NEGTIVE)
	{
		if (b && (m_main._sell == 0 || m_submain._buy == 0))
			return false;
		else if (!b && (m_main._buy == 0 || m_submain._sell == 0))
			return false;
	}
	return true;
}

MdSpi::TREND MdSpi::trend()
{
	return BOTH;
	if (m_main._datas.back() > m_main._datas.front() && m_submain._datas.back() > m_submain._datas.front())
		return UP;
	else if (m_main._datas.back() < m_main._datas.front() && m_submain._datas.back() < m_submain._datas.front())
		return DOWN;
	return BOTH;
}

bool MdSpi::processingCheck(bool stage_to)
{
	return processing.exchange(stage_to);
}

// STATE==EMPTYʱ volΪ0 ���ݸ��Ե�ǰ�ֲ�ƽ�� /û���κ��µ��ɹ�ʱ ����false
bool MdSpi::firstSecondOrder(STATE to, bool normalslilppage, int vol, 
	TThostFtdcDirectionType firDir, TThostFtdcOffsetFlagType firKpp, const std::string& firName, double firPrice,
	TThostFtdcDirectionType secDir, TThostFtdcOffsetFlagType secKpp, const std::string& secName, double secPrice)
{
	m_trader->setCurMd(this);

	auto price = m_future_tick*m_slippage_allowed;
	if (THOST_FTDC_D_Sell == firDir)
		price = -price;

	std::unique_lock<std::mutex> lk(m_mutex);
	order(firName, firPrice+price, vol, firDir, firKpp);
	m_cond.wait(lk);
	lk.unlock();
	if (THOST_FTDC_OST_Canceled != m_orderStatus)
	{
		lk.lock();
		order(secName, secPrice, vol, secDir, secKpp);
		//std::cout << "wait second. " << std::endl;
		m_cond.wait(lk);
		lk.unlock();
		if (THOST_FTDC_OST_Canceled != m_orderStatus)
		{
			m_state = to;
			logCurrentTime();
			std::cout << "ok" << std::endl;
		}
		else
		{
			std::cout << "second failed." << std::endl;
		}
	}
	else    // ��һ���޳ɽ�ֱ�ӷ���
	{
		std::cout << "first failed." << std::endl;
		return false;
	}

	return true;
}

bool MdSpi::order(const std::string& id, double price, int vol, TThostFtdcDirectionType dir, TThostFtdcOffsetFlagType kpp)
{
	CThostFtdcInputOrderField req;
	std::memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_brokerID.c_str());
	strcpy(req.InvestorID, m_userID.c_str());
	strcpy(req.InstrumentID, id.c_str());
	int nRequestID = TraderSpi::nRequestID++;
	sprintf(req.OrderRef, "%012d", nRequestID);

	req.Direction = dir;
	req.CombOffsetFlag[0] = kpp;
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	req.VolumeTotalOriginal = vol;

	req.VolumeCondition = THOST_FTDC_VC_AV;

	req.MinVolume = 1;

	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;

	req.IsAutoSuspend = 0;

	req.UserForceClose = 0;

	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	req.LimitPrice = price;
	
	req.TimeCondition = THOST_FTDC_TC_IOC;

	req.ContingentCondition = THOST_FTDC_CC_Immediately;

	logCurrentTime();
	std::cout << "send order: " << id << " price:" << price << std::endl; //<< "\r\n" ;

	return m_trader->getApi()->ReqOrderInsert(&req, nRequestID);
}