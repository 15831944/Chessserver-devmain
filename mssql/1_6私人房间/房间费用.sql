USE WHJHTreasureDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_CreateTableFee]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_CreateTableFee]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_CreateTableQuit]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_CreateTableQuit]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_InsertCreateRecord]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_InsertCreateRecord]


---����˽�˷�����Ϣ
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_QueryPersonalRoomInfo]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_QueryPersonalRoomInfo]
GO

---֧����ʯ
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_PayRoomCard]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_PayRoomCard]
GO


SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------

-- �����ж�
CREATE PROC GSP_GR_CreateTableFee
	@dwUserID INT,								-- �û� I D
	@dwServerID INT,							-- ���� I D
	@dwDrawCountLimit INT,						-- ʱ������
	@dwDrawTimeLimit INT,						-- ��������
	@cbPayMode		TINYINT,					-- ������ʽ	
	@strClientIP NVARCHAR(15),					-- ���ӵ�ַ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
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
	-- ��ѯ���� �� �����Ƿ������Ϸ
	SELECT  @CreateRight = CreateRight FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 

	--�û���ʯ	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID

		-- ��ѯ����
	DECLARE @LockServerID INT
	SELECT @LockServerID=ServerID FROM GameScoreLocker WHERE UserID=@dwUserID

		-- �����ж�
	IF  @LockServerID IS NOT NULL and @LockServerID<>@dwServerID
	BEGIN
		-- ��ѯ��Ϣ
		DECLARE @ServerName NVARCHAR(31)
		SELECT @ServerName=ServerName FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo WHERE ServerID=@LockServerID

		-- ������Ϣ
		IF @ServerName IS NULL SET @ServerName=N'δ֪����'
		SET @strErrorDescribe=N'������ [ '+@ServerName+N' ] ��Ϸ�����У���������Ϸ��ͬʱ�������䣡'
		RETURN 2
	END

	--���뷽ʽ
	SELECT  @cbIsJoinGame = IsJoinGame,@iCanCreateCount=CanCreateCount FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 
		
	-- ��ѯ����
	SELECT @Fee=TableFee,@AAFee = AAPayFee FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @wKindID
	IF @Fee IS NULL OR @Fee=0
	BEGIN
		SET @strErrorDescribe=N'���ݿ��ѯ����ʧ�ܣ������³��ԣ�'
		RETURN 3
	END
	
	--��ѯ���봴�����з�����Ҫ����ʯ
	DECLARE @MaxPay INT	
	SELECT @MaxPay = SUM(NeedRoomCard) FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.StreamCreateTableFeeInfo WHERE UserID = @dwUserID AND RoomStatus <> 2
	
	--���ƴ���	
/* 	if @cbIsJoinGame = 0
	BEGIN
		DECLARE @CreateCount INT
		SELECT @CreateCount = Count(*) FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.StreamCreateTableFeeInfo WHERE UserID = @dwUserID AND RoomStatus <> 2 and PayMode = 0
		
		IF @CreateCount > @iCanCreateCount
		BEGIN
			SET @strErrorDescribe=N'�����������Ŀ�Ѿ������������'
			RETURN 1
		END
	END */
	
	-- ��ȡ��ʯ	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL SET @lDiamond=0
	
	--�Ƿ�AA��
	IF @cbPayMode = 1
	BEGIN
		IF @lDiamond  < @MaxPay + @AAFee
		BEGIN
			SET @strErrorDescribe=N'�û���ʯ���㣬�빺��'
			RETURN 1
		END
		SELECT @lDiamond AS Diamond, @cbIsJoinGame AS IsJoinGame,@AAFee AS RoomCardFee
	END
	ELSE
	BEGIN
		IF @lDiamond  < @MaxPay + @Fee
		BEGIN
			SET @strErrorDescribe=N'�û���ʯ���㣬�빺��'
			RETURN 1
		END
		SELECT @lDiamond AS Diamond, @cbIsJoinGame AS IsJoinGame,@Fee AS RoomCardFee
	END
END

RETURN 0
GO

----------------------------------------------------------------------------------------------------

