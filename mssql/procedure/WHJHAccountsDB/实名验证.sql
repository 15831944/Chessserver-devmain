
----------------------------------------------------------------------------------------------------

USE WHJHAccountsDB
GO



IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_RealAuth]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_RealAuth]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO


----------------------------------------------------------------------------------------------------

-- 实名认证
CREATE PROC GSP_GP_RealAuth
  @dwUserID INT,                -- 用户 I D 
  @strPassword NCHAR(32),           -- 用户密码
  @strCompellation NVARCHAR(16),        -- 真实名字
  @strPassPortID NVARCHAR(18),        -- 证件号码
  @strClientIP NVARCHAR(15),          -- 连接地址
  @strErrorDescribe NVARCHAR(127) OUTPUT    -- 输出信息
-- WITH ENCRYPTION
AS

-- 属性设置
SET NOCOUNT ON

-- 执行逻辑
BEGIN
  DECLARE @GameID INT
  DECLARE @NickName NVARCHAR(31)
  -- 查询信息
  DECLARE @DynamicPass NCHAR(32)
  SELECT @DynamicPass=DynamicPass,@GameID=GameID,@NickName=NickName FROM AccountsInfo(NOLOCK) WHERE UserID=@dwUserID
  -- 用户判断
  IF @DynamicPass IS NULL OR @DynamicPass<>@strPassword
  BEGIN
    SET @strErrorDescribe=N'用户信息错误，认证失败！'
    RETURN 1
  END
  --简单验证
  IF LEN(@strPassPortID) = 0 OR LEN(@strCompellation) = 0
  BEGIN
    SET @strErrorDescribe=N'证件号码或真实姓名不正确，认证失败！'
    RETURN 1
  END 
  --重复验证
  IF EXISTS (SELECT UserID FROM WHJHRecordDBLink.WHJHRecordDB.dbo.RecordAuthentPresent WHERE UserID=@dwUserID)
  BEGIN
    SET @strErrorDescribe=N'已经认证，认证失败！'
    RETURN 1
  END
  -- 实名修改
  UPDATE AccountsInfo SET Compellation=@strCompellation, PassPortID=@strPassPortID  WHERE UserID=@dwUserID
  -- 成功判断
  IF @@ROWCOUNT=0
  BEGIN
    SET @strErrorDescribe=N'执行实名认证错误，请联系客户服务中心！'
    RETURN 1
  END
  -- 实名记录
  INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordAuthentPresent(UserID,PassPortID,Compellation,IpAddress)
  VALUES(@dwUserID,@strPassPortID,@strCompellation,@strClientIP)  
  
  --查询奖励
  DECLARE @AuthentPresentCount AS INT
  SELECT @AuthentPresentCount=StatusValue FROM SystemStatusInfo(NOLOCK) WHERE StatusName=N'JJRealNameAuthentPresent'
  IF @AuthentPresentCount IS NOT NULL AND @AuthentPresentCount>0
  BEGIN
    -- 查询金币
    DECLARE @CurrScore BIGINT
    DECLARE @CurrInsure BIGINT
    SELECT @CurrScore=Score,@CurrInsure=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@dwUserID
    -- 赠送金币
    UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo SET Score = Score+@AuthentPresentCount WHERE UserID=@dwUserID
    -- 记录金币流水记录
    DECLARE @SerialNumber NVARCHAR(20)
    SELECT @SerialNumber=dbo.WF_GetSerialNumber()
    INSERT INTO WHJHRecordDBLink.WHJHRecordDB.dbo.RecordTreasureSerial(SerialNumber,MasterID,UserID,TypeID,CurScore,CurInsureScore,ChangeScore,ClientIP,CollectDate) 
    VALUES(@SerialNumber,0,@dwUserID,102,@CurrScore,@CurrInsure,@AuthentPresentCount,@strClientIP,GETDATE())

    SET @strErrorDescribe=N'实名认证成功, 奖励金币'+CAST((@AuthentPresentCount/1000) AS NVARCHAR)+N'！'
  END
  ELSE
  BEGIN
    SET @AuthentPresentCount=0
    SET @strErrorDescribe=N'实名认证成功！'
  END
  -- 写后台汇总
  IF @AuthentPresentCount > 0
  BEGIN
    DECLARE @today INT
    SET @today=DATEDIFF(s, '1970-01-01 00:00:00', convert(varchar(10),getdate(),120))-DATEDIFF(SS, SysUTCDateTime(),getdate())

    IF EXISTS (SELECT 1 FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot WITH(NOLOCK)
      WHERE UserID = @dwUserID and DiurnalKnotDate>=@today)
    BEGIN
      UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
      SET SystemDisCounts = SystemDisCounts+1, SystemDisAmount = SystemDisAmount+@AuthentPresentCount
      WHERE UserID = @dwUserID and DiurnalKnotDate>=@today
    END
    ELSE 
    BEGIN
      INSERT INTO WHJHTreasureDBLink.WHJHTreasureDB.dbo.AccountsDiurnalKnot
      (UserID,GameID,NickName,DiurnalKnotDate,SystemDisCounts,SystemDisAmount) 
      VALUES
      (@dwUserID,@GameID,@NickName,dbo.GSF_GR_ConvertDateTimeNowInt(),1,@AuthentPresentCount)
    END
  END
  SELECT @AuthentPresentCount AS Present
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------
