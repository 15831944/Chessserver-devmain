USE WHJHAccountsDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_EfficacyOtherPlatform]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_EfficacyOtherPlatform]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_RegisterAccounts]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_RegisterAccounts]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_RegisterAccounts]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_RegisterAccounts]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_EfficacyAccounts]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_EfficacyAccounts]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_EfficacyLogonVisitor]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_EfficacyLogonVisitor]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_TransAccounts]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_TransAccounts]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------
-- 账号登录
CREATE PROC GSP_MB_EfficacyAccounts
  @strAccounts NVARCHAR(31),  -- 用户账号
  @strPassword NCHAR(32),     -- 用户密码
  @strClientIP NVARCHAR(15),  -- 连接地址
  @strMachineID NVARCHAR(32), -- 机器标识 
  @strErrorDescribe NVARCHAR(127) OUTPUT  -- 输出信息
--WITH ENCRYPTION
AS

-- 属性设置
SET NOCOUNT ON

-- 基本信息
DECLARE @UserID INT
DECLARE @FaceID SMALLINT
DECLARE @Accounts NVARCHAR(31)
DECLARE @NickName NVARCHAR(31)
DECLARE @PlatformID TINYINT
DECLARE @UnderWrite NVARCHAR(63)
DECLARE @SpreaderID INT
DECLARE @PlayTimeCount INT
DECLARE @UserType TINYINT
-- 财富变量
DECLARE @Score BIGINT
DECLARE @Insure BIGINT
-- 扩展信息
DECLARE @GameID INT
DECLARE @MemberOrder SMALLINT
DECLARE @MemberOverDate DATETIME
DECLARE @MemberSwitchDate DATETIME
-- 辅助变量
DECLARE @EnjoinLogon AS INT
DECLARE @DateTimeNow DATETIME

