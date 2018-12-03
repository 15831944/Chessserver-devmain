
----------------------------------------------------------------------------------------------------

USE WHJHPlatformDB
GO


IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_DeleteGameRoom]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_DeleteGameRoom]
GO



---����˽�˷������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_InsertPersonalRoomParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_InsertPersonalRoomParameter]
GO

--�޸�˽�˷������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_ModifyPersonalRoomParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_ModifyPersonalRoomParameter]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_DissumePersonalRoom]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_DissumePersonalRoom]
GO


SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------



----------------------------------------------------------------------------------------------------

-- ɾ������
CREATE  PROCEDURE dbo.GSP_GS_DeleteGameRoom
	@wServerID INT,								-- �����ʶ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	-- ���ҷ���
	DECLARE @ServerID INT
	SELECT @ServerID=ServerID FROM GameRoomInfo(NOLOCK) WHERE ServerID=@wServerID

	-- ����ж�
	IF @ServerID IS NULL
	BEGIN
		SET @strErrorDescribe=N'��Ϸ���䲻���ڻ����Ѿ���ɾ���ˣ������޸�ʧ�ܣ�'
		RETURN 1
	END

	-- ɾ������
	DELETE GameRoomInfo WHERE ServerID=@wServerID

	-- ɾ��˽�˷������
	DELETE PersonalRoomInfo WHERE ServerID=@wServerID
	
	SET @strErrorDescribe=N'ɾ���ɹ���'
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------
-- ���뷿��
CREATE  PROCEDURE dbo.GSP_GS_InsertPersonalRoomParameter

	-- ��������
	@wServerID INT,								-- �����ʶ
	@wKindID INT,								-- ����ID	
	@cbIsJoinGame TINYINT,						-- �Ƿ������Ϸ
	@cbMinPeople TINYINT,						-- ������Ϸ����С����
	@cbMaxPeople TINYINT,						-- ������Ϸ���������

	-- ��������
	@lPersonalRoomTax  BIGINT,					-- ˽�˷�����Ϸ˰��
	@lMaxCellScore BIGINT,						-- ���׷�
	@wCanCreateCount SMALLINT,					-- ���Դ�������󷿼���Ŀ
	@wPlayTurnCount INT,						-- �����ܹ�������Ϸ��������

	-- ��������
	@wPlayTimeLimit	INT,						-- �����ܹ�������Ϸ�����ʱ��
	@wTimeAfterBeginCount	INT,				-- һ����Ϸ��ʼ��೤ʱ����ɢ����
	@wTimeOffLineCount	INT,					-- ��ҵ��߶೤ʱ����ɢ����
	@wTimeNotBeginGame	INT,					-- ������º�೤ʱ���δ��ʼ��Ϸ��ɢ����
	@wTimeNotBeginAfterCreateRoom	INT,		-- ���䴴����೤ʱ���δ��ʼ��Ϸ��ɢ����
	@cbCreateRight	TINYINT,					-- ����˽�˷���Ȩ��
	@wBeginFreeTime   SMALLINT,						-- ���ʱ��-��ʼ
	@wEndFreeTime	 SMALLINT,					-- ���ʱ��-����
	
	-- �����Ϣ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	
	DECLARE @dwServerID INT
	SELECT  @dwServerID = ServerID FROM PersonalRoomInfo WHERE KindID = @wKindID
	IF @dwServerID IS NULL
	BEGIN
		-- ����˽�˷������
		INSERT INTO PersonalRoomInfo (ServerID,  KindID,  IsJoinGame, MinPeople, MaxPeople, PersonalRoomTax, MaxCellScore, CanCreateCount, 
					PlayTurnCount, PlayTimeLimit, TimeAfterBeginCount, TimeOffLineCount,TimeNotBeginGame, TimeNotBeginAfterCreateRoom, CreateRight,BeginFreeTime,EndFreeTime)
		VALUES (@wServerID,@wKindID, @cbIsJoinGame, @cbMinPeople, @cbMaxPeople, @lPersonalRoomTax, @lMaxCellScore, @wCanCreateCount, @wPlayTurnCount, 
		@wPlayTimeLimit, @wTimeAfterBeginCount, @wTimeOffLineCount, @wTimeNotBeginGame, @wTimeNotBeginAfterCreateRoom, @cbCreateRight,@wBeginFreeTime,@wEndFreeTime)
	END
	ELSE
	BEGIN
		--��ɾ��ԭ������
		DELETE PersonalRoomInfo WHERE KindID = @wKindID
		--����˽�˷������
		INSERT INTO PersonalRoomInfo (ServerID,  KindID,  IsJoinGame, MinPeople, MaxPeople, PersonalRoomTax, MaxCellScore, CanCreateCount, 
					PlayTurnCount, PlayTimeLimit, TimeAfterBeginCount, TimeOffLineCount,TimeNotBeginGame, TimeNotBeginAfterCreateRoom, CreateRight,BeginFreeTime,EndFreeTime)
		VALUES (@wServerID,@wKindID, @cbIsJoinGame, @cbMinPeople, @cbMaxPeople, @lPersonalRoomTax, @lMaxCellScore, @wCanCreateCount, @wPlayTurnCount, 
		@wPlayTimeLimit, @wTimeAfterBeginCount, @wTimeOffLineCount, @wTimeNotBeginGame, @wTimeNotBeginAfterCreateRoom, @cbCreateRight,@wBeginFreeTime,@wEndFreeTime)
	END
	
	SET @strErrorDescribe=N'�����ɹ���'
