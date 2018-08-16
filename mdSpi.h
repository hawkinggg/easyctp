#pragma once

#include <list>
#include <string>
#include <mutex>
#include <condition_variable>

#include "ThostFtdcMdApi.h"

class TraderSpi;

class MdSpi : public CThostFtdcMdSpi
{
public:
	MdSpi(CThostFtdcMdApi *);
	virtual ~MdSpi();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
	//virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnHeartBeatWarning(int nTimeLapse);

//---------------------------------------------------------------------------------------------------
	enum MODE
	{
		MARKET,
		TRADER,
	};

	enum STATE
	{
		NEGTIVE = -1,
		EMPTY,
		POSTIVE,
		SWITCHING,
	};

	enum TREND
	{
		DOWN = -1,
		BOTH,
		UP,
	};

	enum ORDER_ACTION
	{
		OPEN_LONG,
		OPEN_SHORT,
		CLOSE_LONG,
		CLOSE_SHORT,
	};

	struct FUTURE
	{
		std::string _name;
		//----------------------------
		double _open_price;
		double _close_price;
		int _open_vol;
		int _wait_vol;
		int _slippage_num;
		int _wait_timer;
		//----------------------------
		std::list<double> _datas;
		double _buy;
		int _vol_buy;
		double _sell;
		int _vol_sell;
		int _hour;
		int _min;
		int _sec;
		int _minisec;
	};
	// 参数配置
	void setLoginInfo(const std::string& brokerID, const std::string& userID) { m_brokerID = brokerID; m_userID = userID; };
	void setTrader(TraderSpi* trader) { m_trader = trader; };
	void setFutures(const std::string& main, const std::string& submain) { m_main._name = main; m_submain._name = submain; };
	void setPriorPara(double u1, double e2, double u2) { m_u1_squ = u1; m_e2 = e2; m_u2_squ = u2; };
	void setGapLen(int len) { m_gapLen = len; };
	void setLimitRatio(double ratio) { m_limitRatio = ratio; };
	void setMaxOrderNum(int num) { m_max_order_num = num; };
	void setMode(MODE md) { m_mode = md; };
	void setGap(std::list<double>& gaps) { m_gap = gaps; };

	// 下单函数
	bool order(const std::string& id, double price, int vol, TThostFtdcDirectionType dir, TThostFtdcOffsetFlagType kpp);

private:
	//void handleData();
	//
	double computeExpectDiff();
	// 下单方式
	bool firstSecondOrder(STATE to, bool normalslilppage, int vol, 
		TThostFtdcDirectionType firDir, TThostFtdcOffsetFlagType firKpp, const std::string& firName, double firPrice,
		TThostFtdcDirectionType secDir, TThostFtdcOffsetFlagType secKpp, const std::string& secName, double secPrice);
	
	// 涨跌停限制时不进行操作
	bool zeroCheck(STATE s, bool b);

	TREND trend();

	bool processingCheck(bool stage_to);

	void logCurrentTime();

private:
	CThostFtdcMdApi* m_api;
	TraderSpi* m_trader;

	std::string m_userID;
	std::string m_brokerID;

	double m_u1_squ;
	double m_e2;
	double m_u2_squ;

	double m_limitRatio;
	int m_gapLen;

	int m_max_order_num;    // 最大下单数
	double m_future_tick;   // 合约最小单位
	int m_slippage_allowed; // 允许的滑点数

	FUTURE m_main;
	FUTURE m_submain;

	FUTURE* m_fir;   // 交易下单时的首单
	FUTURE* m_sec;   // 交易下单时的次单

	MODE m_mode;
	STATE m_state;
	TThostFtdcOffsetFlagType m_kpp;
	
	std::list<double> m_gap;

	std::string m_log;

///< trader模块会用到
public:
	TThostFtdcOrderStatusType m_orderStatus;
	int m_vol_rtn;
	double m_price_rtn;

	std::mutex m_mutex;
	std::condition_variable m_cond;
};