-- 执行逻辑
BEGIN
  -- 系统暂停
  SELECT @EnjoinLogon=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SELECT @strErrorDescribe=StatusString FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
    RETURN 2
  END
  SET @DateTimeNow=GETDATE()
  -- 效验地址
  SELECT @EnjoinLogon=EnjoinLogon FROM ConfineAddress(NOLOCK) WHERE AddrString=@strClientIP AND (EnjoinOverDate>@DateTimeNow OR EnjoinOverDate IS NULL)
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您所在的 IP 地址的登录功能，请联系客户服务中心了解详细情况！'
    RETURN 4
  END
  -- 效验机器
  SELECT @EnjoinLogon=EnjoinLogon FROM ConfineMachine(NOLOCK) WHERE MachineSerial=@strMachineID AND (EnjoinOverDate>@DateTimeNow OR EnjoinOverDate IS NULL)
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
  DECLARE @MachineSerial NCHAR(32)
  DECLARe @strLastLogonIP NCHAR(15)
  DECLARE @MoorMachine AS TINYINT
  SELECT @UserID=UserID, @GameID=GameID, @Accounts=Accounts, @NickName=NickName, @PlatformID=PlatformID, @UnderWrite=UnderWrite,
    @LogonPass=LogonPass, @InsurePass=InsurePass, @FaceID=FaceID, @Nullity=Nullity, @StunDown=StunDown, @MemberOrder=MemberOrder,
    @MemberOverDate=MemberOverDate, @MemberSwitchDate=MemberSwitchDate, @strLastLogonIP=LastLogonIP, @UserType=UserType,
    @MoorMachine=MoorMachine, @MachineSerial=LastLogonMachine, @SpreaderID=SpreaderID, @PlayTimeCount=PlayTimeCount
  FROM AccountsInfo(NOLOCK) WHERE Accounts=@strAccounts

  -- 查询用户
  IF @UserID IS NULL
  BEGIN
    SET @strErrorDescribe=N'您的账号不存在或者密码输入有误，请查证后再次尝试登录！'
    RETURN 1
  END 
  -- 账号禁止
  IF @Nullity<>0
  BEGIN
    SET @strErrorDescribe=N'您的账号暂时处于冻结状态，请联系客户服务中心了解详细情况！'
    RETURN 2
  END 
  -- 账号关闭
  IF @StunDown<>0
  BEGIN
    SET @strErrorDescribe=N'您的账号使用了安全关闭功能，必须重新开通后才能继续使用！'
    RETURN 2
  END 
  -- 固定机器
  IF @MoorMachine<>0
  BEGIN
    IF @MachineSerial<>@strMachineID
    BEGIN
      SET @strErrorDescribe=N'您的账号使用固定机器登录功能，您现所使用的机器不是所指定的机器！'
      RETURN 1
    END
  END
  -- 密码判断
  IF @LogonPass<>@strPassword
  BEGIN
    SET @strErrorDescribe=N'您的账号不存在或者密码输入有误，请查证后再次尝试登录！'
    RETURN 3
  END

  -- 查询金币
  SELECT @Score=Score, @Insure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@UserID
  -- 数据调整
  IF @Score IS NULL SET @Score=0
  IF @Insure IS NULL SET @Insure=0
  
  -- 会员等级
  IF @MemberOrder<>0 AND @DateTimeNow>@MemberSwitchDate
  BEGIN
    DECLARE @UserRight INT  
    SET @UserRight=0
    
    -- 删除会员
    DELETE AccountsMember WHERE UserID=@UserID AND MemberOverDate<=@DateTimeNow

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
    UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1,LastLogonDate=@DateTimeNow, LastLogonIP=@strClientIP,DynamicPassTime=@DateTimeNow,
      LastLogonMachine=@strMachineID
    WHERE UserID=@UserID
  END
  ELSE
  BEGIN
      -- 更新信息
    UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1,LastLogonDate=@DateTimeNow, LastLogonIP=@strClientIP,DynamicPassTime=@DateTimeNow,
      DynamicPass=CONVERT(nvarchar(32),REPLACE(newid(),'-','')),LastLogonMachine=@strMachineID
    WHERE UserID=@UserID
  END

  -- 动态密码
  DECLARE @szDynamicPass  nchar(32)
  SELECT @szDynamicPass=DynamicPass FROM AccountsInfo WHERE UserID=@UserID

  -- 记录日志
  DECLARE @DateID INT
  SET @DateID=CAST(CAST(@DateTimeNow AS FLOAT) AS INT)
  UPDATE SystemStreamInfo SET GameLogonSuccess=GameLogonSuccess+1 WHERE DateID=@DateID
  IF @@ROWCOUNT=0 INSERT SystemStreamInfo (DateID, GameLogonSuccess) VALUES (@DateID, 1)

  -- 银行标识
  DECLARE @InsureEnabled TINYINT
  SET @InsureEnabled=0
  IF @InsurePass <> '' SET @InsureEnabled=1
  
  -- 锁定房间ID
  DECLARE @wKindID INT
  SELECT @LockServerID=ServerID, @wKindID=KindID FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE UserID=@UserID
  IF @LockServerID IS NULL SET @LockServerID=0
  IF @wKindID IS NULL SET @wKindID=0

  -- 输出变量
  SELECT @UserID AS UserID, @GameID AS GameID, @Accounts AS Accounts, @NickName AS NickName, @szDynamicPass AS DynamicPass,
    @UnderWrite AS UnderWrite, @FaceID AS FaceID, @Score AS Score, @Insure AS Insure, 0 AS Present, @MemberOrder AS MemberOrder, 
    @MemberOverDate AS MemberOverDate, @MoorMachine AS MoorMachine, @InsureEnabled AS InsureEnabled, @PlatformID AS LogonMode,
    @UserType AS IsAgent, @LockServerID AS LockServerID, @wKindID AS KindID, @SpreaderID AS SpreaderID
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------
-- 游客登录/分配账号
CREATE PROC GSP_MB_EfficacyLogonVisitor
  @cbLogonMode TINYINT,         -- 登陆方式
  @strClientIP NVARCHAR(15),    -- 连接地址
  @strMachineID NVARCHAR(32),   -- 机器标识
  @strPassword NCHAR(32),       -- 登陆密码
  @cbDeviceType TINYINT,        -- 注册来源   
  @strErrorDescribe NVARCHAR(127) OUTPUT    -- 输出信息
AS

-- 属性设置
SET NOCOUNT ON

-- 基本信息
DECLARE @UserID INT
DECLARE @FaceID SMALLINT
DECLARE @Accounts NVARCHAR(31)
DECLARE @NickName NVARCHAR(31)
DECLARE @PlatformID TINYINT
DECLARE @UnderWrite NVARCHAR(63)
DECLARE @SpreaderID INT
DECLARE @PlayTimeCount INT
DECLARE @UserType TINYINT

-- 积分变量
DECLARE @Score BIGINT
DECLARE @Insure BIGINT
DECLARE @Present BIGINT

-- 扩展信息
DECLARE @GameID INT
DECLARE @UserMedal INT
DECLARE @MemberOrder SMALLINT
DECLARE @MemberOverDate DATETIME
DECLARE @MemberSwitchDate DATETIME
DECLARE @MBUserRight INT

-- 辅助变量
DECLARE @EnjoinLogon AS INT
DECLARE @DateTimeNow DATETIME