-- �˻�����
CREATE PROC GSP_GR_CreateTableQuit
	@dwUserID INT,								-- �û� I D
	@dwPersonalRoomID INT,						-- Լս����ID
	@dwServerID INT,							-- �����ʶ
	@dwDrawCountLimit INT,						-- ʱ������
	@dwDrawTimeLimit INT,						-- ��������
	@strClientIP NVARCHAR(15),					-- ���ӵ�ַ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN	
	DECLARE @ReturnValue INT
	SET @ReturnValue=0

	DECLARE @lDiamond bigint
	DECLARE @cbJoin TINYINT

	DECLARE @wKindID INT
	SELECT  @wKindID = KindID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo WHERE ServerID = @dwServerID 
	-- ��ѯ���� �� �����Ƿ������Ϸ
	SELECT @cbJoin = IsJoinGame FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 

	-- ���¼�¼
	UPDATE  WHJHPlatformDB..StreamCreateTableFeeInfo   SET  DissumeDate = GETDATE(), RoomStatus = 2  WHERE RoomID = @dwPersonalRoomID AND UserID = @dwUserID

	--���������Ϸ��������
	IF @cbJoin = 1
	BEGIN
		DELETE FROM GameScoreLocker WHERE UserID = @dwUserID
	END

	--�û���ʯ	
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL SET @lDiamond=0

	SELECT @lDiamond AS Diamond

END

RETURN @ReturnValue
GO


-----------------------------------------------------------------------
-- ���������¼
CREATE PROC GSP_GR_InsertCreateRecord
	@dwUserID INT,								-- �û� I D
	@dwServerID INT,							-- ���� ��ʶ
	@dwPersonalRoomID INT,						-- Լս�����ʶ
	@lCellScore INT,							-- ���� �׷�
	@dwDrawCountLimit TINYINT,					-- ��������
	@dwDrawTimeLimit INT,						-- ʱ������
	@szPassWord NVARCHAR(15),					-- ���ӵ�ַ
	@wJoinGamePeopleCount TINYINT,				-- ��������
	@dwRoomTax BIGINT,							-- ˽�˷���˰��
	@strClientAddr NVARCHAR(15),
	@cbPayMode TINYINT,							-- ֧��ģʽ
	@lNeedRoomCard TINYINT,						-- ������Ҫ����ʯ
	@cbGameMode TINYINT,						-- ��Ϸģʽ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	DECLARE @Fee INT
	DECLARE @Nicname NVARCHAR(31)
	DECLARE @lPersonalRoomTax BIGINT

	-- ��ѯ���� �� �����Ƿ������Ϸ
	DECLARE @wKindID INT
	SELECT  @wKindID = KindID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.GameRoomInfo  WHERE ServerID = @dwServerID 
	-- ��ѯ����
	SELECT @Fee=TableFee FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalTableFee WHERE DrawCountLimit=@dwDrawCountLimit AND DrawTimeLimit=@dwDrawTimeLimit AND KindID = @wKindID 
	IF @Fee IS NULL OR @Fee=0
	BEGIN
		SET @strErrorDescribe=N'���ݿ��ѯ����ʧ�ܣ������³��ԣ�'
		RETURN 3
	END

	-- ��ȡ����������ҵ��ǳ�
	SELECT @Nicname =NickName FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID = @dwUserID
	IF @Nicname IS NULL
	SET @Nicname =''
		
	SELECT  @lPersonalRoomTax = PersonalRoomTax FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 
	
	--�����������ʯ����ѯ����˰��
	DECLARE @lTaxAgency BIGINT
	SELECT  @lTaxAgency = AgentScale FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsAgent WHERE UserID = @dwUserID 
	IF @lTaxAgency IS NOT NULL
	BEGIN
		SET @lPersonalRoomTax = @lTaxAgency
	END

	-- д�����ķſ���¼
	INSERT INTO WHJHPlatformDB..StreamCreateTableFeeInfo(UserID,NickName, ServerID, RoomID, CellScore, CountLimit,TimeLimit,CreateTableFee,CreateDate, TaxAgency, JoinGamePeopleCount,PayMode, RoomStatus, NeedRoomCard,GameMode)
												VALUES(@dwUserID,@Nicname, @dwServerID, @dwPersonalRoomID, @lCellScore,@dwDrawCountLimit, @dwDrawTimeLimit, @Fee,GETDATE(), @dwRoomTax, @wJoinGamePeopleCount, @cbPayMode, 0, @lNeedRoomCard,@cbGameMode)	


