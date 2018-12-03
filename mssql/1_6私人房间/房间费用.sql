USE WHJHTreasureDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_CreateTableFee]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_CreateTableFee]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_CreateTableQuit]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_CreateTableQuit]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_InsertCreateRecord]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_InsertCreateRecord]


---加载私人房间信息
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_QueryPersonalRoomInfo]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_QueryPersonalRoomInfo]
GO

---支付钻石
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_PayRoomCard]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_PayRoomCard]
GO


SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------

-- 创建判断
CREATE PROC GSP_GR_CreateTableFee
	@dwUserID INT,								-- 用户 I D
	@dwServerID INT,							-- 房间 I D
	@dwDrawCountLimit INT,						-- 时间限制
	@dwDrawTimeLimit INT,						-- 局数限制
	@cbPayMode		TINYINT,					-- 创建方式	
	@strClientIP NVARCHAR(15),					-- 连接地址
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN	
	DECLARE @ReturnValue INT
	DECLARE @dUserBeans DECIMAL(18,2)
	DECLARE @dCurBeans DECIMAL(18,2)
	DECLARE @Fee INT
	DECLARE @AAFee INT
	DECLARE @lDiamond bigint
	DECLARE @cbIsJoinGame TINYINT
	DECLARE @MemberOrder TINYINT
	DECLARE @CreateRight TINYINT
	DECLARE @iCanCreateCount INT

	DECLARE @wKindID INT
	SELECT  @wKindID = KindID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo WHERE ServerID = @dwServerID 
	-- 查询费用 及 房主是否参与游戏
	SELECT  @CreateRight = CreateRight FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 

	--用户钻石	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID

		-- 查询锁定
	DECLARE @LockServerID INT
	SELECT @LockServerID=ServerID FROM GameScoreLocker WHERE UserID=@dwUserID

		-- 锁定判断
	IF  @LockServerID IS NOT NULL and @LockServerID<>@dwServerID
	BEGIN
		-- 查询信息
		DECLARE @ServerName NVARCHAR(31)
		SELECT @ServerName=ServerName FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo WHERE ServerID=@LockServerID

		-- 错误信息
		IF @ServerName IS NULL SET @ServerName=N'未知房间'
		SET @strErrorDescribe=N'您正在 [ '+@ServerName+N' ] 游戏房间中，不能在游戏的同时创建房间！'
		RETURN 2
	END

	--加入方式
	SELECT  @cbIsJoinGame = IsJoinGame,@iCanCreateCount=CanCreateCount FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 
		
	-- 查询费用
	SELECT @Fee=TableFee,@AAFee = AAPayFee FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @wKindID
	IF @Fee IS NULL OR @Fee=0
	BEGIN
		SET @strErrorDescribe=N'数据库查询费用失败，请重新尝试！'
		RETURN 3
	END
	
	--查询参与创建所有房间需要的钻石
	DECLARE @MaxPay INT	
	SELECT @MaxPay = SUM(NeedRoomCard) FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.StreamCreateTableFeeInfo WHERE UserID = @dwUserID AND RoomStatus <> 2
	
	--限制次数	
/* 	if @cbIsJoinGame = 0
	BEGIN
		DECLARE @CreateCount INT
		SELECT @CreateCount = Count(*) FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.StreamCreateTableFeeInfo WHERE UserID = @dwUserID AND RoomStatus <> 2 and PayMode = 0
		
		IF @CreateCount > @iCanCreateCount
		BEGIN
			SET @strErrorDescribe=N'创建房间的数目已经超过最大数！'
			RETURN 1
		END
	END */
	
	-- 获取钻石	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL SET @lDiamond=0
	
	--是否AA制
	IF @cbPayMode = 1
	BEGIN
		IF @lDiamond  < @MaxPay + @AAFee
		BEGIN
			SET @strErrorDescribe=N'用户钻石不足，请购买！'
			RETURN 1
		END
		SELECT @lDiamond AS Diamond, @cbIsJoinGame AS IsJoinGame,@AAFee AS RoomCardFee
	END
	ELSE
	BEGIN
		IF @lDiamond  < @MaxPay + @Fee
		BEGIN
			SET @strErrorDescribe=N'用户钻石不足，请购买！'
			RETURN 1
		END
		SELECT @lDiamond AS Diamond, @cbIsJoinGame AS IsJoinGame,@Fee AS RoomCardFee
	END
