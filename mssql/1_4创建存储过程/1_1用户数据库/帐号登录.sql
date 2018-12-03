
----------------------------------------------------------------------------------------------------

USE WHJHAccountsDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_EfficacyAccounts]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_EfficacyAccounts]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_EfficacyOtherPlatform]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_EfficacyOtherPlatform]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------

-- 帐号登录
CREATE PROC GSP_GP_EfficacyAccounts
	@strAccounts NVARCHAR(31),					-- 用户帐号
	@strPassword NCHAR(32),						-- 用户密码
	@strClientIP NVARCHAR(15),					-- 连接地址
	@strMachineID NVARCHAR(32),					-- 机器标识	
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- 输出信息
WITH ENCRYPTION AS

-- 属性设置
SET NOCOUNT ON

-- 基本信息
DECLARE @UserID INT
DECLARE @CustomID INT
DECLARE @FaceID SMALLINT
DECLARE @Accounts NVARCHAR(31)
DECLARE @NickName NVARCHAR(31)
DECLARE @PlatformID TINYINT
DECLARE @UserUin NVARCHAR(32)
DECLARE @UnderWrite NVARCHAR(63)
DECLARE @SpreaderID INT
DECLARE @AgentID INT
DECLARE @PlayTimeCount INT

-- 财富变量
DECLARE @Score BIGINT
DECLARE @Insure BIGINT

-- 扩展信息
DECLARE @GameID INT
DECLARE @Gender TINYINT
DECLARE @MemberOrder SMALLINT
DECLARE @MemberOverDate DATETIME
DECLARE @MemberSwitchDate DATETIME

-- 辅助变量
DECLARE @EnjoinLogon AS INT