END

RETURN 0
GO

----------------------------------------------------------------------------------------------------------
-- ����Լս������Ϣ
CREATE  PROCEDURE dbo.GSP_GS_QueryPersonalRoomInfo
	@dwPersonalRoomID INT,							-- Լս�����ʶ
	@dwRoomHostID INT,									-- ����ID
	@strErrorDescribe NVARCHAR(127) OUTPUT				-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
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
	-- ���ط���
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
	--��ȡ��ϷID
	IF @dwServerID IS NOT NULL	SELECT @dwKindID=KindID FROM WHJHPlatformDB..GameRoomInfo WHERE ServerID=@dwServerID
	IF @dwKindID IS NULL SET @dwKindID=0
	
	declare @strRoomScoreInfo varchar(8000),@i int
	select @strRoomScoreInfo='',@i=datalength(@bnryRoomScoreInfo)
	while @i>0
		select @strRoomScoreInfo=substring('0123456789ABCDEF',substring(@bnryRoomScoreInfo,@i,1)/16+1,1)
				+substring('0123456789ABCDEF',substring(@bnryRoomScoreInfo,@i,1)%16+1,1)
				+@strRoomScoreInfo
			,@i=@i-1

	-- ��ȡ����ǳ�
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
-- ���������¼
CREATE PROC GSP_GS_PayRoomCard
	@cbPayMode	INT,							-- ֧����ʽ
	@dwUserID INT,								-- �û� I D
	@dwKindID INT,								-- ���� ��ʶ
	@dwPersonalRoomID INT,					-- Լս����ID
	@dwDrawCountLimit INT,						-- ��������
	@dwDrawTimeLimit INT,						-- ʱ������
	@strClientAddr NVARCHAR(15),				-- �ͻ��˵�ַ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN		
	DECLARE @Fee INT
	DECLARE @Nicname NVARCHAR(31)
	DECLARE @lPersonalRoomTax BIGINT
	DECLARE @ReturnValue INT

	-- ��ѯ���� �� �����Ƿ������Ϸ	
	DECLARE @wKindID INT

	-- ��ѯ����
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
		SET @strErrorDescribe=N'���ݿ��ѯ����ʧ�ܣ������³��ԣ�'
		RETURN 3
	END

	--	��ѯ���ķ�ʽ
	SELECT @lPersonalRoomTax = PersonalRoomTax FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomInfo WHERE KindID = @wKindID 	

	DECLARE @lDiamond BIGINT
	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
	IF @lDiamond IS NULL
	BEGIN
		SET @strErrorDescribe=N'�û���ʯ���㣬�빺��'
		RETURN 1
	END

	-- д�����
	IF @lDiamond < @Fee
	BEGIN
		SET @strErrorDescribe=N'������ʯ���㣬���ȳ�ֵ��'
		RETURN 4
	END
	ELSE
	BEGIN
		UPDATE WHJHTreasureDB..UserCurrency SET Diamond=@lDiamond-@Fee WHERE UserID=@dwUserID					
	--	SELECT @lDiamond=Diamond FROM WHJHTreasureDB..UserCurrency WHERE UserID=@dwUserID
		
		--������־
		DECLARE @DateTime DATETIME
		--DECLARE @Random VARCHAR(5)
		DECLARE @SerialNumber NVARCHAR(20)
		SET @DateTime = GETDATE()
		--SET @Random = CAST(FLOOR(89999*RAND()+10000) AS VARCHAR(5))
		SELECT @SerialNumber=dbo.WF_GetSerialNumber()
		
		--һ�˸�����
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
	
	--һ�˸�����
	IF @cbPayMode = 0
	BEGIN
		UPDATE WHJHPlatformDB..StreamCreateTableFeeInfo SET CreateTableFee = @Fee WHERE RoomID = @dwPersonalRoomID AND UserID = @dwUserID
	END

	
	SELECT @lDiamond-@Fee AS Dimaond
END

RETURN 0
GO
