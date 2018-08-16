#pragma once
#include <map>
#include <string>

#include "ThostFtdcTraderApi.h"

class MdSpi;

class TraderSpi : public CThostFtdcTraderSpi
{
public:
	TraderSpi(CThostFtdcTraderApi *);
	virtual ~TraderSpi();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

//----------------------------------------------------------------------------

	void logCurrentTime();

	static int nRequestID;
	
	CThostFtdcTraderApi* getApi() { return m_api; };

	void setCurMd(MdSpi* user) { m_curMd = user; };

	void setMdByOrderRef(MdSpi* user, int nRequestID) { m_mMdByOrderRef[nRequestID] = user; };

	void setLoginInfo(const std::string& brokerID, const std::string& userID) { m_brokerID = brokerID; m_userID = userID; };

private:
	CThostFtdcTraderApi* m_api;

	MdSpi* m_curMd;
	std::map<int,MdSpi*> m_mMdByOrderRef;

	std::string m_userID;
	std::string m_brokerID;
};