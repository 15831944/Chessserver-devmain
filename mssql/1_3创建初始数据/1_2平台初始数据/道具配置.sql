USE WHJHPlatformDB
GO

TRUNCATE TABLE GameProperty

insert GameProperty(ID,Name,Kind,ExchangeRatio,UseArea,ServiceArea,BuyResultsGold,SendLoveLiness,RecvLoveLiness,UseResultsGold,UseResultsValidTime,UseResultsValidTimeScoreMultiple,UseResultsGiftPackage,RegulationsInfo,Recommend,SortID,Nullity) values(	306	,N'大喇叭'	,7,10,7	,1	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	N'使用后，发送的信息将在所有游戏房间的聊天框以醒目的方式显示。'	,	0	,	512	,	0	)
