#include "traderSpi.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

#include "mdSpi.h"

int TraderSpi::nRequestID = 0;

TraderSpi::TraderSpi(CThostFtdcTraderApi *pTraderApi)
	:m_api(pTraderApi)
{
	m_curMd = nullptr;
}

TraderSpi::~TraderSpi()
{
	if (m_api)
	{
		m_api->RegisterSpi(nullptr);
		m_api->Release();
		m_api = nullptr;
	}
	//m_api->Release();
}

void TraderSpi::OnFrontConnected()
{
	std::cout << "trader front connected" << std::endl;
	CThostFtdcReqUserLoginField userLoginReq;
	std::memset(&userLoginReq, 0, sizeof(userLoginReq));
	std::strcpy(userLoginReq.BrokerID, m_brokerID.c_str());
	std::strcpy(userLoginReq.UserID, m_userID.c_str());
	std::strcpy(userLoginReq.Password, "");
	m_api->ReqUserLogin(&userLoginReq, TraderSpi::nRequestID++);
}

void TraderSpi::OnFrontDisconnected(int nReason)
{
	std::cout << "trader front disconnected: " << nReason << std::endl;
}

void TraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0)
	{
		logCurrentTime();
		std::cout << "trader user login success: " << pRspUserLogin->LoginTime << " " << pRspUserLogin->SHFETime << std::endl;
	}
	else
	{
		std::cout << "trader user login failed: " << pRspInfo->ErrorID << "\r\n" << pRspInfo->ErrorMsg << std::endl;
	}
}

void TraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cout << "trader user login out" << std::endl;
}

void TraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//std::cout << "order insert info" << std::endl;
	std::cout << "Order Insert failed: " << pRspInfo->ErrorID << "\r\n" << pRspInfo->ErrorMsg << std::endl;
}

void TraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cout << "cancel order" << std::endl;
}

void TraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	logCurrentTime();
	std::cout << "order status: " << pOrder->OrderStatus << std::endl; //<<"\r\n"

	if (THOST_FTDC_OST_Canceled == pOrder->OrderStatus)
	{
		std::lock_guard<std::mutex> lk(m_curMd->m_mutex);
		m_curMd->m_orderStatus = pOrder->OrderStatus;
		m_curMd->m_cond.notify_all();
	}
}

void TraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	logCurrentTime();
	std::cout << "order trade" << std::endl;

	std::lock_guard<std::mutex> lk(m_curMd->m_mutex);
	m_curMd->m_vol_rtn = pTrade->Volume;
	m_curMd->m_price_rtn = pTrade->Price;
	m_curMd->m_cond.notify_all();
}

void TraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	std::cout << "order insert error" << std::endl;
}

void TraderSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	std::cout << "order cancel error" << std::endl;
}

void TraderSpi::logCurrentTime()
{
	// Because c-style date&time utilities don't support microsecond precison,
	// we have to handle it on our own.
	auto time_now = std::chrono::system_clock::now();
	auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
	auto ms_part = duration_in_ms - std::chrono::duration_cast<std::chrono::seconds>(duration_in_ms);

	std::time_t raw_time = std::chrono::system_clock::to_time_t(time_now);
	std::cout << std::put_time(std::localtime(&raw_time), "%H:%M:%S.") << std::setfill('0') << std::setw(3) << ms_part.count() << "   ";  //%Y%m%d 
}