END

RETURN 0
GO

----------------------------------------------------------------------------------------------------

-- 退还费用
CREATE PROC GSP_GR_CreateTableQuit
	@dwUserID INT,								-- 用户 I D
	@dwPersonalRoomID INT,						-- 约战房间ID
	@dwServerID INT,							-- 房间标识
	@dwDrawCountLimit INT,						-- 时间限制
	@dwDrawTimeLimit INT,						-- 局数限制
	@strClientIP NVARCHAR(15),					-- 连接地址
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN	
	DECLARE @ReturnValue INT
	SET @ReturnValue=0

	DECLARE @lDiamond bigint
	DECLARE @cbJoin TINYINT

	DECLARE @wKindID INT
	SELECT  @wKindID = KindID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo WHERE ServerID = @dwServerID 
	-- 查询费用 及 房主是否参与游戏
	SELECT @cbJoin = IsJoinGame FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 

	-- 更新记录
	UPDATE  WHJHPlatformDB..StreamCreateTableFeeInfo   SET  DissumeDate = GETDATE(), RoomStatus = 2  WHERE RoomID = @dwPersonalRoomID AND UserID = @dwUserID

	--必须参与游戏房主解锁
	IF @cbJoin = 1
	BEGIN
		DELETE FROM GameScoreLocker WHERE UserID = @dwUserID
	END

	--用户钻石	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL SET @lDiamond=0

	SELECT @lDiamond AS Diamond

END

RETURN @ReturnValue
GO


-----------------------------------------------------------------------
-- 创建房间记录
CREATE PROC GSP_GR_InsertCreateRecord
	@dwUserID INT,								-- 用户 I D
	@dwServerID INT,							-- 房间 标识
	@dwPersonalRoomID INT,						-- 约战房间标识
	@lCellScore INT,							-- 房间 底分
	@dwDrawCountLimit TINYINT,					-- 局数限制
	@dwDrawTimeLimit INT,						-- 时间限制
	@szPassWord NVARCHAR(15),					-- 连接地址
	@wJoinGamePeopleCount TINYINT,				-- 人数限制
	@dwRoomTax BIGINT,							-- 私人房间税收
	@strClientAddr NVARCHAR(15),
	@cbPayMode TINYINT,							-- 支付模式
	@lNeedRoomCard TINYINT,						-- 理论需要的钻石
	@cbGameMode TINYINT,						-- 游戏模式
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN
	DECLARE @Fee INT
	DECLARE @Nicname NVARCHAR(31)
	DECLARE @lPersonalRoomTax BIGINT

	-- 查询费用 及 房主是否参与游戏
	DECLARE @wKindID INT
	SELECT  @wKindID = KindID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo  WHERE ServerID = @dwServerID 
	-- 查询费用
	SELECT @Fee=TableFee FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @wKindID 
	IF @Fee IS NULL OR @Fee=0
	BEGIN
		SET @strErrorDescribe=N'数据库查询费用失败，请重新尝试！'
		RETURN 3
	END

	-- 获取创建房间玩家的昵称
	SELECT @Nicname =NickName FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID = @dwUserID
	IF @Nicname IS NULL
	SET @Nicname =''
		
	SELECT  @lPersonalRoomTax = PersonalRoomTax FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 
	
	--如果是消耗钻石，查询代理税收
	DECLARE @lTaxAgency BIGINT
	SELECT  @lTaxAgency = AgentScale FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsAgent WHERE UserID = @dwUserID 
	IF @lTaxAgency IS NOT NULL
	BEGIN
		SET @lPersonalRoomTax = @lTaxAgency
	END

	-- 写入消耗放开记录
	INSERT INTO WHJHPlatformDB..StreamCreateTableFeeInfo(UserID,NickName, ServerID, RoomID, CellScore, CountLimit,TimeLimit,CreateTableFee,CreateDate, TaxAgency, JoinGamePeopleCount,PayMode, RoomStatus, NeedRoomCard,GameMode)
												VALUES(@dwUserID,@Nicname, @dwServerID, @dwPersonalRoomID, @lCellScore,@dwDrawCountLimit, @dwDrawTimeLimit, @Fee,GETDATE(), @dwRoomTax, @wJoinGamePeopleCount, @cbPayMode, 0, @lNeedRoomCard,@cbGameMode)	