END

RETURN 0

GO


----------------------------------------------------------------------------------------------------
-- �޸�˽�˷������
CREATE  PROCEDURE dbo.GSP_GS_ModifyPersonalRoomParameter

	-- ��������
	@wServerID INT,								-- �����ʶ
	@wKindID INT,								-- �����ʶ	
	@cbIsJoinGame TINYINT,						-- �Ƿ������Ϸ
	@cbMinPeople TINYINT,						-- ������Ϸ����С����
	@cbMaxPeople TINYINT,						-- ������Ϸ���������

	-- ��������
	@lPersonalRoomTax  BIGINT,					-- ��ʼ�η���
	@lMaxCellScore BIGINT,						-- ���׷�
	@wCanCreateCount int,						-- ���Դ�������󷿼���Ŀ
	@wPlayTurnCount INT,						-- �����ܹ�������Ϸ��������

	-- ��������
	@wPlayTimeLimit	INT,						-- �����ܹ�������Ϸ�����ʱ��
	@wTimeAfterBeginCount	INT,				-- һ����Ϸ��ʼ��೤ʱ����ɢ����
	@wTimeOffLineCount	INT,					-- ��ҵ��߶೤ʱ����ɢ����
	@wTimeNotBeginGame	INT,					-- ���䴴���೤ʱ���δ��ʼ��Ϸ��ɢ����
	@wTimeNotBeginAfterCreateRoom	INT,		-- ���䴴����೤ʱ���δ��ʼ��Ϸ��ɢ����
	@cbCreateRight	TINYINT,					-- ����˽�˷���Ȩ��

	@wBeginFreeTime   SMALLINT,						-- ���ʱ��-��ʼ
	@wEndFreeTime		SMALLINT,					-- ���ʱ��-����
	-- �����Ϣ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	DECLARE @dwServerID INT
	SELECT  @dwServerID = ServerID FROM PersonalRoomInfo WHERE KindID = @wKindID
	IF @dwServerID IS NULL
	BEGIN
		-- ����˽�˷������
		INSERT INTO PersonalRoomInfo (ServerID,  KindID,   IsJoinGame, MinPeople, MaxPeople, PersonalRoomTax, MaxCellScore, CanCreateCount, 
					PlayTurnCount, PlayTimeLimit, TimeAfterBeginCount, TimeOffLineCount,TimeNotBeginGame, TimeNotBeginAfterCreateRoom, CreateRight,BeginFreeTime,EndFreeTime)
		VALUES (@wServerID,@wKindID,@cbIsJoinGame, @cbMinPeople, @cbMaxPeople, @lPersonalRoomTax, @lMaxCellScore, @wCanCreateCount, @wPlayTurnCount, 
		@wPlayTimeLimit, @wTimeAfterBeginCount, @wTimeOffLineCount, @wTimeNotBeginGame, @wTimeNotBeginAfterCreateRoom, @cbCreateRight,@wBeginFreeTime,@wEndFreeTime)
	END
	ELSE
	BEGIN
		--��ɾ��ԭ������
		DELETE PersonalRoomInfo WHERE KindID = @wKindID
		--����˽�˷������
		INSERT INTO PersonalRoomInfo (ServerID,  KindID,   IsJoinGame, MinPeople, MaxPeople, PersonalRoomTax, MaxCellScore, CanCreateCount, 
					PlayTurnCount, PlayTimeLimit, TimeAfterBeginCount, TimeOffLineCount,TimeNotBeginGame, TimeNotBeginAfterCreateRoom, CreateRight,BeginFreeTime,EndFreeTime)
		VALUES (@wServerID,@wKindID,@cbIsJoinGame, @cbMinPeople, @cbMaxPeople, @lPersonalRoomTax, @lMaxCellScore, @wCanCreateCount, @wPlayTurnCount, 
		@wPlayTimeLimit, @wTimeAfterBeginCount, @wTimeOffLineCount, @wTimeNotBeginGame, @wTimeNotBeginAfterCreateRoom, @cbCreateRight,@wBeginFreeTime,@wEndFreeTime)
	END
	SET @strErrorDescribe=N'�޸ĳɹ���'
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------


