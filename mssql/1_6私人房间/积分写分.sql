
----------------------------------------------------------------------------------------------------

USE WHJHGameScoreDB
GO


----写私人房配置信息
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_WritePersonalGameScore]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_WritePersonalGameScore]
GO



SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO


----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- 游戏写分
CREATE PROC GSP_GR_WritePersonalGameScore

	-- 系统信息
	@dwUserID INT,								-- 用户 I D
	@dwPersonalRoomID INT,					-- 约战房间ID
	@dwDBQuestID INT,							-- 请求标识
	@dwInoutIndex INT,							-- 进出索引

	-- 变更成绩
	@lVariationScore BIGINT,					-- 用户分数
	@lVariationGrade BIGINT,					-- 用户成绩
	@lVariationInsure BIGINT,					-- 用户银行
	@lVariationRevenue BIGINT,					-- 游戏税收
	@lVariationWinCount INT,					-- 胜利盘数
	@lVariationLostCount INT,					-- 失败盘数
	@lVariationDrawCount INT,					-- 和局盘数
	@lVariationFleeCount INT,					-- 断线数目
	@lVariationUserMedal INT,					-- 用户奖牌
	@lVariationExperience INT,					-- 用户经验
	@lVariationLoveLiness INT,					-- 用户魅力
	@dwVariationPlayTimeCount INT,				-- 游戏时间

	-- 属性信息
	@wKindID INT,								-- 游戏 I D
	@wServerID SMALLINT,						-- 房间 I D
	@strClientIP NVARCHAR(15),					-- 连接地址
	@dwRoomHostID INT,							-- 房主 ID
	@dwPersonalTax INT,							-- 私人房税收
	@strPersonalRoomGUID NVARCHAR(31)			-- 私人房间唯一标识
WITH ENCRYPTION AS

-- 属性设置
SET NOCOUNT ON

-- 执行逻辑
BEGIN
	
	-- 如果当前房间玩家不存在
	DECLARE @dwExistUserID INT
	SELECT @dwExistUserID = UserID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomScoreInfo WHERE PersonalRoomGUID = @strPersonalRoomGUID AND UserID = @dwUserID  

	IF @dwExistUserID IS NULL
	BEGIN
		INSERT INTO WHJHPlatformDB..PersonalRoomScoreInfo(UserID, RoomID, Score, WinCount, LostCount, DrawCount, FleeCount, WriteTime, PersonalRoomGUID) 
												VALUES (@dwUserID, @dwPersonalRoomID, @lVariationScore,@lVariationWinCount, @lVariationLostCount, @lVariationDrawCount, @lVariationFleeCount, GETDATE(), @strPersonalRoomGUID) 
	END
	ELSE
	BEGIN
		-- 用户积分
		UPDATE WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomScoreInfo SET Score=Score+@lVariationScore, WinCount=WinCount+@lVariationWinCount, LostCount=LostCount+@lVariationLostCount,
			DrawCount=DrawCount+@lVariationDrawCount, FleeCount=FleeCount+@lVariationFleeCount, WriteTime=GETDATE() 
		WHERE PersonalRoomGUID = @strPersonalRoomGUID AND UserID = @dwUserID  
	END		
END

RETURN 0

GO







