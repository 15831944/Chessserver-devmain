USE WHJHPlatformDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_WritePersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_WritePersonalFeeParameter]
--删除配置
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_DeletePersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_DeletePersonalFeeParameter]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalParameter]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalFeeParameter]

---加载私人房间参数
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_LoadPersonalRoomParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_LoadPersonalRoomParameter]

---加载私人房间参数
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_LoadPersonalRoomParameterByKindID]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].GSP_GS_LoadPersonalRoomParameterByKindID
GO

----获取单个玩家进入过的房间
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_GetPersonalRoomUserScore]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_GetPersonalRoomUserScore]
GO


----写入结束时间
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_CloseRoomWriteDissumeTime]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_CloseRoomWriteDissumeTime]
GO

----获取私人房定制配置
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalRule]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalRule]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

------------------------------------------------------
---费用配置
CREATE PROC GSP_GS_WritePersonalFeeParameter
	@KindID	INT,							-- 房间类型
	@DrawCountLimit	INT,					-- 局数
	@DrawTimeLimit	INT,					-- 玩家掉线多长时间后解散桌子
	@TableFee	INT,						-- 房间创建多长时间后还未开始游戏解散桌子
	@AAPayFee   INT,						-- AA制费用
	@IniScore	INT,						-- 房间创建后多长时间后还未开始游戏解散桌子
	@cbGameMode TINYINT,					-- 游戏模式
	@strErrorDescribe NVARCHAR(127) OUTPUT	-- 输出信息
WITH ENCRYPTION AS

BEGIN
	INSERT INTO PersonalTableFee (KindID, DrawCountLimit,DrawTimeLimit, TableFee,AAPayFee,IniScore,GameMode) 
	VALUES (@KindID, @DrawCountLimit,@DrawTimeLimit, @TableFee,@AAPayFee, @IniScore,@cbGameMode) 
END

return 0
GO

--删除配置
CREATE PROC GSP_GS_DeletePersonalFeeParameter
	@KindID	INT,				-- 
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

BEGIN
	
	DELETE  FROM PersonalTableFee WHERE  KindID = @KindID
	
END

return 0
GO


------------------------------------------------------
---费用配置
CREATE PROC GSP_MB_GetPersonalFeeParameter
	@dwKindID INT,								-- 游戏 I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

BEGIN

	SELECT  DrawCountLimit, DrawTimeLimit, TableFee,AAPayFee,IniScore,GameMode FROM PersonalTableFee WHERE KindID=@dwKindID	
END

return 0
GO

---------------------------------------------------------------
-- 加载配置
CREATE PROC GSP_GS_LoadPersonalRoomParameter
	@dwKindID INT,								-- 房间标识
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 属性设置
SET NOCOUNT ON	
	DECLARE @cbIsJoinGame TINYINT						-- 是否参与游戏
	DECLARE @cbMinPeople TINYINT						-- 参与游戏的最小人数
	DECLARE @cbMaxPeople TINYINT						-- 参与游戏的最大人数

	-- 积分配置
	DECLARE @lPersonalRoomTax  BIGINT					-- 税收
	DECLARE @lMaxCellScore BIGINT						-- 最大底分
	DECLARE @wCanCreateCount int						-- 可以创建的最大房间数目
	DECLARE @wPlayTurnCount INT							-- 房间能够进行游戏的最大局数
	DECLARE @cbCreateRight TINYINT						-- 创建私人房间权限

	-- 限制配置
	DECLARE @wPlayTimeLimit	INT						-- 房间能够进行游戏的最大时间
	DECLARE @wTimeAfterBeginCount	INT				-- 一局游戏开始后多长时间后解散桌子
	DECLARE @wTimeOffLineCount	INT					-- 玩家掉线多长时间后解散桌子
	DECLARE @wTimeNotBeginGame	INT					-- 房间创建多长时间后还未开始游戏解散桌子
	DECLARE @wTimeNotBeginAfterCreateRoom	INT		-- 房间创建后多长时间后还未开始游戏解散桌子

	DECLARE @wBeginFreeTime		INT					-- 免费时段-开始
	DECLARE @wEndFreeTime		INT					 -- 免费时段-结束