-- 执行逻辑
BEGIN
  -- 系统暂停
  SELECT @EnjoinLogon=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SELECT @strErrorDescribe=StatusString FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
    RETURN 1
  END
  SET @DateTimeNow = GETDATE()
  -- 效验地址
  SELECT @EnjoinLogon=EnjoinLogon FROM ConfineAddress(NOLOCK) WHERE AddrString=@strClientIP AND @DateTimeNow<EnjoinOverDate
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您所在的 IP 地址的登录功能，请联系客户服务中心了解详细情况！'
    RETURN 1
  END
  -- 效验机器
  SELECT @EnjoinLogon=EnjoinLogon FROM ConfineMachine(NOLOCK) WHERE MachineSerial=@strMachineID AND @DateTimeNow<EnjoinOverDate
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您的机器的登录功能，请联系客户服务中心了解详细情况！'
    RETURN 1
  END

  DECLARE @DateID INT
  SET @DateID=CAST(CAST(@DateTimeNow AS FLOAT) AS INT)

  -- 查询用户
  DECLARE @Nullity TINYINT
  DECLARE @StunDown TINYINT
  DECLARE @LogonPass AS NCHAR(32)
  DECLARE @InsurePass AS NCHAR(32)
  DECLARE @MoorMachine AS TINYINT
  DECLARE @MachineSerial NCHAR(32)
  DECLARE @Rebate SMALLINT
  DECLARE @IsNewAcc TINYINT
  SELECT @UserID=UserID,@Rebate=Rebate,@GameID=GameID,@Accounts=Accounts,@NickName=NickName,@PlatformID=PlatformID,@UnderWrite=UnderWrite,
    @LogonPass=LogonPass,@FaceID=FaceID,@Nullity=Nullity,@StunDown=StunDown,@InsurePass=InsurePass,@MemberOrder=MemberOrder,
    @MemberOverDate=MemberOverDate,@MemberSwitchDate=MemberSwitchDate,@MoorMachine=MoorMachine,@MachineSerial=LastLogonMachine,
    @SpreaderID=SpreaderID,@PlayTimeCount=PlayTimeCount,@UserType=UserType
  FROM AccountsInfo(NOLOCK) WHERE RegisterMachine=@strMachineID

  -- 注册用户
  SET @IsNewAcc=0
  SET @Present=0
  IF @UserID IS NULL
  BEGIN
    SET @SpreaderID=0
    SET @Rebate=0
    SET @IsNewAcc=1
    -- 注册用户
    INSERT AccountsInfo (Accounts,NickName,RegAccounts,PlatformID,UserUin,LogonPass,InsurePass,Gender,FaceID,
      GameLogonTimes,LastLogonIP,LastLogonMobile,LastLogonMachine,RegisterIP,RegisterMobile,RegisterMachine,RegisterOrigin)
    VALUES (N'',N'',N'',@cbLogonMode,N'',N'd1fd5495e7b727081497cfce780b6456',N'',0,0,
      1,@strClientIP,N'',@strMachineID,@strClientIP,N'',@strMachineID,@cbDeviceType)
    
    -- 玩家账号
    SET @UserID=SCOPE_IDENTITY()
    DECLARE @randomID INT
    DECLARE @strTempName NVARCHAR(31)
    SET @randomID=1000+(ABS(CHECKSUM(NewId()))%99000)
    SET @strTempName=N'游客'+CONVERT(NVARCHAR(5),@randomID)
    --更新账号昵称
    UPDATE AccountsInfo SET NickName=@strTempName WHERE UserID=@UserID
    SET @Accounts=N''
    SET @NickName=@strTempName
    SET @PlatformID=@cbLogonMode
    SET @FaceID=0
    
    -- 记录日志
    UPDATE SystemStreamInfo SET GameRegisterSuccess=GameRegisterSuccess+1 WHERE DateID=@DateID
    IF @@ROWCOUNT=0 INSERT SystemStreamInfo (DateID, GameRegisterSuccess) VALUES (@DateID, 1)

    -- 查询用户
    SELECT @GameID=GameID, @UnderWrite=UnderWrite, @Nullity=Nullity, @StunDown=StunDown, @MemberOrder=MemberOrder, @MemberOverDate=MemberOverDate,
      @MemberSwitchDate=MemberSwitchDate, @MoorMachine=MoorMachine, @PlayTimeCount=PlayTimeCount, @UserType=UserType
    FROM AccountsInfo(NOLOCK) WHERE UserID=@UserID
  
    -- 分配标识
    SELECT @GameID=GameID FROM GameIdentifier(NOLOCK) WHERE UserID=@UserID
    IF @GameID IS NULL 
    BEGIN
      SET @GameID=0
      SET @strErrorDescribe=N'用户注册成功，但未成功获取游戏 ID 号码，系统稍后将给您分配！'
    END
    ELSE UPDATE AccountsInfo SET GameID=@GameID WHERE UserID=@UserID

    -- 注册赠送
    -- 读取变量
    DECLARE @GrantIPCount AS BIGINT
    DECLARE @GrantScoreCount AS BIGINT
    SELECT @GrantIPCount=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'GrantIPCount'
    SELECT @GrantScoreCount=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'GrantScoreCount'

    -- 赠送限制
    IF @GrantScoreCount IS NOT NULL AND @GrantScoreCount>0 AND @GrantIPCount IS NOT NULL AND @GrantIPCount>0
    BEGIN
      -- 赠送次数
      DECLARE @GrantCount AS BIGINT
      DECLARE @GrantMachineCount AS BIGINT
      DECLARE @GrantRoomCardCount AS BIGINT
      SELECT @GrantCount=GrantCount FROM SystemGrantCount(NOLOCK) WHERE DateID=@DateID AND RegisterIP=@strClientIP
      SELECT @GrantMachineCount=GrantCount FROM SystemMachineGrantCount(NOLOCK) WHERE DateID=@DateID AND RegisterMachine=@strMachineID
      SELECT @GrantRoomCardCount=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'GrantRoomCardCount'

      -- 次数判断
      IF (@GrantCount IS NOT NULL AND @GrantCount>=@GrantIPCount) OR (@GrantMachineCount IS NOT NULL AND @GrantMachineCount>=@GrantIPCount)
      BEGIN
        SET @GrantScoreCount=0
      END
    END
    -- 初始化金币记录
    INSERT WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo
    (UserID,LastLogonIP,LastLogonDate,LastLogonMachine,RegisterIP,RegisterDate,RegisterMachine)
    VALUES
    (@UserID,@strClientIP,@DateTimeNow,@strMachineID,@strClientIP,@DateTimeNow,@strMachineID)

    -- 赠送金币
    IF @GrantScoreCount IS NOT NULL AND @GrantScoreCount>0
    BEGIN
      SET @Present=@GrantScoreCount
      -- 更新记录
      UPDATE SystemGrantCount SET GrantScore=GrantScore+@GrantScoreCount, GrantCount=GrantCount+1 WHERE DateID=@DateID AND RegisterIP=@strClientIP
      -- 插入记录
      IF @@ROWCOUNT=0
      BEGIN
        INSERT SystemGrantCount (DateID, RegisterIP, RegisterMachine, GrantScore, GrantCount) VALUES (@DateID, @strClientIP, @strMachineID, @GrantScoreCount, 1)
      END
      -- 更新记录
      UPDATE SystemMachineGrantCount SET GrantScore=GrantScore+@GrantScoreCount, GrantCount=GrantCount+1 WHERE DateID=@DateID AND RegisterMachine=@strMachineID
      -- 插入记录
      IF @@ROWCOUNT=0
      BEGIN
        INSERT SystemMachineGrantCount (DateID, RegisterIP, RegisterMachine, GrantScore, GrantCount) VALUES (@DateID, @strClientIP, @strMachineID, @GrantScoreCount, 1)
      END

      -- 查询金币
      DECLARE @CurrScore BIGINT
      DECLARE @CurrInsure BIGINT
      SELECT @CurrScore=Score,@CurrInsure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo  WHERE UserID=@UserID
        
      -- 赠送金币
      UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo SET Score = Score+@GrantScoreCount WHERE UserID = @UserID
    
      -- 流水账
      INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.RecordPresentInfo(UserID,PreScore,PreInsureScore,PresentScore,TypeID,IPAddress,CollectDate)
      VALUES (@UserID,@CurrScore,@CurrInsure,@GrantScoreCount,1,@strClientIP,@DateTimeNow)  
      
      -- 日统计
      UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.StreamPresentInfo SET DateID=@DateID, PresentCount=PresentCount+1,PresentScore=PresentScore+@GrantScoreCount WHERE UserID=@UserID AND TypeID=1
      IF @@ROWCOUNT=0
      BEGIN
        INSERT WHJHTreasureDBLink.WHJHTreasureDB.dbo.StreamPresentInfo(DateID,UserID,TypeID,PresentCount,PresentScore) VALUES(@DateID,@UserID,1,1,@GrantScoreCount)
      END

      -- 记录金币流水记录
      DECLARE @SerialNumber NVARCHAR(20)
      SELECT @SerialNumber=dbo.WF_GetSerialNumber()
      
      INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordTreasureSerial(SerialNumber,MasterID,UserID,TypeID,CurScore,CurInsureScore,ChangeScore,ClientIP,CollectDate) 
      VALUES(@SerialNumber,0,@UserID,1,@CurrScore,@CurrInsure,@GrantScoreCount,@strClientIP,@DateTimeNow)

      -- 写后台汇总
      DECLARE @today INT
      SET @today=DATEDIFF(s, '1970-01-01 00:00:00', convert(varchar(10),getdate(),120))-DATEDIFF(SS, SysUTCDateTime(),getdate())
      IF EXISTS (SELECT 1 FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot WITH(NOLOCK)
        WHERE UserID = @UserID and DiurnalKnotDate>=@today)
      BEGIN
        UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
        SET SystemDisCounts = SystemDisCounts+1, SystemDisAmount = SystemDisAmount+@GrantScoreCount
        WHERE UserID = @UserID and DiurnalKnotDate>=@today
      END
      ELSE 
      BEGIN
        INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
        (UserID,GameID,NickName,DiurnalKnotDate,SystemDisCounts,SystemDisAmount) 
        VALUES
        (@UserID,@GameID,@NickName,dbo.GSF_GR_ConvertDateTimeNowInt(),1,@GrantScoreCount)
      END
    END
  END
  ELSE IF @LogonPass<>@strPassword
  BEGIN
    SET @strErrorDescribe=N'您的密码已过期, 请重新登陆!'
    RETURN 9
  END
  -- 账号禁止
  IF @Nullity<>0
  BEGIN
    SET @strErrorDescribe=N'您的账号暂时处于冻结状态，请联系客户服务中心了解详细情况！'
    RETURN 1
  END
  -- 账号关闭
  IF @StunDown<>0
  BEGIN
    SET @strErrorDescribe=N'您的账号使用了安全关闭功能，必须重新开通后才能继续使用！'
    RETURN 1
  END 
  -- 固定机器
  IF @MoorMachine<>0
  BEGIN
    IF @MachineSerial<>@strMachineID
    BEGIN
      SET @strErrorDescribe=N'您的账号使用固定机器登录功能，您现所使用的机器不是所指定的机器！'
      RETURN 1
    END
  END

  -- 查询金币
  SELECT @Score=Score, @Insure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@UserID
  -- 数据调整
  IF @Score IS NULL SET @Score=0
  IF @Insure IS NULL SET @Insure=0

  -- 会员等级
  IF @MemberOrder<>0 AND @DateTimeNow>@MemberSwitchDate
  BEGIN
    DECLARE @UserRight INT  
    SET @UserRight=0
    
    -- 删除会员
    DELETE AccountsMember WHERE UserID=@UserID AND MemberOverDate<=@DateTimeNow

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
  DECLARE @LockServerIDCheck INT
  SELECT @LockServerIDCheck = ServerID FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE UserID = @UserID
  IF @LockServerIDCheck IS NOT NULL
  BEGIN
    -- 更新信息
    UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1, LastLogonDate=@DateTimeNow, LastLogonIP=@strClientIP, LastLogonMachine=@strMachineID 
    WHERE UserID=@UserID
  END
  ELSE
  BEGIN
    -- 更新信息
    UPDATE AccountsInfo SET GameLogonTimes=GameLogonTimes+1, LastLogonDate=@DateTimeNow, LastLogonIP=@strClientIP,DynamicPassTime=@DateTimeNow,
      DynamicPass=CONVERT(nvarchar(32),REPLACE(newid(),'-','')),LastLogonMachine=@strMachineID
    WHERE UserID=@UserID
  END
  
  -- 记录日志
  UPDATE SystemStreamInfo SET GameLogonSuccess=GameLogonSuccess+1 WHERE DateID=@DateID
  IF @@ROWCOUNT=0 INSERT SystemStreamInfo (DateID, GameLogonSuccess) VALUES (@DateID, 1)
  
  -- 动态密码
  DECLARE @szDynamicPass  nchar(32)
  SELECT @szDynamicPass=DynamicPass FROM AccountsInfo WHERE UserID=@UserID

  -- 银行标识
  DECLARE @InsureEnabled TINYINT
  SET @InsureEnabled=0
  IF @InsurePass <> '' SET @InsureEnabled=1
  
  -- 锁定房间ID
  DECLARE @LockServerID INT
  DECLARE @wKindID INT
  SELECT @LockServerID=ServerID, @wKindID=KindID FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE UserID=@UserID
  IF @LockServerID IS NULL SET @LockServerID=0
  IF @wKindID IS NULL SET @wKindID=0

  -- 输出变量
  SELECT @UserID AS UserID, @Rebate AS Rebate, @SpreaderID AS SpreaderID, @GameID AS GameID, @Accounts AS Accounts, @NickName AS NickName,
    @szDynamicPass AS DynamicPass, @UnderWrite AS UnderWrite, @FaceID AS FaceID, @Score AS Score, @Insure AS Insure, @Present AS Present,
    @MemberOrder AS MemberOrder, @MemberOverDate AS MemberOverDate, @MoorMachine AS MoorMachine, @InsureEnabled AS InsureEnabled,
    @PlatformID AS LogonMode, @UserType AS IsAgent, @LockServerID AS LockServerID, @wKindID AS KindID, @IsNewAcc AS IsNewAcc
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------
-- 游客账号转注册账号
CREATE PROC GSP_MB_TransAccounts
  @dwUserID INT,
  @strAccounts NVARCHAR(31),  -- 用户账号
  @strNickName NVARCHAR(31),  -- 用户昵称
  @strSpreadCode NCHAR(8),    -- 推广码
  @strLogonPass NCHAR(32),    -- 登录密码
  @strClientIP NVARCHAR(15),  -- 连接地址
  @strMachineID NCHAR(32),    -- 机器标识
  @cbDeviceType TINYINT,      -- 注册来源 
  @strErrorDescribe NVARCHAR(127) OUTPUT  -- 输出信息
