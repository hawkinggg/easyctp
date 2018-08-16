#include <iostream>
#include <thread>
#include <string>

#include "spdlog/spdlog.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/all.hpp>

#include "traderSpi.h"
#include "mdSpi.h"

int main(int argc, char *argv[])
{
//================================================================
	try
	{
		// Create basic file logger (not rotated)
		auto my_logger = spdlog::basic_logger_mt("basic_logger", "logs/basic.txt");

		// create a file rotating logger with 5mb size max and 3 rotated files
		// auto file_logger = spdlog::rotating_logger_mt("file_logger", "myfilename", 1024 * 1024 * 5, 3);
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log initialization failed: " << ex.what() << std::endl;
		return 1;
	}
//=================================================================
	boost::property_tree::ptree pt;
	try
	{
		boost::property_tree::read_json("config.json", pt);
	}
	catch (boost::exception& e)
	{
		std::cout << "can't find config file" << std::endl;
		return 1;
	}

	MdSpi::MODE mode = MdSpi::MARKET;
	if ("trader" == pt.get<std::string>("mode"))
		mode = MdSpi::TRADER;

	auto brokerID = pt.get<std::string>("account.test_broker");
	auto userID = pt.get<std::string>("account.test_user");
	auto traderFront = pt.get<std::string>("front.test_trader");
	auto marketFront = pt.get<std::string>("front.test_market");

	TraderSpi* pTraderSpi = nullptr;
	if (MdSpi::TRADER == mode)
	{
		CThostFtdcTraderApi* pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
		pTraderSpi = new TraderSpi(pTraderApi);
		pTraderSpi->setLoginInfo(brokerID, userID);
		pTraderApi->RegisterSpi(pTraderSpi);
		pTraderApi->RegisterFront((char*)traderFront.c_str());
		pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);
		pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);
		pTraderApi->Init();

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	for (auto x : pt.get_child("bayes"))
	{
		if ( 0 == x.second.get<int>("switch") )
			continue;

		auto main_name = x.second.get<std::string>("main");
		auto submain_name = x.second.get<std::string>("submain");
		auto u1 = x.second.get<double>("u1_squ");
		auto e2 = x.second.get<double>("e2");
		auto u2 = x.second.get<double>("u2_squ");
		auto gap_len = x.second.get<int>("gap_len");
		auto limit_ratio = x.second.get<double>("limit_ratio");
		auto max_num = x.second.get<int>("max_order_num");

		CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();
		MdSpi* pMdSpi = new MdSpi(pMdApi);

		boost::property_tree::ptree pt_gap;
		bool filexist = true;
		try
		{
			boost::property_tree::read_json(main_name + "-" + submain_name + ".json", pt_gap);
		}
		catch (boost::exception& e)
		{
			filexist = false;
			std::cout << "can't find the gap file with future: " << main_name << " and " << submain_name << std::endl;
		}
		if (filexist)
		{
			if (0 != pt_gap.get<int>("use"))
			{
				std::list<double> tmp_gaps;
				for (auto y : pt_gap.get_child("data"))
				{
					tmp_gaps.push_back(y.second.get<double>("gap"));
				}
				pMdSpi->setGap(tmp_gaps);
			}
		}
		pMdSpi->setMode(mode);
		pMdSpi->setTrader(pTraderSpi);
		pMdSpi->setLoginInfo(brokerID, userID);
		pMdSpi->setFutures(main_name, submain_name);
		pMdSpi->setPriorPara(u1, e2, u2);
		pMdSpi->setGapLen(gap_len);
		pMdSpi->setLimitRatio(limit_ratio);
		pMdSpi->setMaxOrderNum(max_num);
		pMdApi->RegisterSpi(pMdSpi);
		pMdApi->RegisterFront((char*)marketFront.c_str());
		pMdApi->Init();
	}

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(60));
	}

	return 0;
}

//normal
//pTraderApi->RegisterFront("tcp://180.168.102.231:41205");
//mini2
//pTraderApi->RegisterFront("tcp://114.80.83.83:31803");
//mini2 loc
//pTraderApi->RegisterFront("tcp://172.18.96.5:31803");
//simnow
//pTraderApi->RegisterFront("tcp://218.202.237.33:10002");
//pTraderApi->RegisterFront("tcp://180.168.146.187:10000");
//pTraderApi->RegisterFront("tcp://180.168.146.187:10001");

//normal
//pMdApi->RegisterFront("tcp://180.168.102.231:41213");
//mini2
//pMdApi->RegisterFront("tcp://117.184.228.3:31807");
//mini2 loc
//pMdApi->RegisterFront("tcp://172.17.103.5:31807");
//simnow
//pMdApi->RegisterFront("tcp://218.202.237.33:10012");
//pMdApi->RegisterFront("tcp://180.168.146.187:10010");
//pMdApi->RegisterFront("tcp://180.168.146.187:10011");