END

RETURN 0
GO

----------------------------------------------------------------------------------------------------------
-- 请求约战房间信息
CREATE  PROCEDURE dbo.GSP_GS_QueryPersonalRoomInfo
	@dwPersonalRoomID INT,							-- 约战房间标识
	@dwRoomHostID INT,									-- 房主ID
	@strErrorDescribe NVARCHAR(127) OUTPUT				-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN
	DECLARE @dwUserID			INT
	DECLARE @dwServerID			INT
	DECLARE @dwKindID			INT
	DECLARE @Nicname 			NVARCHAR(31)
	DECLARE @dwPlayTurnCount 	TINYINT
	DECLARE @dwPlayTimeLimit 	INT
	DECLARE @cbIsDisssumRoom 	TINYINT
	DECLARE @sysCreateTime 		DATETIME
	DECLARE @sysDissumeTime 	DATETIME
	DECLARE @lTaxCount 			BIGINT
	DECLARE @lCreateFee 		TINYINT
	DECLARE @bnryRoomScoreInfo 	varbinary(MAX)
	DECLARE @cbPayMode 			TINYINT
	DECLARE @lNeedRoomCard 		TINYINT
	DECLARE @lJoinPeopleCount 	TINYINT
	DECLARE @cbRoomStatus		TINYINT
	DECLARE @cbGameMode 			TINYINT
	-- 加载房间
	SELECT @dwUserID = UserID, @dwPlayTurnCount=CountLimit, @dwPlayTimeLimit = TimeLimit, @sysCreateTime = CreateDate, @sysDissumeTime = DissumeDate, @lTaxCount = TaxRevenue, @lCreateFee = CreateTableFee,
	  @bnryRoomScoreInfo = RoomScoreInfo, @cbPayMode = PayMode, @lNeedRoomCard = NeedRoomCard, @lJoinPeopleCount = JoinGamePeopleCount, @cbRoomStatus = RoomStatus,@dwServerID=ServerID
	  ,@cbGameMode = GameMode
	FROM WHJHPlatformDB..StreamCreateTableFeeInfo WHERE RoomID = @dwPersonalRoomID AND UserID = @dwRoomHostID
	IF @sysDissumeTime IS NULL
	BEGIN
		SET @cbIsDisssumRoom = 0
		SET @sysDissumeTime = @sysCreateTime
	END
	ELSE
	BEGIN
		SET @cbIsDisssumRoom = 1
	END
	--获取游戏ID
	IF @dwServerID IS NOT NULL	SELECT @dwKindID=KindID FROM WHJHPlatformDB..GameRoomInfo WHERE ServerID=@dwServerID
	IF @dwKindID IS NULL SET @dwKindID=0
	
	declare @strRoomScoreInfo varchar(8000),@i int
	select @strRoomScoreInfo='',@i=datalength(@bnryRoomScoreInfo)
	while @i>0
		select @strRoomScoreInfo=substring('0123456789ABCDEF',substring(@bnryRoomScoreInfo,@i,1)/16+1,1)
				+substring('0123456789ABCDEF',substring(@bnryRoomScoreInfo,@i,1)%16+1,1)
				+@strRoomScoreInfo
			,@i=@i-1

	-- 获取玩家昵称
	SELECT @Nicname =NickName FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID = @dwUserID
	IF @Nicname IS NULL
	SET @Nicname =''


	if @dwPlayTurnCount is null
	set @dwPlayTurnCount=0

	if @dwPlayTimeLimit is null
	set @dwPlayTimeLimit=0

	if @cbIsDisssumRoom is null
	set @cbIsDisssumRoom=0

	if @sysDissumeTime is null
	set @sysDissumeTime=GETDATE()

	if @sysCreateTime is null
	set @sysCreateTime=GETDATE()

	if @lTaxCount is null
	set @lTaxCount=0	

	if @lCreateFee is null
	set @lCreateFee=0

	SELECT @Nicname AS UserNicname, @dwPlayTurnCount AS dwPlayTurnCount, @dwPlayTimeLimit AS dwPlayTimeLimit, @cbIsDisssumRoom AS cbIsDisssumRoom, @sysCreateTime AS sysCreateTime, 
	@sysDissumeTime AS sysDissumeTime, @lTaxCount AS lTaxCount, @lCreateFee AS CreateTableFee,@bnryRoomScoreInfo AS RoomScoreInfo, @strRoomScoreInfo AS strRoomScoreInfo,
	@cbPayMode AS PayMode, @lNeedRoomCard AS NeedRoomCard, @lJoinPeopleCount AS JoinGamePeopleCount, @cbRoomStatus AS RoomStatus,@dwKindID AS wKindID,@cbGameMode AS GameMode