AS

-- 属性设置
SET NOCOUNT ON

-- 基本信息
DECLARE @UserID INT
DECLARE @FaceID SMALLINT
DECLARE @MoorMachine TINYINT
DECLARE @Accounts NVARCHAR(31)
DECLARE @NickName NVARCHAR(31)
DECLARE @DynamicPass NCHAR(32)
DECLARE @UnderWrite NVARCHAR(63)
DECLARE @PlatformID SMALLINT

DECLARE @SpreaderID INT
DECLARE @Rebate SMALLINT
DECLARE @UserType TINYINT

-- 积分变量
DECLARE @Score BIGINT
DECLARE @Insure BIGINT
DECLARE @Present BIGINT

-- 扩展信息
DECLARE @GameID INT
DECLARE @UserMedal INT
DECLARE @MemberOrder SMALLINT
DECLARE @MemberOverDate DATETIME
DECLARE @MBUserRight INT
-- 辅助变量
DECLARE @EnjoinLogon AS INT
DECLARE @EnjoinRegister AS INT
DECLARE @EnjoinRegister2 AS INT
DECLARE @DateTimeNow DATETIME

-- 执行逻辑
BEGIN
  -- 注册暂停
  SELECT @EnjoinRegister=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinRegister'
  IF @EnjoinRegister IS NULL OR @EnjoinRegister=0
  BEGIN
    SELECT @strErrorDescribe=StatusString FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinRegister'
    RETURN 1
  END
  -- 登录暂停
  SELECT @EnjoinLogon=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
  IF @EnjoinLogon IS NOT NULL AND @EnjoinLogon<>0
  BEGIN
    SELECT @strErrorDescribe=StatusString FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'EnjoinLogon'
    RETURN 1
  END
  -- 效验名字
  IF (SELECT COUNT(*) FROM ConfineContent(NOLOCK) WHERE CHARINDEX(String,@strAccounts)>0)>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，您所输入的登录账号名含有限制字符串，请更换账号名后再次申请账号！'
    RETURN 1
  END
  -- 效验昵称
  IF (SELECT COUNT(*) FROM ConfineContent(NOLOCK) WHERE CHARINDEX(String,@strNickName)>0)>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，您所输入的游戏昵称名含有限制字符串，请更换昵称名后再次申请账号！'
    RETURN 1
  END
  SET @DateTimeNow = GETDATE()
  -- 效验地址
  SELECT @EnjoinRegister2=EnjoinRegister FROM ConfineAddress(NOLOCK) WHERE AddrString=@strClientIP AND (EnjoinOverDate>@DateTimeNow OR EnjoinOverDate IS NULL)
  IF @EnjoinRegister2 IS NOT NULL AND @EnjoinRegister2<>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您所在的 IP 地址的注册功能，请联系客户服务中心了解详细情况！'
    RETURN 1
  END
  -- 效验机器
  SELECT @EnjoinRegister2=EnjoinRegister FROM ConfineMachine(NOLOCK) WHERE MachineSerial=@strMachineID AND (EnjoinOverDate>@DateTimeNow OR EnjoinOverDate IS NULL)
  IF @EnjoinRegister2 IS NOT NULL AND @EnjoinRegister2<>0
  BEGIN
    SET @strErrorDescribe=N'抱歉地通知您，系统禁止了您的机器的注册功能，请联系客户服务中心了解详细情况！'
    RETURN 1
  END
  -- 校验频率
  DECLARE @LimitRegisterIPCount INT
  DECLARE @CurrRegisterCountIP INT
  SET @LimitRegisterIPCount = 0
  SET @CurrRegisterCountIP = 0
  SELECT @LimitRegisterIPCount=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'LimitRegisterIPCount'
  SELECT @CurrRegisterCountIP = COUNT(RegisterIP) FROM AccountsInfo WHERE RegisterIP=@strClientIP AND DateDiff(hh,RegisterDate,@DateTimeNow)<24
  IF @LimitRegisterIPCount <>0
  BEGIN
    IF @LimitRegisterIPCount<=@CurrRegisterCountIP
    BEGIN
      SET @strErrorDescribe = N'抱歉地通知您，您的机器当前注册超过次数限制！'
      RETURN 10
    END
  END

  IF EXISTS(SELECT 1 FROM AccountsInfo(NOLOCK) WHERE Accounts=@strAccounts)
  BEGIN
    SET @strErrorDescribe=N'手机号码已绑定!'
    RETURN 1
  END
  -- 查询游客账户
  DECLARE @Hierarchy INT
  SELECT @UserID=UserID, @SpreaderID=SpreaderID, @Rebate=Rebate, @UserType=UserType, @GameID=GameID, @Accounts=Accounts, @NickName=NickName,
    @PlatformID=PlatformID, @DynamicPass=DynamicPass, @UnderWrite=UnderWrite, @FaceID=FaceID, @MemberOrder=MemberOrder,
    @MemberOverDate=MemberOverDate, @MoorMachine=MoorMachine, @MBUserRight=UserRight, @Hierarchy=Hierarchy
  FROM AccountsInfo(NOLOCK) WHERE RegisterMachine=@strMachineID
  IF @USerID IS NULL
  BEGIN
    SET @strErrorDescribe=N'您的设备未登陆过游戏!'
    RETURN 1
  END
  IF @PlatformID <> 2 OR @UserID <> @dwUserID
  BEGIN
    SET @strErrorDescribe=N'您的设备已绑定账号!'
    RETURN 1
  END
  IF @NickName <> @strNickname
  BEGIN
    IF EXISTS (SELECT UserID FROM AccountsInfo(NOLOCK) WHERE NickName=@strNickName)
    BEGIN
      SET @strErrorDescribe=N'昵称已被使用了'
      RETURN 1
    END
  END
  -- 只有上级未0时才可绑定上级
  IF @SpreaderID=0 AND @EnjoinRegister<>3
  BEGIN
    IF @EnjoinRegister=2 AND @strSpreadCode=N''
    BEGIN
      SET @strErrorDescribe=N'错误的邀请码'
      RETURN 1
    END

    DECLARE @tgSpreaderID INT
    DECLARE @tgRebate SMALLINT
    DECLARE @spRebate SMALLINT
    DECLARE @tgUserType TINYINT
    DECLARE @tgState TINYINT
    SELECT @tgSpreaderID=UserID, @tgRebate=Rebate, @tgUserType=UserType, @tgState=State FROM SpreadCode(NOLOCK)
    WHERE Code=@strSpreadCode
    IF @EnjoinRegister=1
    BEGIN
      IF @strSpreadCode<>N'' AND (@tgSpreaderID IS NULL OR @tgState=0)
      BEGIN
        SET @strErrorDescribe=N'错误的邀请码!'
        RETURN 1
      END
    END
    ELSE IF @tgSpreaderID IS NULL OR @tgState=0
    BEGIN
      SET @strErrorDescribe=N'错误的邀请码!'
      RETURN 1
    END

    IF @tgSpreaderID IS NOT NULL AND @tgState<>0
    BEGIN
      SELECT @spRebate=Rebate, @Hierarchy=Hierarchy FROM AccountsInfo(NOLOCK) WHERE UserID=@tgSpreaderID
      -- 上级不存在或推广返点大于上级
      IF @spRebate IS NULL OR @spRebate < @tgRebate
      BEGIN
        SET @strErrorDescribe=N'错误的邀请码!!'
        RETURN 1
      END

      SET @SpreaderID=@tgSpreaderID
      SET @Rebate=@tgRebate
      SET @UserType=@tgUserType
      SET @Hierarchy=@Hierarchy+1
      UPDATE SpreadCode SET Registed=Registed+1 WHERE Code=@strSpreadCode
    END
  END
  -- 转换游客为注册用户
  UPDATE AccountsInfo SET SpreaderID=@SpreaderID, Rebate=@Rebate, UserType=@UserType, Accounts=@strAccounts, NickName=@strNickname,
    LogonPass=@strLogonPass, LastLogonIP=@strClientIP, LastLogonMachine=@strMachineID, RegisterIP=@strClientIP, RegisterOrigin=@cbDeviceType,
    PlatformID=0, LastLogonDate=@DateTimeNow, RegisterDate=@DateTimeNow, Hierarchy=@Hierarchy
  WHERE UserID=@UserID
  -- 错误判断
  IF @@ERROR<>0
  BEGIN
    SET @strErrorDescribe=N'由于意外的原因，账号注册失败，请尝试再次注册！'
    RETURN 1
  END

  SET @Present=0
  -- 绑定赠送
  DECLARE @DateID INT
  SET @DateID=CAST(CAST(@DateTimeNow AS FLOAT) AS INT)
  DECLARE @BindPresentGameScore AS BIGINT
  SELECT @BindPresentGameScore=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'BindPresentGameScore'
  IF @BindPresentGameScore IS NOT NULL AND @BindPresentGameScore>0
  BEGIN
    SET @Present=@BindPresentGameScore
    -- 更新记录
    UPDATE SystemGrantCount SET GrantScore=GrantScore+@BindPresentGameScore, GrantCount=GrantCount+1 WHERE DateID=@DateID AND RegisterIP=@strClientIP
    IF @@ROWCOUNT=0
    BEGIN
      INSERT SystemGrantCount (DateID, RegisterIP, RegisterMachine, GrantScore, GrantCount) VALUES (@DateID, @strClientIP, @strMachineID, @BindPresentGameScore, 1)
    END
    -- 更新记录
    UPDATE SystemMachineGrantCount SET GrantScore=GrantScore+@BindPresentGameScore, GrantCount=GrantCount+1 WHERE DateID=@DateID AND RegisterMachine=@strMachineID
    IF @@ROWCOUNT=0
    BEGIN
      INSERT SystemMachineGrantCount (DateID, RegisterIP, RegisterMachine, GrantScore, GrantCount) VALUES (@DateID, @strClientIP, @strMachineID, @BindPresentGameScore, 1)
    END
    -- 查询金币
    DECLARE @CurrScore BIGINT
    DECLARE @CurrInsure BIGINT
    SELECT @CurrScore=Score,@CurrInsure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo  WHERE UserID=@UserID
      
    -- 赠送金币
    UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo SET Score = Score+@BindPresentGameScore WHERE UserID=@UserID
    -- 流水账
    INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.RecordPresentInfo(UserID,PreScore,PreInsureScore,PresentScore,TypeID,IPAddress,CollectDate)
    VALUES (@UserID,@CurrScore,@CurrInsure,@BindPresentGameScore,1,@strClientIP,@DateTimeNow)
    -- 日统计
    UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.StreamPresentInfo SET DateID=@DateID, PresentCount=PresentCount+1,PresentScore=PresentScore+@BindPresentGameScore WHERE UserID=@UserID AND TypeID=1
    IF @@ROWCOUNT=0
    BEGIN
      INSERT WHJHTreasureDBLink.WHJHTreasureDB.dbo.StreamPresentInfo(DateID,UserID,TypeID,PresentCount,PresentScore) VALUES(@DateID,@UserID,15,1,@BindPresentGameScore)
    END

    -- 记录金币流水记录
    DECLARE @SerialNumber NVARCHAR(20)
    SELECT @SerialNumber=dbo.WF_GetSerialNumber()
    
    INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordTreasureSerial(SerialNumber,MasterID,UserID,TypeID,CurScore,CurInsureScore,ChangeScore,ClientIP,CollectDate) 
    VALUES(@SerialNumber,0,@UserID,101,@CurrScore,@CurrInsure,@BindPresentGameScore,@strClientIP,@DateTimeNow)

    -- 写后台汇总
    DECLARE @today INT
    SET @today=DATEDIFF(s, '1970-01-01 00:00:00', convert(varchar(10),getdate(),120))-DATEDIFF(SS, SysUTCDateTime(),getdate())
    IF EXISTS (SELECT 1 FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot WITH(NOLOCK)
      WHERE UserID = @UserID and DiurnalKnotDate>=@today)
    BEGIN
      UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
      SET SystemDisCounts = SystemDisCounts+1, SystemDisAmount = SystemDisAmount+@BindPresentGameScore
      WHERE UserID = @UserID and DiurnalKnotDate>=@today
    END
    ELSE 
    BEGIN
      INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
      (UserID,GameID,NickName,DiurnalKnotDate,SystemDisCounts,SystemDisAmount) 
      VALUES
      (@UserID,@GameID,@NickName,dbo.GSF_GR_ConvertDateTimeNowInt(),1,@BindPresentGameScore)
    END
  END
  -- 查询金币
  SELECT @Score=Score,@Insure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@UserID
  -- 数据调整
  IF @Score IS NULL SET @Score=0
  IF @Insure IS NULL SET @Insure=0

  -- 输出变量
  SELECT @UserID AS UserID, @Rebate AS Rebate, @SpreaderID AS SpreaderID, @GameID AS GameID, @strAccounts AS Accounts,
    @strNickname AS NickName, @DynamicPass AS DynamicPass, @UnderWrite AS UnderWrite, @FaceID AS FaceID,
    @Score AS Score, @Insure AS Insure, @Present AS Present, @MemberOrder AS MemberOrder, @MemberOverDate AS MemberOverDate,
    @MoorMachine AS MoorMachine, 0 AS InsureEnabled, 0 AS LogonMode, @UserType AS IsAgent, 0 AS LockServerID, 0 AS KindID
END

RETURN 0

GO
----------------------------------------------------------------------------------------------------

-- 第三方登录(废弃)
-- CREATE PROC GSP_MB_EfficacyOtherPlatform
-- 账号注册(废弃)
-- CREATE PROC GSP_GP_RegisterAccounts
-- 账号注册(废弃)
-- CREATE PROC GSP_MB_RegisterAccounts