-- ��ɢ˽�˷�
CREATE PROC GSP_GR_DissumePersonalRoom
	@dwPersonalRoomID INT,						-- Լս����ID
	@lRoomHostID INT,							-- ���� ID
	@lTaxCount varchar(MAX),
	@cbPersonalRoomInfo varbinary(MAX),
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN	
	DECLARE @tDissumDate datetime
	DECLARE @TaxRatio INT
	SET @TaxRatio = 0
	DECLARE @RoomTaxRatio INT
	DECLARE @ServerID INT
	DECLARE @TaxAgency	INT 
	SET @RoomTaxRatio = 0
	SET @ServerID = 0

	-- ���ʱ��,�����ʶ
	select @tDissumDate = DissumeDate, @ServerID = ServerID, @TaxAgency = TaxAgency from WHJHPlatformDB..StreamCreateTableFeeInfo where RoomID = @dwPersonalRoomID AND UserID = @lRoomHostID

	-- ��ȡ��Ӧ�����˰�ձ���
	select @RoomTaxRatio = RevenueRatio from WHJHPlatformDB..GameRoomInfo where ServerID = @ServerID

	if @TaxAgency is null
	begin
		set @TaxAgency = 0
	end

	if @RoomTaxRatio is null or @RoomTaxRatio = 0
	begin
		set @TaxAgency = 1
	end

	declare @TaxRevenue int
	set @TaxRevenue = 0
	if @RoomTaxRatio is null or @RoomTaxRatio = 0
	begin
		set @RoomTaxRatio = @lTaxCount
	end
	else
	begin
		set @RoomTaxRatio =  @lTaxCount * @TaxAgency/@RoomTaxRatio
	end


	-- д�봴����¼
	UPDATE  WHJHPlatformDB..StreamCreateTableFeeInfo   SET  DissumeDate = GETDATE(),TaxCount = @lTaxCount, RoomScoreInfo = @cbPersonalRoomInfo , RoomStatus = 2  WHERE RoomID = @dwPersonalRoomID AND UserID = @lRoomHostID

	if @tDissumDate is null
	SET  @tDissumDate = GETDATE()

	--����
	DECLARE @temp TABLE
	(
	UserID INT
	);

	-- ��Դ���е����ݲ��뵽�������
	INSERT INTO @temp(UserID)
	SELECT UserID FROM PersonalRoomScoreInfo WHERE RoomID = @dwPersonalRoomID
	ORDER BY UserID;

	-- ��������
	DECLARE
	@UserID AS INT,
	@firstname AS NVARCHAR(10),
	@lastname AS NVARCHAR(20);
     
	WHILE EXISTS(SELECT UserID FROM @temp)
	BEGIN
	-- Ҳ����ʹ��top 1
	SET ROWCOUNT 1
	SELECT @UserID= UserID FROM @temp;
	DELETE FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker where UserID = @UserID;
	SET ROWCOUNT 0 
	DELETE FROM @temp WHERE UserID=@UserID;
	END

	select @tDissumDate AS DissumeDate
												
END

RETURN 0
GO

-----------------------------------------------


