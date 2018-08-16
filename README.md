# easyctp
a simple system which use ctp API for future arbitrage trading


这是我第一次写期货交易的程序（本工程生命期：17年7月 - 8月）

策略是同品种的跨期贝叶斯套利，没记错的话是参考广发证券的一份另类交易策略研报

这是一个低延迟策略，需要速度抢单，并且尽量少的裸头寸

我们经过一系列调整（更换物理机location/更换CTP版本等）将速度提到了10ms以内

但实盘测试发现能抢到的单子十分有限，最终放弃了- -！

由于之前是private库，提交涉及了账户敏感信息，现清除历史提交