-- 执行逻辑
BEGIN		
	SET @cbIsJoinGame = 0
	SET @cbMinPeople = 0
	SET @cbMaxPeople = 0	
	SET @lMaxCellScore = 0
	SET @lPersonalRoomTax = 0
	SET @lMaxCellScore = 0
	SET @wCanCreateCount = 0
	SET @wPlayTurnCount = 0
	SET @wPlayTimeLimit = 0
	SET @wTimeAfterBeginCount = 0
	SET @wTimeOffLineCount = 0
	SET @wTimeNotBeginGame = 0
	SET @wTimeNotBeginAfterCreateRoom = 0
	SET @cbCreateRight = 0
	SET @wBeginFreeTime = 0
	SET @wEndFreeTime = 0

	----查询最近一次相同类型游戏修改配置
	--DECLARE @dwLastServerID INT
	--SET @dwLastServerID = 0
	--SELECT @dwLastServerID=KindID FROM GameRoomInfo WHERE KindID = @wServerID

	-- 加载房间
	SELECT @cbIsJoinGame=IsJoinGame,@cbMinPeople=MinPeople, @cbMaxPeople=MaxPeople, @lPersonalRoomTax =PersonalRoomTax,
	@lMaxCellScore= MaxCellScore, @wCanCreateCount= CanCreateCount, @wPlayTurnCount=PlayTurnCount, @wPlayTimeLimit= PlayTimeLimit, @wTimeAfterBeginCount = TimeAfterBeginCount, 
	@wTimeOffLineCount = TimeOffLineCount, @wTimeNotBeginGame = TimeNotBeginGame, @wTimeNotBeginAfterCreateRoom = TimeNotBeginAfterCreateRoom, @cbCreateRight = CreateRight,
	@wBeginFreeTime = BeginFreeTime,@wEndFreeTime = EndFreeTime
	FROM PersonalRoomInfo WHERE KindID = @dwKindID



	SELECT @cbIsJoinGame AS IsJoinGame,@cbMinPeople AS MinPeople, @cbMaxPeople AS MaxPeople, @lPersonalRoomTax AS PersonalRoomTax,
	@lMaxCellScore AS MaxCellScore, @wCanCreateCount AS CanCreateCount, @wPlayTurnCount AS PlayTurnCount, @wPlayTimeLimit AS PlayTimeLimit, @wTimeAfterBeginCount AS TimeAfterBeginCount, 
	@wTimeOffLineCount AS TimeOffLineCount, @wTimeNotBeginGame AS TimeNotBeginGame, @wTimeNotBeginAfterCreateRoom AS TimeNotBeginAfterCreateRoom, @cbCreateRight AS CreateRight,
	@wBeginFreeTime as BeginFreeTime,@wEndFreeTime as EndFreeTime

END

RETURN 0

GO
----------------------------------------------------------------------------------------------------

-- 请求私人房间信息
CREATE PROC GSP_GS_GetPersonalRoomUserScore
	@dwUserID INT,								-- 用户 I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN

	-- 加载房间
	SELECT * FROM PersonalRoomScoreInfo WHERE UserID = @dwUserID ORDER BY WriteTime DESC

END

RETURN 0

GO


----------------------------------------------------------------------------------------------------

-- 请求私人房间信息
CREATE PROC GSP_GS_CloseRoomWriteDissumeTime
	@dwServerID INT,								-- 用户 I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 设置属性
SET NOCOUNT ON

-- 执行逻辑
BEGIN
	
	--删除此房间的所有锁表
	DELETE FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE ServerID = @dwServerID;
	-- 加载房间
	UPDATE StreamCreateTableFeeInfo SET DissumeDate = GetDATE(), RoomStatus = 2 WHERE ServerID = @dwServerID AND DissumeDate IS NULL

END

RETURN 0

GO

------------------------------------------------------
---费用配置
CREATE PROC GSP_MB_GetPersonalRule
	@dwKindID INT,								-- 游戏 I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

BEGIN
	DECLARE @dwServerID INT						-- 房间 I D
	SELECT  @dwServerID=ServerID FROM PersonalRoomInfo WHERE KindID=@dwKindID	
	IF @dwServerID IS NOT NULL
	BEGIN
		SELECT PersonalRule FROM GameRoomInfo WHERE ServerID=@dwServerID
		return 0		
	END
	ELSE 
	BEGIN		
		SET @strErrorDescribe=N'找不到该游戏！'
		RETURN 1
	END
END

return 0
GO

---------------------------------------------------------------