-- 执行逻辑
BEGIN
	-- 系统暂停
	SELECT @EnjoinLogon=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
	IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
	BEGIN
		SELECT @strErrorDescribe=StatusString FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
		RETURN 2
	END

	-- 效验地址
	SELECT @EnjoinLogon=EnjoinLogon FROM ConfineAddress(NOLOCK) WHERE AddrString=@strClientIP AND (EnjoinOverDate>GETDATE() OR EnjoinOverDate IS NULL)
	IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
	BEGIN
		SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您所在的 IP 地址的登录功能，请联系客户服务中心了解详细情况！'
		RETURN 4
	END
	
	-- 效验机器
	SELECT @EnjoinLogon=EnjoinLogon FROM ConfineMachine(NOLOCK) WHERE MachineSerial=@strMachineID AND (EnjoinOverDate>GETDATE() OR EnjoinOverDate IS NULL)
	IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
	BEGIN
		SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您的机器的登录功能，请联系客户服务中心了解详细情况！'
		RETURN 7
	END
 
	-- 查询用户
	DECLARE @Nullity TINYINT
	DECLARE @StunDown TINYINT
	DECLARE @LogonPass AS NCHAR(32)
	DECLARE @InsurePass AS NCHAR(32)
	DECLARE	@MachineSerial NCHAR(32)
	DECLARe @strLastLogonIP NCHAR(15)
	DECLARE @MoorMachine AS TINYINT
	SELECT @UserID=UserID, @GameID=GameID, @Accounts=Accounts, @NickName=NickName, @PlatformID=PlatformID, @UserUin=UserUin, @UnderWrite=UnderWrite, @LogonPass=LogonPass,@InsurePass=InsurePass,
		@FaceID=FaceID, @CustomID=CustomID, @Gender=Gender, @Nullity=Nullity, @StunDown=StunDown,
		 @MemberOrder=MemberOrder, @MemberOverDate=MemberOverDate, @MemberSwitchDate=MemberSwitchDate,@strLastLogonIP=LastLogonIP,
		@MoorMachine=MoorMachine, @MachineSerial=LastLogonMachine,@SpreaderID=SpreaderID,@PlayTimeCount=PlayTimeCount,@AgentID=AgentID
	FROM AccountsInfo(NOLOCK) WHERE Accounts=@strAccounts

	-- 查询用户
	IF @UserID IS NULL
	BEGIN
		SET @strErrorDescribe=N'您的帐号不存在或者密码输入有误，请查证后再次尝试登录！'
		RETURN 1
	END	
	
	-- 查询代理
	IF EXISTS (SELECT * FROM AccountsAgent WHERE @UserID = UserID and Nullity=0)
	BEGIN
		SET @strErrorDescribe=N'您是代理商帐号，不可登录！'
		RETURN 1
	END		
	

	-- 帐号禁止
	IF @Nullity<>0
	BEGIN
		SET @strErrorDescribe=N'您的帐号暂时处于冻结状态，请联系客户服务中心了解详细情况！'
		RETURN 2
	END	

	-- 帐号关闭
	IF @StunDown<>0
	BEGIN
		SET @strErrorDescribe=N'您的帐号使用了安全关闭功能，必须重新开通后才能继续使用！'
		RETURN 2
	END	
	
	-- 固定机器
	IF @MoorMachine=1
	BEGIN
		IF @MachineSerial<>@strMachineID
		BEGIN
			SET @strErrorDescribe=N'您的帐号使用固定机器登录功能，您现所使用的机器不是所指定的机器！'
			RETURN 1
		END
	END

	-- 密码判断
	IF @LogonPass<>@strPassword
	BEGIN
		SET @strErrorDescribe=N'您的帐号不存在或者密码输入有误，请查证后再次尝试登录！'
		RETURN 3
	END

	
	-- 固定机器
	IF @MoorMachine=2
	BEGIN
		SET @MoorMachine=1
		SET @strErrorDescribe=N'您的帐号成功使用了固定机器登录功能！'
		UPDATE AccountsInfo SET MoorMachine=@MoorMachine, LastLogonMachine=@strMachineID WHERE UserID=@UserID
	END

	-- 推广员提成
	IF @SpreaderID<>0 AND @AgentID=0
	BEGIN
		DECLARE @GrantTime	INT
		DECLARE @GrantScore	BIGINT
		DECLARE @Note NVARCHAR(512)
		SET @Note = N'游戏时长达标一次性奖励'

		SELECT @GrantTime=PlayTimeCount,@GrantScore=PlayTimeGrantScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GlobalSpreadInfo WHERE ID=1
		IF @GrantTime IS NULL
		BEGIN
			SET @GrantTime = 108000 -- 30小时
			SET @GrantScore = 200000
		END	
		
		IF @GrantScore>0 AND @GrantTime>0 AND @PlayTimeCount>=@GrantTime
		BEGIN
			-- 获取提成信息
			DECLARE @RecordID INT
			SELECT @RecordID=RecordID FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.RecordSpreadInfo
			WHERE UserID = @SpreaderID AND ChildrenID = @UserID AND TypeID = 2
			
			IF @RecordID IS NULL
			BEGIN
				INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.RecordSpreadInfo(
					UserID,Score,TypeID,ChildrenID,CollectNote)
				VALUES(@SpreaderID,@GrantScore,2,@UserID,@Note)	
			END		
		END
	END

	-- 查询金币
	SELECT @Score=Score, @Insure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@UserID
	
	-- 数据调整
	IF @Score IS NULL SET @Score=0
	IF @Insure IS NULL SET @Insure=0

	-- 会员等级
	IF @MemberOrder<>0 AND GETDATE()>@MemberSwitchDate
	BEGIN
		DECLARE @UserRight INT	
		SET @UserRight=0
		
		-- 删除会员
		DELETE AccountsMember WHERE UserID=@UserID AND MemberOverDate<=GETDATE()

		-- 搜索会员
		SELECT @MemberOverDate=MAX(MemberOverDate), @MemberOrder=MAX(MemberOrder), @MemberSwitchDate=MIN(MemberOverDate)
			FROM AccountsMember(NOLOCK) WHERE UserID=@UserID

		-- 数据调整
		IF @MemberOrder IS NULL 
		BEGIN
			SET @MemberOrder=0
			SET @UserRight=512
		END
		IF @MemberOverDate IS NULL SET @MemberOverDate='1980-1-1'
		IF @MemberSwitchDate IS NULL SET @MemberSwitchDate='1980-1-1'

		-- 更新数据
		UPDATE AccountsInfo SET MemberOrder=@MemberOrder, MemberOverDate=@MemberOverDate, MemberSwitchDate=@MemberSwitchDate,
			UserRight=UserRight&~@UserRight WHERE UserID=@UserID
	END

	--判断玩家是否在房间中，如果在房间中不更新动态密码
	DECLARE @LockServerID INT
	SELECT @LockServerID = ServerID FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE UserID = @UserID
	IF @LockServerID IS NOT NULL
	BEGIN
		-- 更新信息
		UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1,LastLogonDate=GETDATE(), LastLogonIP=@strClientIP,DynamicPassTime=GETDATE(),
			LastLogonMachine=@strMachineID 
		WHERE UserID=@UserID
	END
	ELSE
	BEGIN
			-- 更新信息
		UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1,LastLogonDate=GETDATE(), LastLogonIP=@strClientIP,DynamicPassTime=GETDATE(),
			DynamicPass=CONVERT(nvarchar(32),REPLACE(newid(),'-','')),LastLogonMachine=@strMachineID 
		WHERE UserID=@UserID
	END

	-- 动态密码
	DECLARE @szDynamicPass  nchar(32)
	SELECT @szDynamicPass=DynamicPass FROM AccountsInfo WHERE UserID=@UserID

	-- 记录日志
	DECLARE @DateID INT
	SET @DateID=CAST(CAST(GETDATE() AS FLOAT) AS INT)
	UPDATE SystemStreamInfo SET GameLogonSuccess=GameLogonSuccess+1 WHERE DateID=@DateID
	IF @@ROWCOUNT=0 INSERT SystemStreamInfo (DateID, GameLogonSuccess) VALUES (@DateID, 1)

	-- 银行标识
	DECLARE @InsureEnabled TINYINT
	SET @InsureEnabled=0
	IF @InsurePass <> '' SET @InsureEnabled=1
	
	-- 代理标识
	DECLARE @IsAgent TINYINT
	SET @IsAgent =0
	IF EXISTS (SELECT * FROM AccountsAgent WHERE UserID=@UserID and Nullity=0) SET @IsAgent=1
		
	-- 输出变量
	SELECT @UserID AS UserID, @GameID AS GameID, @Accounts AS Accounts, @NickName AS NickName,@szDynamicPass AS DynamicPass,
		@UnderWrite AS UnderWrite,@FaceID AS FaceID, @CustomID AS CustomID, @Gender AS Gender,
		@Score AS Score, @Insure AS Insure, @MemberOrder AS MemberOrder, 
		@MemberOverDate AS MemberOverDate, @MoorMachine AS MoorMachine, @InsureEnabled AS InsureEnabled, @PlatformID AS LogonMode,@IsAgent AS IsAgent
END

RETURN 0

GO


----------------------------------------------------------------------------------------------------

-- 第三方登录(废弃)
-- CREATE PROC GSP_MB_EfficacyOtherPlatform