END

RETURN 0

GO

-------------------------------------
-----------------------------------------------------------------------
-- 创建房间记录
CREATE PROC GSP_GS_PayRoomCard
	@cbPayMode	INT,							-- 支付方式
	@dwUserID INT,								-- 用户 I D
	@dwKindID INT,								-- 房间 标识
	@dwPersonalRoomID INT,					-- 约战房间ID
	@dwDrawCountLimit INT,						-- 局数限制
	@dwDrawTimeLimit INT,						-- 时间限制
	@strClientAddr NVARCHAR(15),				-- 客户端地址
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN		
	DECLARE @Fee INT
	DECLARE @Nicname NVARCHAR(31)
	DECLARE @lPersonalRoomTax BIGINT
	DECLARE @ReturnValue INT

	-- 查询费用 及 房主是否参与游戏	
	DECLARE @wKindID INT

	-- 查询费用
	IF @cbPayMode=1
	BEGIN
		SELECT @Fee=AAPayFee FROM WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @dwKindID 
	END
	ELSE
	BEGIN
		SELECT @Fee=TableFee FROM WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @dwKindID 
	END
	
	IF @Fee IS NULL OR @Fee=0
	BEGIN
		SET @strErrorDescribe=N'数据库查询费用失败，请重新尝试！'
		RETURN 3
	END

	--	查询消耗方式
	SELECT @lPersonalRoomTax = PersonalRoomTax FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 	

	DECLARE @lDiamond BIGINT
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL
	BEGIN
		SET @strErrorDescribe=N'用户钻石不足，请购买！'
		RETURN 1
	END

	-- 写入费用
	IF @lDiamond < @Fee
	BEGIN
		SET @strErrorDescribe=N'您的钻石不足，请先充值。'
		RETURN 4
	END
	ELSE
	BEGIN
		UPDATE WHJHTreasureDB..UserCurrency SET Diamond=@lDiamond-@Fee WHERE UserID=@dwUserID					
	--	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
		
		--插入日志
		DECLARE @DateTime DATETIME
		--DECLARE @Random VARCHAR(5)
		DECLARE @SerialNumber NVARCHAR(20)
		SET @DateTime = GETDATE()
		--SET @Random = CAST(FLOOR(89999*RAND()+10000) AS VARCHAR(5))
		SELECT @SerialNumber=dbo.WF_GetSerialNumber()
		
		--一人付费制
		IF @cbPayMode = 0
		BEGIN
			INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordDiamondSerial(SerialNumber,MasterID,UserID,TypeID,CurDiamond,ChangeDiamond,ClientIP,CollectDate) 
			VALUES(@SerialNumber,0,@dwUserID,10,@lDiamond,-@Fee,@strClientAddr,@DateTime)
		END
		ELSE
		BEGIN
			INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordDiamondSerial(SerialNumber,MasterID,UserID,TypeID,CurDiamond,ChangeDiamond,ClientIP,CollectDate) 
			VALUES(@SerialNumber,0,@dwUserID,11,@lDiamond,-@Fee,@strClientAddr,@DateTime)
		END
	END			
	
	--一人付费制
	IF @cbPayMode = 0
	BEGIN
		UPDATE WHJHPlatformDB..StreamCreateTableFeeInfo SET CreateTableFee = @Fee WHERE RoomID = @dwPersonalRoomID AND UserID = @dwUserID
	END

	
	SELECT @lDiamond-@Fee AS Dimaond
END

RETURN 0
GO
